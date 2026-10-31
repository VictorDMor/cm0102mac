// MATCH_ENG.CPP, routes: where the ball ends up after a kick, and the lines of players it
// has to get past.
#include "match_eng_routes.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
constexpr std::uint32_t play_state = 0x7928, kickoff_side = 0x7924, ball_column_at = 0x791d, ball_row_at = 0x791e;
constexpr std::uint32_t message_target = 0x79f4;
constexpr std::uint32_t pending_player = 0xe000;
constexpr std::uint32_t lowest_player = 0xe00c, highest_player = 0xe010, lowest_far = 0xe018, highest_near = 0xe014;
constexpr std::uint32_t random_below = 0x101d3c90, post_message_routine = 0x101d5810, release_pending = 0x1022e710;

// 0x101d5810: a message to the match's target object about the kick-off side.
void post_message(Cpu& c, std::uint32_t match, std::uint32_t id, std::uint32_t back) {
    c.set({{3, c.u32(match + message_target)}, {4, id}, {5, match}, {6, c.u8(match + kickoff_side)}, {7, 0}});
    call(c, post_message_routine, back);
}

std::uint32_t random(Cpu& c, std::uint32_t bound, std::uint32_t back) {
    c.set(3, bound);
    call(c, random_below, back);
    return c.r(3);
}
}

std::uint32_t stop_for_ball_out(Cpu& c, std::uint32_t match, std::uint32_t side_flag) {
    Frame frame(c, 0x101f0c00, 0x40, {.first_gpr = 30});
    keep(c, {{31, match}, {30, side_flag}});
    if (compare(c, s8(c.u8(match + play_state))) != 0) return result(c, 1);
    if (compare(c, s8(c.u8(match + kickoff_side)), -1) != 0) c.put8(match + kickoff_side, 0xff);
    c.set(3, match + 0x10000);
    if (compare_logical(c, c.u32(match + pending_player)) != 0) {
        c.set(3, c.u32(match + pending_player));
        call(c, release_pending, 0x101f0c68);
    }
    // Which break follows: a throw-in or goal kick for either side, set by where the ball left.
    const auto stop = [&](std::uint32_t state, std::uint32_t kick_side, std::uint32_t id, std::uint32_t back) {
        c.put8(match + play_state, state);
        c.put8(match + kickoff_side, kick_side);
        post_message(c, match, id, back);
        return result(c, 1);
    };
    const auto flag_is_one = [&] { return compare(c, s8(side_flag), 1) == 0; };
    if (compare(c, s8(c.u8(match + ball_column_at)), 0) < 0 || compare(c, s8(c.u8(match + ball_column_at)), 9) >= 0)
        return stop(2, s8(side_flag) == 0, 0x1f76, 0x101f0cbc);
    if (compare(c, s8(c.u8(match + ball_row_at)), 0) < 0) {
        if (flag_is_one()) return stop(4, 0, 0x1f77, 0x101f0d08);
        return stop(3, 1, 0x1f75, 0x101f0d34);
    }
    if (compare(c, s8(c.u8(match + ball_row_at)), 0xc) >= 0) {
        if (flag_is_one()) return stop(3, 0, 0x1f75, 0x101f0d80);
        return stop(4, 1, 0x1f77, 0x101f0dac);
    }
    return result(c, 0);
}

std::uint32_t highest_column_of_line(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101f0e80, 0x40, {.first_gpr = 31});
    keep(c, {{31, match}});
    c.set(3, match + 0x10000);
    const auto player = c.u32(match + highest_player);
    if (compare_logical(c, player) == 0) return result(c, u(-1));
    c.set(3, player);
    return column(c, player);
}

std::uint32_t lowest_column_of_line(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101f0ed0, 0x40, {.first_gpr = 31});
    keep(c, {{31, match}});
    c.set(3, match + 0x10000);
    const auto player = c.u32(match + lowest_player);
    if (compare_logical(c, player) == 0) return result(c, 0xc);
    c.set(3, player);
    return column(c, player);
}

std::uint32_t column_within_line(Cpu& c, std::uint32_t match, std::uint32_t player) {
    Frame frame(c, 0x101f0dd0, 0x50, {.first_gpr = 29});
    keep(c, {{30, match}, {31, player}});
    c.set(3, player);
    call(c, 0x10283690, 0x101f0df8);
    if (compare(c, s8(c.r(3))) == 0) {
        enter(c, 0x101f0e0c, {match});
        Kept<29> lowest(c, u(s8(lowest_column_of_line(c, match))));
        // Not further forward than the line's rearmost player.
        return result(c, compare(c, s8(column(c, player)), s8(lowest)) > 0 ? 0 : 1);
    }
    enter(c, 0x101f0e3c, {match});
    Kept<29> highest(c, u(s8(highest_column_of_line(c, match))));
    return result(c, compare(c, s8(column(c, player)), s8(highest)) < 0 ? 0 : 1);
}

void find_line_extremes(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101f0f20, 0x60, {.first_gpr = 25});
    Kept<30> r_match(c, match);
    Kept<28> r28(c, 0xc);   // the smallest column seen (first side), then the largest (second side)
    Kept<27> r27(c, u(-1)); // the largest column seen (first side), then the smallest (second side)
    Kept<31> player(c, match + players_at);
    Kept<29> index(c, 0);
    Kept<26> previous(c, c.r(26));
    Kept<25> previous_far(c, c.r(25));
    // A player on the pitch that 0x101e3d50 does not rule out.
    const auto eligible = [&](std::uint32_t back) {
        c.set(3, player);
        if (compare(c, s8(position(c, player))) < 0) return false;
        c.set({{3, match}, {4, player}});
        call(c, 0x101e3d50, back);
        return compare(c, std::int16_t(c.r(3)), 1) != 0;
    };
    const auto column_of = [&] { c.set(3, player); return s8(column(c, player)); };
    const auto remember = [&](std::uint32_t at) {
        c.set(3, match + 0x10000);
        c.put32(match + at, player);
    };
    while (compare(c, s8(index), 0x14) < 0) {
        if (eligible(0x101f0f68)) {
            if (compare(c, s8(r28), column_of()) > 0) {
                c.set(3, player);
                r28 = column(c, player);
                remember(lowest_player);
            }
            if (eligible(0x101f0fc0)) {
                if (compare(c, s8(r27), column_of()) < 0) {
                    c.set(3, player);
                    r27 = column(c, player);
                    remember(highest_near);
                }
            }
        }
        index = std::uint32_t(index) + 1;
        previous = player;
        player = std::uint32_t(player) + player_size;
    }
    r28 = u(-1);
    r27 = 0xc;
    player = match + side_size + players_at;
    index = 0;
    while (compare(c, s8(index), 0x14) < 0) {
        if (eligible(0x101f1044)) {
            if (compare(c, s8(r28), column_of()) < 0) {
                c.set(3, player);
                r28 = column(c, player);
                remember(highest_player);
            }
            if (eligible(0x101f109c)) {
                if (compare(c, s8(r27), column_of()) > 0) {
                    c.set(3, player);
                    r27 = column(c, player);
                    remember(lowest_far);
                }
            }
        }
        index = std::uint32_t(index) + 1;
        previous_far = player;
        player = std::uint32_t(player) + player_size;
    }
}

void scatter_ball(Cpu& c, std::uint32_t match, std::uint32_t column, std::uint32_t row, std::uint32_t spread, std::uint32_t side,
                  std::uint32_t confined, std::uint32_t everyone, std::uint32_t keeper) {
    Frame frame(c, 0x101f1100, 0x70, {.first_gpr = 18});
    // The arguments: the target cell and the spread around it (cells), the restart side (-1: drawn),
    // whether the landing may be anywhere on the pitch (0) or must stay on it, whether every
    // player may be knocked over, and the goalkeeper who may be sent to the goal line.
    keep(c, {{31, match}, {29, spread}, {24, side}, {19, confined}, {30, keeper}});
    c.put8(frame.at(0x8f), column);
    c.put8(frame.at(0x93), row);
    c.put8(frame.at(0xa3), everyone);
    const auto pool = c.toc(-0x52c8);
    keep(c, {{23, pool}});
    Kept<24> r_side(c, side);
    Kept<18> r18(c, c.r(18));
    Kept<26> new_column(c, c.r(26));
    Kept<28> new_row(c, c.r(28));
    Kept<21> flight_time(c, c.r(21));
    Kept<25> side_index(c, c.r(25));
    Kept<22> player_index(c, c.r(22));
    Kept<27> player(c, c.r(27));
    Kept<20> previous(c, c.r(20));
    const auto ball_column = [&] { return s8(c.u8(match + ball_column_at)); };
    const auto ball_row = [&] { return s8(c.u8(match + ball_row_at)); };

    c.set(3, match + 0x10000);
    if (compare_logical(c, c.u32(match + pending_player)) != 0) {
        c.set(3, c.u32(match + pending_player));
        call(c, release_pending, 0x101f1150);
    }
    c.put8(match + ball_column_at, c.u8(frame.at(0x8f)));
    c.put8(match + ball_row_at, c.u8(frame.at(0x93)));
    if (compare(c, s8(c.u8(match + kickoff_side)), -1) != 0) c.put8(match + kickoff_side, 0xff);
    if (compare(c, s8(r_side), -1) == 0) r_side = nonzero(c, std::int32_t(random(c, 2, 0x101f118c))) ? 0 : 1;
    // A landing cell within `spread` of the ball in each direction: the sum of one draw less another.
    const auto scatter = [&](std::uint32_t from, std::uint32_t first_back, std::uint32_t second_back) {
        const auto up = random(c, u(s8(spread) + 1), first_back);
        r18 = c.u8(from) + up;
        const auto down = random(c, u(s8(spread) + 1), second_back);
        return u(s8(std::uint32_t(r18) - down));
    };
    bool crossed = false;
    if (compare(c, s8(confined)) == 0) {
        // Kicked from a wing (columns outside 2 to 6), the ball often crosses to the far side (column -1 or 9).
        const bool centre = compare(c, ball_column(), 2) >= 0 && compare(c, ball_column(), 6) <= 0;
        if (!centre && nonzero(c, std::int32_t(random(c, 4, 0x101f11d4)))) {
            new_column = compare(c, ball_column(), 2) >= 0 ? 9 : u(-1);
            new_row = scatter(match + ball_row_at, 0x101f1204, 0x101f1218);
            crossed = true;
        }
    }
    while (!crossed) {
        new_column = scatter(match + ball_column_at, 0x101f1230, 0x101f1244);
        new_row = scatter(match + ball_row_at, 0x101f1258, 0x101f126c);
        if (compare(c, s8(confined)) == 0) break;
        if (compare(c, s8(new_row), 0) >= 0 && compare(c, s8(new_row), 0xb) <= 0) break;  // a landing on the pitch
    }
    if (compare(c, s8(spread)) == 0) {
        flight_time = 0;
    } else {
        const auto drawn = random(c, u(s8(spread) + 1), 0x101f12b8);
        flight_time = drawn + 1;
    }
    c.put8(match + 0x7920, flight_time);
    c.set({{3, match}, {4, c.u8(match + ball_column_at)}, {5, c.u8(match + ball_row_at)}, {6, new_column}, {7, new_row}});
    call(c, 0x101ee610, 0x101f12dc);
    c.set({{4, c.r(3)}, {3, match}});
    call(c, 0x101f1520, 0x101f12e8);
    c.put8(match + 0x791f, random(c, 4, 0x101f12f0));
    c.put8(match + ball_column_at, new_column);
    c.put8(match + ball_row_at, new_row);
    c.set({{3, match}, {4, r_side}});
    call(c, 0x101f14b0, 0x101f130c);
    // With `everyone` set, each unrated player on the pitch has one chance in three of being knocked over.
    side_index = 0;
    for (; compare(c, s8(side_index), 2) < 0 && compare(c, s8(c.u8(frame.at(0xa3)))) != 0; side_index = std::uint32_t(side_index) + 1) {
        player = match + u(s8(side_index)) * side_size + players_at;
        for (player_index = 0; compare(c, s8(player_index), 0x14) < 0;
             player_index = std::uint32_t(player_index) + 1, previous = player, player = std::uint32_t(player) + player_size) {
            c.set(3, player);
            if (compare(c, s8(position(c, player))) < 0) continue;
            c.set(3, player);
            if (compare(c, s8(secondary_rating(c, player))) != 0) continue;
            if (nonzero(c, std::int32_t(random(c, 3, 0x101f135c)))) continue;
            const auto cause = random(c, 2, 0x101f136c);
            c.set({{4, u(s8(cause))}, {3, player}});
            call(c, 0x10285ed0, 0x101f1378);
        }
    }
    // A goalkeeper standing on the goal line: sent to the goal area now and then.
    const auto keeper_acts = [&] {
        if (compare_logical(c, keeper) == 0) return false;
        if (compare(c, ball_column(), 4) != 0) return false;
        if (compare(c, ball_row(), -1) != 0 && compare(c, ball_row(), 0xc) != 0) return false;
        return nonzero(c, std::int32_t(random(c, 4, 0x101f13f0)));
    };
    if (keeper_acts()) {
        const auto act = [&](std::uint32_t routine, std::uint32_t back) {
            c.set(3, keeper);
            c.set_f(1, c.f32(pool + 0x10));
            call(c, routine, back);
        };
        const auto home_side = [&] { return compare(c, s8(c.u8(keeper + 0x27))) == 0; };
        if (compare(c, ball_row(), 0xc) == 0) {
            if (home_side()) act(0x1026a1b0, 0x101f1464);
            else act(0x1026d660, 0x101f1474);
        } else if (compare(c, ball_row(), 0xc) < 0 && compare(c, ball_row(), -1) == 0) {
            if (home_side()) act(0x1026d660, 0x101f1434);
            else act(0x1026a1b0, 0x101f1444);
        }
        if (compare(c, s8(c.u8(match + play_state))) != 0) return;
    }
    enter(c, 0x101f1490, {match, r_side});
    stop_for_ball_out(c, match, r_side);
}
}
