// MATCH_ENG.CPP's call-free routines too long to read as one line.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101d3ad0: a side's ratio of two doubles, or the pool's fallback when the
// divisor equals the pool's zero. The red-zone save of r31 stays on the stack.
double side_rate(Cpu& c, std::uint32_t record, std::uint32_t second_side) {
    c.put32(c.sp() - 4, c.r(31));
    const auto pool = c.toc(-0x52c4);
    guest::Fpu fpu(c);
    const auto divided = [&](std::uint32_t dividend_at, std::uint32_t divisor_at, double& rate) {
        const double divisor = c.f64(divisor_at);
        fpu.fcmp(0, divisor, c.f64(pool + 0x50));
        if (c.cr(0) & 2) return false;
        rate = fpu.fdiv(c.f64(dividend_at), divisor);
        return true;
    };
    double rate = 0;
    bool done = false;
    if (compare(c, s8(second_side)) != 0) done = divided(record + 0x20, record + 0x28, rate);
    if (!done && compare(c, s8(second_side)) == 0) done = divided(record + 0x10, record + 0x18, rate);
    if (!done) rate = c.f64(pool + 0x68);
    c.set_f(1, rate);
    return rate;
}

// 0x101d3bf0: whether a fixture flag byte (+0x40 first side, +0x41 second) is zero.
std::uint32_t fixture_flag_40(Cpu& c, std::uint32_t match, std::uint32_t second_side) {
    c.put32(c.sp() - 4, c.r(31));
    const bool second = compare(c, s8(second_side)) != 0;
    const auto record = c.u32(match + fixture_at);
    c.set(5, record);
    return result(c, u(c.i8(record + (second ? 0x41 : 0x40)) == 0));
}

// 0x101d3c50: a fixture team pointer (+0x1c first side, +0x20 second).
std::uint32_t fixture_team(Cpu& c, std::uint32_t match, std::uint32_t second_side) {
    c.put32(c.sp() - 4, c.r(31));
    const bool second = compare(c, s8(second_side)) != 0;
    const auto record = c.u32(match + fixture_at);
    c.set(5, record);
    return result(c, c.u32(record + (second ? 0x20 : 0x1c)));
}

// 0x101d5700: appends to a player's three-slot marker list.
void append_marker(Cpu& c, std::uint32_t player, std::uint32_t marker) {
    if (compare(c, c.i8(player + 0x104), 3) >= 0) return;
    const auto count = c.u8(player + 0x104);
    c.put8(player + 0x104, count + 1u);
    const auto slot = u(s8(count)) * 4;
    c.set(5, slot);
    c.put32(player + slot + 0x1b4, marker);
}

// 0x101d57d0: stores the adjustment, then clamps it to [-200, 200] in memory.
void set_pressure_adjustment(Cpu& c, std::uint32_t player, std::uint32_t amount) {
    c.put16(player + 0x1a0, amount);
    const auto at = player + 0x1a0;
    if (compare(c, c.i16(at), 0xc8) > 0) c.put16(at, 0xc8);
    else if (compare(c, c.i16(at), -0xc8) < 0) c.put16(at, u(-0xc8));
}

// 0x101d68f0: records a pending restart unless one is already held. Its last two
// bytes come from the caller's argument area.
void set_pending_restart(Cpu& c, std::uint32_t match, std::uint32_t kind, std::uint32_t player, std::uint32_t other,
                         std::uint32_t mode, std::uint32_t flag, std::uint32_t row, std::uint32_t column) {
    if (compare_logical(c, c.u32(match + 0x30e4)) != 0) return;
    c.put16(match + 0x30e2, kind);
    c.put32(match + 0x30e4, player);
    c.put32(match + 0x30e8, other);
    c.put8(match + 0x30ec, mode);
    c.put8(match + 0x30dc, flag);
    c.put8(match + restart_row, row);
    c.put8(match + restart_column, column);
    c.put8(match + 0x30df, c.u8(c.sp() + 0x3b));
    c.put8(match + 0x30e0, c.u8(c.sp() + 0x3f));
}

// 0x101d6930: the pending restart's empty state.
void clear_pending_restart(Cpu& c, std::uint32_t match) {
    c.put16(match + 0x30e2, 0xffff);
    c.put32(match + 0x30e4, 0);
    c.put32(match + 0x30e8, 0);
    c.put8(match + 0x30ec, 0);
    c.put8(match + 0x30dc, 0);
    c.put8(match + restart_row, 0xff);
    c.put8(match + restart_column, 0xff);
    c.put8(match + 0x30df, 0xff);
    c.put8(match + 0x30e0, 0xff);
}

// 0x101d6980: the side opposing the pending restart's player, or -1.
std::uint32_t pending_restart_side(Cpu& c, std::uint32_t match) {
    if (compare(c, c.i8(match + 0x30dc)) == 0 || compare_logical(c, c.u32(match + 0x30e4)) == 0) return result(c, u(-1));
    const auto player = c.u32(match + 0x30e4);
    c.set(4, player);
    return result(c, c.i8(player + 0x27) == 0);
}

// 0x101da6d0: whether signed byte +0x1b is nonzero; r4 keeps its value.
std::uint32_t nonzero_byte_1b(Cpu& c, std::uint32_t object) {
    const auto value = u(c.i8(object + 0x1b));
    c.set(4, value);
    return result(c, value != 0);
}

// 0x101da6f0: whether r4 is a record in the last 16 ids with +0x6b set.
std::uint32_t valid_competition_record(Cpu& c, std::uint32_t record) {
    if (compare_logical(c, record) == 0) return result(c, 0);
    const auto limit_at = c.toc(-0x7be4);
    const auto id = c.i32(record);
    c.set(5, u(id));
    const auto limit = c.i32(limit_at);
    if (compare(c, id, std::int32_t(u(limit) - 0x10)) < 0) return result(c, 0);
    if (compare(c, id, limit) >= 0) return result(c, 0);
    return result(c, compare(c, c.i8(record + 0x6b)) != 0);
}

// 0x101e15e0: moves +0x36 by five toward a factor-scaled pool constant, comparing
// through the stack conversion the original leaves in its red zone.
void adjust_score_toward(Cpu& c, std::uint32_t player, double factor) {
    const auto at = player + 0x36;
    const auto flipped = u(c.i16(at)) ^ 0x80000000u;
    const double bias = c.f64(c.toc(-0x52c4) + 0x60);
    c.put32(c.sp() - 4, flipped);
    c.put32(c.sp() - 8, 0x43300000);
    guest::Fpu fpu(c);
    const double value = fpu.fsubs(c.f64(c.sp() - 8), bias);
    const double limit = fpu.fmuls(double(c.f32(c.toc(-0x52c8) + 0x94)), factor);
    fpu.fcmp(0, value, limit);
    const auto before = u(c.i16(at));
    c.set(3, before);
    // ble: not greater, unordered included.
    c.put16(at, c.cr(0) & 4 ? before - 5 : before + 5);
}

// 0x101e3150: whether signed byte +0x27 equals one. r4 keeps the byte.
std::uint32_t second_side_player(Cpu& c, std::uint32_t player) {
    const auto side = u(c.i8(player + 0x27));
    c.set(4, side);
    return result(c, side == 1);
}

// 0x101e4ea0: how far a column lies past the side's limit.
std::uint32_t column_excess(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t column) {
    if (compare(c, s8(side), 1) == 0) return result(c, u(s8(c.u8(match + 0x7926) - column)));
    return result(c, u(s8(column - c.u8(match + 0x7925))));
}

// 0x101e5080: the record's text pointer, or the pool's empty text.
std::uint32_t player_text(Cpu& c, std::uint32_t record) {
    c.put32(c.sp() - 4, c.r(31));
    if (compare_logical(c, c.u32(record + 0x30)) != 0) return result(c, c.u32(record + 0x30));
    const auto pool = c.toc(-0x52cc);
    c.set(4, pool);
    return result(c, pool + 0x3e8);
}

// 0x101ee610: the turn cost table entry for a move, or the default when none.
std::uint32_t turn_cost(Cpu& c, std::uint32_t match, std::uint32_t from_row, std::uint32_t from_column,
                        std::uint32_t to_row, std::uint32_t to_column) {
    c.put32(c.sp() - 4, c.r(31));
    c.put32(c.sp() - 8, c.r(30));
    const auto rows = s8(to_row - from_row), columns = s8(to_column - from_column);
    if (compare(c, rows) == 0 && compare(c, columns) == 0) return result(c, u(c.i16(match + 0x7950)));
    const auto table = c.toc(-0x641c);
    const auto across = u(rows + 9) * 0x30;
    c.set(8, u(columns));
    c.set(9, across);
    c.set(10, table);
    return result(c, u(c.i16(table + across + (u(columns + 12) << 1))));
}

// 0x101ee9c0: whether a row lies outside 2..6.
std::uint32_t outside_central_rows(Cpu& c, std::uint32_t row) {
    if (compare(c, s8(row), 2) < 0) return result(c, 1);
    return result(c, compare(c, s8(row), 6) > 0);
}

// 0x101ee9f0: whether a side's column lies past its attacking limit.
std::uint32_t beyond_column(Cpu& c, std::uint32_t side, std::uint32_t column) {
    if (compare(c, s8(side), 1) == 0 && compare(c, s8(column), 3) < 0) return result(c, 1);
    if (compare(c, s8(side)) != 0) return result(c, 0);
    return result(c, compare(c, s8(column), 8) > 0);
}

// 0x101eea30: whether a cell is in the end area a side defends.
std::uint32_t inside_end_area(Cpu& c, std::uint32_t side, std::uint32_t row, std::uint32_t column) {
    const auto low = compare(c, s8(side), 1) == 0 ? 0xa : 0, high = low + 1;
    if (compare(c, s8(row), 2) < 0 || compare(c, s8(row), 6) > 0) return result(c, 0);
    if (compare(c, s8(column), low) < 0) return result(c, 0);
    return result(c, compare(c, s8(column), high) <= 0);
}

// 0x101f14b0: records the last touching side and updates the two side tallies.
void set_last_touch_side(Cpu& c, std::uint32_t match, std::uint32_t side) {
    c.put8(match + last_touch_side, side);
    if (compare(c, s8(side), -1) == 0) return;
    const auto own = u(s8(side)) + 0x77bc;
    c.put8(match + own, c.u8(match + own) + 1u);
    const auto other_side = u(s8(side) == 0);
    c.set(5, other_side);
    c.set(6, own);
    if (compare(c, c.i8(match + other_side + 0x77bc)) <= 0) return;
    const auto other = other_side + 0x77bc;
    const auto before = c.u8(match + other);
    c.set(5, before);
    c.set(6, other);
    c.put8(match + other, before - 10u);
}

// 0x101f1de0: forgets a side's cached goalkeeper.
void clear_goalkeeper(Cpu& c, std::uint32_t match, std::uint32_t side) {
    const auto high = match + 0x10000;
    const bool second = compare(c, s8(side)) != 0;
    c.set(5, high);
    c.put32(high - (second ? 0x1fe0 : 0x1fe4), 0);
}

// 0x101fcde0: fixture flag 0x80, except at the two period boundary clocks.
std::uint32_t scheduled_flag(Cpu& c, std::uint32_t match) {
    c.put32(c.sp() - 4, c.r(31));
    c.put32(c.sp() - 8, c.r(30));
    bool active = compare(c, c.i16(match + period_end), 0x1ef) != 0 && compare(c, c.i16(match + period_end), 0x3de) != 0;
    if (active) active = compare(c, 1) != 0;
    else compare(c, 0);
    if (!active) return result(c, 0);
    const auto record = c.u32(match + fixture_at);
    c.set(4, record);
    return result(c, compare(c, c.i16(record + 0x38) & 0x80) != 0);
}

// 0x101fce40: whether the fixture id is -2, the statistics fixture.
std::uint32_t is_statistics_fixture(Cpu& c, std::uint32_t match) {
    const auto id = c.u32(c.u32(match + fixture_at));
    c.set(4, id);
    return result(c, id == 0xfffffffe);
}

// 0x101fdb20: signed halfword +8 divided by 1000, by the compiler's multiply.
std::uint32_t thousands(Cpu& c, std::uint32_t object) {
    const auto value = std::int64_t(c.i16(object + 8)) * 0x10624dd3;
    const auto high = std::int32_t(value >> 32) >> 6;
    const auto sign = u(high) >> 31;
    c.set(4, sign);
    return result(c, u(std::int16_t(u(high) + sign)));
}

// 0x101fdb50: the list's first word, or -1 without a list.
std::uint32_t first_of_list(Cpu& c, std::uint32_t list) {
    if (compare_logical(c, c.u32(list + 4)) == 0) return result(c, u(-1));
    const auto head = c.u32(list + 4);
    c.set(4, head);
    return result(c, c.u32(head));
}

// 0x101fe9f0: copies a five-byte cell record field by field.
void copy_cell_record(Cpu& c, std::uint32_t to, std::uint32_t from) {
    c.put16(to, c.u16(from));
    c.put16(to + 2, c.u16(from + 2));
    c.put8(to + 4, c.u8(from + 4));
}

// 0x101fea90: whether the clock passed eleven minutes per the side's byte.
std::uint32_t past_substitution_time(Cpu& c, std::uint32_t match) {
    const auto high = match + 0x10000;
    c.set(4, high);
    const auto quarter = s8(c.u8(high - 0x1f48));
    if (compare(c, quarter, -1) == 0) return result(c, 0);
    const auto limit = std::int16_t(quarter * 0xb);
    c.set(4, u(limit));
    return result(c, compare(c, limit, c.i16(match + clock_at)) < 0);
}
}
