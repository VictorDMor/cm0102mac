// MATCH_ENG.CPP, restarts: how each stoppage of play puts the 40 players on their
// target cells before the next action (play states 2 to 8, 0x7928).
#include "match_eng_report.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
// Match object fields. A cell of the pitch is (across 0..8, along 0..11).
constexpr std::uint32_t last_side_at = 0x7924;        // side that last touched the ball (-1: none)
constexpr std::uint32_t across_at = 0x791d, along_at = 0x791e;  // the ball's cell
constexpr std::uint32_t shot_state_at = 0x791f;       // -1: no shot in progress
constexpr std::uint32_t direction_at = 0x7927;
constexpr std::uint32_t pending_player_at = 0xe000;

constexpr std::uint32_t kStrings = 0x45;  // pool offsets of the assertion texts
constexpr std::uint32_t kFormat = 0x53, kTitle = 0x60;

// A player of a side, as the original computes its address.
std::uint32_t player_of(std::uint32_t match, std::uint32_t side, std::uint32_t index) {
    return match + side * side_size + index * player_size + players_at;
}

// 0x102374f0: sets where a player is heading.
void send_to(Cpu& c, std::uint32_t player, std::uint32_t across, std::uint32_t along, std::uint32_t back) {
    c.set({{3, player}, {4, across}, {5, along}});
    c.call(0x102374f0, back);
}
// 0x10285ee0: clears a player's pending action.
void clear_action(Cpu& c, std::uint32_t player, std::uint32_t back) {
    c.set({{3, player}, {4, 0}});
    c.call(0x10285ee0, back);
}
// 0x1022e3a0: the player takes the restart.
void take_restart(Cpu& c, std::uint32_t player, std::uint32_t back) {
    c.set(3, player);
    c.call(0x1022e3a0, back);
}
// 0x101d3c90: a random number below `bound`.
std::uint32_t draw(Cpu& c, std::uint32_t bound, std::uint32_t back) {
    c.set(3, bound);
    call(c, 0x101d3c90, back);
    return c.r(3);
}
// 0x1022e710: releases the player the match was holding.
void release_pending(Cpu& c, std::uint32_t match, std::uint32_t back) {
    const auto pending = c.u32(match + pending_player_at);
    if (compare_logical(c, pending) != 0) {
        c.set(3, c.u32(match + pending_player_at));
        c.call(0x1022e710, back);
    }
}

// The restart counters (bytes at 0x10000 - low + 0x36 * side) of the side that touched last.
std::uint32_t count_restart(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t low) {
    const auto offset = u(s8(side) * 0x36) + 0x10000 - low;
    c.put8(match + offset, c.u8(match + offset) + 1u);
    return offset;
}

// A side's formation table (0xdfec, 0xdff0) holds, per position, two grids of 9 x 12 cells
// (defending, attacking): low nibble across, high nibble along.
std::uint32_t formation_byte(Cpu& c, std::uint32_t table, std::uint32_t position, std::uint32_t mode,
                             std::uint32_t across, std::uint32_t along) {
    return c.u8(table + u(s8(position)) * 0xd8 + u(s8(mode)) * 0x6c + u(s8(across)) * 0xc + u(s8(along)));
}
std::uint32_t formation_byte_at(Cpu& c, std::uint32_t table, std::uint32_t position, std::uint32_t mode,
                                std::uint32_t offset) {
    return c.u8(table + u(s8(position)) * 0xd8 + u(s8(mode)) * 0x6c + offset);
}
std::uint32_t across_of(std::uint32_t cell) { return u(s8(cell & 0xf)); }
std::uint32_t along_of(std::uint32_t cell) { return u(s8((cell & 0xf0) >> 4)); }
std::uint32_t team_table(Cpu& c, std::uint32_t match, std::uint32_t side) {
    return c.u32(match + 0xdfec + u(s8(side)) * 4);
}

// The assertion every placement routine reports for a cell outside the pitch or a
// missing taker (MATCH_ENG.CPP's pool at `pool`).
void report(Cpu& c, std::uint32_t at, std::uint32_t pool, std::uint32_t name, std::uint32_t extension,
            std::uint32_t line) {
    report_assertion(c, {.at = at, .file = pool + kStrings, .format = pool + kFormat, .title = pool + kTitle,
                         .name = name, .extension = extension, .line = line});
}
// 0x1076f7d0(-1) after the report.
void abort_after_report(Cpu& c, std::uint32_t back) {
    c.set(3, UINT32_MAX);
    c.call(0x1076f7d0, back);
}
}

// 0x101e50c0: the play is restarted where the ball lies: every player takes his formation
// cell for the ball's cell, and the chosen taker goes to the ball.
void restart_state_2(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101e50c0, 0x90, {.first_gpr = 20});
    Kept<31> self(c, match);
    Kept<25> pool(c, c.toc(-0x52cc));
    Kept<26> last_side(c, c.u8(match + last_side_at));
    count_restart(c, match, last_side, 0x1fb8);
    if (compare(c, c.i8(match + across_at), 0) < 0) c.put8(match + across_at, 0);
    if (compare(c, c.i8(match + across_at), 8) > 0) c.put8(match + across_at, 8);
    if (compare(c, c.i8(match + along_at), 0) < 0) c.put8(match + along_at, 0);
    else if (compare(c, c.i8(match + along_at), 0xb) > 0) c.put8(match + along_at, 0xb);
    c.set({{3, match}, {4, last_side}, {5, 2}, {6, 0}});
    call(c, 0x101f3a30, 0x101e5170);
    Kept<28> taker(c, c.r(3));
    for (Kept<30> side(c, 0); compare(c, side, 2) < 0; side = side + 1) {
        Kept<24> attacking(c, std::uint32_t(side == u(s8(last_side))));
        for (Kept<29> index(c, 0); compare(c, index, 0x14) < 0; index = index + 1) {
            const auto player = player_of(match, side, index);
            Kept<27> position_of(c, match_eng::position(c, player));
            if (compare(c, s8(position_of), 0) < 0) continue;
            Kept<23> second(c, second_side_player(c, player));
            const auto cell = formation_byte_at(c, team_table(c, match, second), position_of, attacking,
                                                u(s8(c.u8(match + across_at))) * 0xc + u(s8(c.u8(match + along_at))));
            Kept<21> across(c, across_of(cell));
            Kept<20> along(c, along_of(cell));
            send_to(c, player, across, along, 0x101e52a0);
            clear_action(c, player, 0x101e52bc);
        }
    }
    if (compare_logical(c, taker) != 0) {
        send_to(c, taker, c.u8(match + across_at), c.u8(match + along_at), 0x101e52ec);
        take_restart(c, taker, 0x101e52f4);
        const auto heading = compare(c, c.i8(match + across_at)) == 0 ? 0x5au : 0x10eu;
        c.set({{3, taker}, {4, u(std::int16_t(heading))}});
        set_action_19e(c, taker, u(std::int16_t(heading)));
        c.set({{3, taker}, {4, 0xc8}});
        set_pressure_adjustment(c, taker, 0xc8);
    } else {
        report(c, 0x101e5340, pool, frame.at(0x3d), frame.at(0x38), 0x2370);
        c.set(3, c.toc(-0x7bac));
        c.put8(match, 1);
    }
}


// A pair of roles reads the same way in every restart: `high` first, then `low`
// (cmpwi, bge/blt, cmpwi). 2 = the `high` case, 1 = the `low` case, 0 = neither.
namespace {
int role_case(Cpu& c, std::int32_t kind, std::int32_t high, std::int32_t low) {
    const auto against_high = compare(c, kind, high);
    if (against_high == 0) return 2;
    if (against_high > 0) return 0;
    return compare(c, kind, low) >= 0 ? 1 : 0;
}
// Role classes of a player: 0x102860c0 for the side taking the restart, 0x102861b0 for the other.
std::int32_t role_of(Cpu& c, std::uint32_t entry, std::uint32_t player, std::uint32_t back) {
    c.set(3, player);
    c.call(entry, back);
    return s8(c.r(3));
}
// A cell of a side's formation grid with the nibbles split.
struct Cell { std::uint32_t across, along; };
Cell split(std::uint32_t byte) { return {across_of(byte), along_of(byte)}; }
}

// 0x101e53a0: a restart from the side line or end line (play state 3): the players spread
// over the pitch by their roles, with a little randomness, and the taker goes to the edge.
void restart_state_3(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101e53a0, 0xa0, {.first_gpr = 16});
    Kept<28> self(c, match);
    Kept<29> flag_at(c, c.toc(-0x7bac));
    Kept<30> text_at(c, c.toc(-0x7ba8));
    Kept<31> pool(c, c.toc(-0x52cc));
    c.set(4, count_restart(c, match, c.u8(match + last_side_at), 0x1fb9));
    c.put8(match + direction_at, 3);
    Kept<20> kicking(c, c.u8(match + last_side_at));
    c.put8(match + last_side_at, 0xff);
    Kept<16> spread_base(c, c.r(16));  // 4 plus a random number, kept by the spreads
    Kept<17> last_seen(c, c.r(17));
    Kept<26> along(c, c.r(26));
    Kept<27> across(c, c.r(27));
    Kept<24> position_of(c, c.r(24));
    const auto spread = [&](std::uint32_t first_back, std::uint32_t second_back) {
        spread_base = draw(c, 2, first_back) + 4;
        const auto second = draw(c, 2, second_back);
        across = u(std::int8_t(spread_base - second));
    };
    for (Kept<21> side(c, 0); compare(c, side, 2) < 0; side = side + 1) {
        Kept<23> attacking(c, side == u(s8(kicking)) ? 1u : 0u);
        Kept<22> player(c, player_of(match, side, 0));
        for (Kept<19> index(c, 0); compare(c, index, 0x14) < 0; index = index + 1) {
            position_of = position(c, player);
            if (compare(c, s8(position_of), 0) >= 0) {
                const auto from_grid = [&](std::uint32_t table_at, std::uint32_t offset) {
                    const auto cell = split(formation_byte_at(c, c.u32(match + table_at), position_of, attacking, offset));
                    across = cell.across;
                    along = cell.along;
                };
                // The second side attacks toward along 0, the first toward 11.
                const bool second_side = compare(c, side, 1) == 0;
                const bool takes = compare(c, side, s8(kicking)) == 0;
                const auto kind = role_of(c, takes ? 0x102860c0 : 0x102861b0, player,
                                          second_side ? (takes ? 0x101e545c : 0x101e5530) : (takes ? 0x101e5614 : 0x101e56ec));
                if (second_side && takes) {
                    switch (role_case(c, kind, 2, 1)) {
                    case 2: spread(0x101e54ac, 0x101e54b8); along = 6; break;
                    case 1: spread(0x101e5480, 0x101e548c); along = u(std::int8_t(draw(c, 2, 0x101e549c))); break;
                    default: from_grid(0xdff0, 0x30);
                    }
                } else if (second_side) {
                    switch (role_case(c, kind, 4, 3)) {
                    case 2: spread(0x101e5578, 0x101e5584); along = u(std::int8_t(0xb - draw(c, 2, 0x101e5594))); break;
                    case 1: spread(0x101e5554, 0x101e5560); along = 6; break;
                    default: from_grid(0xdff0, 0x3b);
                    }
                } else if (takes) {
                    switch (role_case(c, kind, 2, 1)) {
                    case 2: spread(0x101e5668, 0x101e5674); along = 5; break;
                    case 1: spread(0x101e5638, 0x101e5644); along = u(std::int8_t(0xb - draw(c, 2, 0x101e5654))); break;
                    default: from_grid(0xdfec, 0x3b);
                    }
                } else {
                    switch (role_case(c, kind, 4, 3)) {
                    case 2: spread(0x101e5734, 0x101e5740); along = u(std::int8_t(draw(c, 2, 0x101e5750))); break;
                    case 1: spread(0x101e5710, 0x101e571c); along = 5; break;
                    default: from_grid(0xdfec, 0x30);
                    }
                }
                if (compare(c, s8(across), 0) < 0 || compare(c, s8(across), 9) >= 0 || compare(c, s8(along), 0) < 0 ||
                    compare(c, s8(along), 12) >= 0) {
                    report(c, 0x101e57f8, pool, frame.at(0x3d), frame.at(0x38), 0x2408);
                    abort_after_report(c, 0x101e583c);
                }
                send_to(c, player, across, along, 0x101e584c);
                clear_action(c, player, 0x101e5858);
            }
            last_seen = player;
            player = player + player_size;
        }
    }
    c.set({{3, match}, {4, kicking}, {5, 3}, {6, 0}});
    call(c, 0x101f3a30, 0x101e588c);
    Kept<25> taker(c, c.r(3));
    // The taker goes to the near or far corner depending on a random threshold on the ball's column.
    const auto go = [&](std::uint32_t to_across, std::uint32_t to_along, std::uint32_t send_back, std::uint32_t take_back) {
        send_to(c, taker, to_across, to_along, send_back);
        take_restart(c, taker, take_back);
        c.set({{3, taker}, {4, 0xc8}});
        set_pressure_adjustment(c, taker, 0xc8);
    };
    const auto near_side = [&](std::uint32_t back) {
        const auto limit = 4 - draw(c, 2, back);
        return compare(c, c.i8(match + across_at), std::int32_t(limit)) > 0;
    };
    if (compare(c, s8(kicking), 0) != 0) {
        if (near_side(0x101e58a4)) go(0, 0, 0x101e58c8, 0x101e58d0);
        else go(8, 0, 0x101e58f0, 0x101e58f8);
    } else {
        if (near_side(0x101e5910)) go(0, 0xb, 0x101e5934, 0x101e593c);
        else go(8, 0xb, 0x101e595c, 0x101e5964);
        const auto heading = compare(c, c.i8(match + across_at)) == 0 ? 0x10eu : 0x5au;
        c.set({{3, taker}, {4, u(std::int16_t(heading))}});
        set_action_19e(c, taker, u(std::int16_t(heading)));
    }
}

// 0x101e6420: a restart from the goal area (play state 4): the ball goes to the middle
// of the end line, everybody takes the attacking cell for it, and the side's chosen taker kicks.
void restart_state_4(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101e6420, 0xa0, {.first_gpr = 17});
    Kept<31> self(c, match);
    Kept<24> pool(c, c.toc(-0x52cc));
    Kept<27> last_side(c, c.u8(match + last_side_at));
    Kept<22> chosen(c, compare(c, s8(last_side)) != 0 ? c.u32(match + 0xe020) : c.u32(match + 0xe01c));
    Kept<29> taker(c, chosen);
    if (compare(c, s8(last_side), 1) == 0) {
        c.put8(match + across_at, 4);
        c.put8(match + along_at, 0xb);
    } else if (compare(c, s8(last_side)) == 0) {
        c.put8(match + across_at, 4);
        c.put8(match + along_at, 0);
    }
    release_pending(c, match, 0x101e64b8);
    Kept<17> last_seen(c, c.r(17));
    for (Kept<28> side(c, 0); compare(c, side, 2) < 0; side = side + 1) {
        Kept<20> attacking(c, side == u(s8(last_side)) ? 1u : 0u);
        Kept<30> player(c, player_of(match, side, 0));
        for (Kept<25> index(c, 0); compare(c, index, 0x14) < 0; index = index + 1) {
            Kept<26> position_of(c, position(c, player));
            if (compare(c, s8(position_of), 0) >= 0) {
                Kept<23> second(c, second_side_player(c, player));
                const auto cell = split(formation_byte_at(c, team_table(c, match, second), position_of, 1, 0x36));
                Kept<19> across(c, cell.across);
                Kept<18> along(c, cell.along);
                send_to(c, player, across, along, 0x101e6578);
                c.set(3, match);
                call(c, 0x101e4130, 0x101e6580);
                c.set({{3, match}, {4, player}});
                call(c, 0x101e4e10, 0x101e658c);
                if (compare(c, s8(c.r(3)), 0) >= 0 && compare(c, s8(attacking), 0) != 0) {
                    c.set({{3, player}, {4, 1}});
                    c.call(0x10239e50, 0x101e65b0);
                }
                clear_action(c, player, 0x101e65bc);
            }
            last_seen = player;
            player = player + player_size;
        }
    }
    if (compare_logical(c, taker) != 0) {
        c.put8(match + shot_state_at, 0);
        send_to(c, taker, c.u8(match + across_at), c.u8(match + along_at), 0x101e65fc);
        take_restart(c, taker, 0x101e6604);
        c.set({{3, taker}, {4, 0xc8}});
        set_pressure_adjustment(c, taker, 0xc8);
        const auto heading = compare(c, c.i8(match + along_at)) == 0 ? 0u : 0xb4u;
        c.set({{3, taker}, {4, heading}});
        set_action_19e(c, taker, heading);
    } else {
        report(c, 0x101e6650, pool, frame.at(0x3d), frame.at(0x38), 0x25e0);
        c.set(3, c.toc(-0x7bac));
        c.put8(match, 1);
    }
}

// 0x101e66b0: a penalty (play state 5): the taker stands at the spot, everybody else is
// kept inside columns 2 to 9, and the side's designated player goes to the middle of the end line.
void restart_state_5(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101e66b0, 0x50, {.first_gpr = 26});
    Kept<30> self(c, match);
    Kept<27> last_side(c, c.u8(match + last_side_at));
    c.set({{3, match}, {4, last_side}, {5, 5}, {6, 0}});
    call(c, 0x101f3a30, 0x101e66dc);
    Kept<28> taker(c, c.r(3));
    take_restart(c, taker, 0x101e66e8);
    c.put8(match + across_at, 4);
    Kept<26> along(c, compare(c, s8(last_side)) != 0 ? 1u : 0xau);
    c.put8(match + along_at, along);
    send_to(c, taker, c.u8(match + across_at), c.u8(match + along_at), 0x101e6720);
    for (Kept<29> index(c, 0); compare(c, index, 0x14) < 0; index = index + 1) {
        // The first side's player of this index, then the second side's.
        const auto settle = [&](std::uint32_t side, std::uint32_t designated_at, std::uint32_t designated_along,
                                std::uint32_t designated_back, std::uint32_t low_back, std::uint32_t high_back,
                                std::uint32_t clear_back) {
            Kept<31> player(c, player_of(match, side, index));
            if (compare(c, s8(position(c, player)), 0) >= 0 && compare_logical(c, player, taker) != 0) {
                if (compare_logical(c, player, c.u32(match + designated_at)) == 0) {
                    send_to(c, player, 4, designated_along, designated_back);
                } else if (compare(c, s8(column(c, player)), 2) < 0) {
                    const auto ahead = row(c, player);
                    send_to(c, player, ahead, 2, low_back);
                } else if (compare(c, s8(column(c, player)), 9) > 0) {
                    const auto ahead = row(c, player);
                    send_to(c, player, ahead, 9, high_back);
                }
            }
            clear_action(c, player, clear_back);
        };
        settle(0, 0xe01c, 0, 0x101e6770, 0x101e67a0, 0x101e67d0, 0x101e67dc);
        settle(1, 0xe020, 0xb, 0x101e6824, 0x101e6854, 0x101e6884, 0x101e6890);
    }
}

// 0x101e68b0: the kick-off (play state 8): both sides take the cells of the centre spot,
// the player of the kicking side nearest to the ball takes it.
void restart_state_8(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101e68b0, 0xb0, {.first_gpr = 16, .first_fpr = 30});
    Fpu fpu(c);
    Kept<31> self(c, match);
    Kept<24> flag_at(c, c.toc(-0x7bac));
    Kept<29> text_at(c, c.toc(-0x7ba8));
    Kept<30> pool(c, c.toc(-0x52cc));
    const auto constants = c.toc(-0x52c8);
    c.set(3, constants);
    c.set_f(31, c.f32(constants + 0xcc));  // the start for the minimum distance
    Kept<20> last_side(c, c.u8(match + last_side_at));
    c.put8(match + last_side_at, 0xff);
    c.put8(match + across_at, 4);
    c.put8(match + along_at, compare(c, s8(last_side)) != 0 ? 6 : 5);
    Kept<21> nearest(c, 0);
    Kept<23> across(c, c.r(23));
    Kept<28> along(c, c.r(28));
    Kept<16> second(c, c.r(16));
    Kept<19> forward(c, c.r(19));
    Kept<18> back_off(c, c.r(18));
    for (Kept<27> side(c, 0); compare(c, side, 2) < 0; side = side + 1) {
        for (Kept<25> index(c, 0); compare(c, index, 0x14) < 0; index = index + 1) {
            const auto player = player_of(match, side, index);
            Kept<26> position_of(c, position(c, player));
            if (compare(c, s8(position_of), 0) < 0) continue;
            second = second_side_player(c, player);
            if (compare(c, s8(second), 0) != 0) {
                const auto cell = split(formation_byte_at(c, c.u32(match + 0xdff0), position_of, 1, 0xa6 - 0x6c));
                across = cell.across;
                along = cell.along;
                if (compare(c, s8(along), 6) < 0) along = 6;
            } else {
                const auto cell = split(formation_byte_at(c, team_table(c, match, 0), position_of, 1, 0x9e - 0x6c));
                across = cell.across;
                along = cell.along;
                if (compare(c, s8(along), 6) >= 0) along = 5;
            }
            send_to(c, player, across, along, 0x101e6a3c);
            if (compare(c, side) == 0) {
                forward = compare(c, s8(along), 9) < 0 ? s8(along) + 3 : 0xb;
                c.set({{3, player}, {4, across}, {5, along}, {6, 4}, {7, u(s8(forward))}});
                c.call(0x10237830, 0x101e6a88);
            } else {
                back_off = compare(c, s8(along), 3) > 0 ? u(s8(along) - 3) : 0;
                c.set({{3, player}, {4, across}, {5, along}, {6, 4}, {7, u(s8(back_off))}});
                c.call(0x10237830, 0x101e6ad0);
            }
        }
    }
    Kept<22> candidate(c, match + players_at + u(s8(last_side)) * side_size);
    for (Kept<27> index(c, 0); compare(c, index, 0x14) < 0; index = index + 1) {
        Kept<26> position_of(c, position(c, candidate));
        if (compare(c, s8(position_of), 0) >= 0) {
            if (compare(c, s8(position_of), 0xa) > 0) {
                report(c, 0x101e6b38, pool, frame.at(0x41), frame.at(0x3c), 0x26bf);
                abort_after_report(c, 0x101e6b7c);
            }
            c.set({{3, candidate}, {4, c.u8(match + across_at)}, {5, c.u8(match + along_at)}});
            c.call(0x1022e760, 0x101e6b8c);
            c.set_f(30, c.f(1));
            fpu.fcmp(0, c.f(30), c.f(31));
            if (c.cr(0) & 8) {  // closer than the best so far
                c.set_f(31, c.f(30));
                nearest = candidate;
            }
        }
        c.put32(frame.at(0x38), candidate);
        candidate = candidate + player_size;
    }
    if (compare_logical(c, nearest) != 0) {
        take_restart(c, nearest, 0x101e6bc8);
        const auto to_row = compare(c, s8(last_side)) == 0 ? 0xbu : 0u;
        c.set({{3, match}, {4, across}, {5, along}, {6, 4}, {7, to_row}});
        Kept<17> turns(c, turn_cost(c, match, across, along, 4, to_row));
        c.set({{3, nearest}, {4, u(s8(turns))}});
        set_action_19e(c, nearest, u(s8(turns)));
    } else {
        c.put8(match, 1);
    }
}

// 0x101e59b0: a free kick or throw-in (play states 6 and 7): the players take cells around
// the ball, kept away from it by a random margin, and the nearest of the taking side goes to it.
void restart_state_6(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101e59b0, 0xc0, {.first_gpr = 15});
    Kept<29> self(c, match);
    Kept<28> flag_at(c, c.toc(-0x7bac));
    Kept<30> text_at(c, c.toc(-0x7ba8));
    Kept<31> pool(c, c.toc(-0x52cc));
    const auto counter = u(s8(c.u8(match + last_side_at)) * 0x36) + 0x10000 - 0x1fba;
    Kept<15> scratch(c, c.u8(match + counter));
    c.set(3, counter);
    c.put8(match + counter, scratch + 1u);
    Kept<20> last_side(c, c.u8(match + last_side_at));
    c.put8(match + last_side_at, 0xff);
    release_pending(c, match, 0x101e5a18);
    scratch = u(s8(last_side));
    bool near_own_goal = false;
    if (compare(c, scratch) == 0 && compare(c, s8(c.u8(match + along_at)), 3) < 0) near_own_goal = true;
    if (!near_own_goal && compare(c, s8(last_side), 1) == 0) {
        const auto along_now = c.u8(match + along_at);
        c.set(4, along_now);
        near_own_goal = compare(c, s8(along_now), 8) > 0;
    }
    Kept<22> attacking(c, c.r(22));
    Kept<26> across(c, c.r(26));
    Kept<27> along(c, c.r(27));
    Kept<23> position_of(c, c.r(23));
    if (near_own_goal) {
        // The ball is near the goal of the side that did not take it: players keep their
        // set cell, but not the ball's own cell.
        Kept<18> player(c, c.r(18));
        for (Kept<17> side(c, 0); compare(c, side, 2) < 0; side = side + 1) {
            attacking = u(s8(side == u(s8(last_side))));
            player = player_of(match, side, 0);
            c.put32(frame.at(0x48), 0);
            while (compare(c, c.i32(frame.at(0x48)), 0x14) < 0) {
                position_of = position(c, player);
                if (compare(c, s8(position_of), 0) >= 0) {
                    const auto second = second_side_player(c, player);
                    const auto cell = split(formation_byte_at(c, team_table(c, match, second), position_of, 1, 0x36));
                    across = cell.across;
                    along = cell.along;
                    scratch = u(s8(across));
                    if (compare(c, std::int32_t(scratch), s8(c.u8(match + across_at))) == 0) {
                        scratch = c.u8(match + along_at);
                        if (compare(c, s8(along), s8(scratch)) == 0 && compare(c, s8(attacking)) == 0) {
                            if (compare(c, s8(last_side)) == 0) {
                                if (compare(c, s8(along), 0xb) < 0) along = along + 1;
                                else along = along - 1;
                            } else if (compare(c, s8(along)) > 0) {
                                along = along - 1;
                            } else {
                                along = along + 1;
                            }
                        }
                    }
                    send_to(c, player, across, along, 0x101e5b8c);
                    c.set(3, match);
                    call(c, 0x101e4130, 0x101e5b94);
                    c.set({{3, match}, {4, player}});
                    call(c, 0x101e4e10, 0x101e5ba0);
                    if (compare(c, s8(c.r(3)), 0) >= 0 && compare(c, s8(attacking)) != 0) {
                        c.set({{3, player}, {4, 1}});
                        c.call(0x10239e50, 0x101e5bc4);
                    }
                    clear_action(c, player, 0x101e5bd0);
                }
                c.put32(frame.at(0x48), c.u32(frame.at(0x48)) + 1);
                c.put32(frame.at(0x40), player);
                player = player + player_size;
            }
        }
    } else {
        // Elsewhere the ball's cell is pulled in, and the players spread around a base cell
        // (`base_across`, `base_along`) a few cells ahead of the ball.
        if (compare(c, s8(c.u8(match + along_at)), 0xc) >= 0) c.put8(match + along_at, 0xb);
        else if (compare(c, s8(c.u8(match + along_at)), 0) < 0) c.put8(match + along_at, 0);
        c.set(4, c.u8(match + across_at));
        if (compare(c, s8(c.u8(match + across_at)), 9) >= 0) c.put8(match + across_at, 8);
        else if (compare(c, s8(c.u8(match + across_at)), 0) < 0) c.put8(match + across_at, 0);
        Kept<25> base_across(c, c.r(25));
        Kept<24> base_along(c, c.r(24));
        const auto base = [&](bool second_draw_in_r5, std::uint32_t first_back, std::uint32_t second_back, std::uint32_t direction,
                              std::int32_t along_shift) {
            c.put8(match + direction_at, direction);
            scratch = draw(c, 2, first_back);
            const auto second = draw(c, 2, second_back);
            if (second_draw_in_r5) c.set(5, second);
            base_across = u(std::int8_t(scratch + 4 - second));
            base_along = u(std::int8_t(c.u8(match + along_at) + u(along_shift)));
        };
        if (compare(c, s8(last_side)) == 0) {
            if (compare(c, s8(c.u8(match + along_at)), 6) > 0) base(false, 0x101e5c98, 0x101e5ca4, 7, 4);
            else base(true, 0x101e5cd0, 0x101e5cdc, 0xff, 3);
        } else if (compare(c, s8(c.u8(match + along_at)), 5) < 0) {
            base(false, 0x101e5d1c, 0x101e5d28, 7, -4);
        } else {
            base(false, 0x101e5d54, 0x101e5d60, 0xff, -3);
        }
        if (compare(c, s8(base_along), 0xc) >= 0) base_along = 0xb;
        else if (compare(c, s8(base_along)) < 0) base_along = 0;
        Kept<21> player(c, c.r(21));
        for (Kept<19> side(c, 0); compare(c, side, 2) < 0; side = side + 1) {
            attacking = std::uint32_t(s8(last_side) == std::int32_t(std::uint32_t(side)));
            player = player_of(match, side, 0);
            c.put32(frame.at(0x44), 0);
            for (;;) {
                scratch = c.u32(frame.at(0x44));
                if (compare(c, scratch, 0x14) >= 0) break;
                position_of = position(c, player);
                if (compare(c, s8(position_of), 0) >= 0) {
                    const auto from_grid = [&](std::uint32_t table, bool leaves_index) {
                        const auto cell = split(formation_byte(c, table, position_of, attacking, base_across, base_along));
                        across = cell.across;
                        along = cell.along;
                        if (leaves_index) {
                            scratch = u(s8(position_of)) * 0xd8 + u(s8(attacking)) * 0x6c + u(s8(base_across)) * 0xc + u(s8(base_along));
                        }
                    };
                    const auto spread = [&](std::uint32_t base_value, std::uint32_t first_back, std::uint32_t second_back) {
                        scratch = base_value + draw(c, 2, first_back);
                        const auto second = draw(c, 2, second_back);
                        across = u(std::int8_t(scratch - second));
                    };
                    if (compare(c, s8(c.u8(match + direction_at)), 0) <= 0) {
                        from_grid(c.u32(match + 0xdfec + side * 4), false);
                    } else if (compare(c, side, 1) == 0) {
                        const bool takes = compare(c, side, s8(last_side)) == 0;
                        const auto kind = role_of(c, takes ? 0x102860c0 : 0x102861b0, player, takes ? 0x101e5ea8 : 0x101e5f9c);
                        if (takes) {
                            switch (role_case(c, kind, 2, 1)) {
                            case 2: scratch = draw(c, 2, 0x101e5ef8) + 4; across = u(std::int8_t(scratch - draw(c, 2, 0x101e5f04))); along = 6; break;
                            case 1: spread(base_across, 0x101e5ecc, 0x101e5ed8); along = u(std::int8_t(draw(c, 2, 0x101e5ee8))); break;
                            default: from_grid(c.u32(match + 0xdff0), false);
                            }
                        } else {
                            switch (role_case(c, kind, 4, 3)) {
                            case 2: spread(base_across, 0x101e5fe4, 0x101e5ff0); along = u(std::int8_t(0xb - draw(c, 2, 0x101e5ffc))); break;
                            case 1: scratch = draw(c, 2, 0x101e5fc0) + 4; across = u(std::int8_t(scratch - draw(c, 2, 0x101e5fcc))); along = 6; break;
                            default: from_grid(c.u32(match + 0xdff0), false);
                            }
                        }
                    } else {
                        const bool takes = compare(c, side, s8(last_side)) == 0;
                        const auto kind = role_of(c, takes ? 0x102860c0 : 0x102861b0, player, takes ? 0x101e60a0 : 0x101e619c);
                        if (takes) {
                            switch (role_case(c, kind, 2, 1)) {
                            case 2: scratch = draw(c, 2, 0x101e60f8) + 4; across = u(std::int8_t(scratch - draw(c, 2, 0x101e6104))); along = 5; break;
                            case 1: spread(base_across, 0x101e60c4, 0x101e60d0); along = u(std::int8_t(0xb - draw(c, 2, 0x101e60e4))); break;
                            default: from_grid(c.u32(match + 0xdfec), false);
                            }
                        } else {
                            switch (role_case(c, kind, 4, 3)) {
                            case 2: spread(base_across, 0x101e61e4, 0x101e61f0); along = u(std::int8_t(draw(c, 2, 0x101e6200))); break;
                            case 1: scratch = draw(c, 2, 0x101e61c0) + 4; across = u(std::int8_t(scratch - draw(c, 2, 0x101e61cc))); along = 5; break;
                            default: from_grid(c.u32(match + 0xdfec), true);
                            }
                        }
                    }
                    bool outside = compare(c, s8(across), 0) < 0 || compare(c, s8(across), 9) >= 0 || compare(c, s8(along), 0) < 0;
                    if (!outside) {
                        scratch = u(s8(along));
                        outside = compare(c, std::int32_t(scratch), 12) >= 0;
                    }
                    if (outside) {
                        report(c, 0x101e62cc, pool, frame.at(0x51), frame.at(0x4c), 0x254e);
                        abort_after_report(c, 0x101e6310);
                    }
                    send_to(c, player, across, along, 0x101e6320);
                    c.set(3, match);
                    call(c, 0x101e4130, 0x101e6328);
                    c.set({{3, match}, {4, player}});
                    call(c, 0x101e4e10, 0x101e6334);
                    if (compare(c, s8(c.r(3)), 0) >= 0) {
                        c.set({{3, player}, {4, 1}});
                        c.call(0x10239e50, 0x101e634c);
                    }
                    clear_action(c, player, 0x101e6358);
                }
                c.put32(frame.at(0x44), c.u32(frame.at(0x44)) + 1);
                c.put32(frame.at(0x3c), player);
                player = player + player_size;
            }
        }
    }
    c.set({{3, match}, {4, last_side}, {5, c.u8(match + 0x7928)}, {6, 0}});
    call(c, 0x101f3a30, 0x101e639c);
    Kept<16> taker(c, c.r(3));
    c.put8(match + shot_state_at, 0);
    send_to(c, taker, c.u8(match + across_at), c.u8(match + along_at), 0x101e63b8);
    take_restart(c, taker, 0x101e63c0);
    c.put32(frame.at(0x38), compare(c, s8(last_side)) == 0 ? 0xau : 1u);
    c.set({{3, taker}, {4, c.u8(match + across_at)}, {5, c.u8(match + along_at)}, {6, 4}, {7, u(s8(c.u32(frame.at(0x38))))}});
    c.call(0x10237830, 0x101e63fc);
    c.set({{3, taker}, {4, 0xc8}});
    set_pressure_adjustment(c, taker, 0xc8);
}
}
