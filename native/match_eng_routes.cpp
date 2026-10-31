// MATCH_ENG.CPP, routes: straight lines across the pitch grid, the players standing on their
// cells, and the distances between cells.
#include "match_eng_routes.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
// The pitch grid: 9 columns by 12 rows of cells at +0xa08, 0x5c bytes each, a column 0x450.
// A cell holds up to 11 occupants per side (pointers, 0x2c bytes per side) and, at +0x58, the
// number standing there for each side.
constexpr std::uint32_t grid_at = 0xa08, column_size = 0x450, cell_size = 0x5c;
constexpr std::uint32_t occupants_count_at = 0x58, occupant_side_size = 0x2c;
constexpr std::int32_t highest_column = 8, highest_row = 11;
constexpr std::uint32_t kickoff_side = 0x7924;  // byte
constexpr std::uint32_t clear_flags = 0x1be;    // two bytes cleared when a cell holds a taker
constexpr std::uint32_t nearest_blocker = 0xe02c;

// 0x10775540: |value| in r3 and the sign mask in r4; the leaf is run in line.
std::int32_t absolute_value(Cpu& c, std::int32_t value) {
    const auto mask = u(value >> 31);
    c.set(4, mask);
    const auto magnitude = (u(value) ^ mask) - mask;
    c.set(3, magnitude);
    return std::int32_t(magnitude);
}

std::uint32_t cell_at(std::uint32_t match, std::uint32_t column, std::uint32_t row) {
    return match + u(s8(column)) * column_size + u(s8(row)) * cell_size + grid_at;
}

// Whether the cell lies on the pitch; `column_limit` is 8, except where a routine allows more.
bool on_pitch(Cpu& c, std::uint32_t column, std::uint32_t row, std::int32_t column_limit = highest_column) {
    if (compare(c, s8(column), 0) < 0) return false;
    if (compare(c, s8(column), column_limit) > 0) return false;
    if (compare(c, s8(row), 0) < 0) return false;
    return compare(c, s8(row), highest_row) <= 0;
}

// The side the kick-off byte does not name (1 when it is 0).
std::uint32_t other_side(Cpu& c, std::uint32_t match) { return u(s8(s8(c.u8(match + kickoff_side)) == 0)); }

// A cell count turned into a single float the way the original does, through the stack.
double count_as_single(Cpu& c, Fpu& fpu, Frame& frame, std::int32_t value) {
    const auto pool = c.toc(-0x52c4);
    c.put32(frame.at(0x40), u(value) ^ 0x80000000);
    c.put32(frame.at(0x3c), 0x43300000);
    return fpu.fsubs(c.f64(frame.at(0x3c)), c.f64(pool + 0x60));
}

// How the two walkers differ: the registers that hold their start cell and total, and what
// they ask of each cell. The second also visits the last cell of a straight line, and lets a
// horizontal line run up to column 11.
struct LineWalk {
    std::uint32_t entry;
    unsigned total, from_column, from_row;  // registers
    bool blocking;                          // the second walker: cell_obstruction, inclusive lines
    std::uint32_t arguments[4];             // r7-r10 of the cell routine
    std::uint32_t back[6];                  // after each call: vertical, horizontal, then the distance and the cell routine of the column-major and the row-major walks
};

template <LineWalk W>
std::uint32_t walk_line(Cpu& c, std::uint32_t match, std::uint32_t column_1, std::uint32_t row_1, std::uint32_t column_2,
                        std::uint32_t row_2) {
    Frame frame(c, W.entry, 0xb0, {.first_gpr = 16, .first_fpr = 28});
    Fpu fpu(c);
    Kept<29> r_match(c, match);
    Kept<W.from_column> from_column(c, column_1);
    Kept<W.from_row> from_row(c, row_1);
    Kept<23> to_column(c, column_2);
    Kept<24> to_row(c, row_2);
    Kept<W.total> total(c, 0);
    Kept<22> scratch(c, c.r(22));
    Kept<19> cell(c, c.r(19));
    Kept<31> x(c, c.r(31));
    Kept<30> y(c, c.r(30));
    Kept<25> error(c, c.r(25));
    Kept<16> r16(c, c.r(16));
    Kept<17> row_step(c, c.r(17));
    Kept<18> column_step(c, c.r(18));
    Kept<21> column_span(c, u(s8(absolute_value(c, s8(to_column) - s8(from_column)))));
    Kept<20> row_span(c, u(s8(absolute_value(c, s8(to_row) - s8(from_row)))));

    // The cell routine over one cell.
    const auto visit = [&](double distance, std::uint32_t back) {
        const auto side = other_side(c, match);
        c.set_f(1, distance);
        enter(c, back, {match, side, cell});
        c.set({{7, W.arguments[0]}, {8, W.arguments[1]}, {9, W.arguments[2]}, {10, W.arguments[3]}});
        const auto found = W.blocking ? cell_obstruction(c, match, side, cell, W.arguments[0], W.arguments[1], W.arguments[2], W.arguments[3])
                                      : cell_occupants(c, match, side, cell, W.arguments[0], W.arguments[1], W.arguments[2], W.arguments[3]);
        total = std::uint32_t(total) + found;
    };
    // The distance from the start cell to another, as a single float.
    const auto cell_distance_from_start = [&](std::uint32_t back) {
        enter(c, back, {match, x, y, from_column, from_row});
        return cell_distance(c, x, y, from_column, from_row);
    };
    const auto swap_rows = [&] { scratch = from_row; from_row = std::uint32_t(to_row); to_row = std::uint32_t(scratch); };
    const auto swap_columns = [&] { scratch = from_column; from_column = std::uint32_t(to_column); to_column = std::uint32_t(scratch); };

    if (compare(c, s8(column_span)) == 0) {
        // Straight along the rows.
        if (compare(c, s8(from_row), s8(to_row)) > 0) swap_rows();
        y = from_row;
        while (W.blocking ? compare(c, s8(y), s8(to_row)) <= 0 : compare(c, s8(y), s8(to_row)) < 0) {
            if (!on_pitch(c, from_column, y)) return result(c, total);
            cell = cell_at(match, from_column, y);
            c.set_f(31, count_as_single(c, fpu, frame, s8(y) - s8(from_row)));
            visit(c.f(31), W.back[0]);
            y = std::uint32_t(y) + 1;
        }
        return result(c, total);
    }
    if (compare(c, s8(row_span)) == 0) {
        // Straight along the columns (the second walker lets them reach 11).
        if (compare(c, s8(from_column), s8(to_column)) > 0) swap_columns();
        x = from_column;
        while (W.blocking ? compare(c, s8(x), s8(to_column)) <= 0 : compare(c, s8(x), s8(to_column)) < 0) {
            if (!on_pitch(c, x, from_row, W.blocking ? 11 : highest_column)) return result(c, total);
            cell = cell_at(match, x, from_row);
            c.set_f(30, count_as_single(c, fpu, frame, s8(x) - s8(from_column)));
            visit(c.f(30), W.back[1]);
            x = std::uint32_t(x) + 1;
        }
        return result(c, total);
    }
    // A slanted line, walked from its lower end along its longer side.
    if ((compare(c, s8(column_span), s8(row_span)) >= 0 && compare(c, s8(from_column), s8(to_column)) > 0) ||
        (compare(c, s8(row_span), s8(column_span)) > 0 && compare(c, s8(from_row), s8(to_row)) > 0)) {
        swap_columns();
        swap_rows();
    }
    if (compare(c, s8(column_span), s8(row_span)) >= 0) {
        // Longer along the columns: one cell per column, the row steps when the error builds up.
        row_step = compare(c, s8(to_row) - s8(from_row)) < 0 ? u(-1) : 1;
        x = from_column;
        y = from_row;
        error = 0;
        r16 = error;
        while (compare(c, s8(x), s8(to_column)) <= 0) {
            if (compare(c, s8(error), s8(column_span)) >= 0) {
                error = std::uint32_t(error) - std::uint32_t(column_span);
                y = std::uint32_t(y) + std::uint32_t(row_step);
            }
            if (!on_pitch(c, x, y)) return result(c, total);
            cell = cell_at(match, x, y);
            c.set_f(29, cell_distance_from_start(W.back[2]));
            visit(c.f(29), W.back[3]);
            x = std::uint32_t(x) + 1;
            error = std::uint32_t(error) + std::uint32_t(row_span);
            c.put8(frame.at(0x3a), error);
        }
        return result(c, total);
    }
    // Longer along the rows.
    column_step = compare(c, s8(to_column) - s8(from_column)) < 0 ? u(-1) : 1;
    x = from_column;
    y = from_row;
    error = 0;
    c.put8(frame.at(0x39), error);
    while (compare(c, s8(y), s8(to_row)) <= 0) {
        if (compare(c, s8(error), s8(row_span)) >= 0) {
            error = std::uint32_t(error) - std::uint32_t(row_span);
            x = std::uint32_t(x) + std::uint32_t(column_step);
        }
        if (!on_pitch(c, x, y)) return result(c, total);
        cell = cell_at(match, x, y);
        c.set_f(28, cell_distance_from_start(W.back[4]));
        visit(c.f(28), W.back[5]);
        y = std::uint32_t(y) + 1;
        error = std::uint32_t(error) + std::uint32_t(column_span);
        c.put8(frame.at(0x38), error);
    }
    return result(c, total);
}

// Both walkers: see LineWalk. The first counts the players on the cells of a line, the second
// looks for the first one that stands in the way.
constexpr LineWalk counting{0x101eeeb0, 28, 26, 27, false, {0xffffffff, 0xffffffff, 0xffffffff, 2},
                            {0x101eefec, 0x101ef0f0, 0x101ef230, 0x101ef268, 0x101ef348, 0x101ef380}};
constexpr LineWalk blocking{0x101ef3c0, 26, 27, 28, true, {0, 2, 6, 0},
                            {0x101ef4fc, 0x101ef600, 0x101ef740, 0x101ef778, 0x101ef854, 0x101ef88c}};
}

std::uint32_t count_line_occupants(Cpu& c, std::uint32_t match, std::uint32_t column_1, std::uint32_t row_1,
                                   std::uint32_t column_2, std::uint32_t row_2) {
    return walk_line<counting>(c, match, column_1, row_1, column_2, row_2);
}

std::uint32_t find_line_blocker(Cpu& c, std::uint32_t match, std::uint32_t column_1, std::uint32_t row_1,
                                std::uint32_t column_2, std::uint32_t row_2) {
    return walk_line<blocking>(c, match, column_1, row_1, column_2, row_2);
}

double cell_distance(Cpu& c, std::uint32_t column_a, std::uint32_t row_a, std::uint32_t column_b, std::uint32_t row_b) {
    Frame frame(c, 0x101ef8d0, 0x40, {.first_gpr = 30});
    // The arguments go to the caller's parameter area, as bytes.
    c.put8(frame.at(0x5f), column_a);
    c.put8(frame.at(0x63), row_a);
    c.put8(frame.at(0x67), column_b);
    c.put8(frame.at(0x6b), row_b);
    const auto columns = s8(absolute_value(c, c.i8(frame.at(0x5f)) - c.i8(frame.at(0x67))));
    const auto rows = s8(absolute_value(c, c.i8(frame.at(0x63)) - c.i8(frame.at(0x6b))));
    double distance;
    if (compare(c, columns, 0) < 0 || compare(c, columns, highest_column) > 0 || compare(c, rows, 0) < 0 ||
        compare(c, rows, highest_row) > 0) {
        // Off the grid: the pool's default.
        const auto pool = c.toc(-0x52c8);
        c.set(3, pool);
        distance = c.f32(pool + 0xdc);
    } else {
        // The distance table: 9 floats a row.
        const auto table = c.toc(-0x6424);
        c.set(4, table);
        c.set(3, u(rows) * 0x24);
        distance = c.f32(table + u(rows) * 0x24 + u(columns) * 4);
    }
    c.set_f(1, distance);
    return distance;
}

std::uint32_t cell_occupants(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t cell, std::uint32_t r7,
                             std::uint32_t r8, std::uint32_t r9, std::uint32_t r10) {
    Frame frame(c, 0x101f09d0, 0x50, {.first_gpr = 26});
    Kept<27> r_match(c, match);
    Kept<26> r_side(c, side);
    Kept<28> r_cell(c, cell);
    Kept<29> counted(c, 0);
    Kept<30> index(c, 0);
    Kept<31> occupant(c, c.r(31));
    // The arguments wait in the caller's parameter area: the distance, a halfword and three bytes.
    c.put_f32(frame.at(0x74), float(c.f(1)));
    c.put16(frame.at(0x7a), r7);
    c.put8(frame.at(0x7f), r8);
    c.put8(frame.at(0x83), r9);
    c.put8(frame.at(0x87), r10);
    for (;; index = std::uint32_t(index) + 1) {
        if (compare(c, std::int32_t(index), s8(c.u8(cell + occupants_count_at + u(s8(side))))) >= 0) break;
        occupant = c.u32(cell + u(s8(side)) * occupant_side_size + (std::uint32_t(index) << 2 & ~3u));
        if (compare(c, s8(c.u8(occupant + 0x2c)), 1) > 0) continue;  // not on the pitch
        if (compare(c, s8(c.u8(occupant + 0x19)), 0) < 0) continue;
        if (compare(c, s8(c.u8(frame.at(0x87))), 2) == 0) {
            counted = std::uint32_t(counted) + 1;
            continue;
        }
        c.set({{3, occupant}, {4, u(c.i16(frame.at(0x7a)))}, {5, c.u8(frame.at(0x7f))}, {7, c.u8(frame.at(0x83))}});
        c.set_f(1, c.f32(frame.at(0x74)));
        c.call(0x10239f10, 0x101f0a70);
        if (compare(c, s8(c.r(3)), 0) == 0) continue;
        c.put8(match + clear_flags + 1, 0);
        c.put8(match + clear_flags, 0);
        return result(c, 1);
    }
    return result(c, counted);
}

std::uint32_t cell_obstruction(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t cell, std::uint32_t r7,
                               std::uint32_t r8, std::uint32_t r9, std::uint32_t r10) {
    Frame frame(c, 0x101f0ad0, 0x50, {.first_gpr = 26});
    Kept<27> r_match(c, match);
    Kept<26> r_side(c, side);
    Kept<28> r_cell(c, cell);
    Kept<29> counted(c, 0);
    Kept<30> index(c, 0);
    Kept<31> occupant(c, c.r(31));
    c.put_f32(frame.at(0x74), float(c.f(1)));
    c.put16(frame.at(0x7a), r7);
    c.put8(frame.at(0x7f), r8);
    c.put8(frame.at(0x83), r9);
    c.put8(frame.at(0x87), r10);
    for (;; index = std::uint32_t(index) + 1) {
        if (compare(c, std::int32_t(index), s8(c.u8(cell + occupants_count_at + u(s8(side))))) >= 0) break;
        occupant = c.u32(cell + u(s8(side)) * occupant_side_size + (std::uint32_t(index) << 2 & ~3u));
        if (compare(c, s8(c.u8(occupant + 0x2c)), 1) > 0) continue;
        if (compare(c, s8(c.u8(occupant + 0x19)), 0) < 0) continue;
        if (compare(c, s8(c.u8(frame.at(0x7f))), 2) == 0) {
            counted = std::uint32_t(counted) + 1;
            continue;
        }
        c.set({{3, occupant}, {4, u(c.i16(frame.at(0x7a)))}, {6, c.u8(frame.at(0x83))}, {7, c.u8(frame.at(0x87))}});
        c.set_f(1, c.f32(frame.at(0x74)));
        c.call(0x10272240, 0x101f0b70);
        const auto status = s8(c.r(3));
        if (compare(c, status, 2) == 0) {
            c.put32(match + nearest_blocker, occupant);
            return result(c, 2);
        }
        if (status > 2) continue;  // bge: the compare above left it greater
        if (compare(c, status, 1) < 0) continue;
        // Status 1: the occupant is counted and remembered as the nearest blocker.
        c.set(3, occupant);
        increment_byte(c, occupant + 0x12);
        c.put32(match + nearest_blocker, occupant);
        return result(c, 1);
    }
    return result(c, counted);
}
}
