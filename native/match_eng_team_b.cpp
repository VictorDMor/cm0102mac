// MATCH_ENG.CPP, team_b: small queries over a side's players and a player's neighbouring cells.
#include "match_eng_team_b.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
std::int32_t s16(std::uint32_t value) { return std::int16_t(value); }
// The player table's flag byte for a cell: [row 0..8][column 0..11] of 0x5c bytes, plus the side's slot.
std::uint32_t cell_flag(Cpu& c, std::uint32_t match, std::int32_t row, std::int32_t column, std::int32_t slot) {
    return c.u8(match + u(row * 0x450) + u(column * 0x5c) + u(slot) + 0xa60);
}
}  // namespace

// 0x101ee530
std::uint32_t protected_action(Cpu& c, std::uint32_t, std::uint32_t kind) {
    const auto k = s8(kind);
    // The original's comparison tree; cr0 keeps the last compare of the path.
    if (const auto order = compare(c, k, 0x2f); order == 0) return result(c, 1);
    else if (order > 0) {
        const auto high = compare(c, k, 0x60);
        if (high == 0) return result(c, 1);
        if (high > 0) return result(c, compare(c, k, 0x62) < 0);
        const auto mid = compare(c, k, 0x32);
        if (mid == 0) return result(c, 1);
        if (mid > 0) return result(c, 0);
        compare(c, k, 0x31);
        return result(c, 1);
    }
    const auto low = compare(c, k, 0x2b);
    if (low == 0) return result(c, 1);
    if (low > 0) { compare(c, k, 0x2d); return result(c, 1); }
    const auto bottom = compare(c, k, 0x25);
    if (bottom == 0) return result(c, 1);
    if (bottom < 0) return result(c, 0);
    return result(c, compare(c, k, 0x2a) >= 0);
}

// 0x101ee680
std::uint32_t count_players_ahead(Cpu& c, std::uint32_t match, std::uint32_t threshold, std::uint32_t side,
                                  std::uint32_t skip_pending) {
    Frame frame(c, 0x101ee680, 0x60, {.first_gpr = 25});
    c.put8(frame.at(0x87), skip_pending);  // the byte argument lands in the caller's parameter area
    std::uint32_t count = 0, player = match + u(s8(side) * std::int32_t(side_size)) + players_at;
    for (std::uint32_t index = 0; index < 20; ++index, player += player_size) {
        if (s8(position(c, player)) < 0) continue;  // not on the pitch
        if (s8(skip_pending) != 0 && player == c.u32(match + 0xe000)) continue;
        const auto here = s8(column(c, player));
        if (s8(side) == 1) { if (here < s8(threshold)) ++count; }
        else if (s8(side) == 0 && here > s8(threshold)) ++count;
    }
    compare(c, 20, 20);  // the loop's last test
    return result(c, count);
}

// 0x101ee770
std::uint32_t lane_status(Cpu& c, std::uint32_t match, std::uint32_t player) {
    Frame frame(c, 0x101ee770, 0x70, {.first_gpr = 25, .gpr_offset = -0x2c, .first_fpr = 31});
    Fpu fpu(c);
    keep(c, {{28, match}, {31, player}});
    c.set(3, player);
    c.call(0x102721a0, 0x101ee794);
    if (compare(c, s8(c.r(3))) != 0) return result(c, 0);
    const auto column_slot = u(s8(column(c, player)) * 0x5c);
    keep(c, {{25, column_slot}});
    const auto row_slot = u(s8(row(c, player)) * 0x450);
    const auto side = c.u8(player + 0x27);
    // The cell's flag byte for the player's side; a flagged cell is taken.
    if (compare(c, s8(c.u8(match + row_slot + column_slot + u(s8(side) == 0) + 0xa60))) != 0) return result(c, 0);
    c.set(3, player);
    c.call(0x10245160, 0x101ee7fc);
    double reach = fpu.frsp(fpu.fadd(c.f64(c.toc(-0x52c4) + 0x90), c.f(1)));
    c.set_f(31, reach);
    c.set(3, player);
    c.call(0x10237420, 0x101ee814);
    if (compare(c, s8(c.r(3))) == 0) {
        reach = fpu.frsp(fpu.fadd(c.f(31), c.f64(c.toc(-0x52c4) + 0x58)));
        c.set_f(31, reach);
    }
    const auto base = [&](std::uint32_t group) { return match + u(std::int32_t(group) * std::int32_t(side_size)) + players_at; };
    // Opponents first: one at least as near as the reach blocks the lane.
    std::uint32_t other = base(s8(side) == 0 ? 1 : 0);
    keep(c, {{30, other}, {29, 0}});
    for (std::uint32_t index = 0; s8(index) < 20; ++index, other += player_size) {
        const auto previous = other;
        const auto next = [&] { keep(c, {{29, index + 1}, {27, previous}, {30, other + player_size}}); };
        if (compare(c, s8(position(c, other))) < 0) { next(); continue; }
        c.set(3, other);
        c.call(0x10289560, 0x101ee870);
        if (compare(c, s16(c.r(3)), 1) == 0) { next(); continue; }
        const auto toward_high = compare(c, s8(c.u8(player + 0x27))) == 0;
        c.set({{3, other}, {4, 4}, {5, toward_high ? 0xbu : 0u}});
        c.call(0x1022e760, toward_high ? 0x101ee89c : 0x101ee8c0);
        fpu.fcmp(0, c.f(1), c.f(31));
        c.set_cr(0, c.cr(0) | ((c.cr(0) & 8) >> 2));  // cror eq,lt,eq
        if (c.cr(0) & 2) return result(c, 0);
        next();
    }
    compare(c, 20, 20);
    // Then his own side: one nearer than the reach (less) puts him at 2.
    other = base(s8(side));
    keep(c, {{30, other}, {29, 0}});
    for (std::uint32_t index = 0; s8(index) < 20; ++index, other += player_size) {
        const auto previous = other;
        const auto next = [&] { keep(c, {{29, index + 1}, {26, previous}, {30, other + player_size}}); };
        if (compare(c, s8(position(c, other))) < 0) { next(); continue; }
        c.set(3, other);
        c.call(0x10289560, 0x101ee924);
        if (compare(c, s16(c.r(3)), 1) == 0) { next(); continue; }
        const auto toward_high = compare(c, s8(c.u8(player + 0x27))) == 0;
        c.set({{3, other}, {4, 4}, {5, toward_high ? 0xbu : 0u}});
        c.call(0x1022e760, toward_high ? 0x101ee950 : 0x101ee970);
        fpu.fcmp(0, c.f(1), c.f(31));
        if (c.cr(0) & 8) return result(c, 2);
        next();
    }
    compare(c, 20, 20);
    return result(c, 1);
}

// 0x101eeac0
std::uint32_t run_allowed(Cpu& c, std::uint32_t match, std::uint32_t player, std::uint32_t target_row,
                          std::uint32_t target_column) {
    Frame frame(c, 0x101eeac0, 0x50, {.first_gpr = 26});
    Fpu fpu(c);
    keep(c, {{27, match}, {31, player}, {30, target_row}, {29, target_column}});
    c.set(3, player);
    c.call(0x10283690, 0x101eeae8);
    const auto team = c.r(3);
    keep(c, {{28, team}});
    const auto flag = std::uint32_t(s8(team) == 0);
    c.set({{3, match}, {4, flag}, {5, target_row}, {6, target_column}});
    inside_end_area(c, flag, target_row, target_column);
    if (compare(c, s8(c.r(3))) == 0) return result(c, 0);
    if (compare(c, s8(row(c, player)), 4) == 0) return result(c, 0);
    const auto here_row = [&] { return s8(row(c, player)); };
    const auto here_column = [&] { return s8(column(c, player)); };
    // The first test holds for the two sides alike; then each side has its own column limit.
    const auto beside_target = [&](bool low) {
        return compare(c, here_row(), 6) < 0 && compare(c, here_row(), 2) > 0 &&
               (low ? compare(c, s8(target_column), here_column()) < 0 : compare(c, s8(target_column), here_column()) > 0);
    };
    if (compare(c, s8(team)) == 0) {
        if (beside_target(true)) return result(c, 0);
        if (compare(c, here_column(), 9) < 0) return result(c, 0);
    } else {
        if (beside_target(false)) return result(c, 0);
        if (compare(c, here_column(), 2) > 0) return result(c, 0);
    }
    if (compare(c, s8(target_row), 2) > 0 && compare(c, s8(target_row), 6) < 0) {
        c.set(3, u(here_row() - s8(target_row)));
        c.call(0x10775540, 0x101eec40);
        if (compare(c, std::int32_t(c.r(3)), 1) > 0) {
            keep(c, {{26, column(c, player)}});
            leave(c, {{3, match}, {4, target_row}, {5, target_column}, {6, row(c, player)}, {7, c.r(26)}});
            c.call(0x101ef8d0, 0x101eec74);
            fpu.fcmp(0, c.f(1), c.f64(c.toc(-0x52c4) + 0x1b8));
            return result(c, (c.cr(0) & 4) != 0);
        }
    }
    return result(c, 0);
}

// 0x101eecb0
std::uint32_t adjacent_cell_free(Cpu& c, std::uint32_t match, std::uint32_t player) {
    Frame frame(c, 0x101eecb0, 0x50, {.first_gpr = 27});
    keep(c, {{29, match}, {28, player}});
    const auto raw_row = row(c, player);
    keep(c, {{30, raw_row}});
    c.set(3, player);
    c.call(0x10283690, 0x101eecdc);
    keep(c, {{27, std::uint32_t(s8(c.r(3)) == 0)}});
    c.set(3, player);
    c.call(0x10283690, 0x101eecf4);
    const auto towards_goal_line = compare(c, s8(c.r(3)), 1) == 0;
    std::int32_t beside;
    if (towards_goal_line) {
        beside = s8(column(c, player) - 1u);
        if (compare(c, beside) < 0) return result(c, 0);
    } else {
        beside = s8(column(c, player) + 1u);
        if (compare(c, beside, 0xb) > 0) return result(c, 0);
    }
    const auto slot = s8(c.r(27));
    const auto here = s8(raw_row);
    if (compare(c, here, 8) < 0 && compare(c, s8(cell_flag(c, match, here + 1, beside, slot))) == 0) return result(c, 1);
    if (compare(c, here) > 0 && compare(c, s8(cell_flag(c, match, here - 1, beside, slot))) == 0) return result(c, 1);
    return result(c, compare(c, s8(cell_flag(c, match, here, beside, slot))) == 0);
}

// 0x101eee20
std::uint32_t count_side_players(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101eee20, 0x50, {.first_gpr = 26});
    std::uint32_t count = 0, player = match + u(s8(side) * std::int32_t(side_size)) + players_at;
    for (std::uint32_t index = 0; index < 20; ++index, player += player_size)
        if (compare(c, s8(position(c, player))) >= 0) ++count;
    compare(c, 20, 20);
    c.put8(match + u(s8(side)) + 4, count);
    return result(c, count);
}
}
