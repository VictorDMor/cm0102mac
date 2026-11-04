// MATCH_ENG.CPP, space: target cells, nearest players and the team's depth limits.
#include "match_eng_space.hpp"
#include "match_pl.hpp"
#include "match_pl_adapter.hpp"
#include "match_turbo.hpp"
#include "routine_check.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
// Match object fields.
constexpr std::uint32_t line_after_side1 = 0x7925; // one past the last column of side 1's rearmost player
constexpr std::uint32_t line_before_side0 = 0x7926;
constexpr std::uint32_t tactics_mode = 0x7927;
constexpr std::uint32_t formation_tables = 0xdfec; // per side: pointer to the cell table
constexpr std::uint32_t flag_bytes = 6;            // per side
// Player fields.
constexpr std::uint32_t player_side = 0x27;
constexpr std::uint32_t player_restricted = 0x2c;

// Team tactical flags.
constexpr std::uint32_t pushes_up = 0x10000, drops_back = 0x8000, wide_play = 0x4000;

std::int16_t s16(std::uint32_t value) { return std::int16_t(value); }

// The pool entries the distance routines read: the double 0 ("on the cell") at +0x50.
std::uint32_t pool(Cpu& c) { return c.toc(-0x52c4); }

// A side's first player.
std::uint32_t first_player(std::uint32_t match, std::uint32_t side) { return match + u(s8(side) * std::int32_t(side_size)) + players_at; }

std::uint32_t team_of_side(std::uint32_t match, std::uint32_t side) {
    return match + u(s8(side) * std::int32_t(team_tactics_size)) + team_tactics;
}

bool direct_spatial_call(const Cpu& c, std::uint32_t entry) {
    // These queries preserve their guest frames and cannot transfer control
    // outside the call. Keep the dispatcher when diagnostics can intervene.
    const auto& memory = c.s.memory;
    return !dispatch_probe && !routine_checker && !memory.observer && !memory.date_observer &&
           !memory.read_trace && !match_pc_profile_enabled() && match_pl_route_allowed(entry);
}

// 0x10289560 (another module) says whether the player is a goalkeeper-like one: 1.
std::int16_t kind_of(Cpu& c, std::uint32_t player, std::uint32_t back) {
    constexpr std::uint32_t entry = 0x10289560;
    c.set(3, player);
    if (!direct_spatial_call(c, entry)) {
        c.call(entry, back);
        return s16(c.r(3));
    }
    c.link(back);
    c.s.pc = entry;
    const auto role = match_pl::current_role(c, player);
    c.finish();
    return s16(role);
}

// The loops over a side's 20 players, called with the distance routine 0x1022e760.
double distance_to_cell(Cpu& c, std::uint32_t player, std::uint32_t target_row, std::uint32_t target_column, std::uint32_t back) {
    constexpr std::uint32_t entry = 0x1022e760;
    c.set({{3, player}, {4, target_row}, {5, target_column}});
    if (!direct_spatial_call(c, entry)) {
        c.call(entry, back);
        return c.f(1);
    }
    c.link(back);
    c.s.pc = entry;
    const double distance = match_pl::distance_to(c, player, std::int8_t(target_row), std::int8_t(target_column));
    c.finish();
    return distance;
}
}  // namespace

// ---------------------------------------------------------------------------
// 0x101e3060
std::uint32_t side_has_ball(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101e3060, 0x40, {.first_gpr = 30});
    keep(c, {{31, match}, {30, side}});
    const auto last = s8(c.u8(match + possession_side));
    const auto other = std::uint32_t(s8(side) == 0);
    if (compare(c, last, std::int32_t(other)) == 0) return result(c, 0);
    if (compare(c, s8(side), last) == 0) return result(c, 1);
    const auto holder = c.u32(match + ball_holder);
    if (compare_logical(c, holder) != 0 && compare(c, s8(c.u8(holder + player_side)), s8(side)) == 0) return result(c, 1);
    c.set({{3, match}, {4, other}, {5, c.u8(match + ball_row_at)}, {6, c.u8(match + ball_column_at)}});
    call(c, 0x101eea30, 0x101e3110);
    if (compare(c, s8(c.r(3))) != 0 && compare(c, s8(c.u8(match + shot_state)), -1) != 0) return result(c, 1);
    return result(c, 0);
}

// ---------------------------------------------------------------------------
// 0x101e28b0
void home_cell(Cpu& c, std::uint32_t match, std::uint32_t player, std::uint32_t row_at, std::uint32_t column_at,
               std::uint32_t flag) {
    Frame frame(c, 0x101e28b0, 0x80, {.first_gpr = 16});
    c.put8(frame.at(0xab), flag);
    // The ball's cell, clamped onto the pitch (the bytes keep their unsigned value when in range).
    auto ball_row = c.u8(match + ball_row_at);
    auto ball_column = c.u8(match + ball_column_at);
    if (compare(c, s8(ball_row)) < 0) ball_row = 0;
    if (compare(c, s8(ball_row), 9) >= 0) ball_row = 8;
    if (compare(c, s8(ball_column)) < 0) ball_column = 0;
    if (compare(c, s8(ball_column), 12) >= 0) ball_column = 11;
    keep(c, {{24, ball_column}, {25, ball_row}, {27, player}, {29, match}, {30, row_at}, {31, column_at}});
    const auto side = second_side_player(c, player);
    keep(c, {{28, side}});
    c.set(3, player);
    c.call(0x10283690, 0x101e2930);
    enter(c, 0x101e293c, {match, c.r(3)});
    const auto has_ball = side_has_ball(c, match, c.r(4));
    keep(c, {{26, has_ball}});

    // The table cell for the ball's cell: the low nibble is the row, the high nibble the column.
    const auto table_cell = [&](bool column_nibble) {
        const auto kind = s8(position(c, player));
        const auto table = c.u32(match + formation_tables + (u(s8(side)) << 2));
        const auto offset = u(kind * 0xd8) + u(s8(has_ball) * 0x6c) + u(s8(ball_row) * 0xc);
        const auto cell = c.u8(table + offset + u(s8(ball_column)));
        c.set({{3, table}, {4, offset}});
        return column_nibble ? (cell & 0xf0) >> 4 : cell & 0xf;
    };
    const auto clamp = [&] {
        if (compare(c, s8(c.u8(column_at))) < 0) c.put8(column_at, 0);
        if (compare(c, s8(c.u8(column_at)), 12) >= 0) c.put8(column_at, 11);
        if (compare(c, s8(c.u8(row_at))) < 0) c.put8(row_at, 0);
        if (compare(c, s8(c.u8(row_at)), 9) >= 0) c.put8(row_at, 8);
    };
    const auto step_column = [&](std::int32_t delta) { c.put8(column_at, c.u8(column_at) + u(delta)); };
    const auto cell_to_table = [&] {
        c.put8(row_at, table_cell(false));
        c.put8(column_at, table_cell(true));
    };
    const auto tactical = [&] { return tactical_flags(c, team_of_side(match, side)); };

    const auto mode = c.u8(match + tactics_mode);
    c.set({{3, mode}, {4, u(s8(mode))}});
    if (compare(c, s8(mode)) > 0) {
        // The player stays on a cell the game fixes (two other modules decide), else the table's.
        bool own_cell = false;
        if (compare(c, s8(has_ball)) != 0) {
            c.set(3, player);
            c.call(0x102860c0, 0x101e2964);
            own_cell = compare(c, s8(c.r(3))) != 0;
        }
        if (!own_cell && compare(c, s8(has_ball)) == 0) {
            c.set(3, player);
            c.call(0x102861b0, 0x101e29a8);
            own_cell = compare(c, s8(c.r(3))) != 0;
        }
        if (own_cell) {
            c.put8(row_at, row(c, player));
            c.put8(column_at, column(c, player));
        } else {
            cell_to_table();
        }
        clamp();
        return;
    }

    cell_to_table();
    if (compare(c, kind_of(c, player, 0x101e2b9c), 1) == 0) return;
    c.set({{3, match}, {4, side}});
    call(c, 0x101d5760, 0x101e2bb4);
    if (compare(c, s8(c.r(3))) != 0 && compare(c, s8(c.u8(match + flag_bytes + s8(side)))) == 0) {
        // The team holds its shape: one column up or down by side.
        step_column(compare(c, s8(side)) != 0 ? 1 : -1);
        clamp();
    } else if (compare(c, tactical() & pushes_up) != 0 && compare(c, s8(has_ball)) != 0) {
        step_column(compare(c, s8(side)) != 0 ? -1 : 1);
        clamp();
    } else {
        if (compare(c, tactical() & drops_back) != 0 && compare(c, s8(has_ball)) == 0) {
            step_column(compare(c, s8(side)) != 0 ? 1 : -1);
            clamp();
        }
    }
    const auto flags = tactical();
    if (compare(c, flags & wide_play) == 0) return;
    const auto stored = [&] { return std::int32_t(s8(c.u8(column_at))); };
    const auto ball = [&] { return s8(c.u8(match + ball_column_at)); };
    if (compare(c, s8(has_ball)) != 0 && compare(c, s8(c.u8(match + flag_bytes + s8(side)))) == 0) {
        c.set(3, c.u8(column_at));
        if (compare(c, s8(side)) != 0) {
            step_column(2);
            c.put8(column_at, compare(c, stored(), 0xb) > 0 ? 0xb : c.u8(column_at));
        } else {
            step_column(-2);
            c.put8(column_at, compare(c, stored()) < 0 ? 0 : c.u8(column_at));
        }
        return;
    }
    if (compare(c, s8(side)) != 0) {
        // Beyond the ball: at least two columns past it, at most four, and on the pitch.
        c.set(3, u(ball() + 2));
        auto value = stored();
        if (compare(c, ball() + 2, value) > 0) { c.set(3, u(ball())); value = ball() + 2; }
        c.put8(column_at, u(value));
        value = stored();
        if (compare(c, value, 9) < 0) value = 9;
        c.put8(column_at, u(value));
        c.set(3, u(ball() + 4));
        value = stored();
        if (compare(c, ball() + 4, value) < 0) { c.set(3, u(ball())); value = ball() + 4; }
        c.put8(column_at, u(value));
        value = stored();
        if (compare(c, value, 0xb) > 0) value = 0xb;
        c.put8(column_at, u(value));
        return;
    }
    // Behind the ball: at most two columns before it, at least four before it, and on the pitch.
    c.set(3, u(ball() - 2));
    auto value = stored();
    if (compare(c, ball() - 2, value) < 0) { c.set(3, u(ball())); value = ball() - 2; }
    c.put8(column_at, u(value));
    value = stored();
    c.set(3, u(value));
    if (compare(c, value, 2) > 0) value = 2;
    c.set(4, u(value));
    c.put8(column_at, u(value));
    c.set(3, u(ball()));
    c.set(4, u(ball() - 4));
    value = stored();
    c.set(3, u(value));
    if (compare(c, ball() - 4, value) > 0) {
        c.set(3, u(ball()));
        c.put32(frame.at(0x3c), u(ball() - 4));
    } else {
        c.set(3, c.u8(column_at));
        c.put32(frame.at(0x3c), u(stored()));
    }
    c.set(3, u(s8(c.u32(frame.at(0x3c)))));
    c.put8(column_at, c.r(3));
    value = stored();
    c.set(3, u(value));
    if (compare(c, value) < 0) {
        c.put32(frame.at(0x38), 0);
    } else {
        c.set(3, c.u8(column_at));
        c.put32(frame.at(0x38), u(stored()));
    }
    c.set(3, c.u32(frame.at(0x38)));
    c.put8(column_at, c.r(3));
}

// ---------------------------------------------------------------------------
// 0x101e31a0
std::uint32_t is_nearest_to_cell(Cpu& c, std::uint32_t match, std::uint32_t player, std::uint32_t target_row,
                                 std::uint32_t target_column) {
    Frame frame(c, 0x101e31a0, 0x70, {.first_gpr = 25, .first_fpr = 30});
    Fpu fpu(c);
    c.put32(frame.at(0x88), match);
    keep(c, {{30, player}, {25, target_row}, {26, target_column}});
    c.set(3, player);
    c.call(0x10283690, 0x101e31d0);
    keep(c, {{28, c.r(3)}});
    const double own = distance_to_cell(c, player, target_row, target_column, 0x101e31e4);
    c.set_f(31, own);
    fpu.fcmp(0, c.f64(pool(c) + 0x50), own);
    // The player stands on the cell: a lone one is the nearest.
    if ((c.cr(0) & 2) && compare(c, kind_of(c, player, 0x101e3200), 1) != 0) return result(c, 1);
    c.set(4, c.u32(frame.at(0x88)));
    Kept<31> other(c, first_player(c.u32(frame.at(0x88)), c.r(28)));
    Kept<29> index(c, 0);
    for (; compare(c, s8(index), 0x14) < 0; index = index + 1, keep(c, {{27, other}}), other = other + player_size) {
        if (compare(c, s8(position(c, other))) < 0) continue;
        if (compare_logical(c, player, other) == 0) continue;
        if (compare(c, s8(c.u8(other + player_restricted))) != 0) continue;
        const double there = distance_to_cell(c, other, target_row, target_column, 0x101e326c);
        c.set_f(30, there);
        fpu.fcmp(0, there, own);
        if (c.cr(0) & 8) return result(c, 0);
        if (compare(c, kind_of(c, player, 0x101e3280), 1) != 0) continue;
        fpu.fcmp(0, there, own);
        if (c.cr(0) & 2) return result(c, 0);
    }
    return result(c, 1);
}

// ---------------------------------------------------------------------------
// 0x101e32e0
std::uint32_t nearest_player_to_cell(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t target_row,
                                     std::uint32_t target_column, std::uint32_t any_condition) {
    Frame frame(c, 0x101e32e0, 0x60, {.first_gpr = 26, .first_fpr = 30});
    Fpu fpu(c);
    c.put32(frame.at(0x78), match);
    c.put8(frame.at(0x7f), side);
    keep(c, {{26, target_row}, {27, target_column}, {28, any_condition}, {29, pool(c)}});
    Kept<30> best(c, 0);
    c.set_f(30, c.f32(c.toc(-0x52c8) + 0xc8));  // the farthest distance counted
    c.set(4, match);
    Kept<31> other(c, first_player(c.u32(frame.at(0x78)), c.u8(frame.at(0x7f))));
    for (unsigned n = 0; n < 20; ++n, other = other + player_size) {
        // The unrolled loop's calls return to a different address in each copy.
        if (compare(c, s8(position(c, other))) < 0) continue;
        if (compare(c, kind_of(c, other, 0x101e3350 + 0x80 * n), 1) == 0) continue;
        if (compare(c, s8(any_condition)) == 0 && compare(c, s8(c.u8(other + player_restricted)), 1) > 0) continue;
        const double there = distance_to_cell(c, other, target_row, target_column, 0x101e3388 + 0x80 * n);
        c.set_f(31, there);
        fpu.fcmp(0, c.f64(pool(c) + 0x50), there);
        if (c.cr(0) & 2) return result(c, other);
        fpu.fcmp(0, there, c.f(30));
        if (c.cr(0) & 8) { best = other; c.set_f(30, there); }
    }
    return result(c, best);
}

// ---------------------------------------------------------------------------
// 0x101e3d50, 0x101e3de0, 0x101e3e70
namespace {
std::uint32_t side_table_value(Cpu& c, std::uint32_t entry, std::uint32_t match, std::uint32_t player, std::uint32_t table,
                               bool previous) {
    Frame frame(c, entry, 0x40, {.first_gpr = 30});
    keep(c, {{31, player}, {30, match}});
    const auto kind = [&] { return s8(previous ? previous_position(c, player) : position(c, player)); };
    const bool observed = c.s.memory.date_observer || c.s.memory.read_trace;
    const auto initial = kind();
    if (compare(c, initial) < 0) return result(c, 0);
    // Read observers may replace the live position between the original loads.
    if (compare(c, observed ? kind() : initial, 10) > 0) return result(c, 0);
    const auto offset = u(observed ? kind() : initial) << 1;
    c.set(4, offset);
    const auto values = c.u32(match + (u(s8(c.u8(player + player_side))) << 2) + table);
    return result(c, u(c.i16(values + offset)));
}
}  // namespace

std::uint32_t position_value_a(Cpu& c, std::uint32_t match, std::uint32_t player) {
    return side_table_value(c, 0x101e3d50, match, player, 0x7934, false);
}
std::uint32_t position_value_b(Cpu& c, std::uint32_t match, std::uint32_t player) {
    return side_table_value(c, 0x101e3de0, match, player, 0x793c, false);
}
std::uint32_t previous_position_value_a(Cpu& c, std::uint32_t match, std::uint32_t player) {
    return side_table_value(c, 0x101e3e70, match, player, 0x7934, true);
}

// ---------------------------------------------------------------------------
// 0x101e3f10
double nearest_distance(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t target_row,
                        std::uint32_t target_column, std::uint32_t any_condition) {
    Frame frame(c, 0x101e3f10, 0x60, {.first_gpr = 27, .first_fpr = 30});
    Fpu fpu(c);
    c.put32(frame.at(0x78), match);
    c.put8(frame.at(0x7f), side);
    c.put8(frame.at(0x83), target_row);
    c.put8(frame.at(0x87), target_column);
    c.put8(frame.at(0x8b), any_condition);
    const auto constants = c.toc(-0x52c8);
    keep(c, {{31, constants}, {30, pool(c)}});
    c.set_f(31, c.f32(constants + 0xc8));
    c.set(4, c.u32(frame.at(0x78)));
    Kept<29> other(c, first_player(c.u32(frame.at(0x78)), c.u8(frame.at(0x7f))));
    Kept<28> index(c, 0);
    for (; compare(c, s8(index), 0x14) < 0; index = index + 1, keep(c, {{27, other}}), other = other + player_size) {
        if (compare(c, s8(position(c, other))) < 0) continue;
        if (compare(c, kind_of(c, other, 0x101e3f84) & 1) != 0) continue;
        if (compare(c, s8(c.u8(frame.at(0x8b)))) == 0 && compare(c, s8(c.u8(other + player_restricted))) != 0) continue;
        const double there = distance_to_cell(c, other, c.u8(frame.at(0x83)), c.u8(frame.at(0x87)), 0x101e3fc4);
        c.set_f(30, there);
        fpu.fcmp(0, c.f64(pool(c) + 0x50), there);
        if (c.cr(0) & 2) {  // standing on the cell
            const double none = c.f32(constants + 0xc);
            c.set_f(1, none);
            return none;
        }
        fpu.fcmp(0, there, c.f(31));
        if (c.cr(0) & 8) c.set_f(31, there);
    }
    c.set_f(1, c.f(31));
    return c.f(31);
}

// ---------------------------------------------------------------------------
// 0x101e4020
std::uint32_t count_nearby_players(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t target_row,
                                   std::uint32_t target_column, std::uint32_t any_condition) {
    // f30 has no slot: the frame saves f31 above a gap the size of one double.
    Frame frame(c, 0x101e4020, 0x60, {.first_gpr = 27, .gpr_offset = -0x24, .first_fpr = 31});
    Fpu fpu(c);
    c.put32(frame.at(0x78), match);
    c.put8(frame.at(0x7f), side);
    c.put8(frame.at(0x83), target_row);
    c.put8(frame.at(0x87), target_column);
    c.put8(frame.at(0x8b), any_condition);
    keep(c, {{31, pool(c)}});
    Kept<29> count(c, 0);
    c.set(4, c.u32(frame.at(0x78)));
    Kept<30> other(c, first_player(c.u32(frame.at(0x78)), c.u8(frame.at(0x7f))));
    Kept<28> index(c, 0);
    for (; compare(c, s8(index), 0x14) < 0; index = index + 1, keep(c, {{27, other}}), other = other + player_size) {
        if (compare(c, s8(position(c, other))) < 0) continue;
        if (compare(c, kind_of(c, other, 0x101e408c), 1) == 0) continue;
        if (compare(c, s8(c.u8(frame.at(0x8b)))) == 0 && compare(c, s8(c.u8(other + player_restricted))) != 0) continue;
        const double there = distance_to_cell(c, other, c.u8(frame.at(0x83)), c.u8(frame.at(0x87)), 0x101e40c8);
        c.set_f(31, there);
        fpu.fcmp(0, c.f64(pool(c) + 0x50), there);
        if (c.cr(0) & 2) { count = count + 3; continue; }
        fpu.fcmp(0, there, c.f64(pool(c) + 0x1b8));
        c.set_cr(0, std::uint8_t(c.cr(0) | (c.cr(0) & 8 ? 2 : 0)));  // cror eq,lt,eq: within range
        if (c.cr(0) & 2) count = count + 1;
    }
    return result(c, count);
}

// ---------------------------------------------------------------------------
// 0x101e4130
std::uint32_t update_line_limits(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101e4130, 0x50, {.first_gpr = 29});
    keep(c, {{29, match}});
    // Side 1's rearmost column is the greatest one, side 0's the least; the loops are unrolled,
    // so each copy's call returns to its own address.
    const auto scan = [&](std::uint32_t side, std::uint32_t start, bool greatest, std::uint32_t back) {
        Kept<30> limit(c, start);
        Kept<31> other(c, first_player(match, side));
        for (unsigned n = 0; n < 20; ++n, other = other + player_size) {
            if (compare(c, s8(position(c, other))) < 0) continue;
            if (compare(c, kind_of(c, other, back + 0x50 * n), 1) == 0) continue;
            const auto here = s8(column(c, other));
            if (compare(c, here, s8(limit)) * (greatest ? 1 : -1) <= 0) continue;
            limit = column(c, other);
        }
        return std::uint32_t(limit);
    };
    c.put8(match + line_after_side1, scan(1, 6, true, 0x101e4170) + 1);
    const auto least = scan(0, 5, false, 0x101e47c0);
    c.put8(match + line_before_side0, least - 1);
    return result(c, u(s8(least - 1)));
}

// ---------------------------------------------------------------------------
// 0x101e4e10
std::uint32_t distance_behind_line(Cpu& c, std::uint32_t match, std::uint32_t player) {
    Frame frame(c, 0x101e4e10, 0x50, {.first_gpr = 29});
    keep(c, {{31, match}, {29, player}});
    keep(c, {{30, column(c, player)}});
    if (compare_logical(c, player, c.u32(match + pending_player)) == 0) return result(c, 0);
    c.set(3, player);
    c.call(0x10283690, 0x101e4e5c);
    const std::uint32_t at = c.r(30);
    if (compare(c, s8(c.r(3)), 1) == 0) return result(c, u(s8(c.u8(match + line_before_side0) - at)));
    return result(c, u(s8(at - c.u8(match + line_after_side1))));
}

// ---------------------------------------------------------------------------
// 0x101e4ed0
std::uint32_t lane_is_open(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t target_row,
                           std::uint32_t target_column) {
    Frame frame(c, 0x101e4ed0, 0x60, {.first_gpr = 25});
    c.put32(frame.at(0x78), match);
    keep(c, {{28, side}, {27, target_row}, {30, target_column}});
    const auto opponent = std::uint32_t(s8(side) == 0);
    keep(c, {{26, opponent}});
    if (compare(c, s8(side)) == 0) {
        if (compare(c, s8(target_column), 8) > 0) return result(c, 0);
    } else if (compare(c, s8(target_column), 3) < 0) {
        return result(c, 0);
    }
    c.set(4, match);
    Kept<31> other(c, first_player(c.u32(frame.at(0x78)), opponent));
    Kept<29> index(c, 0);
    for (; compare(c, std::int32_t(index), 0x14) < 0; index = index + 1, keep(c, {{25, other}}), other = other + player_size) {
        if (compare(c, s8(position(c, other))) < 0) continue;
        const auto here_row = s8(row(c, other));
        if (compare(c, s8(target_row), here_row) == 0) {
            if (compare(c, s8(target_column), s8(column(c, other))) == 0) return result(c, 0);
        }
        if (compare(c, kind_of(c, other, 0x101e4fa4), 1) == 0) continue;
        bool ahead = false;
        if (compare(c, s8(side), 1) == 0 && compare(c, s8(target_column), s8(column(c, other))) > 0) ahead = true;
        if (!ahead) {
            if (compare(c, s8(side)) != 0) continue;
            if (compare(c, s8(target_column), s8(column(c, other))) >= 0) continue;
        }
        // The opponent is ahead of the column: it blocks the row it stands in or beside.
        auto there = s8(row(c, other));
        c.set(4, u(there));
        if (compare(c, s8(target_row) + 1, there) <= 0) continue;
        there = s8(row(c, other));
        c.set(4, u(there));
        if (compare(c, s8(target_row) - 1, there) < 0) return result(c, 0);
    }
    return result(c, 1);
}
}
