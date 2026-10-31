// MATCH_ENG.CPP, routes: the two routes from a cell to a target cell (the goal mouth, or a
// given cell) that draw a random side for each cell they cross and ask its players.
#include "match_eng_routes.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
constexpr std::uint32_t grid_at = 0xa08, column_size = 0x450, cell_size = 0x5c;
constexpr std::int32_t highest_column = 8, highest_row = 11;
constexpr std::uint32_t nearest_blocker = 0xe02c;
constexpr std::uint32_t random_below = 0x101d3c90;

// 0x10775540: |value| in r3 and the sign mask in r4; the leaf is run in line.
std::int32_t absolute_value(Cpu& c, std::int32_t value) {
    const auto mask = u(value >> 31);
    c.set(4, mask);
    const auto magnitude = (u(value) ^ mask) - mask;
    c.set(3, magnitude);
    return std::int32_t(magnitude);
}

bool on_pitch(Cpu& c, std::uint32_t column, std::uint32_t row) {
    if (compare(c, s8(column), 0) < 0) return false;
    if (compare(c, s8(column), highest_column) > 0) return false;
    if (compare(c, s8(row), 0) < 0) return false;
    return compare(c, s8(row), highest_row) <= 0;
}

// One of the four walks of a route: toward higher or lower numbers, along the columns or the
// rows. They differ in where they keep their bytes on the stack and where the calls return to.
struct Leg {
    std::uint32_t start_slot, step_slot, end_slot;  // stack bytes: the error at the start and after each cell, the other axis' step
    unsigned float_register;                        // holds the cell's distance
    bool draws_first;                               // the random side is drawn before the distance is read
    std::uint32_t back_distance, back_draw, back_first, back_second;
    std::uint32_t answer_first, answer_second;      // stack bytes that keep the cell routine's answers
};
// Which registers a route keeps its walk in, and how the two routes differ.
struct Route {
    unsigned start_column, start_row, target_column, target_row, cell, side;
    bool bounds_first;   // the pitch is checked before the error is advanced, too
    bool skips_target;   // the target cell itself is not asked, unless the walk has no length
    Leg legs[4];         // up, along the columns; up, along the rows; down, along the columns; down, along the rows
};

// The walk from the start cell to the target, one cell per step of the longer side (a line
// drawn with an error that builds up on the shorter side). `ask` puts a question to a cell's
// players and answers whether the walk ends there.
template <Route R>
class Walk {
public:
    Walk(Cpu& c, Frame& frame, std::uint32_t match, std::uint32_t start_column, std::uint32_t start_row,
         std::uint32_t target_column, std::uint32_t target_row)
        : c(c), frame(frame), match(c, match), start_column(c, start_column), start_row(c, start_row),
          target_column(c, target_column), target_row(c, target_row), cell(c, c.r(R.cell)), side(c, c.r(R.side)),
          column_span(c, c.r(27)), row_span(c, c.r(26)), error(c, c.r(29)), y(c, c.r(30)), x(c, c.r(31)),
          column_step(c, c.r(16)) {}

    // The distances to the target along each axis.
    void measure() {
        column_span = u(s8(absolute_value(c, s8(target_column) - s8(start_column))));
        row_span = u(s8(absolute_value(c, s8(target_row) - s8(start_row))));
    }
    bool nowhere_to_go() {
        return compare(c, s8(column_span)) == 0 && compare(c, s8(row_span)) == 0;
    }
    // Whether to walk toward higher numbers: the start lies before the target along the longer side.
    bool upward() {
        if (compare(c, s8(column_span), s8(row_span)) >= 0 && compare(c, s8(start_column), s8(target_column)) < 0) return true;
        if (compare(c, s8(row_span), s8(column_span)) > 0 && compare(c, s8(start_row), s8(target_row)) < 0) return true;
        return false;
    }
    std::uint32_t along_columns() { return compare(c, s8(column_span), s8(row_span)) >= 0; }

    template <class Ask>
    std::uint32_t run(bool up, bool columns, Ask&& ask) {
        const Leg& leg = R.legs[(up ? 0 : 2) + (columns ? 0 : 1)];
        // The step on the shorter axis: a byte on the stack for the column walks (added zero-extended), a register for the row walks.
        if (columns) {
            c.put8(frame.at(std::int32_t(leg.step_slot)), compare(c, s8(target_row) - s8(start_row)) < 0 ? 0xff : 1);
        } else {
            column_step = compare(c, s8(target_column) - s8(start_column)) < 0 ? u(-1) : 1;
        }
        x = start_column;
        y = start_row;
        error = 0;
        c.put8(frame.at(std::int32_t(leg.start_slot)), error);
        const auto major = [&] { return columns ? std::uint32_t(x) : std::uint32_t(y); };
        const auto target_major = [&] { return columns ? std::uint32_t(target_column) : std::uint32_t(target_row); };
        const auto major_span = [&] { return columns ? std::uint32_t(column_span) : std::uint32_t(row_span); };
        const auto minor_span = [&] { return columns ? std::uint32_t(row_span) : std::uint32_t(column_span); };
        // After each cell: one step along the longer side, and the shorter side's share builds up.
        const auto advance = [&] {
            const auto next = major() + (up ? 1u : u(-1));
            if (columns) x = next; else y = next;
            error = std::uint32_t(error) + minor_span();
            c.put8(frame.at(std::int32_t(leg.end_slot)), error);
        };
        for (;; advance()) {
            const auto ordered = compare(c, s8(major()), s8(target_major()));
            if (up ? ordered > 0 : ordered < 0) break;
            if (R.bounds_first && !on_pitch(c, x, y)) return result(c, 0);
            if (compare(c, s8(error), s8(major_span())) >= 0) {
                error = std::uint32_t(error) - major_span();
                if (columns) y = std::uint32_t(y) + c.u8(frame.at(std::int32_t(leg.step_slot)));
                else x = std::uint32_t(x) + std::uint32_t(column_step);
            }
            if (R.skips_target && compare(c, s8(x), s8(target_column)) == 0 && compare(c, s8(y), s8(target_row)) == 0 &&
                (compare(c, s8(column_span)) != 0 || compare(c, s8(row_span)) != 0))
                continue;  // the target cell itself is not asked
            if (!on_pitch(c, x, y)) return result(c, 0);
            cell = match + u(s8(x)) * column_size + u(s8(y)) * cell_size + grid_at;
            const auto distance = [&] {
                enter(c, leg.back_distance, {match, x, y, start_column, start_row});
                c.set_f(leg.float_register, cell_distance(c, x, y, start_column, start_row));
            };
            const auto draw = [&] {
                c.set(3, 2);
                call(c, random_below, leg.back_draw);
                side = u(s8(c.r(3)));
            };
            if (leg.draws_first) { draw(); distance(); } else { distance(); draw(); }
            if (ask(std::uint32_t(side), false, leg)) return c.r(3);
            if (ask(u(s8(side) == 0), true, leg)) return c.r(3);
        }
        return result(c, 0);
    }

    Cpu& c;
    Frame& frame;
    Kept<28> match;
    Kept<R.start_column> start_column;
    Kept<R.start_row> start_row;
    Kept<R.target_column> target_column;
    Kept<R.target_row> target_row;
    Kept<R.cell> cell;
    Kept<R.side> side;
    Kept<27> column_span;
    Kept<26> row_span;
    Kept<29> error;
    Kept<30> y;
    Kept<31> x;
    Kept<16> column_step;
};

constexpr Route first_route{22, 23, 17, 21, 24, 25, false, false, {
    {0x3f, 0x48, 0x3e, 31, false, 0x101efb5c, 0x101efb68, 0x101efb90, 0x101efbd8, 0x47, 0x46},
    {0x3d, 0, 0x3c, 30, false, 0x101efcc8, 0x101efcd4, 0x101efcfc, 0x101efd44, 0x45, 0x44},
    {0x3b, 0x48, 0x3a, 29, false, 0x101efe50, 0x101efe5c, 0x101efe84, 0x101efecc, 0x43, 0x42},
    {0x39, 0, 0x38, 28, false, 0x101effbc, 0x101effc8, 0x101efff0, 0x101f0038, 0x41, 0x40}}};
constexpr Route second_route{21, 22, 19, 20, 23, 24, true, true, {
    {0x3f, 0x40, 0x3e, 31, false, 0x101f0368, 0x101f0374, 0x101f039c, 0x101f03e0, 0, 0},
    {0x3d, 0, 0x3c, 30, true, 0x101f0548, 0x101f052c, 0x101f0570, 0x101f05b4, 0, 0},
    {0x3b, 0x40, 0x3a, 29, true, 0x101f0738, 0x101f071c, 0x101f0760, 0x101f07a4, 0, 0},
    {0x39, 0, 0x38, 28, true, 0x101f090c, 0x101f08f0, 0x101f0934, 0x101f0978, 0, 0}}};

// What the second route's flag byte makes of the number it passes along (r25).
enum class Style { fixed, roll, chance, none };
Style classify(Cpu& c, std::int32_t v) {
    auto o = compare(c, v, 0x36);
    if (o == 0) return Style::roll;
    if (o > 0) {
        o = compare(c, v, 0x5a);
        if (o == 0) return Style::chance;
        if (o > 0) {
            o = compare(c, v, 0x5e);
            if (o == 0) return Style::fixed;
            if (o > 0) return Style::none;
            return compare(c, v, 0x5d) >= 0 ? Style::none : Style::fixed;
        }
        o = compare(c, v, 0x3e);
        if (o == 0) return Style::chance;
        if (o > 0) return Style::none;
        if (compare(c, v, 0x3c) >= 0) return Style::roll;
        return compare(c, v, 0x38) >= 0 ? Style::none : Style::fixed;
    }
    o = compare(c, v, 0x1e);
    if (o == 0) return Style::fixed;
    if (o > 0) {
        o = compare(c, v, 0x34);
        if (o == 0) return Style::fixed;
        if (o > 0) return Style::none;
        return compare(c, v, 0x20) >= 0 ? Style::none : Style::roll;
    }
    o = compare(c, v, 8);
    if (o == 0) return Style::fixed;
    if (o > 0) return compare(c, v, 0xa) >= 0 ? Style::none : Style::chance;
    return compare(c, v, 6) == 0 ? Style::fixed : Style::none;
}
}

std::uint32_t first_blocker_to_goal(Cpu& c, std::uint32_t match, std::uint32_t column, std::uint32_t row, std::uint32_t r6,
                                    std::uint32_t attacking_down, std::uint32_t r8, std::uint32_t r9) {
    Frame frame(c, 0x101ef9a0, 0xb0, {.first_gpr = 16, .first_fpr = 28});
    keep(c, {{18, r6}, {19, r8}, {20, r9}});
    c.put8(frame.at(0xdb), attacking_down);
    c.set(3, match + 0x10000);
    c.put32(match + nearest_blocker, 0);
    // The target is the middle column of the far goal line: row 11 when the flag is 1, else row 0.
    const std::uint32_t target_row = compare(c, s8(c.u8(frame.at(0xdb))), 1) == 0 ? 0xb : 0;
    Walk<first_route> walk(c, frame, match, column, row, 4, target_row);
    walk.measure();
    if (walk.nowhere_to_go()) return result(c, 0);
    const bool up = walk.upward();
    const bool columns = walk.along_columns();
    return walk.run(up, columns, [&](std::uint32_t side, bool second, const Leg& leg) {
        c.set_f(1, c.f(leg.float_register));
        enter(c, second ? leg.back_second : leg.back_first, {match, side, walk.cell});
        c.set({{7, r6}, {8, 1}, {9, r8}, {10, r9}});
        const auto answer = cell_obstruction(c, match, side, walk.cell, r6, 1, r8, r9);
        const auto at = frame.at(std::int32_t(second ? leg.answer_second : leg.answer_first));
        c.put8(at, answer);
        if (compare(c, s8(answer)) == 0) return false;
        c.set(3, c.u8(at));
        return true;
    });
}

std::uint32_t first_taker_to_cell(Cpu& c, std::uint32_t match, std::uint32_t column, std::uint32_t row, std::uint32_t target_column,
                                  std::uint32_t target_row, std::uint32_t flag, std::uint32_t r9, std::uint32_t r10) {
    Frame frame(c, 0x101f0090, 0xb0, {.first_gpr = 16, .first_fpr = 28});
    keep(c, {{18, r9}, {17, r10}});
    c.put8(frame.at(0xdf), flag);
    Walk<second_route> walk(c, frame, match, column, row, target_column, target_row);
    // The number the players are asked about, by the flag byte.
    Kept<25> number(c, c.r(25));
    switch (classify(c, c.i8(frame.at(0xdf)))) {
    case Style::fixed: number = 4; break;
    case Style::roll:
        c.set(3, 4);
        call(c, random_below, 0x101f01ac);
        number = u(s8(c.r(3) + 1));
        break;
    case Style::chance:
        c.set(3, u(std::int16_t(r9)) + 0xbb8);
        call(c, random_below, 0x101f0174);
        // 5 on a draw other than 0, else 4, through a stack word.
        c.put32(frame.at(0x44), compare(c, std::int32_t(c.r(3))) != 0 ? 5 : 4);
        number = u(s8(c.u32(frame.at(0x44))));
        break;
    case Style::none: number = 0; break;
    }
    walk.measure();
    const bool up = walk.upward();
    const bool columns = walk.along_columns();
    return walk.run(up, columns, [&](std::uint32_t side, bool second, const Leg& leg) {
        c.set_f(1, c.f(leg.float_register));
        enter(c, second ? leg.back_second : leg.back_first, {match, side, walk.cell});
        c.set({{7, r9}, {8, number}, {9, r10}, {10, 1}});
        const auto found = cell_occupants(c, match, side, walk.cell, r9, number, r10, 1);
        if (compare(c, s8(found)) == 0) return false;
        c.set(3, 1);
        return true;
    });
}
}
