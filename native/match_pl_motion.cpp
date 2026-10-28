#include "match_pl_detail.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
struct Division {
    std::int32_t high, shifted, remainder;
    std::uint32_t sign;
};
Division divide_heading(Cpu& c, std::uint32_t sum, std::uint32_t magic, unsigned shift, int divisor) {
    const auto high = std::int32_t(std::uint64_t(std::int64_t(std::int32_t(magic)) * std::int32_t(sum)) >> 32);
    const auto combined = std::int32_t(std::uint32_t(high) + sum);
    const auto shifted = combined >> shift;
    const auto sign = std::uint32_t(shifted) >> 31;
    c.s.wr<1>(reg::xer_ca, combined < 0 && (std::uint32_t(combined) & ((1u << shift) - 1)));
    return {high, shifted, std::int32_t(sum) % divisor, sign};
}
}

void set_coordinates(Cpu& c, std::uint32_t player, std::uint32_t column, std::uint32_t row) {
    Frame frame(c, 0x102374f0, 0xb0, {.first_gpr = 25});
    const auto success = c.toc(-0x7bac), buffer = c.toc(-0x7ba8), strings = c.toc(-0x51e4);
    keep(c, {{26, column}, {25, player}, {29, success}, {27, row}, {30, buffer}, {31, strings}});
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(column))));
    bool changed = compare(c, std::int8_t(column), c.i8(player + 0x105)) != 0;
    if (!changed) {
        c.set(3, std::uint32_t(std::int32_t(std::int8_t(row))));
        changed = compare(c, std::int8_t(row), c.i8(player + 0x106)) != 0;
    }
    if (!changed) return;
    const auto cell = [&](std::int8_t x, std::int8_t y, std::uint32_t owner) {
        return owner + 0xa08 + std::uint32_t(x * 0x450 + y * 0x5c);
    };
    const auto diagnostic = [&](bool full) {
        const auto first = frame.at(full ? 0x48 : 0x68), second = frame.at(full ? 0x38 : 0x40);
        leave(c, {{3, strings + 0x8f}, {4, 0}, {5, 0}, {6, first}, {7, second}});
        c.call(0x10739890, full ? 0x1023778c : 0x102375c8);
        c.call(0x10768810, full ? 0x10237790 : 0x102375cc);
        leave(c, {{5, c.r(3)}, {3, buffer}, {4, strings + 0x9c}, {6, first}, {7, second}, {8, full ? 0x132cu : 0x1309u}});
        c.call(0x1077b720, full ? 0x102377ac : 0x102375e8);
        leave(c, {{4, buffer}, {3, strings + 0xa9}, {5, 0}});
        c.call(0x10769b60, full ? 0x102377bc : 0x102375f8);
        c.put8(success, 0);
        if (full) { c.set(3, UINT32_MAX); c.call(0x1076f7d0, 0x102377cc); }
    };
    if (compare(c, c.i8(player + 0x28), -1) > 0) {
        const auto owner = c.u32(player + 0x1a4);
        const auto previous = cell(c.i8(player + 0x105), c.i8(player + 0x106), owner);
        const auto side = std::uint32_t(std::int32_t(c.i8(player + 0x27)));
        const auto destination = cell(std::int8_t(column), std::int8_t(row), owner);
        keep(c, {{28, destination}});
        leave(c, {{3, previous}, {4, std::uint32_t(std::int8_t(column) * 0x450)},
                  {5, std::uint32_t(std::int32_t(std::int8_t(row)))}, {6, std::uint32_t(std::int32_t(std::int8_t(column)))},
                  {7, owner}, {8, owner}});
        if (compare(c, c.i8(previous + side + 0x58)) == 0) { diagnostic(false); return; }
        const auto count = std::uint32_t(std::int32_t(c.i8(previous + side + 0x58)));
        const auto index = std::uint32_t(std::int32_t(c.i8(player + 0x28)));
        const auto last = c.u32(previous + side * 0x2c + count * 4 - 4);
        c.put32(previous + side * 0x2c + index * 4, last);
        c.put8(previous + side + 0x58, c.u8(previous + side + 0x58) - 1);
        const auto moved = c.u32(previous + side * 0x2c + index * 4);
        leave(c, {{4, side * 0x2c}, {5, previous + side}, {6, previous + side * 0x2c}});
        if (compare_logical(c, moved) != 0) {
            c.set(5, c.u8(player + 0x28)); c.put8(moved + 0x28, c.r(5));
        }
        c.put8(player + 0x28, c.u8(destination + side + 0x58));
        c.set(4, destination + side);
        c.put8(destination + side + 0x58, c.u8(destination + side + 0x58) + 1);
        c.set(3, side * 0x2c);
        c.put32(destination + side * 0x2c + std::uint32_t(std::int32_t(c.i8(player + 0x28))) * 4, player);
    } else {
        const auto side = std::uint32_t(std::int32_t(c.i8(player + 0x27)));
        const auto owner = c.u32(player + 0x1a4);
        const auto destination = cell(std::int8_t(column), std::int8_t(row), owner);
        keep(c, {{28, destination}}); c.set(5, owner);
        c.put8(player + 0x28, c.u8(destination + side + 0x58));
        c.set(4, destination + side); c.set(3, destination + side);
        c.put8(destination + side + 0x58, c.u8(destination + side + 0x58) + 1);
        if (compare(c, c.i8(destination + side + 0x58), 11) > 0) diagnostic(true);
        c.set(3, side * 0x2c);
        c.put32(destination + side * 0x2c + std::uint32_t(std::int32_t(c.i8(player + 0x28))) * 4, player);
    }
    c.put8(player + 0x105, column); c.put8(player + 0x106, row);
    enter(c, 0x102377fc, {player}); has_possession(c, player);
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        c.set(3, c.u32(player + 0x1a4)); c.put8(c.r(3) + 0x791d, c.u8(player + 0x105));
        c.set(3, c.u32(player + 0x1a4)); c.put8(c.r(3) + 0x791e, c.u8(player + 0x106));
    }
}

void set_heading(Cpu& c, std::uint32_t player, std::uint32_t from_column, std::uint32_t from_row,
                 std::uint32_t to_column, std::uint32_t to_row) {
    Frame frame(c, 0x10237830, 0x60, {.first_gpr = 25});
    const auto seed = c.toc(-0x7b90), begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    keep(c, {{28, seed}, {27, player}, {29, begin}, {30, end}, {31, cursor}});
    const auto dx = std::int8_t(to_column - from_column), dy = std::int8_t(to_row - from_row);
    leave(c, {{4, std::uint32_t(std::int32_t(dx))}, {5, std::uint32_t(std::int32_t(dy))}});
    bool displaced = compare(c, dx) != 0;
    if (!displaced) displaced = compare(c, dy) != 0;
    const auto advance = [&](std::uint32_t back, bool traits = false) {
        const auto next = c.u32(cursor) + 2;
        c.set(3, next);
        if (traits) keep(c, {{26, c.u8(player + 0x108)}, {25, c.u8(player + 0x10f)}});
        const auto ordering = compare_logical(c, next, c.u32(end));
        c.put32(cursor, next);
        if (ordering > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto sample = [&] { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); };
    if (displaced) {
        const auto table = c.toc(-0x641c);
        c.set(4, table); c.set(3, table + std::uint32_t(dx * 48 + dy * 2));
        c.put16(player + 0x19e, c.u16(c.r(3) + 0x1c8));
    } else {
        bool goal = compare(c, std::int8_t(to_row), 11) == 0;
        if (goal) goal = compare(c, std::int8_t(to_column), 4) == 0;
        if (goal) c.put16(player + 0x19e, 0);
        else {
            goal = compare(c, std::int8_t(to_row)) == 0;
            if (goal) goal = compare(c, std::int8_t(to_column), 4) == 0;
            if (goal) c.put16(player + 0x19e, 180);
            else {
                advance(0x102378fc);
                const auto sum = sample(); const auto division = divide_heading(c, sum, 0xb60b60b7, 8, 360);
                leave(c, {{3, division.sign}, {4, sum}}); c.put16(player + 0x19e, std::uint32_t(division.remainder));
            }
        }
    }
    advance(0x10237964, true);
    auto sum = sample(); auto division = divide_heading(c, sum, 0x88888889, 6, 120);
    leave(c, {{3, std::uint32_t(std::int32_t(std::int8_t(c.r(26))))}, {4, std::uint32_t(division.remainder)},
              {5, division.sign}, {6, sum}});
    if (compare(c, std::int8_t(c.r(26)) * std::int8_t(c.r(25)), division.remainder) < 0) {
        advance(0x102379d4);
        const auto word = c.u16(c.u32(cursor)); sum = std::uint32_t(word) + c.u32(seed);
        const auto next = c.u32(cursor) + 2;
        c.put32(cursor, next); compare_logical(c, next, c.u32(end));
        division = divide_heading(c, sum, 0x88888889, 4, 30);
        keep(c, {{25, std::uint32_t(division.remainder)}});
        leave(c, {{3, division.sign}, {4, std::uint32_t(division.high)}, {5, sum}, {6, word}});
        if (next > c.u32(end)) { c.put32(cursor, begin); c.call(0x1077b770, 0x10237a30); c.put32(seed, c.r(3) & 0xffff); }
        sum = sample(); division = divide_heading(c, sum, 0x88888889, 4, 30);
        leave(c, {{3, std::uint32_t(division.remainder)}, {4, division.sign}, {5, sum}});
        c.put16(player + 0x19e, std::uint32_t(c.i16(player + 0x19e)) + c.r(25) - std::uint32_t(division.remainder));
    }
    const auto profile = c.u32(player + 0x70);
    const auto bias = std::int16_t((c.i8(profile + 0x3b) - c.i8(profile + 0x30)) * 2);
    const auto heading = c.i16(player + 0x19e);
    leave(c, {{3, std::uint32_t(std::int32_t(c.i8(profile + 0x30)))}, {4, std::uint32_t(std::int32_t(heading))}});
    c.put16(player + 0x19e, std::uint32_t(heading + bias));
    auto adjusted = c.i16(player + 0x19e);
    if (compare(c, adjusted) < 0) { c.set(3, std::uint32_t(std::int32_t(adjusted))); c.put16(player + 0x19e, std::uint32_t(adjusted + 360)); }
    else if (compare(c, adjusted, 359) > 0) { c.set(3, std::uint32_t(std::int32_t(adjusted))); c.put16(player + 0x19e, std::uint32_t(adjusted - 360)); }
}
}
