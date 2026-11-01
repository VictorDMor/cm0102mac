// MATCH_ENG.CPP, match flow: period ends and the penalty shoot-out.
#include "match_eng_flow.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
// Match object fields (offsets from the match).
constexpr std::uint32_t kickoff_side = 0x792c;
constexpr std::uint32_t restart_wait = 0x1e3;
constexpr std::uint32_t kicking_side = 0x7914;
constexpr std::uint32_t kicks_taken = 0x7915;         // per side
constexpr std::uint32_t kicks_left = 0x7917;          // per side
constexpr std::uint32_t announced = 0x7919;
constexpr std::uint32_t saved_target_byte = 0x791a;
constexpr std::uint32_t kick_order = 0x1c8;           // 2 x 11 player indexes
constexpr std::uint32_t has_kicked = 0x7952;          // 2 x 20 flags

// Per-side counters at stride 0x36: the period's goals, and the shoot-out's.
constexpr std::uint32_t period_goals(unsigned side) { return 0xe03d + 0x36 * side; }
constexpr std::uint32_t shootout_goals(unsigned side) { return 0xe03c + 0x36 * side; }

// Fixture fields (+0x3118 of the match); the score fields are two bytes, one per side.
constexpr std::uint32_t fx_flags = 0x38;              // tie rules (int16)
constexpr std::uint32_t fx_full_time = 0x43;
constexpr std::uint32_t fx_half_time = 0x45;
constexpr std::uint32_t fx_extra_time = 0x47;
constexpr std::uint32_t fx_shootout = 0x49;
constexpr std::uint32_t fx_first_leg = 0x4b;          // -1 when none
constexpr std::int32_t rule_1 = 1, rule_two_legs = 2, rule_4 = 4, rule_20 = 0x20, rule_4000 = 0x4000;

std::uint32_t fixture_of(Cpu& c, std::uint32_t match) { return c.u32(match + fixture_at); }

// A kick-off: the side due to kick gets the ball, and the next one is the other side's (1 when this was 0).
std::uint32_t flip_kickoff(Cpu& c, std::uint32_t match) {
    c.put8(match + possession_side, c.u8(match + kickoff_side));
    const auto flipped = std::uint32_t(c.i8(match + kickoff_side) == 0);
    c.put8(match + kickoff_side, flipped);
    return flipped;
}

// Calls the line-up check for both sides.
void check_both_sides(Cpu& c, std::uint32_t match, std::uint32_t back_first, std::uint32_t back_second) {
    enter(c, back_first, {match, 0});
    check_lineup_balance(c, match, 0);
    enter(c, back_second, {match, 1});
    check_lineup_balance(c, match, 1);
}

// 0x101d98c8: play stops for the break and the next period is announced.
std::uint32_t break_after_checks(Cpu& c, std::uint32_t match) {
    c.put8(match + play_state, 8);
    c.put8(match + restart_wait, 0);
    c.set({{3, match}, {4, 1}});
    call(c, 0x101d9510, 0x101d98e4);
    c.set({{3, match}, {4, 0}});
    call(c, 0x101f2400, 0x101d98f0);
    c.put16(match + period_end, 0);
    return result(c, 4);
}

// Phase 0x1ef: half time.
std::uint32_t half_time(Cpu& c, std::uint32_t match) {
    const auto clock = c.i16(match + clock_at);
    c.set(4, u(clock));
    if (compare(c, clock, s8(c.u8(match + added_time)) + 0x1ef) <= 0) return result(c, 0);
    const auto fixture = fixture_of(c, match);
    c.put8(fixture + fx_half_time, c.u8(match + period_goals(0)));
    c.put8(fixture + fx_half_time + 1, c.u8(match + period_goals(1)));
    post_message(c, match, 0x20f1, 0, 0x101d9674);
    c.put16(match + clock_at, 0x1ef);
    c.put8(match + added_time, 0);
    c.set({{3, match}, {4, 0x3de}, {5, 0}});
    call(c, 0x101f2400, 0x101d9690);
    c.set({{3, match}, {4, 0}});
    call(c, 0x101d9510, 0x101d969c);
    c.put16(match + period_end, 0x3de);
    c.put8(match + play_state, 8);
    c.set(4, 0);
    c.put8(match + restart_wait, 0);
    const auto pending = c.u32(match + pending_player);
    if (compare_logical(c, pending) != 0) { c.set(3, pending); call(c, 0x1022e710, 0x101d96d0); }
    c.set(5, flip_kickoff(c, match));  // the original leaves the new side in r5
    return result(c, 1);
}

// Phase 0x3de: full time. A tie goes to extra time, the shoot-out or ends the match,
// by the fixture's rules and the first leg.
std::uint32_t full_time(Cpu& c, std::uint32_t match) {
    const auto clock = c.i16(match + clock_at);
    c.set(4, u(clock));
    if (compare(c, clock, s8(c.u8(match + added_time)) + 0x3de) <= 0) return result(c, 0);
    const auto fixture = fixture_of(c, match);
    c.put8(fixture + fx_full_time, c.u8(match + period_goals(0)));
    c.put8(fixture + fx_full_time + 1, c.u8(match + period_goals(1)));
    const auto flags = [&] { return std::int32_t(c.i16(fixture + fx_flags)); };
    const auto has = [&](std::int32_t rule) { return compare(c, flags() & rule) != 0; };
    const auto first_leg = [&](unsigned side) { return std::int32_t(c.i8(fixture + fx_first_leg + side)); };
    const auto full = [&](unsigned side) { return std::int32_t(c.i8(fixture + fx_full_time + side)); };
    // The period ends; the line-ups are checked, then the next period is announced.
    const auto closed = [&](std::uint32_t back_first, std::uint32_t back_second, std::uint32_t back_period,
                            std::uint32_t back_clock) {
        c.put8(match + play_state, 8);
        c.put8(match + restart_wait, 0);
        c.put16(match + period_end, 0);
        check_both_sides(c, match, back_first, back_second);
        c.set({{3, match}, {4, 1}});
        call(c, 0x101d9510, back_period);
        c.set({{3, match}, {4, 0}});
        call(c, 0x101f2400, back_clock);
        return result(c, 4);
    };
    // The tie goes to extra time (two variants, differing in the order of the calls).
    const auto extra_time = [&](bool release_last) {
        post_message(c, match, 0x20f2, 0, release_last ? 0x101d99c8 : 0x101d9b2c);
        c.put16(match + clock_at, 0x3de);
        c.put8(match + added_time, 0);
        c.put16(match + period_end, 0x483);
        const auto announce = [&] {
            c.set({{3, match}, {4, 0x3de}});
            call(c, 0x101f2400, release_last ? 0x101d99ec : 0x101d9b6c);
        };
        const auto release = [&] {
            const auto pending = c.u32(match + pending_player);
            if (compare_logical(c, pending) != 0) {
                c.set(3, pending);
                call(c, 0x1022e710, release_last ? 0x101d9a08 : 0x101d9b60);
            }
        };
        if (release_last) { announce(); release(); } else { release(); announce(); }
        c.put8(match + play_state, 8);
        c.put8(match + restart_wait, 0);
        flip_kickoff(c, match);
        return result(c, 2);
    };

    c.set(5, u(flags()));  // lha r5,0x38
    if (!has(rule_two_legs)) {
        const auto sum = [&] { return first_leg(0) + full(0); };
        const auto level = [&] { return compare(c, sum(), first_leg(1) + full(1)) == 0; };
        if (compare(c, first_leg(0), -1) != 0 && !has(rule_4) && compare(c, full(1), first_leg(0)) != 0) {
            c.set({{5, u(first_leg(1))}, {6, u(sum())}});
            if (level()) {
                check_both_sides(c, match, 0x101d97e0, 0x101d97ec);
                return break_after_checks(c, match);
            }
        }
        c.set(6, u(flags()));
        if (has(rule_1) && (c.set({{5, u(sum())}}), level())) {
            c.put8(match + shootout_stage, 4);
            return break_after_checks(c, match);
        }
        if (has(rule_20)) {
            c.set({{5, u(first_leg(1))}, {6, u(sum())}});
            if (level()) {
                c.put8(match + shootout_stage, 1);
                return break_after_checks(c, match);
            }
        } else {
            c.set(5, 0);
        }
        check_both_sides(c, match, 0x101d98bc, 0x101d98c8);
        return break_after_checks(c, match);
    }
    if (compare(c, first_leg(0), -1) != 0) {
        const auto sum = full(0) + first_leg(0);
        c.set(5, u(sum));
        if (compare(c, sum, full(1) + first_leg(1)) == 0) {
            // Level on aggregate: extra time unless the rules decide.
            if ((!has(rule_4) && compare(c, full(1), first_leg(0)) == 0) || (has(rule_4) || has(rule_4000)))
                return extra_time(true);
            return closed(0x101d9a64, 0x101d9a70, 0x101d9a7c, 0x101d9a88);
        }
        return closed(0x101d9ab4, 0x101d9ac0, 0x101d9acc, 0x101d9ad8);
    }
    // (the first leg is -1 here, as the compare above left cr0)
    if (compare(c, full(0), full(1)) == 0) return extra_time(false);
    return closed(0x101d9bc8, 0x101d9bd4, 0x101d9be0, 0x101d9bec);
}

// Phase 0x483: the end of the first half of extra time.
std::uint32_t extra_half(Cpu& c, std::uint32_t match) {
    if (compare(c, c.i8(match + extra_time_flag)) != 0) {
        const auto fixture = fixture_of(c, match);
        c.put8(fixture + fx_extra_time, c.u8(match + period_goals(0)));
        c.put8(fixture + fx_extra_time + 1, c.u8(match + period_goals(1)));
        c.put8(match + play_state, 8);
        c.put8(match + restart_wait, 0);
        c.put16(match + period_end, 1);
        check_both_sides(c, match, 0x101d9c50, 0x101d9c5c);
        c.set({{3, match}, {4, 1}});
        call(c, 0x101d9510, 0x101d9c68);
        c.set({{3, match}, {4, 1}});
        call(c, 0x101f2400, 0x101d9c74);
        return result(c, 4);
    }
    const auto clock = c.i16(match + clock_at);
    c.set(4, u(clock));
    if (compare(c, clock, s8(c.u8(match + added_time)) + 0x483) <= 0) return result(c, 0);
    post_message(c, match, 0x20f3, 0, 0x101d9cac);
    c.put16(match + clock_at, 0x483);
    c.put8(match + added_time, 0);
    c.put16(match + period_end, 0x528);
    c.put8(match + play_state, 8);
    c.put8(match + restart_wait, 0);
    const auto pending = c.u32(match + pending_player);
    if (compare_logical(c, pending) != 0) { c.set(3, pending); call(c, 0x1022e710, 0x101d9cf0); }
    flip_kickoff(c, match);
    return result(c, 3);
}

// Phase 0x528: the end of extra time; a level score goes to the shoot-out when the rules say so.
std::uint32_t extra_time_ended(Cpu& c, std::uint32_t match, Kept<30>& shootout_needed) {
    if (compare(c, c.i8(match + extra_time_flag)) == 0) {
        const auto clock = c.i16(match + clock_at);
        c.set(4, u(clock));
        if (compare(c, clock, s8(c.u8(match + added_time)) + 0x528) <= 0) return result(c, 0);
    }
    const auto fixture = fixture_of(c, match);
    c.put8(fixture + fx_extra_time, c.u8(match + period_goals(0)));
    c.put8(fixture + fx_extra_time + 1, c.u8(match + period_goals(1)));
    const auto flags = [&] { return std::int32_t(c.i16(fixture + fx_flags)); };
    const auto has = [&](std::int32_t rule) { return compare(c, flags() & rule) != 0; };
    const auto score = [&](std::uint32_t at) { return std::int32_t(c.i8(fixture + at)); };
    const auto needed = [&] { return s8(shootout_needed); };
    const auto rules_apply = [&] { return has(rule_1) || has(rule_20); };
    const auto extra_sum = [&] { return score(fx_extra_time) + score(fx_first_leg); };
    // The aggregate counts and the rules call for a shoot-out.
    if (compare(c, score(fx_first_leg), -1) == 0) {
        if (rules_apply()) shootout_needed = 1;
    } else if (has(rule_4000)) {
        c.set(5, u(extra_sum()));
        if (compare(c, extra_sum(), score(fx_extra_time + 1) + score(fx_first_leg + 1)) == 0) {
            c.set(5, u(score(fx_extra_time + 1)));
            if (compare(c, score(fx_extra_time + 1), score(fx_first_leg)) == 0 && rules_apply()) shootout_needed = 1;
        }
    } else {
        c.set(5, u(extra_sum()));
        if (compare(c, extra_sum(), score(fx_extra_time + 1) + score(fx_first_leg + 1)) == 0 && rules_apply())
            shootout_needed = 1;
    }
    c.set(6, fixture);
    // Extra time scored as much by both sides as regular time did.
    const auto extra_level = [&] {
        return compare(c, score(fx_extra_time) - score(fx_full_time), score(fx_extra_time + 1) - score(fx_full_time + 1)) == 0;
    };
    bool decided = false;
    if (has(rule_1)) {
        c.set({{5, u(score(fx_full_time + 1))}, {6, u(score(fx_extra_time) - score(fx_full_time))}});
        if (extra_level()) {
            c.set(5, u(needed()));
            if (compare(c, needed()) != 0) {
                c.put8(match + shootout_stage, 4);
                decided = true;
            }
        }
    }
    if (!decided && has(rule_20)) {
        c.set({{5, u(score(fx_extra_time) - score(fx_full_time))}, {6, u(score(fx_extra_time + 1) - score(fx_full_time + 1))}});
        if (extra_level() && compare(c, needed()) != 0) {
            c.put8(match + shootout_stage, 1);
            decided = true;
        }
    }
    if (!decided) check_both_sides(c, match, 0x101d9fa0, 0x101d9fac);
    c.put8(match + play_state, 8);
    c.set(5, 8);
    c.put8(match + restart_wait, 0);
    c.put16(match + period_end, 1);
    c.set({{3, match}, {4, 1}});
    call(c, 0x101d9510, 0x101d9fd0);
    c.set({{3, match}, {4, 1}});
    call(c, 0x101f2400, 0x101d9fdc);
    return result(c, 4);
}
}  // namespace

std::uint32_t period_ended(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101d95a0, 0x40, {.first_gpr = 30});
    Kept<31> r_match(c, match);
    Kept<30> shootout_needed(c, 0);
    if (compare(c, s8(c.u8(match + play_state)), 5) == 0) return result(c, 0);
    const auto mark = c.i16(match + period_end);
    // The original's chain of compares: 0x3de first, then 0x1ef, 2, 0 below it, 0x528, 0x483 above it.
    if (const auto o = compare(c, mark, 0x3de); o == 0) {
        return full_time(c, match);
    } else if (o < 0) {
        const auto p = compare(c, mark, 0x1ef);
        if (p == 0) return half_time(c, match);
        if (p < 0 && compare(c, mark, 2) < 0 && compare(c, mark, 0) >= 0) return result(c, 4);
    } else {
        const auto p = compare(c, mark, 0x528);
        if (p == 0) return extra_time_ended(c, match, shootout_needed);
        if (p < 0 && compare(c, mark, 0x483) == 0) return extra_half(c, match);
    }
    // An unknown mark: reported through sprintf.
    c.set({{3, c.toc(-0x7bb8)}, {4, c.toc(-0x52cc) + 0x90}, {5, u(c.i16(match + period_end))}});
    c.call(0x1077b720, 0x101da000);
    return result(c, 0);
}

void check_lineup_balance(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101da020, 0x90, {.first_gpr = 26, .gpr_offset = -0x38, .first_fpr = 29});
    Fpu fp(c);
    Kept<31> r_match(c, match);
    Kept<30> r_side(c, side);
    const auto pool = c.toc(-0x52c4);
    Kept<29> r_pool(c, pool);
    Kept<27> counted(c, 0);
    double rating_sum = c.f32(c.toc(-0x52c8) + 0xc);
    c.set_f(30, rating_sum);
    const double magic = c.f64(pool + 0x60);  // 2^52 + 2^31: the int-to-double bias
    // An integer through the stack as the original converts it: the high word 0x43300000, the low word biased.
    const auto widen = [&](std::int32_t at, std::int32_t value) {
        c.put32(frame.at(at + 4), u(value) ^ 0x80000000);
        c.put32(frame.at(at), 0x43300000);
        return c.f64(frame.at(at));
    };
    const auto team = [&](std::uint32_t at) { return c.u32(fixture_of(c, match) + at); };
    const auto strength = [&](std::uint32_t at, std::int32_t temp) {  // lha 0x1ec of a team, widened and biased
        c.set(3, team(at));
        c.set_f(1, magic);
        return fp.fsubs(widen(temp, c.i16(team(at) + 0x1ec)), magic);
    };

    c.set({{3, match}, {4, side}});
    call(c, 0x101fce60, 0x101da060);
    if (compare(c, s8(c.r(3))) != 0) { post_message(c, match, 0x2171, side, 0x101da084); return; }
    c.set({{3, match}, {4, side}});
    call(c, 0x101fcf20, 0x101da094);
    if (compare(c, s8(c.r(3))) != 0) { post_message(c, match, 0x2170, side, 0x101da0b8); return; }
    c.set(3, match);
    if (compare(c, s8(byte_7931(c, match))) != 0) {
        c.set({{3, match}, {4, side}, {5, frame.at(0x3c)}, {6, frame.at(0x3b)}, {7, frame.at(0x3a)},
               {8, frame.at(0x39)}, {9, frame.at(0x38)}, {10, 0}});
        call(c, 0x101f21d0, 0x101da0f4);
        const auto a = s8(c.u8(frame.at(0x3c))), b = s8(c.u8(frame.at(0x3b))), d = s8(c.u8(frame.at(0x3a)));
        bool unbalanced = compare(c, a, b) > 0;
        if (!unbalanced) unbalanced = compare(c, a, b) == 0 && compare(c, d) != 0;
        if (unbalanced) {
            post_message(c, match, 0x2120, side, 0x101da14c);
        } else {
            // The average rating of the players on the pitch (substitutes only when present).
            std::uint32_t player = 0;
            for (std::int32_t i = 0; compare(c, i, 0x14) < 0; ++i) {
                const auto offset = u(s8(side) * 0x2350) + u(i * 0x1c4);
                player = match + offset + players_at;
                c.set(3, offset);
                if (compare(c, i, 10) > 0 && compare(c, s8(c.u8(player + 0x1f)), -1) == 0) continue;
                c.set_f(1, magic);
                rating_sum = fp.fadds(rating_sum, fp.fsubs(widen(0x48, s8(c.u8(player + 0x1b))), magic));
                c.set_f(30, rating_sum);
                counted = counted + 1;
            }
            keep(c, {{26, player}, {28, 20}});
            if (compare(c, s8(counted)) != 0) {
                c.set_f(1, magic);
                c.set_f(31, fp.fdivs(rating_sum, fp.fsubs(widen(0x48, s8(counted)), magic)));
            }
            const double average = c.f(31);
            if (compare(c, s8(side)) == 0) {
                // fmsub then frsp: the allowed spread.
                const double spread = fp.frsp(fp.multiply_add(0x101da210, average, c.f64(pool + 0x90), c.f64(pool + 0x98)));
                c.set_f(29, spread);
                c.set_f(1, c.f64(pool + 0x90));
                fp.fcmp(0, average, c.f64(pool + 0xf8));
                if (c.cr(0) & 8) {
                    const double home = strength(0x1c, 0x48);
                    const double away = fp.fmuls(strength(0x20, 0x40), spread);
                    fp.fcmp(0, home, away);
                    if (c.cr(0) & 4) post_message(c, match, 0x2121, side, 0x101da298);
                }
            } else {
                const double spread = fp.frsp(fp.multiply_add(0x101da2a4, average, c.f64(pool + 0x90), c.f64(pool + 0x98)));
                c.set_f(29, spread);
                c.set_f(1, c.f64(pool + 0x90));
                fp.fcmp(0, average, c.f64(pool + 0x100));
                if (c.cr(0) & 8) {
                    const double factor = c.f64(pool + 0x108);
                    c.set(3, team(0x20));
                    c.set_f(1, magic);
                    const double home = fp.fmul(factor, fp.fsub(widen(0x40, c.i16(team(0x20) + 0x1ec)), magic));
                    const double away = fp.fmuls(strength(0x1c, 0x48), spread);
                    fp.fcmp(0, home, away);
                    if (c.cr(0) & 4) post_message(c, match, 0x2121, side, 0x101da334);
                }
            }
        }
    }
    if (compare(c, s8(side), 1) == 0 && compare(c, s8(c.u8(match + extra_time_flag))) == 0) {
        c.set({{3, match}, {4, 0}});
        call(c, 0x101fce60, 0x101da35c);
        if (compare(c, s8(c.r(3))) != 0) return;
        c.set({{3, match}, {4, 0}});
        call(c, 0x101fcf20, 0x101da374);
        if (compare(c, s8(c.r(3))) != 0) return;
        post_message(c, match, 0x2001, 0, 0x101da398);
    }
}

void choose_shootout_order(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101da3c0, 0x60, {.first_gpr = 24});
    Kept<29> r_match(c, match);
    const auto pool = c.toc(-0x7a6c);
    Kept<31> r_pool(c, pool);
    c.set({{3, match + has_kicked}, {4, 0}, {5, 0x28}});
    c.call(0x10778300, 0x101da3e8);  // clears both sides' has-kicked flags
    for (std::int32_t side = 0; side < 2; ++side)
        for (std::int32_t slot = 0; slot < 11; ++slot) c.put8(match + kick_order + u(side * 11 + slot), 0xff);

    Kept<30> r_side(c, 0);
    Kept<28> r_slot(c, 0);
    for (; compare(c, s8(r_side), 2) < 0; r_side = r_side + 1) {
        for (r_slot = 0; compare(c, s8(r_slot), 11) < 0; r_slot = r_slot + 1) {
            const auto side = s8(r_side), slot = s8(r_slot);
            keep(c, {{27, UINT32_MAX}});
            enter(c, 0x101da454, {match, r_side});
            const auto available = side_can_supply_kicker(c, match, r_side);
            std::uint32_t kicker;
            if (compare(c, s8(available)) != 0) {
                c.set({{3, match}, {4, r_side}});
                const auto flag = fixture_flag_40(c, match, r_side);
                keep(c, {{24, flag}});
                c.set({{3, match}, {4, r_side}});
                const auto team = fixture_team(c, match, r_side);
                c.set({{4, team}, {3, c.u32(pool)}, {5, r_slot}, {6, flag}});
                call(c, 0x103b1670, 0x101da490);
                kicker = c.r(3);
                keep(c, {{27, kicker}});
            } else {
                c.set({{3, match}, {4, r_side}, {5, 5}, {6, match + has_kicked}});
                call(c, 0x101f3a30, 0x101da4e0);
                const auto found = c.r(3);
                keep(c, {{25, found}});
                if (compare_logical(c, found) == 0) {
                    // Nobody left: all flags of the side are cleared and the slot is tried again.
                    for (std::int32_t k = 0; k < 20; ++k) c.put8(match + has_kicked + u(side * 0x14 + k), 0);
                    c.set(4, 0);
                    keep(c, {{26, 20}});
                    r_slot = r_slot - 1;
                    continue;
                }
                kicker = c.u8(found);
                keep(c, {{27, kicker}});
            }
            c.put8(match + kick_order + u(side * 11 + slot), kicker);
            c.set(4, 1);
            c.put8(match + has_kicked + u(side * 0x14 + s8(kicker)), 1);
            c.set(3, u(side * 0x14 + s8(kicker)));
        }
    }
}

std::uint32_t side_can_supply_kicker(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101da5a0, 0x50, {.first_gpr = 28});
    Kept<31> r_match(c, match);
    Kept<30> r_side(c, side);
    const auto pool = c.toc(-0x7a54);
    Kept<29> r_pool(c, pool);
    const auto stats = [&] { return match + u(s8(side) * std::int32_t(side_stats_size)) + side_stats_at; };
    const auto stats_word = [&] {  // 0x101da750
        c.set(3, stats());
        return word_20(c, stats());
    };
    const auto test = [&](std::uint32_t entry, std::uint32_t back, std::uint32_t word) {
        c.set({{4, word}, {3, c.u32(pool)}});
        if (entry == 0x101da6f0) valid_competition_record(c, word);
        else call(c, entry, back);
        return s8(c.r(3));
    };
    if (compare(c, test(0x101da6f0, 0x101da5e8, stats_word())) == 0) return result(c, 0);
    if (compare(c, test(0x10147e70, 0x101da614, stats_word())) != 0) return result(c, 0);
    c.set(3, stats());
    if (compare(c, s8(nonzero_byte_1b(c, stats()))) != 0) {
        const auto fixture = fixture_of(c, match);
        const auto team = c.u32(fixture + (compare(c, s8(side)) == 0 ? 0x1c : 0x20));
        keep(c, {{28, team}});
        c.set(3, team);
        call(c, 0x100c7d00, 0x101da668);
        if (compare(c, s8(c.r(3))) == 0) {
            if (compare(c, test(0x101481d0, 0x101da694, stats_word())) == 0) return result(c, 0);
        }
    }
    return result(c, 1);
}

std::uint32_t player_index(Cpu& c, std::uint32_t match, std::uint32_t target, std::uint32_t side) {
    Frame frame(c, 0x101da760, 0x50, {.first_gpr = 28});
    c.put32(frame.at(0x68), match);
    Kept<28> r_target(c, target);
    c.put8(frame.at(0x73), side);
    c.set(4, match);
    Kept<31> r_player(c, match + u(s8(side) * std::int32_t(side_size)) + players_at);
    if (compare_logical(c, target) == 0) return result(c, UINT32_MAX);
    Kept<30> r_index(c, 0);
    for (; compare(c, s8(r_index), 0x14) < 0; r_index = r_index + 1) {
        const auto player = std::uint32_t(r_player);
        if (compare_logical(c, c.u32(player + 0x6c), target) == 0) {
            c.set(3, player);
            if (compare(c, s8(position(c, player))) >= 0) return result(c, r_index);
        }
        keep(c, {{29, player}});
        r_player = player + player_size;
    }
    return result(c, UINT32_MAX);
}

namespace {
// Stages 1 and 4: the shoot-out begins. Stage 1 starts with side 1; stage 4 draws the side
// (the return addresses differ between the two copies in the original).
void begin_shootout(Cpu& c, std::uint32_t match, bool drawn) {
    c.set(3, match);
    clear_pending_restart(c, match);
    c.set(3, c.u32(match + message_target));
    c.put8(match + saved_target_byte, byte_0(c, c.r(3)));
    c.set({{3, c.u32(match + message_target)}, {4, 0}});
    set_byte_0(c, c.r(3), 0);
    if (drawn) {
        draw(c, 2, 0x101da964);
        c.set(3, u(s8(c.r(3))));
        c.put8(match + kicking_side, c.r(3));
    } else {
        c.put8(match + kicking_side, 1);
    }
    c.put8(match + 0x1de, 0);
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
    c.put8(match + kicks_left + 1, 5);
    c.put8(match + kicks_left, 5);
    c.put8(match + kicks_taken + 1, 0);
    c.put8(match + kicks_taken, 0);
    if (drawn) c.set(4, match + 0x10000);  // addis r4,r31,1 at 0x101da99c
    enter(c, drawn ? 0x101da9f8 : 0x101da928, {match});
    choose_shootout_order(c, match);
    if (!drawn) c.set(3, 0);
    c.put8(match + announced, 0);
    c.put8(match + shootout_stage, drawn ? 5 : 2);
}

// Stages 3 and 6: the shoot-out is over; its score goes to the fixture.
void finish_shootout(Cpu& c, std::uint32_t match) {
    const auto goals0 = c.u8(match + shootout_goals(0)), goals1 = c.u8(match + shootout_goals(1));
    if (compare_logical(c, goals0, goals1) > 0) post_message(c, match, 0x2004, 0, 0x101db2a8);
    else post_message(c, match, 0x2004, 1, 0x101db2c4);
    const auto fixture = fixture_of(c, match);
    c.put8(fixture + fx_shootout, c.u8(match + shootout_goals(0)));
    c.put8(fixture + fx_shootout + 1, c.u8(match + shootout_goals(1)));
    c.put8(match + shootout_stage, 0);
    c.set({{3, c.u32(match + message_target)}, {4, c.u8(match + saved_target_byte)}});
    set_byte_0(c, c.r(3), c.r(4));
}

// Stages 2 and 5: the next kick. The kicker's and the keeper's states are set from the scores
// (a clutch kick is worth more), the kick is played out and the turn passes to the other side.
void kick(Cpu& c, Frame& frame, std::uint32_t match, std::uint32_t mode) {
    c.put8(match + play_state, 0);
    if (compare(c, s8(c.u8(match + kicks_left))) == 0 && compare(c, s8(c.u8(match + kicks_left + 1))) == 0) {
        c.put8(match + kicks_left + 1, 1);
        c.put8(match + kicks_left, 1);
        c.put8(match + announced, 1);
    }
    const auto side = s8(c.u8(match + kicking_side));
    const auto other = std::uint32_t(side == 0);
    const auto side_reloaded = [&] { return s8(c.u8(match + kicking_side)); };
    const auto taken = s8(c.u8(match + kicks_taken + u(side)));
    // The original divides by 11 with a multiply-high sequence, exact for any byte.
    const auto slot = side * 11 + taken % 11;
    Kept<22> r_slot(c, c.u8(match + kick_order + u(slot)));
    Kept<30> r_taker(c, match + u(side * std::int32_t(side_size)) + u(s8(r_slot) * std::int32_t(player_size)) + players_at);
    const std::uint32_t taker = r_taker;
    c.set({{3, match}, {4, other}, {5, u(side)}, {6, u(taken)}, {7, u(side * 11)}});
    call(c, 0x101f1e10, 0x101daad4);
    Kept<27> r_keeper(c, c.r(3));
    const std::uint32_t keeper = r_keeper;
    c.put8(match + kicks_taken + u(side_reloaded()), c.u8(match + kicks_taken + u(side_reloaded())) + 1);
    c.set({{3, taker}, {4, 0}});
    call(c, 0x10285ee0, 0x101daafc);
    c.set({{3, keeper}, {4, 0}});
    call(c, 0x10285ee0, 0x101dab08);
    c.put8(match + has_kicked + u(side_reloaded() * 0x14 + s8(c.u8(taker))), 1);
    c.set({{3, taker}, {4, 1}});
    call(c, 0x1022e3a0, 0x101dab34);
    c.put8(match + shot_state, 0);
    // A random draw with the taker's skill; the base counts how many kicks remain for the side.
    draw(c, u(s8(c.u8(c.u32(taker + 0x70) + 0x35))), 0x101dab4c);
    Kept<26> r_score(c, 0x190 - ((u(s8(c.u8(match + kicks_left + u(side_reloaded())))) << 1) + 8) * c.r(3));

    Fpu fp(c);
    const auto draw_scaled = [&](std::uint32_t scale_at, std::int32_t temp, std::int32_t stored, std::uint32_t back) {
        const auto pool = c.toc(-0x52c4);
        c.put32(frame.at(temp + 4), std::uint32_t(r_score) ^ 0x80000000);
        c.put32(frame.at(temp), 0x43300000);
        const double scaled = fp.fmul(c.f64(pool + scale_at), fp.fsub(c.f64(frame.at(temp)), c.f64(pool + 0x60)));
        c.set_f(1, c.f64(pool + 0x60));
        c.put64(frame.at(stored), fp.fctiwz(0, scaled));
        draw(c, c.u32(frame.at(stored + 4)), back);
        r_score = std::uint32_t(r_score) + c.r(3);
    };
    const auto announce = [&](std::uint32_t id, std::uint32_t back) {
        if (compare(c, s8(c.u8(match + announced))) != 0) return false;
        {
            c.set(3, taker);
            const auto target = byte_2f(c, taker);
            c.set({{6, target}, {3, c.u32(match + message_target)}, {4, id}, {5, match}, {7, c.u8(match + kicking_side)}, {8, 0}});
            call(c, 0x101d6710, back);
        }
        return true;
    };
    const auto goals_of = [&](std::uint32_t who) { return std::uint32_t(c.u8(match + shootout_goals(who))); };
    const auto left_of = [&](std::uint32_t who) { return s8(c.u8(match + kicks_left + who)); };
    // The original reloads the kicking side (0x7914) at each use.
    const auto side_now = [&] { return s8(c.u8(match + kicking_side)); };
    const auto other_now = [&] { return std::uint32_t(side_now() == 0); };
    if (compare(c, std::int32_t(goals_of(other_now())), std::int32_t(goals_of(u(side_now()))) + left_of(u(side_now())) - 1) > 0) {
        // The other side is already ahead of what this side can still reach.
        const auto goals = goals_of(u(side_now()));  // lbzx r4 at 0x101daba4
        if (!announce(0x2005, 0x101dac00)) c.set(4, goals);
        draw_scaled(0x110, 0x44, 0x3c, 0x101dac3c);
    } else if (compare(c, std::int32_t(goals_of(u(side_now())) + 1), std::int32_t(goals_of(other_now())) + left_of(other_now())) > 0) {
        const auto goals = goals_of(other_now());  // lbzx r4 at 0x101dac7c
        if (!announce(0x2006, 0x101dacdc)) c.set(4, goals);
        draw_scaled(0x118, 0x3c, 0x44, 0x101dad18);
    }

    if (compare(c, s8(mode), 3) == 0) {
        c.put8(frame.at(0x38), 0x33);
        c.set({{3, taker}, {4, frame.at(0x38)}, {5, frame.at(0x3a)}});
        call(c, 0x10252080, 0x101dad40);
        Kept<20> r_value(c, u(std::int16_t(c.r(3))));
        draw(c, r_score, 0x101dad4c);
        Kept<25> r_offset(c, std::uint32_t(r_value) - c.r(3));
        c.set({{3, taker}, {4, c.u8(frame.at(0x38))}, {5, u(std::int16_t(std::uint32_t(r_offset)))},
               {6, u(c.i16(frame.at(0x3a)))}, {7, 3}, {8, 0}, {9, 0}});
        call(c, 0x102576e0, 0x101dad70);
    } else {
        // Both players are readied: the kicker (and the keeper) get their approach, everyone else is held back.
        const bool first_side = compare(c, s8(c.u8(match + kicking_side))) == 0;
        Kept<28> r_mod(c, 4);
        const std::uint32_t branch = first_side ? 0x101dae40 : 0x101dafd8;
        c.set({{3, taker}, {4, first_side ? 0u : 0xb4u}});
        set_action_19e(c, taker, first_side ? 0 : 0xb4);
        const auto skilled = compare(c, s8(c.u8(taker + 0x2e)), 3) > 0;
        if (skilled) {
            const auto steady = compare(c, s8(c.u8(taker + 0x2d))) == 0;
            draw(c, 2, first_side ? (steady ? 0x101dadbc : 0x101dadd0) : (steady ? 0x101daf54 : 0x101daf68));
            // the two sides add and subtract the draw the other way round
            const auto delta = u(s8(c.r(3)));
            r_mod = (first_side == steady) ? std::uint32_t(r_mod) - delta : std::uint32_t(r_mod) + delta;
        }
        c.set({{3, taker}, {4, r_mod}, {5, first_side ? 7u : 4u}});
        call(c, 0x102374f0, first_side ? 0x101dade8 : 0x101daf80);
        c.set({{3, keeper}, {4, 4}, {5, first_side ? 0xbu : 0u}});
        call(c, 0x102374f0, first_side ? 0x101dadf8 : 0x101daf90);
        Kept<29> r_index(c, 0);
        for (; compare(c, s8(r_index), 0x14) < 0; r_index = r_index + 1) {
            for (std::uint32_t array = 0; array < 2; ++array) {
                const auto player = match + u(s8(r_index) * std::int32_t(player_size)) + (array ? 0x546cu : players_at);
                if (compare_logical(c, player, taker) == 0 || compare_logical(c, player, keeper) == 0) continue;
                c.set(3, player);
                if (compare(c, s8(position(c, player))) < 0) continue;
                c.set({{3, player}, {4, first_side ? 0u : 8u}, {5, first_side ? 0u : 0xbu}});
                call(c, 0x102374f0, branch + array * 0x84 + 0x28 + 4);
                c.set({{3, player}, {4, 0xa}});
                call(c, 0x10285ee0, branch + array * 0x84 + 0x40 + 4);
            }
        }
        // The kick itself.
        draw(c, r_score, 0x101db0b8);
        c.set({{3, taker}, {4, u(std::int16_t(0xc8 - c.r(3)))}});
        set_pressure_adjustment(c, taker, c.r(4));
        Kept<23> r_flag(c, 0);
        Kept<24> r_tries(c, 0);
        bool again;
        do {
            c.set(3, keeper);
            call(c, 0x102836a0, 0x101db0d8);
            if (compare(c, s8(c.u8(match + play_state))) == 0 && compare(c, s8(c.u8(match + shot_state)), -1) != 0) {
                c.set({{3, taker}, {4, 4}});
                call(c, 0x10278220, 0x101db104);
                r_flag = c.r(3);
            }
            c.set({{3, taker}, {4, UINT32_MAX}});
            call(c, 0x10285ed0, 0x101db114);
            c.set({{3, keeper}, {4, UINT32_MAX}});
            call(c, 0x10285ed0, 0x101db120);
            c.set(4, c.u8(match + play_state));  // lbz r4,0x7928 at 0x101db120
            again = false;
            if (compare(c, s8(c.u8(match + play_state))) == 0 && compare(c, s8(c.u8(match + shot_state)), -1) != 0) {
                const auto o = compare(c, s8(r_tries), 4);
                r_tries = r_tries + 1;
                again = o < 0 && compare(c, s8(r_flag)) == 0 && compare_logical(c, c.u32(match + pending_player), taker) == 0;
            }
        } while (again);
        if (compare_logical(c, c.u32(match + pending_player), taker) == 0 && compare(c, s8(c.u8(match + play_state))) == 0 &&
            compare(c, s8(c.u8(match + shot_state)), -1) != 0) {
            c.put8(frame.at(0x38), 0xff);
            c.set({{3, taker}, {4, frame.at(0x38)}, {5, frame.at(0x3a)}});
            call(c, 0x10252080, 0x101db1b4);
            Kept<25> r_value(c, u(std::int16_t(c.r(3))));
            c.set({{3, taker}, {4, c.u8(frame.at(0x38))}, {5, u(std::int16_t(std::uint32_t(r_value)))},
                   {6, u(c.i16(frame.at(0x3a)))}, {7, 4}, {8, 0}, {9, 0}});
            call(c, 0x102576e0, 0x101db1d8);
        }
    }
    // The turn passes: one kick fewer for this side, the other side next.
    c.put8(match + kicks_left + u(side_reloaded()), c.u8(match + kicks_left + u(side_reloaded())) - 1);
    const auto old_side = c.u8(match + kicking_side);  // lbz r7: zero-extended
    c.put8(match + kicking_side, c.i8(match + kicking_side) == 0);
    const auto first_goals = c.u8(match + shootout_goals(0)), second_goals = c.u8(match + shootout_goals(1));
    c.set({{3, match + 0x10000}, {4, second_goals}, {5, first_goals}, {6, match + 0x10000}, {7, old_side}});
    if (compare(c, std::int32_t(first_goals), std::int32_t(second_goals) + s8(c.u8(match + kicks_left + 1))) <= 0) {
        c.set(3, first_goals);
        if (compare(c, std::int32_t(second_goals), std::int32_t(first_goals) + s8(c.u8(match + kicks_left))) <= 0) return;
    }
    c.set(3, u(s8(mode)));
    c.put8(match + shootout_stage, compare(c, s8(mode), 4) == 0 ? 3 : 6);
}
}  // namespace

void shootout_step(Cpu& c, std::uint32_t match, std::uint32_t mode_arg) {
    Frame frame(c, 0x101da820, 0x80, {.first_gpr = 20});
    Kept<31> r_match(c, match);
    Kept<21> r_mode(c, mode_arg);
    const auto stage = s8(c.u8(match + shootout_stage));
    c.set(3, u(stage));
    const auto against_4 = compare(c, stage, 4);
    if (against_4 == 0) return begin_shootout(c, match, true);
    if (against_4 > 0) {
        const auto against_6 = compare(c, stage, 6);
        if (against_6 == 0) finish_shootout(c, match);
        else if (against_6 < 0) kick(c, frame, match, mode_arg);
    } else {
        const auto against_2 = compare(c, stage, 2);
        if (against_2 == 0) kick(c, frame, match, mode_arg);
        else if (against_2 > 0) finish_shootout(c, match);
        else if (compare(c, stage, 1) >= 0) begin_shootout(c, match, false);
    }
}
}  // namespace cm::aot::match_eng
