// MATCH_ENG.CPP, contact: the hurt players of each side leave the pitch (0x101f6c60, the entry host code
// reaches through run_until).
#include "match_eng_contact.hpp"
#include "match_eng_shots.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace shots_detail;

namespace {
constexpr std::uint32_t side_of = 0x10283690;
}

// 0x101f6c60: every player flagged (+0x1d) is dealt with: hurt players (+0x1c) leave the pitch with a message,
// the others may get one, and the side's bookkeeping is brought up to date.
void handle_injured_players(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101f6c60, 0x80, {.first_gpr = 17});
    Kept<30> match_kept(c, match);
    const auto text_object = c.toc(-0x7a6c);
    keep(c, {{28, text_object}});
    const auto stats_of = [&](std::uint32_t side) { return match + u(s8(side) * 0xd8) + side_stats_at; };
    for (Kept<29> side(c, 0); compare(c, s8(side), 2) < 0; side = side + 1) {
        Kept<27> flagged(c, 0);
        Kept<31> player(c, match + u(s8(side) * std::int32_t(side_size)) + players_at);
        Kept<26> index(c, 0);
        const auto side_report = [&](std::uint32_t who, std::uint32_t back) {
            c.set(3, who);
            call(c, side_of, back);
            keep(c, {{19, c.r(3)}});
        };
        const auto position_of = [&](std::uint32_t who) { c.set(3, who); return s8(position(c, who)); };
        // bl 0x101d6710: a message about the player, with the side report in r19.
        const auto message = [&](std::uint32_t id, std::uint32_t back_side, std::uint32_t back_message) {
            side_report(player, back_side);
            c.set(3, player);
            c.set(6, byte_2f(c, player));
            c.set({{3, c.u32(match + 0x79f4)}, {4, id}, {5, match}, {7, c.r(19)}, {8, 0}});
            call(c, 0x101d6710, back_message);
        };
        for (; compare(c, std::int32_t(index), 0x14) < 0; index = index + 1, keep(c, {{20, player}}), player = player + player_size) {
            if (compare(c, position_of(player), 0) < 0 && compare(c, position_of(player), -3) != 0) continue;
            if (compare(c, s8(c.u8(player + 0x1d)), 0) == 0) continue;
            flagged = flagged + 1;
            if (compare(c, s8(c.u8(player + 0x1c)), 0) != 0) {
                if (compare(c, position_of(player), 0) >= 0) {
                    message(0x1f56, 0x101f6d04, 0x101f6d2c);
                    c.set(3, stats_of(side));
                    const bool counted = compare(c, s8(position(c, c.r(3))), 0) != 0;
                    keep(c, {{25, counted ? UINT32_MAX : UINT32_MAX - 1}});
                    c.set({{3, match}, {4, player}, {5, c.r(25)}});
                    call(c, 0x101f1950, 0x101f6d68);
                    c.set(3, player);
                    c.set(4, position(c, player));
                    c.set(3, player);
                    set_previous_position(c, player, c.r(4));
                    c.set({{3, player}, {4, UINT32_MAX - 1}});
                    set_position(c, player, c.r(4));
                } else {
                    message(0x1f57, 0x101f6d94, 0x101f6dbc);
                    c.set({{3, match}, {4, player}, {5, 0}});
                    call(c, 0x101f1950, 0x101f6dcc);
                }
                c.put8(player + 0x22, u(contact_detail::divide_by_11(c.i16(match + 0x7948))));
                c.set(3, match);
                // The player's minute is capped at the end of the period the clock has reached.
                const auto cap_minute = [&](std::int32_t cap) {
                    const auto stored = s8(c.u8(player + 0x22));
                    const auto capped = stored < cap ? u(stored) : u(cap);
                    c.put8(player + 0x22, capped);
                    return capped;
                };
                switch (std::int16_t(minute_clock(c, match))) {
                    case 0x1ef: keep(c, {{24, cap_minute(0x2d)}}); break;  // end of the first half
                    case 0x3de: keep(c, {{23, cap_minute(0x5a)}}); break;  // end of normal time
                    case 0x483: keep(c, {{22, cap_minute(0x69)}}); break;  // end of the first extra period
                    case 0x528: keep(c, {{21, cap_minute(0x78)}}); break;  // end of extra time
                    default: break;
                }
            } else if (compare(c, std::int32_t(draw(c, 4, 0x101f6ee4)), 0) == 0 || c.u32(player + 0x58) != 0) {
                message(0x1f58, 0x101f6f00, 0x101f6f28);
            }
            if (compare_logical(c, c.u32(player + 0x58)) != 0) {
                const auto culprit = c.u32(player + 0x58);
                side_report(culprit, 0x101f6f3c);
                c.set(3, c.u32(player + 0x58));
                keep(c, {{18, byte_2f(c, c.r(3))}});
                c.set(3, player);
                call(c, side_of, 0x101f6f54);
                keep(c, {{17, c.r(3)}});
                c.set(3, player);
                c.set(6, byte_2f(c, player));
                c.set({{3, c.u32(match + 0x79f4)}, {4, 0x20f6}, {5, match}, {7, c.r(17)}, {8, c.r(18)}, {9, c.r(19)}, {10, 0}});
                c.call(0x10201180, 0x101f6f84);
            }
            c.put8(player + 0x1d, 0);
        }
        if (compare(c, s8(flagged), 0) == 0) continue;
        c.set({{3, match}, {4, side}});
        call(c, 0x101da5a0, 0x101f6fb8);
        if (compare(c, s8(c.r(3)), 0) != 0) continue;
        c.set(3, stats_of(side));
        const auto stamp_clock = [&] { c.put16(match + 0x7944 + u(s8(side)) * 2, u(c.i16(match + 0x7948))); };
        if (compare(c, s8(position(c, c.r(3))), 0) != 0) {
            c.set({{3, stats_of(side)}, {4, 0}});
            call(c, 0x1020ef70, 0x101f6ffc);
            stamp_clock();
        } else {
            c.set(3, stats_of(side));
            call(c, 0x102104d0, 0x101f7028);
            c.set({{3, match}, {4, side}});
            call(c, 0x101d3b60, 0x101f7034);
            if (compare(c, s8(position(c, c.r(3))), 0) < 0) {
                c.set({{3, match}, {4, side}});
                call(c, 0x101d3bf0, 0x101f7050);
                keep(c, {{17, c.r(3)}});
                c.set(3, stats_of(side));
                call(c, 0x10210430, 0x101f7068);
                keep(c, {{18, c.r(3)}});
                c.set({{3, match}, {4, side}});
                call(c, 0x101d3c50, 0x101f7078);
                c.set({{4, c.r(3)}, {3, c.u32(c.r(28))}, {5, c.r(18)}, {6, c.r(17)}});
                c.call(0x103b0bf0, 0x101f708c);
            }
            c.set({{3, match}, {4, side}});
            call(c, 0x101f1de0, 0x101f7098);
            c.set({{3, match}, {4, side}});
            call(c, 0x101f1e10, 0x101f70a4);
            stamp_clock();
        }
        c.set({{3, match}, {4, side}});
        call(c, 0x101d6a90, 0x101f70c4);
        c.set(3, match);
        call(c, 0x101d6f90, 0x101f70cc);
    }
    c.put8(match + 0x792b, 0);
}
}
