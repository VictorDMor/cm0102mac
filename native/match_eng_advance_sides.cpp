// MATCH_ENG.CPP, match advancement: routines over a side's players, and the ball's drift
// while play is stopped.
#include "match_eng_advance.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
constexpr std::uint32_t side_player_count = 20;
constexpr std::uint32_t record_at = 0x6c;           // a player's pointer to its record
constexpr std::uint32_t turn_at = 0x7950;           // the default turn, int16
constexpr std::uint32_t drift_timer = 0x7920;
constexpr std::uint32_t pressure_total = 0x1e8;     // per side, int16
constexpr std::uint32_t pressure_count = 0xaf;      // per side, byte

std::uint32_t side_players(std::uint32_t match, std::uint32_t side) {
    return match + u(s8(side) * std::int32_t(side_size)) + players_at;
}
std::uint32_t team_of_side(std::uint32_t match, std::uint32_t side) {
    return match + u(s8(side) * std::int32_t(team_tactics_size)) + team_tactics;
}
}  // namespace

void adjust_side_around(Cpu& c, std::uint32_t match, std::uint32_t offender, double factor) {
    Frame frame(c, 0x101d6780, 0x60, {.first_gpr = 27, .gpr_offset = -0x24, .first_fpr = 31});
    Fpu fpu(c);
    c.put32(frame.at(0x78), match);
    Kept<28> r_offender(c, offender);
    c.set_f(31, factor);
    const auto pool = c.toc(-0x52c4);
    Kept<31> r_pool(c, pool);
    const auto base = c.u32(frame.at(0x78));
    c.set(4, base);
    c.set(3, u(s8(c.u8(offender + 0x27)) * std::int32_t(side_size)));  // stays when no player has a record
    Kept<30> r_player(c, side_players(base, c.u8(offender + 0x27)));
    Kept<29> r_index(c, 0);
    Kept<27> r_previous(c, c.r(27));
    for (; compare(c, std::int32_t(r_index), std::int32_t(side_player_count)) < 0; r_index = r_index + 1) {
        const std::uint32_t player = r_player;
        if (compare_logical(c, c.u32(player + record_at)) != 0) {
            // The offender at full weight, the others reduced; a player out of play gets the other term.
            if (compare_logical(c, player, r_offender) != 0) {
                c.set(3, player);
                if (compare(c, s8(position(c, player)), 0) < 0) {
                    c.set(3, player);
                    const double step = fpu.fdiv(c.f(31), c.f64(pool + 0x88));
                    c.set_f(1, fpu.frsp(fpu.fadd(c.f64(pool + 0x80), step)));
                    c.call(0x102865a0, 0x101d6808);
                } else {
                    c.set(3, player);
                    c.set_f(1, c.f64(pool + 0x90));
                    c.set_f(1, fpu.frsp(fpu.multiply_add(0x101d6818, c.f(31), c.f(1), c.f64(pool + 0x90))));
                    c.call(0x102865a0, 0x101d6824);
                }
            } else {
                c.set(3, player);
                c.set_f(1, c.f(31));
                c.call(0x102865a0, 0x101d6834);
            }
        }
        r_previous = player;
        r_player = player + player_size;
    }
}

void adjust_side(Cpu& c, std::uint32_t match, std::uint32_t side, double amount) {
    Frame frame(c, 0x101d6860, 0x50, {.first_gpr = 29});
    c.put32(frame.at(0x68), match);
    c.put8(frame.at(0x6f), side);
    c.put_f32(frame.at(0x70), float(amount));
    const auto base = c.u32(frame.at(0x68));
    c.set(4, base);
    c.set(3, u(s8(c.u8(frame.at(0x6f))) * std::int32_t(side_size)));  // stays when no player has a record
    Kept<31> r_player(c, side_players(base, c.u8(frame.at(0x6f))));
    Kept<30> r_index(c, 0);
    Kept<29> r_previous(c, c.r(29));
    for (; compare(c, std::int32_t(r_index), std::int32_t(side_player_count)) < 0; r_index = r_index + 1) {
        const std::uint32_t player = r_player;
        if (compare_logical(c, c.u32(player + record_at)) != 0) {
            c.set(3, player);
            c.set_f(1, c.f32(frame.at(0x70)));
            c.call(0x102865a0, 0x101d68bc);
        }
        r_previous = player;
        r_player = player + player_size;
    }
}

void side_pressure(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101d6a90, 0x80, {.first_gpr = 17});
    Kept<30> r_match(c, match);
    Kept<27> r_side(c, side);
    Kept<26> r_nodes(c, c.toc(-0x7be8));
    Kept<28> r_found(c, c.toc(-0x7b78));
    Kept<29> r_list(c, c.toc(-0x7b7c));
    Kept<22> r_total(c, 0);
    Kept<31> r_player(c, side_players(match, side));
    const auto total_at = match + pressure_total + u(s8(side) * 2);
    c.set(4, 0);
    c.put16(total_at, 0);
    c.put8(match + pressure_count + u(s8(side)), 0);
    Kept<25> r_index(c, 0);
    Kept<24> r_record(c, c.r(24));
    Kept<23> r_node(c, c.r(23));
    Kept<21> r_flag(c, c.r(21));
    Kept<20> r_entry(c, c.r(20));
    Kept<19> r_base(c, c.r(19));
    Kept<18> r_previous(c, c.r(18));
    Kept<17> r_previous2(c, c.r(17));
    const auto adjust_total = [&](std::int32_t delta) {
        c.put16(total_at, u(c.i16(total_at)) + u(delta));
    };
    for (; compare(c, s8(r_index), 0x14) < 0; r_index = r_index + 1) {
        const std::uint32_t player = r_player;
        c.set(3, player);
        if (compare(c, s8(position(c, player)), 0) >= 0) {
            c.set(3, player);
            c.call(0x10289560, 0x101d6b10);
            c.set({{4, c.r(3)}, {3, team_of_side(match, r_side)}});
            c.call(0x10114d40, 0x101d6b28);
            r_flag = c.r(3);
            if (compare(c, std::int32_t(std::uint32_t(r_total) & std::uint32_t(r_flag)), 0) != 0) adjust_total(-0x1f4);
            r_total = std::uint32_t(r_total) + std::uint32_t(r_flag);
            r_record = c.u32(player + record_at);
            c.set({{3, c.u32(r_list)}, {4, r_record}});
            c.call(0x10082f20, 0x101d6b64);
            c.put32(r_found, c.r(3));
            if (compare_logical(c, c.r(3)) != 0) {
                c.set(4, c.u32(r_nodes));
                c.set(3, c.u32(r_found));
                r_base = c.r(4) + c.u32(c.r(3) + 4) * 0x248;
            } else {
                r_base = 0;
            }
            r_node = r_base;
            c.set({{3, c.u32(r_list)}, {4, r_record}, {5, r_node}});
            c.call(0x10082f90, 0x101d6ba0);
            r_entry = c.r(3);
            [&] {
                if (compare_logical(c, r_entry) == 0 || compare_logical(c, r_node) == 0) return;
                std::int32_t roll = draw(c, 0x14, 0x101d6bbc);
                c.set(4, c.u32(player + 0x70));
                if (compare(c, s8(c.u8(c.r(4) + 0x2f)), roll) <= 0) return;
                roll = draw(c, 0x1b58, 0x101d6bd8);
                c.set(4, c.u32(player + 0x70));
                if (compare(c, c.i16(c.r(4) + 0xa), roll) <= 0) return;
                roll = draw(c, 6, 0x101d6bf0);
                if (compare(c, s8(c.u8(r_record + 0x67)), roll) >= 0) return;
                c.set(3, c.u32(player + 0x70));
                const auto height = c.i16(c.r(3) + 0xa);
                c.set(4, u(height));
                c.set(3, u(c.i16(r_node + 0x1ec)));
                if (compare(c, height, std::int16_t(c.i16(r_node + 0x1ec) - 0x2ee)) <= 0) return;
                c.set(3, r_entry);
                const auto bits = word_50(c, r_entry);
                c.set(4, 0x17a0b);
                if (compare(c, std::int32_t(bits & 0x17a0b), 0) == 0) return;
                c.set({{3, match}, {4, r_side}});
                call(c, 0x101d3b60, 0x101d6c48);
                if (compare_logical(c, player, c.r(3)) == 0) {
                    c.set(3, player);
                    c.call(0x10227140, 0x101d6c58);
                    adjust_total(-s8(c.r(3)) * 3);
                    return;
                }
                c.set(3, u(s8(c.u8(r_record + 0x67))));
                if (compare(c, draw(c, c.r(3), 0x101d6c8c), 0) == 0) {
                    c.set(3, player);
                    c.call(0x10227140, 0x101d6c9c);
                    const auto doubled = std::int16_t(u(s8(c.r(3)) * 2));
                    c.set(4, u(doubled));
                    adjust_total(-std::int32_t(doubled));
                } else {
                    c.set(3, player);
                    c.call(0x10227140, 0x101d6ccc);
                    c.set(4, u(s8(c.r(3))));
                    adjust_total(-s8(c.r(3)));
                }
            }();
            c.set({{3, match}, {4, r_side}});
            call(c, 0x101d3b60, 0x101d6cf4);
            if (compare_logical(c, player, c.r(3)) != 0) {
                c.set(3, player);
                c.call(0x10227140, 0x101d6d04);
                c.set(4, u(s8(c.r(3))));
                adjust_total(s8(c.r(3)));
            }
        }
        r_previous = player;
        r_player = player + player_size;
    }
    r_player = side_players(match, side);
    for (r_index = 0; compare(c, s8(r_index), 0x14) < 0; r_index = r_index + 1) {
        const std::uint32_t player = r_player;
        c.set(3, player);
        if (compare(c, s8(position(c, player)), 0) >= 0) {
            c.set({{3, player}, {4, 1}});
            c.call(0x1022ba10, 0x101d6d70);
        }
        r_previous2 = player;
        r_player = player + player_size;
    }
}

void drift_ball(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101d6da0, 0x40, {.first_gpr = 30});
    Kept<31> r_match(c, match);
    Kept<30> r_roll(c, c.r(30));
    c.set(3, match + 0x10000);
    if (compare_logical(c, c.u32(match + pending_player)) != 0) return;
    c.set(3, match + 0x10000);
    if (compare_logical(c, c.u32(match + ball_holder)) != 0) return;
    if (compare(c, s8(c.u8(match + drift_timer)), 0) == 0) return;
    if (compare(c, s8(c.u8(match + play_state)), 0) != 0) {
        c.put8(match + drift_timer, 0);
        c.put8(match + shot_state, 0);
        return;
    }
    bool step = true;
    if (compare(c, s8(c.u8(match + drift_timer)), 2) < 0) {
        if (compare(c, s8(c.u8(match + drift_timer)), 0) <= 0) {
            step = false;
        } else {
            r_roll = u(draw(c, 2, 0x101d6e38));
            const auto timer = s8(c.u8(match + drift_timer));
            const auto squared = u(timer) * u(s8(c.u8(match + drift_timer)));
            step = compare(c, draw(c, squared, 0x101d6e54), std::int32_t(std::uint32_t(r_roll))) >= 0;
        }
    }
    if (step) {
        // Each byte is stepped through r3 (the old value stays there).
        const auto bump = [&](std::uint32_t at, std::uint32_t by) {
            const auto before = c.u8(at);
            c.set(3, before);
            c.put8(at, before + by);
        };
        bump(match + drift_timer, UINT32_MAX);
        if (compare(c, s8(c.u8(match + shot_state)), 0) != 0) bump(match + shot_state, UINT32_MAX);
        const auto turn = [&] { return std::int32_t(c.i16(match + turn_at)); };
        if (compare(c, turn(), 0xdc) > 0 && compare(c, turn(), 0x140) < 0) {
            bump(match + ball_row_at, UINT32_MAX);
        } else if (compare(c, turn(), 0x28) > 0 && compare(c, turn(), 0x8c) < 0) {
            bump(match + ball_row_at, 1);
        } else if (compare(c, turn(), 0x82) > 0 && compare(c, turn(), 0xe6) < 0) {
            bump(match + ball_column_at, UINT32_MAX);
        } else if (compare(c, turn(), 0x32) < 0 || compare(c, turn(), 0x136) > 0) {
            bump(match + ball_column_at, 1);
        }
    }
    // The ball left the pitch's cells (9 rows by 12 columns): the restart is set up.
    if (compare(c, s8(c.u8(match + ball_row_at)), 0) >= 0 && compare(c, s8(c.u8(match + ball_column_at)), 0) >= 0 &&
        compare(c, s8(c.u8(match + ball_row_at)), 9) < 0 && compare(c, s8(c.u8(match + ball_column_at)), 0xc) < 0)
        return;
    c.set({{3, match}, {4, c.u8(match + last_touch_side)}});
    call(c, 0x101f0c00, 0x101d6f6c);
}

void update_teams_players(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101d6f90, 0x50, {.first_gpr = 26});
    Kept<29> r_match(c, match);
    Kept<31> r_side(c, 0);
    Kept<30> r_player(c, c.r(30));
    Kept<28> r_index(c, c.r(28));
    Kept<27> r_substitute(c, c.r(27));
    Kept<26> r_previous(c, c.r(26));
    for (; compare(c, s8(r_side), 2) < 0; r_side = r_side + 1) {
        r_player = side_players(match, r_side);
        for (r_index = 0; compare(c, s8(r_index), 0x14) < 0; r_index = r_index + 1) {
            const std::uint32_t player = r_player;
            c.set(3, player);
            if (compare(c, s8(position(c, player)), 0) >= 0) {
                c.set(3, player);
                c.set({{4, position(c, player)}, {3, team_of_side(match, r_side)}});
                c.call(0x101149f0, 0x101d6ff8);
                if (compare(c, s8(c.r(3)) & 0x10, 0) != 0) {
                    c.set(3, player);
                    c.set({{4, position(c, player)}, {3, team_of_side(match, r_side)}});
                    c.call(0x101149f0, 0x101d7028);
                    if (compare(c, s8(c.r(3)) & 4, 0) != 0) {
                        c.set({{3, match}, {4, r_side}, {5, r_index}});
                        call(c, 0x101fd100, 0x101d7048);
                        r_substitute = c.r(3);
                        c.set(3, player);
                        const auto position_now = position(c, player);
                        const auto other = u(s8(r_side) == 0);
                        const auto record = c.u32(match + other * side_size + u(s8(r_substitute) * std::int32_t(player_size)) + 0x3188);
                        c.set({{4, position_now}, {3, team_of_side(match, r_side)}, {5, record}});
                        c.call(0x10114b50, 0x101d7090);
                    }
                }
            }
            r_previous = player;
            r_player = player + player_size;
        }
    }
}
}  // namespace cm::aot::match_eng
