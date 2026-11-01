// MATCH_ENG.CPP, the review of the match state after each step: tired players, tactic
// changes, substitutions, commentary, and the restart for the current play state.
#include "match_eng_report.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
constexpr std::uint32_t tactic_at = 0x1c4;       // per side: the tactic in force
constexpr std::uint32_t aggression_at = 0x1c6;   // per side, signed
constexpr std::uint32_t announced_at = 0x1ec;    // per side, int16: the clock of the last announcement
constexpr std::uint32_t floats_at = 0x30f0;      // per side

// 11 clock ticks make a minute: the original divides by multiplying with 0x2e8ba2e9.
std::int32_t ticks_to_minutes(std::int32_t ticks) {
    const auto high = std::int32_t((std::int64_t(0x2e8ba2e9) * ticks) >> 32);
    const auto half = high >> 1;
    return half + std::int32_t(std::uint32_t(half) >> 31);
}
// The compiler's branch-free signed `elapsed > 0xa5`, as the byte it stores.
std::uint32_t after_165(std::int32_t elapsed) {
    const auto t = elapsed ^ 0xa5;
    return std::uint32_t((t >> 1) - (t & elapsed)) >> 31;
}

// 0x10113b60: adds a tendency to a side's flags.
void add_tendency(Cpu& c, std::uint32_t flags, std::uint32_t mask, std::uint32_t back) {
    c.set({{3, flags}, {4, mask}});
    c.call(0x10113b60, back);
}
// 0x101d3c50, the fixture's team record of a side.
std::uint32_t team_of(Cpu& c, std::uint32_t match, std::uint32_t side) {
    c.set({{3, match}, {4, side}});
    return fixture_team(c, match, side);
}
}

// 0x101e6c50: after each step of the match: a tired player is announced, tactic changes
// are noticed and answered, substitutions are weighed, the manager's lines are said, and
// the players are placed for the current play state (0x7928).
void review_play(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101e6c50, 0xe0, {.first_gpr = 15});
    Fpu fpu(c);
    Kept<31> self(c, match);
    Kept<27> pool(c, c.toc(-0x52cc));
    Kept<29> singles(c, c.toc(-0x52c8));
    Kept<28> doubles(c, c.toc(-0x52c4));
    Kept<15> scratch(c, c.r(15));
    Kept<16> r16(c, c.r(16));
    Kept<17> r17(c, c.r(17));
    Kept<18> tactic(c, c.r(18));
    Kept<19> extra(c, c.r(19));
    Kept<20> index(c, c.r(20));
    Kept<21> found(c, c.r(21));
    Kept<22> defensive(c, c.r(22));
    Kept<23> side_index(c, c.r(23));
    Kept<24> player(c, c.r(24));
    Kept<26> team(c, c.r(26));
    Kept<30> side(c, c.r(30));
    const auto clock = [&] { return std::int32_t(c.i16(match + clock_at)); };

    c.put8(frame.at(0x41), 0);
    c.set(5, 0);
    c.put8(frame.at(0x42), 0);
    c.put8(match + 0x1de, 0);
    c.set(3, match);
    clear_word_3108(c, match);
    c.set(3, match);
    clear_word_310c(c, match);
    c.set(4, 0);
    c.put32(match + 0x3104, 0);
    c.put8(match + 0x7920, 0);
    c.put8(match + shot_state, 0);
    c.put32(match + 0xe028, 0);
    c.put32(match + 0xe024, 0);
    c.put32(match + 0xdff4, 0);
    c.put32(match + ball_holder, 0);
    c.put8(match + 0x791c, 0xff);
    c.put8(match + 0x792f, 0xff);
    c.put16(match + 0x794e, 0);
    c.put32(match + 0x7c68, 0);
    c.put8(match + 0x1e6, 0);
    c.set(3, match);
    call(c, 0x101d6930, 0x101e6d0c);
    c.set(3, u(c.i8(match + extra_time_flag)));
    if (compare(c, c.i8(match + extra_time_flag)) != 0) return;

    // Whether each side's tactic has been left alone for long (both, at the kick-off).
    if (compare(c, c.i8(match + play_state), 8) == 0) {
        c.put_f32(match + floats_at, c.f32(singles));
        c.put_f32(match + floats_at + 4, c.f32(singles));
        c.put8(frame.at(0x3f), 1);
        c.put8(frame.at(0x40), 1);
    } else {
        const auto first = clock() - c.i16(match + announced_at);
        c.set(5, u(first));
        c.put8(frame.at(0x3f), after_165(first));
        c.put8(frame.at(0x40), after_165(clock() - c.i16(match + announced_at + 2)));
    }
    c.set(3, match);
    const auto raw_minute = ticks_to_minutes(std::int16_t(match_clock(c, match)));
    Kept<25> minute(c, u(std::int8_t(raw_minute)));
    c.set(4, u(raw_minute));

    // Late in the match, now and then, the first tired player without a status is announced.
    if (compare(c, s8(minute), 0x3c) > 0 && compare(c, draw(c, 6, 0x101e6de0)) == 0) {
        found = 0;
        for (side_index = 0; compare(c, s8(side_index), 2) < 0 && compare(c, s8(found)) == 0; side_index = side_index + 1) {
            player = match + players_at + u(s8(side_index)) * side_size;
            c.set(4, u(s8(side_index)) * side_size);
            for (index = 0; compare(c, s8(index), 0x14) < 0 && compare(c, s8(found)) == 0; index = index + 1) {
                if (compare(c, s8(position(c, player)), 0) >= 0) {
                    fitness(c, player);
                    fpu.fcmp(0, c.f(1), c.f32(singles + 0x48));
                    if ((c.cr(0) & 8) && compare(c, s8(status_110(c, player))) == 0) {
                        c.set(3, player);
                        c.call(0x10283690, 0x101e6e54);
                        scratch = c.r(3);
                        c.set(3, player);
                        const auto energy = byte_2f(c, player);
                        c.set({{3, c.u32(match + message_target)}, {4, 0x21cf}, {5, match}, {6, energy}, {7, scratch}, {8, 0}});
                        call(c, 0x101d6710, 0x101e6e7c);
                        found = 1;
                    }
                }
                c.set(3, player);
                c.put32(frame.at(0x5c), player);
                player = player + player_size;
            }
        }
    }

    // A side whose tactic changed is recorded, with the old one.
    for (side = 0; ; side = side + 1) {
        c.set(4, u(s8(side)));
        if (compare(c, s8(side), 2) >= 0) break;
        if (compare(c, c.i8(match + play_state), 5) == 0) break;
        if (compare(c, c.i8(match + extra_time_flag)) != 0) break;
        c.set(3, match + side_stats_at + u(s8(side)) * 0xd8);
        c.call(0x1020c5d0, 0x101e6ee0);
        tactic = c.r(3);
        c.set(4, u(s8(tactic)));
        const auto old = c.u8(match + tactic_at + u(s8(side)));
        c.set(3, u(s8(old)));
        if (compare(c, s8(tactic), s8(old)) != 0 || compare(c, c.i16(match + clock_at), 1) <= 0) {
            c.put8(frame.at(0x41) + u(s8(side)), 1);
            c.put8(frame.at(0x43) + u(s8(side)), old);
            c.put8(match + tactic_at + u(s8(side)), tactic);
        }
    }

    // A side that has just kicked off with a time-wasting tactic gets added time.
    if (compare(c, c.i8(match + possession_side), -1) != 0) {
        c.set(4, u(c.i8(match + possession_side)));
        auto wasting = compare(c, s8(c.u8(match + tactic_at + u(c.i8(match + possession_side)))), 3) == 0;
        if (!wasting) {
            c.set(4, u(c.i8(match + possession_side)));
            const auto kind = s8(c.u8(match + tactic_at + u(c.i8(match + possession_side))));
            c.set(5, u(kind));
            wasting = compare(c, kind, 4) == 0;
        }
        if (wasting) {
            extra = u(std::int8_t(draw(c, 10, 0x101e6fc0)));
            c.put16(match + clock_at, u(c.i16(match + clock_at) + s8(extra)));
            const auto sign = std::uint32_t(s8(extra)) >> 31;
            c.set(4, sign);
            const auto half = std::int32_t(s8(extra) + sign) >> 1;
            c.put8(match + added_time, c.u8(match + added_time) + u(s8(draw(c, u(half), 0x101e6fe8))));
            const auto other = std::uint32_t(c.i8(match + possession_side) == 0);
            if (compare(c, s8(c.u8(match + tactic_at + other)), 1) == 0) {
                c.set(3, match);
                c.set(4, u(std::int8_t(other)));
                c.set_f(2, c.f64(doubles + 0x58));
                c.set(5, u(s8(extra)));
                c.set_f(1, c.f64(doubles + 0x60));
                c.put32(frame.at(0x88), u(s8(extra)) ^ 0x80000000);
                c.set(5, 0x43300000);
                c.put32(frame.at(0x84), 0x43300000);
                const auto biased = fpu.fsub(c.f64(frame.at(0x84)), c.f(1));
                c.set_f(1, fpu.frsp(fpu.fadd(c.f(2), fpu.fdiv(biased, c.f64(doubles + 0x268)))));
                call(c, 0x101d6860, 0x101e706c);
            }
        }
    }

    // Late in the match a side that can still supply a substitute may plan one, once the
    // clock has moved on since its last plan (side 0 first, then side 1 after the period lines).
    c.set({{3, match}, {4, 0}});
    call(c, 0x101da5a0, 0x101e7078);
    if (compare(c, s8(c.r(3))) == 0) {
        const auto limit = draw(c, 0x32, 0x101e708c) + 10;
        if (compare(c, s8(minute), std::int32_t(limit)) > 0 && compare(c, clock() - c.i16(match + 0x7944), 0x37) > 0) {
            c.set({{3, match + 0x7aac}, {4, 0}});
            c.call(0x1020ef70, 0x101e70bc);
            c.put16(match + 0x7944, u(clock()));
            c.set({{3, match}, {4, 0}});
            call(c, 0x101d6a90, 0x101e70d0);
            c.set(3, match);
            call(c, 0x101d6f90, 0x101e70d8);
        }
    }

    // The announcements of added time, at the end of the first half and of normal time.
    c.set(3, match);
    const auto period = std::int16_t(minute_clock(c, match));
    const auto announce_added = [&](std::uint32_t flag_at, std::uint32_t kept_slot, std::uint32_t back) {
        const auto added = ticks_to_minutes(s8(c.u8(match + added_time)));
        const auto minutes = added > 2 ? added : 2;
        if (kept_slot == 0x39) r17 = u(minutes);
        else r16 = u(minutes);
        c.put8(frame.at(kept_slot), u(minutes));
        c.set({{3, c.u32(match + message_target)}, {4, 0x21c1}, {5, match}, {6, c.u8(match + possession_side)}, {7, 0}, {8, 0},
               {9, 0}, {10, u(s8(c.u8(frame.at(kept_slot))))}});
        c.call(0x10201180, back);
        c.put8(match + flag_at, 1);
    };
    if (period == 0x1ef) {
        if (compare(c, s8(minute), 0x27) > 0 && compare(c, s8(minute), 0x2e) < 0 && compare(c, c.i8(match + 0x77be)) == 0)
            announce_added(0x77be, 0x39, 0x101e71bc);
    } else if (period == 0x3de) {
        if (compare(c, s8(minute), 0x54) > 0 && compare(c, s8(minute), 0x5b) < 0 && compare(c, c.i8(match + 0x77bf)) == 0)
            announce_added(0x77bf, 0x38, 0x101e7270);
    }

    c.set({{3, match}, {4, 1}});
    call(c, 0x101da5a0, 0x101e7284);
    c.set(4, u(s8(c.r(3))));
    if (compare(c, s8(c.r(4))) == 0) {
        const auto result = draw(c, 0x32, 0x101e7298);
        c.set(4, result);
        const auto limit = result + 10;
        if (compare(c, s8(minute), std::int32_t(limit)) > 0 && compare(c, clock() - c.i16(match + 0x7946), 0x37) > 0) {
            c.set({{3, match + 0x7b84}, {4, 0}});
            c.call(0x1020ef70, 0x101e72cc);
            c.put16(match + 0x7946, u(clock()));
            c.set({{3, match}, {4, 1}});
            call(c, 0x101d6a90, 0x101e72e0);
            c.set(3, match);
            call(c, 0x101d6f90, 0x101e72e8);
        }
    }

    // What each side whose tactic changed does about it (the tendency flags 0x10113b60 adds,
    // the announcements), by the new tactic.
    for (side = 0; compare(c, s8(side), 2) < 0; side = side + 1) {
        c.set({{3, match}, {4, side}});
        call(c, 0x101da5a0, 0x101e72fc);
        c.set(4, u(s8(c.r(3))));
        if (compare(c, s8(c.r(4))) != 0) continue;
        const auto changed_flag = c.u8(frame.at(0x41) + u(s8(side)));
        c.set(3, changed_flag);
        c.set(4, u(s8(changed_flag)));
        if (compare(c, s8(changed_flag)) == 0) continue;
        const auto stats = match + side_stats_at + u(s8(side)) * 0xd8;
        const auto flags = match + team_tactics + u(s8(side)) * team_tactics_size;
        const auto tendency = [&](std::uint32_t mask, std::uint32_t back) { add_tendency(c, flags, mask, back); };
        const auto previous = [&] { return c.u8(frame.at(0x43) + u(s8(side))); };
        const auto kicked = [&] { return c.u8(frame.at(0x3f) + u(s8(side))); };
        const auto message = [&](std::uint32_t id, std::uint32_t back) { post_message(c, match, id, side, back); };
        const auto announced_now = [&] { c.put16(match + announced_at + 2 * u(s8(side)), u(clock())); };
        const auto aggression_sign = [&](std::uint32_t slot) {
            if (compare(c, s8(c.u8(match + aggression_at + u(s8(side)))), 0) > 0) c.put32(frame.at(slot), 3);
            else {
                c.set(4, 0);
                c.put32(frame.at(slot), 0);
            }
        };
        const auto rating_of = [&](std::uint32_t offset) { return s8(c.u8(word_24(c, stats) + offset)); };
        // The tail of two cases: when the previous tactic was 2 or 1, a line may be said.
        const auto said_after_change = [&](std::uint32_t check_back, std::uint32_t good_back, std::uint32_t poor_back) {
            if (compare(c, s8(previous()), 2) == 0 || compare(c, s8(previous()), 1) == 0) {
                if (compare(c, s8(kicked())) == 0) return;
                c.set({{3, match}, {4, side}, {5, frame.at(0x3e)}, {6, frame.at(0x3d)}, {7, frame.at(0x3c)}, {8, frame.at(0x3b)},
                       {9, frame.at(0x3a)}, {10, 0}});
                call(c, 0x101f21d0, check_back);
                if (compare(c, s8(c.r(3))) > 0) message(0x213b, good_back);
                else message(0x213d, poor_back);
                announced_now();
            }
        };
        c.put32(frame.at(0x58), 0);
        c.set(3, stats);
        auto text = player_text(c, stats);
        c.set(4, pool + 0x3f0);
        c.set(3, text);
        c.call(0x1077dc90, 0x101e7344);
        if (compare(c, std::int32_t(c.r(3))) != 0) {
            c.set(3, stats);
            text = player_text(c, stats);
            c.set(4, pool + 0x3f6);
            c.set(3, text);
            c.call(0x1077dc90, 0x101e7368);
            if (compare(c, std::int32_t(c.r(3))) != 0) c.put32(frame.at(0x58), 1);
        }
        defensive = u(s8(c.u32(frame.at(0x58))));
        const auto in_force = s8(c.u8(match + tactic_at + u(s8(side))));
        c.set(3, c.u8(match + tactic_at + u(s8(side))));
        if (in_force == 3) {
            // 0x7880
            const auto style = word_24(c, stats);
            c.set(4, style);
            if (compare_logical(c, style) != 0) {
                const auto rating = rating_of(0x2e);
                c.set(5, u(rating));
                if (compare(c, rating, 0x10) > 0) tendency(0x20000, 0x101e78e4);
                else tendency(0x8000, 0x101e7904);
                aggression_sign(0x4c);
                if (compare(c, rating_of(0x33), std::int32_t(c.u32(frame.at(0x4c)) + 10)) < 0) tendency(0x1000, 0x101e7974);
                else tendency(0x800, 0x101e7990);
            } else {
                tendency(0x8000, 0x101e79b0);
                tendency(0x1000, 0x101e79cc);
            }
            tendency(0x2000, 0x101e79e4);
            if (compare(c, clock(), 1) <= 0) message(0x2136, 0x101e7a08);
            else said_after_change(0x101e7a78, 0x101e7ab8, 0x101e7a9c);
            c.set({{3, match}, {4, side}});
            call(c, 0x101f7500, 0x101e7ad8);
        } else if (in_force == 1) {
            // 0x73bc
            bool rated = false;
            if (compare_logical(c, word_24(c, stats)) != 0) {
                c.set_f(1, c.f32(match + floats_at + 4 * u(s8(side))));
                fpu.fcmp(0, c.f(1), c.f64(doubles + 0xe8));
                rated = (c.cr(0) & 8) != 0;
            }
            if (rated) {
                if (compare(c, rating_of(0x2e), 6) > 0 && compare(c, s8(defensive)) != 0) tendency(0x10000, 0x101e7440);
                else tendency(0x20000, 0x101e7460);
            } else if (compare(c, s8(defensive)) != 0) {
                c.set(5, u(s8(side)));
                tendency(0x10000, 0x101e748c);
            } else {
                tendency(0x20000, 0x101e74ac);
            }
            tendency(0x800, 0x101e74c4);
            c.set(5, u(s8(side)) * team_tactics_size);
            tendency(0x2000, 0x101e74dc);
            if (compare(c, clock(), 1) <= 0) {
                message(0x2137, 0x101e7500);
            } else if (compare(c, s8(kicked())) != 0) {
                message(0x2139, 0x101e7534);
                c.set(4, u(clock()));
                announced_now();
            }
            c.set({{3, match}, {4, side}});
            call(c, 0x101f7280, 0x101e7554);
        } else if (in_force == 2) {
            // 0x7558
            if (compare_logical(c, word_24(c, stats)) != 0) {
                const auto aggression = c.u8(match + aggression_at + u(s8(side)));
                c.set(5, aggression);
                aggression_sign(0x54);
                if (compare(c, rating_of(0x2e), std::int32_t(c.u32(frame.at(0x54)) + 0xf)) > 0 && compare(c, s8(defensive)) != 0) {
                    tendency(0x10000, 0x101e75f4);
                    tendency(0x800, 0x101e760c);
                } else {
                    bool heavy = true;
                    if (compare(c, rating_of(0x2e), 5) <= 0 && compare(c, s8(c.u8(match + aggression_at + u(s8(side))))) != 0) {
                        c.set_f(1, c.f32(match + floats_at + 4 * u(s8(side))));
                        fpu.fcmp(0, c.f(1), c.f64(doubles + 0x58));
                        heavy = (c.cr(0) & 4) != 0;
                    }
                    if (heavy) {
                        tendency(0x20000, 0x101e7684);
                        scratch = u(s8(side));
                        c.set(6, u(s8(scratch)) * team_tactics_size);
                        tendency(0x800, 0x101e769c);
                    } else {
                        tendency(0x8000, 0x101e76bc);
                        aggression_sign(0x50);
                        if (compare(c, rating_of(0x33), std::int32_t(c.u32(frame.at(0x50)) + 4)) < 0) tendency(0x1000, 0x101e772c);
                        else tendency(0x800, 0x101e7748);
                    }
                }
            } else {
                tendency(0x20000, 0x101e7768);
                tendency(0x800, 0x101e7780);
            }
            tendency(0x2000, 0x101e7798);
            bool matches = false;
            if (compare(c, clock(), 1) > 0) {
                const auto before = s8(previous());
                if (compare(c, before, 3) == 0) {
                    matches = true;
                } else {
                    c.set(5, u(before));
                    matches = compare(c, before, 4) == 0 || compare(c, before, 5) == 0;
                }
            }
            if (matches) {
                message(0x2138, 0x101e7804);
            } else if (compare(c, clock(), 1) > 0 && compare(c, s8(previous()), 1) == 0 && compare(c, s8(kicked())) != 0) {
                message(0x213a, 0x101e785c);
                announced_now();
            }
            c.set({{3, match}, {4, side}});
            call(c, 0x101f7500, 0x101e787c);
        } else if (in_force == 5) {
            // 0x7adc
            if (compare_logical(c, word_24(c, stats)) != 0) {
                if (compare(c, rating_of(0x2e), 0x10) > 0 && compare(c, s8(defensive)) != 0) {
                    tendency(0x10000, 0x101e7b44);
                } else if (compare(c, rating_of(0x2e), 0xc) > 0) {
                    tendency(0x20000, 0x101e7b88);
                } else {
                    tendency(0x8000, 0x101e7ba8);
                }
                aggression_sign(0x48);
                if (compare(c, rating_of(0x33), std::int32_t(c.u32(frame.at(0x48)) + 10)) < 0) tendency(0x1000, 0x101e7c18);
                else tendency(0x800, 0x101e7c34);
            } else {
                tendency(0x20000, 0x101e7c54);
                c.set(5, u(s8(side)));
                tendency(0x1000, 0x101e7c6c);
            }
            tendency(0x2000, 0x101e7c84);
            if (compare(c, s8(kicked())) != 0) {
                message(0x213c, 0x101e7cb4);
                c.set(5, u(clock()));
                announced_now();
            }
            c.set({{3, match}, {4, side}});
            call(c, 0x101f7500, 0x101e7cd4);
        } else if (in_force == 4) {
            // 0x7cd8
            if (compare_logical(c, word_24(c, stats)) != 0) {
                if (compare(c, rating_of(0x2e), 0x12) > 0) {
                    tendency(0x20000, 0x101e7d34);
                } else {
                    c.set(5, u(s8(side)) * team_tactics_size);
                    tendency(0x8000, 0x101e7d54);
                }
            } else {
                tendency(0x8000, 0x101e7d74);
            }
            tendency(0x800, 0x101e7d8c);
            tendency(0x4000, 0x101e7da4);
            if (compare(c, clock(), 1) <= 0) message(0x2135, 0x101e7dc8);
            else said_after_change(0x101e7e38, 0x101e7e7c, 0x101e7e60);
            c.set({{3, match}, {4, side}});
            call(c, 0x101f7500, 0x101e7e9c);
        }
    }

    // The lines about the teams when a shot has just happened: which team they are about
    // (`first`) follows from the two numbers 0x101f21d0 reports, then up to seven random lines
    // about the other team's record, and a line about its kind.
    struct Line { std::int32_t slot; std::uint32_t id, draw_back, post_back; };
    static constexpr Line lines_first_zero[7] = {{-0x7300, 0x2127, 0x101e8100, 0x101e8140}, {-0x7304, 0x2128, 0x101e814c, 0x101e818c},
                                                 {-0x7308, 0x2163, 0x101e8198, 0x101e81e0}, {-0x7318, 0x2166, 0x101e81ec, 0x101e822c},
                                                 {-0x730c, 0x212b, 0x101e8238, 0x101e827c}, {-0x7310, 0x212c, 0x101e8288, 0x101e82c8},
                                                 {-0x7498, 0x212d, 0x101e82d4, 0x101e8318}};
    static constexpr Line lines_first_one[7] = {{-0x7300, 0x2127, 0x101e8500, 0x101e8540}, {-0x7304, 0x2128, 0x101e854c, 0x101e858c},
                                                {-0x7308, 0x2163, 0x101e8598, 0x101e85d8}, {-0x7318, 0x2166, 0x101e85e4, 0x101e8624},
                                                {-0x730c, 0x212b, 0x101e8630, 0x101e8670}, {-0x7310, 0x212c, 0x101e867c, 0x101e86bc},
                                                {-0x7498, 0x212d, 0x101e86c8, 0x101e870c}};
    const auto team_lines = [&](bool first_is_one) {
        const std::uint32_t first = first_is_one ? 1 : 0, other = 1 - first;
        const auto& lines = first_is_one ? lines_first_one : lines_first_zero;
        team = team_of(c, match, first);
        c.set({{3, c.u32(c.toc(-0x79f0))}, {4, team}});
        c.call(0x101b0020, first_is_one ? 0x101e84b8 : 0x101e80bc);
        if (compare(c, s8(c.r(3))) != 0) {
            const auto detail = c.u32(team + 0x2c);
            c.set(3, detail);
            if (compare(c, s8(c.u8(detail + 0x119)), 8) == 0) {
                post_message(c, match, 0x2129, first, first_is_one ? 0x101e84f4 : 0x101e80f4);
                return;
            }
        }
        for (unsigned k = 0; k < 7; ++k) {
            if (compare(c, draw(c, 2, lines[k].draw_back)) != 0) continue;
            const auto record = team_of(c, match, other);
            const auto slot_at = c.toc(lines[k].slot);
            const auto word = c.u32(record), value = c.u32(slot_at);
            // The original's scratch registers after the loads differ per line.
            if (!first_is_one) {
                switch (k) {
                case 0: c.set({{4, word}, {3, value}}); break;
                case 1: c.set({{5, word}, {4, slot_at}}); break;
                case 2: c.set({{5, slot_at}, {3, value}}); break;
                case 3: c.set(3, value); break;
                case 6: c.set({{4, value}, {3, slot_at}}); break;
                default: c.set({{4, word}, {3, slot_at}}); break;
                }
            } else {
                switch (k) {
                case 0: c.set({{3, word}, {4, slot_at}}); break;
                case 2: c.set({{3, word}, {4, value}}); break;
                case 3: case 4: case 6: c.set(3, value); break;
                default: c.set({{4, word}, {3, slot_at}}); break;
                }
            }
            if (compare(c, std::int32_t(value), std::int32_t(word)) == 0) {
                post_message(c, match, lines[k].id, other, lines[k].post_back);
                return;
            }
        }
        const auto record = team_of(c, match, other);
        const auto detail = c.u32(record + 0x2c);
        const auto raw_kind = c.u8(detail + 0x118);
        c.set(3, u(s8(raw_kind)));
        if (!first_is_one) c.set(4, raw_kind);
        const auto kind = s8(raw_kind);
        const auto say = [&](std::uint32_t id, std::uint32_t back) { post_message(c, match, id, other, back); };
        const auto by_draw = [&](std::uint32_t draw_back, std::uint32_t high_id, std::uint32_t high_back, std::uint32_t low_id,
                                 std::uint32_t low_back) {
            if (compare(c, draw(c, 2, draw_back)) != 0) say(high_id, high_back);
            else say(low_id, low_back);
        };
        const auto against_c = compare(c, kind, 0xc);
        if (against_c == 0) {
            by_draw(first_is_one ? 0x101e8760 : 0x101e836c, 0x216a, first_is_one ? 0x101e8780 : 0x101e838c, 0x216b,
                    first_is_one ? 0x101e879c : 0x101e83a8);
        } else if (against_c > 0) {
            const auto against_e = compare(c, kind, 0xe);
            if (against_e == 0) say(0x2118, first_is_one ? 0x101e8800 : 0x101e840c);
            else if (against_e < 0)
                by_draw(first_is_one ? 0x101e87a8 : 0x101e83b4, 0x216a, first_is_one ? 0x101e87c8 : 0x101e83d4, 0x2167,
                        first_is_one ? 0x101e87e4 : 0x101e83f0);
        } else if (compare(c, kind, 4) == 0) {
            by_draw(first_is_one ? 0x101e8760 : 0x101e836c, 0x216a, first_is_one ? 0x101e8780 : 0x101e838c, 0x216b,
                    first_is_one ? 0x101e879c : 0x101e83a8);
        }
    };
    const auto commentary = [&] {
        if (compare(c, s8(byte_7931(c, match))) == 0) return;
        scratch = c.u8(match + play_state);
        c.set(6, u(s8(scratch)));
        if (compare(c, s8(scratch), 3) == 0) return;
        if (compare(c, c.i8(match + play_state), 5) == 0) return;
        if (compare(c, c.i8(match + play_state), 6) == 0) {
            const auto too_close = [&](std::uint32_t to_along, std::uint32_t back) {
                c.set({{3, match}, {4, c.u8(match + ball_row_at)}, {5, c.u8(match + ball_column_at)}, {6, 4}, {7, to_along}});
                call(c, 0x101ef8d0, back);
                fpu.fcmp(0, c.f(1), c.f64(doubles + 0x30));
                return (c.cr(0) & 8) != 0;
            };
            if (compare(c, c.i8(match + possession_side)) == 0 && too_close(0xb, 0x101e7f18)) return;
            if (compare(c, c.i8(match + possession_side), 1) == 0 && too_close(0, 0x101e7f4c)) return;
        }
        c.set({{3, match}, {4, 0}, {5, frame.at(0x3e)}, {6, frame.at(0x3d)}, {7, frame.at(0x3c)}, {8, frame.at(0x3b)},
               {9, frame.at(0x3a)}, {10, 0}});
        call(c, 0x101f21d0, 0x101e7f7c);
        c.set(4, c.r(3));
        const auto low = [&] { return s8(c.u8(frame.at(0x3e))); };
        const auto high = [&] { return s8(c.u8(frame.at(0x3d))); };
        const auto third = [&] { return s8(c.u8(frame.at(0x3c))); };
        bool even = false;
        if (compare(c, low(), high()) < 0) {
            even = true;
        } else {
            c.set(4, c.u8(frame.at(0x3d)));
            if (compare(c, low(), high()) == 0) even = compare(c, third()) == 0;
        }
        if (even && compare(c, draw(c, 10, 0x101e7fc8)) == 0) {
            c.set(3, match + side_stats_at);
            const auto name = word_20(c, match + side_stats_at);
            c.set({{3, name}, {4, pool + 0x406}});
            c.call(0x1036ff00, 0x101e7fe8);
            c.set(4, c.r(3));
            if (compare(c, s8(c.r(3))) != 0) {
                post_message(c, match, 0x212a, 1, 0x101e8010);
                return;
            }
        }
        c.set(4, u(low()));
        const auto first_draw = draw(c, u(high() - low() + 1), 0x101e8030);
        bool zero_first = false;
        if (compare(c, std::int32_t(first_draw)) != 0) {
            c.set(4, u(low()));
            if (compare(c, low(), high()) < 0) {
                zero_first = true;
            } else if (compare(c, low(), high()) == 0) {
                c.set(4, c.u8(frame.at(0x3c)));
                zero_first = compare(c, third()) == 0;
            }
        }
        if (zero_first) return team_lines(false);
        c.set({{4, u(low())}, {5, u(high())}});
        if (compare(c, std::int32_t(draw(c, u(low() + 5 - high()), 0x101e8094))) == 0) return team_lines(false);
        const auto second_draw = draw(c, u(low() - high() + 1), 0x101e842c);
        bool one_first = false;
        if (compare(c, std::int32_t(second_draw)) != 0) {
            if (compare(c, low(), high()) > 0) one_first = true;
            else if (compare(c, low(), high()) == 0) {
                one_first = compare(c, third()) != 0;
            }
        }
        if (!one_first) {
            c.set(4, u(5 - low()));
            if (compare(c, std::int32_t(draw(c, u(5 - low() + high()), 0x101e8490))) != 0) return;
        }
        team_lines(true);
    };
    commentary();

    // The restart for the current play state.
    const auto state = c.u8(match + play_state);
    c.set(3, state);
    if (compare_logical(c, u(s8(state)), 8) <= 0) {
        switch (s8(state)) {
        case 1: c.set(3, match); return;
        case 2: c.set(3, match); c.link(0x101e8838); restart_state_2(c, match); return;
        case 3: c.set(3, match); c.link(0x101e8844); restart_state_3(c, match); return;
        case 4: c.set(3, match); c.link(0x101e8850); restart_state_4(c, match); return;
        case 5: c.set(3, match); c.link(0x101e885c); restart_state_5(c, match); return;
        case 6: case 7: c.set(3, match); c.link(0x101e8868); restart_state_6(c, match); return;
        case 8: c.set(3, match); c.link(0x101e8874); restart_state_8(c, match); return;
        default: break;
        }
    }
    report_assertion(c, {.at = 0x101e888c, .file = pool + 0x45, .format = pool + 0x53, .title = pool + 0x60,
                         .name = frame.at(0x65), .extension = frame.at(0x60), .line = 0x295b});
    c.set(4, c.toc(-0x7bac));
    c.set(3, UINT32_MAX);
    c.call(0x1076f7d0, 0x101e88d4);
}
}
