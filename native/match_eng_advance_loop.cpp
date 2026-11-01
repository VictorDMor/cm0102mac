// MATCH_ENG.CPP, match advancement: the loop that plays a match forward tick by tick.
#include "match_eng_advance.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
// Match object fields (offsets from the match).
constexpr std::uint32_t over_flag = 0;                 // set when the match was already completed
constexpr std::uint32_t kickoff_wait = 0x1e3;          // the kick-off message was sent
constexpr std::uint32_t carrier_wait = 0x1de;          // countdown while the ball owner is switched
constexpr std::uint32_t restart_stage = 0x792a;
constexpr std::uint32_t tick_at = 0x794a;
constexpr std::uint32_t actors_count = 0x792e, actors_at = 0x79f8;
constexpr std::uint32_t pending_word_28 = 0xe028, pending_word_24 = 0xe024;
constexpr std::uint32_t carrier_at = 0x3104;
constexpr std::uint32_t fixture_ptr = 0x3118;

// The club-report calls after a match ends: for each side's team (0x1c, 0x20 in the fixture),
// when its flag (0x40, 0x41) is clear: ask, check, and publish. The copies in the original
// differ only in what they leave in r4/r5.
enum class Reports { Done, Early, Late, Full };
struct ReportBacks { std::uint32_t a1, b1, c1, a2, b2, c2; };
constexpr ReportBacks report_backs(Reports r) {
    switch (r) {
        case Reports::Done: return {0x101d3fec, 0x101d400c, 0x101d4034, 0x101d4064, 0x101d4084, 0x101d40ac};
        case Reports::Early: return {0x101d415c, 0x101d417c, 0x101d41a8, 0x101d41dc, 0x101d41fc, 0x101d4224};
        case Reports::Late: return {0x101d429c, 0x101d42bc, 0x101d42e4, 0x101d4314, 0x101d4334, 0x101d435c};
        case Reports::Full: return {0x101d4a08, 0x101d4a28, 0x101d4a50, 0x101d4a80, 0x101d4aa0, 0x101d4ac8};
    }
    return {};
}
}  // namespace

std::uint32_t advance(Cpu& c, std::uint32_t match, std::uint32_t count) {
    Frame frame(c, 0x101d3f60, 0xd0, {.first_gpr = 15});
    Fpu fpu(c);
    Kept<31> r_match(c, match);
    Kept<20> remaining(c, count);
    keep(c, {{27, c.toc(-0x7a54)}, {28, c.toc(-0x7a6c)}, {19, c.toc(-0x7bac)}, {24, c.toc(-0x7ba8)},
             {25, c.toc(-0x52cc)}, {30, c.toc(-0x52c8)}, {29, c.toc(-0x52c4)}});
    Kept<15> r15(c, c.r(15));    // a scratch word the original keeps across calls
    Kept<16> tries(c, c.r(16));
    Kept<17> pick_index(c, c.r(17));
    Kept<18> pick_side(c, c.r(18));
    Kept<21> found(c, c.r(21));
    Kept<22> actor_i(c, c.r(22));
    Kept<23> actor_j(c, c.r(23));
    Kept<26> actor(c, c.r(26));
    const std::uint32_t reports_a = c.r(27), reports_b = c.r(28), pool = c.r(29), constants = c.r(30);
    const std::uint32_t buffer_file = c.r(25);

    const auto fixture = [&] { return c.u32(match + fixture_ptr); };
    const auto field = [&](std::uint32_t at) { return s8(c.u8(match + at)); };
    const auto cell = [&](std::int32_t side, std::int32_t index) {
        return match + u(side * std::int32_t(side_size)) + u(index * std::int32_t(player_size)) + players_at;
    };
    // The calls that ask about a team's report and publish it (the original's 3 calls per team).
    const auto publish_reports = [&](Reports which) {
        const auto b = report_backs(which);
        const auto team_report = [&](std::uint32_t flag_at, std::uint32_t team_at, std::uint32_t a, std::uint32_t bb, std::uint32_t cc,
                                     bool second) {
            if (compare(c, s8(c.u8(fixture() + flag_at)), 0) != 0) return;
            const auto team = c.u32(fixture() + team_at);
            c.set({{3, team}, {4, 1}, {5, 1}, {6, 0}, {7, 0}});
            c.call(0x1014c2e0, a);
            if (which == Reports::Full && second) c.set(4, u(s8(c.r(3))));
            if (compare(c, s8(c.r(3)), 0) == 0) return;
            c.set({{3, c.u32(reports_a)}, {4, c.u32(c.u32(fixture() + team_at) + 0x90)}});
            if (which == Reports::Late && second) c.set(5, c.u32(fixture() + team_at));
            c.call(0x101481d0, bb);
            if (which == Reports::Full && second) c.set(4, u(s8(c.r(3))));
            if (compare(c, s8(c.r(3)), 0) != 0) return;
            c.set({{3, c.u32(reports_b)}, {4, c.u32(fixture() + team_at)}, {5, 1}, {6, 0}, {7, 0}});
            c.call(0x103afff0, cc);
        };
        if (which == Reports::Full) c.set(4, fixture());
        team_report(0x40, 0x1c, b.a1, b.b1, b.c1, false);
        if (which == Reports::Done || which == Reports::Late) c.set(4, fixture());
        if (which == Reports::Early) c.set(5, c.u8(fixture() + 0x41));
        team_report(0x41, 0x20, b.a2, b.b2, b.c2, true);
    };
    // A message through post_event to the match's target.
    const auto event = [&](std::uint32_t id, std::uint32_t side, std::uint32_t back, std::uint32_t target) {
        enter(c, back, {target, id, match, side, 0});
        post_event(c, target, id, match, side, 0);
    };
    // The match's next random byte-sized scale: (double)value / divisor truncated, built in the stack
    // as the original does. `hi` is the stack offset of the double's high word.
    const auto scaled_draw = [&](std::int32_t value, std::uint32_t hi, std::uint32_t store, std::uint32_t back) {
        const double bias = c.f64(pool + 0x60);
        c.set_f(1, bias);
        c.put32(frame.at(hi + 4), u(value) ^ 0x80000000u);
        c.put32(frame.at(hi), 0x43300000);
        const double built = fpu.fsub(c.f64(frame.at(hi)), bias);
        c.set_f(1, built);
        const double scaled = fpu.fdiv(built, c.f64(pool + 0x78));
        c.put64(frame.at(store), fpu.fctiwz(0, scaled));
        return std::int32_t(draw(c, c.u32(frame.at(store + 4)), back));
    };
    // The ball's cells for the two sides: who is in the end area, the nearest players, and so on.
    const auto ball_cells = [&](bool again) {
        const auto row = [&] { return c.u8(match + ball_row_at); };
        const auto column = [&] { return c.u8(match + ball_column_at); };
        for (std::uint32_t side = 0; side < 2; ++side) {
            c.set({{3, match}, {4, side}, {5, row()}, {6, column()}});
            const auto inside = inside_end_area(c, side, row(), column());
            c.set(7, u(s8(inside) == 0));
            c.set({{3, match}, {4, side}, {5, row()}, {6, column()}});
            call(c, 0x101e32e0, again ? (side ? 0x101d55c0 : 0x101d557c) : (side ? 0x101d4cd0 : 0x101d4c90));
            c.set(4, match + 0x10000);
            c.put32(match + (side ? 0xe008 : 0xe004), c.r(3));
        }
        for (std::uint32_t side = 0; side < 2; ++side) {
            c.set({{3, match}, {4, side}});
            call(c, 0x101f1e10, again ? (side ? 0x101d55f8 : 0x101d55d4) : (side ? 0x101d4d08 : 0x101d4ce4));
            c.set({{4, c.r(3)}, {3, match}, {5, row()}, {6, column()}});
            call(c, 0x101e31a0, again ? (side ? 0x101d560c : 0x101d55e8) : (side ? 0x101d4d1c : 0x101d4cf8));
            c.put8(match + 0x7922 + side, c.r(3));
        }
        c.put8(frame.at(0x3d), row());
        c.put8(frame.at(0x3c), column());
        if (again) c.set(4, column());
    };

    // The match was completed before: the closing calls, then 2.
    const auto flag = c.u8(match + over_flag);
    c.set({{4, flag}, {5, u(s8(flag))}});
    if (compare(c, s8(flag), 0) != 0) {
        c.set(3, match);
        call(c, 0x101db340, 0x101d3fac);
        c.set(3, match);
        call(c, 0x101f7100, 0x101d3fb4);
        c.set(3, match);
        call(c, 0x101f84e0, 0x101d3fbc);
        publish_reports(Reports::Done);
        c.set(3, match);
        call(c, 0x101fd900, 0x101d40b4);
        return result(c, 2);
    }
    // One pass of the loop: -1 to go on with the next count, otherwise the value to return.
    const auto pass = [&]() -> std::int32_t {
        const auto restart = field(restart_stage);
        c.set(4, u(restart));
        if (compare(c, restart, 1) == 0) {
            c.set({{3, match}, {4, 0}});
            call(c, 0x101f75a0, 0x101d40d8);
            remaining = remaining - 1;
            return -1;
        }
        const auto stage_byte = c.u8(match + shootout_stage);
        c.set({{4, stage_byte}, {3, u(s8(stage_byte))}});
        const auto stage = s8(stage_byte);
        enum class Phase { Early, Late, Play } phase;
        if (compare(c, stage, 4) < 0) phase = compare(c, stage, 1) >= 0 ? Phase::Early : Phase::Play;
        else phase = compare(c, stage, 7) >= 0 ? Phase::Play : Phase::Late;

        // Shoot-out stages 1-3 and 4-6: the step is played, and when it finishes the match is over.
        if (phase == Phase::Early) {
            c.set({{3, match}, {4, 4}});
            call(c, 0x101da820, 0x101d4114);
            const auto now = field(shootout_stage);
            c.set(4, u(now));
            if (compare(c, now, 0) != 0) {
                remaining = remaining - 1;
                return -1;
            }
            c.set(3, match);
            call(c, 0x101f84e0, 0x101d412c);
            publish_reports(Reports::Early);
            c.set({{3, match}, {4, u(c.i16(match + period_end))}});
            call(c, 0x101f2400, 0x101d4230);
            c.set(3, match);
            call(c, 0x101fd900, 0x101d4238);
            return 2;
        }
        if (phase == Phase::Late) {
            c.set({{3, match}, {4, 3}});
            call(c, 0x101da820, 0x101d4254);
            if (compare(c, field(shootout_stage), 0) != 0) {
                remaining = remaining - 1;
                return -1;
            }
            c.set(3, match);
            call(c, 0x101f84e0, 0x101d426c);
            publish_reports(Reports::Late);
            c.set(3, match);
            call(c, 0x101fd900, 0x101d4364);
            return 2;
        }

        // The kick-off of a stopped match is announced once.
        const auto wait = field(kickoff_wait);
        c.set(7, u(wait));
        if (compare(c, wait, 0) == 0) {
            c.set(4, u(field(play_state)));
            if (compare(c, field(play_state), 8) == 0 && compare(c, field(extra_time_flag), 0) == 0) {
                const auto clock = c.i16(match + clock_at);
                c.set(3, match);
                const auto to = word_79f4(c, match);
                if (compare(c, clock, 1) <= 0) event(0x212f, c.u8(match + possession_side), 0x101d43cc, to);
                else event(0x1f74, c.u8(match + possession_side), 0x101d43f4, to);
                c.set(3, 1);
                c.put8(match + kickoff_wait, 1);
                return -1;
            }
        }

        // While play is stopped, a player is picked to take the restart.
        const auto stopped = field(play_state);
        c.set(4, u(stopped));
        bool picking = compare(c, stopped, 0) != 0;
        if (picking) {
            const auto stage_now = field(restart_stage);
            c.set(4, u(stage_now));
            picking = compare(c, stage_now, 0) == 0;
        }
        if (picking) picking = compare(c, field(extra_time_flag), 0) == 0;
        if (picking) {
            tries = 0;
            for (;;) {
                pick_side = u(s8(draw(c, 2, 0x101d443c)));
                pick_index = u(s8(draw(c, 0x14, 0x101d4448)));
                tries = tries + 1;
                c.set(7, u(s8(pick_index)));
                const auto candidate = cell(s8(pick_side), s8(pick_index));
                c.set(3, candidate);
                if (compare(c, s8(position(c, candidate)), 0) >= 0) break;
                if (compare(c, s8(tries), 0x14) >= 0) break;
                c.set({{7, u(s8(pick_side) * std::int32_t(side_size))}, {3, candidate}, {4, 1}, {5, 0}});
                c.call(0x10286650, 0x101d44b0);
                c.set(4, u(s8(c.r(3))));
                if (compare(c, s8(c.r(4)), 0) != 0) break;
            }
            c.set(3, u(s8(tries)));
            if (compare(c, s8(tries), 0x14) < 0) {
                const auto candidate = cell(s8(pick_side), s8(pick_index));
                c.set(3, candidate);
                const auto kind = position(c, candidate);
                c.set(4, kind);
                if (compare(c, s8(kind), 0) >= 0) {
                    c.set({{3, match}, {5, u(s8(pick_side) * std::int32_t(side_size))}, {4, candidate}});
                    call(c, 0x101f75a0, 0x101d451c);
                    if (compare(c, field(restart_stage), 0) != 0) return -1;
                }
            }
        }
        if (compare(c, field(restart_stage), 2) == 0) c.put8(match + restart_stage, 0);

        // A pending restart (foul, throw-in...) is looked at.
        c.set(3, match);
        if (compare(c, s8(pending_restart_side(c, match)), -1) > 0) {
            if (compare(c, field(play_state), 0) != 0) {
                enter(c, 0x101d4570, {match});
                if (compare(c, s8(restart_still_valid(c, match)), 0) != 0 && compare(c, draw(c, 5, 0x101d4584), 0) == 0) {
                    enter(c, 0x101d4594, {match});
                    if (compare(c, s8(resolve_restart(c, match)), 0) != 0) return -1;
                }
            } else if (compare(c, draw(c, 7, 0x101d45ac), 0) == 0) {
                c.set(3, match);
                r15 = pending_restart_side(c, match);
                c.set(3, match);
                const auto to = word_79f4(c, match);
                event(0x20ef, r15, 0x101d45e8, to);
                enter(c, 0x101d45f0, {match});
                if (compare(c, s8(resolve_restart(c, match)), 0) != 0) return -1;
            }
        }
        if (compare(c, field(extra_time_flag), 0) == 0 && compare(c, field(play_state), 0) != 0) {
            c.set(3, match);
            call(c, 0x101e6c50, 0x101d4628);
        }
        // Nobody holds the stopped ball: the restart taker is chosen and put on the ball.
        if (compare(c, field(extra_time_flag), 0) == 0 && compare(c, field(play_state), 0) != 0) {
            c.set(3, match + 0x10000);
            bool choose = compare_logical(c, c.u32(match + pending_player)) == 0;
            if (!choose) {
                c.set(3, match + 0x10000);
                const auto holder = c.u32(match + pending_player);
                c.set(3, holder);
                choose = compare(c, s8(position(c, holder)), 0) < 0;
            }
            if (choose) {
                c.set({{3, match}, {4, c.u8(match + possession_side)}, {5, c.u8(match + play_state)}, {6, 0}});
                call(c, 0x101f3a30, 0x101d4684);
                c.set(4, match + 0x10000);
                c.put32(match + pending_player, c.r(3));
                c.set(3, match + 0x10000);
                const auto holder = c.u32(match + pending_player);
                c.set({{3, holder}, {4, c.u8(match + ball_row_at)}, {5, c.u8(match + ball_column_at)}});
                c.call(0x102374f0, 0x101d46a0);
                c.set({{3, c.u32(match + pending_player)}, {4, 0xc8}});
                set_pressure_adjustment(c, c.r(3), 0xc8);
                c.set({{5, match + 0x10000}, {3, c.u32(match + pending_player)}});
                c.call(0x1022e3a0, 0x101d46bc);
                c.put32(frame.at(0x40), compare(c, field(possession_side), 0) == 0 ? 10 : 1);
                c.set(3, c.u32(match + pending_player));
                c.set({{4, c.u8(match + ball_row_at)}, {5, c.u8(match + ball_column_at)}, {6, 4},
                       {7, u(s8(c.u32(frame.at(0x40))))}});
                c.call(0x10237830, 0x101d4700);
            }
        }

        // The tick counter: ticks pass quietly while it is negative.
        const auto counter = c.i16(match + tick_at);
        r15 = u(counter);
        c.set(5, u(counter) + 1);
        c.put16(match + tick_at, u(counter) + 1);
        const auto ticks = c.i16(match + tick_at);
        c.set(3, u(ticks));
        if (compare(c, ticks, 1) >= 0 || compare(c, field(extra_time_flag), 0) != 0) {
            remaining = remaining - 1;
            c.set(3, match + 0x10000);
            const auto deadline = s8(c.u8(match + 0xe0b8));
            bool reached = false;
            if (compare(c, deadline, -1) != 0) {
                c.set(3, match + 0x10000);
                const auto stored = s8(c.u8(match + 0xe0b8));
                const auto limit = std::int16_t(stored * 11);
                c.set(3, u(std::int32_t(limit)));
                reached = compare(c, limit, c.i16(match + clock_at)) < 0;
            }
            if (reached) {
                // The deadline for the stoppage: the match is told which kind of end it is.
                c.set(3, match + 0x10000);
                c.put8(match + 0xe03d, 0xfd);
                c.set({{4, 0xfd}, {3, match + 0x10000}});
                c.put8(match + 0xe073, 0xfd);
                c.set({{4, 0xfffffffd}, {3, fixture()}});
                c.put8(fixture() + 0x43, 0xfd);
                c.put8(match + play_state, 0);
                c.set(3, fixture());
                r15 = c.u8(fixture() + 0x4f);
                // The original shifts the fixture's rule byte before masking bits it cannot hold: only the last message is reachable.
                const auto rule = std::rotl(std::uint32_t(r15), 30) & 3;
                c.set(3, rule);
                const auto target = c.u32(match + message_target);
                if (compare(c, std::int32_t(rule & 4), 0) != 0) {
                    event(0x217a, 0, 0x101d47bc, target);
                } else {
                    c.set(3, fixture());
                    const auto rule2 = std::rotl(std::uint32_t(c.u8(fixture() + 0x4f)), 30) & 3;
                    c.set(3, rule2);
                    if (compare(c, std::int32_t(rule2 & 0x10), 0) != 0) event(0x2179, 0, 0x101d47f0, target);
                    else event(0x217b, 0, 0x101d480c, target);
                }
                return 8;
            }
            c.set(3, match);
            if (compare(c, s8(byte_7931(c, match)), 0) != 0) {
                c.set({{3, match}, {4, 0x3de}});
                call(c, 0x101f2400, 0x101d4834);
            }
            c.put16(match + clock_at, u(c.i16(match + clock_at)) + 1);
            c.set(3, 0);
            c.put16(match + tick_at, 0);
            c.set(3, match);
            call(c, 0x101d95a0, 0x101d4850);
            const auto ended = s8(c.r(3));
            c.set(3, u(ended));
            // The period routine says what happened: 0 nothing, 1-3 a break, 4 the match is over.
            bool unknown = false;
            if (const auto order = compare(c, ended, 2); order == 0) return 4;
            else if (order > 0) {
                const auto upper = compare(c, ended, 4);
                if (upper == 0) {
                    // the match ended: the first leg's totals, then the closing calls
                    c.set(3, match);
                    call(c, 0x101f7100, 0x101d48a0);
                    const auto first_leg = s8(c.u8(fixture() + 0x4b));
                    c.set(3, u(first_leg));
                    if (compare(c, first_leg, -1) != 0) {
                        const auto extra = s8(c.u8(fixture() + 0x47));
                        if (compare(c, extra, -1) != 0) {
                            c.set(4, fixture());
                            c.put8(fixture() + 0x4b, c.u8(fixture() + 0x4b) + c.u8(fixture() + 0x47));
                            c.set(4, fixture());
                            c.put8(fixture() + 0x4c, c.u8(fixture() + 0x4c) + c.u8(fixture() + 0x48));
                        } else {
                            c.put8(fixture() + 0x4b, c.u8(fixture() + 0x4b) + c.u8(fixture() + 0x43));
                            c.set(5, c.u8(fixture() + 0x4c));
                            c.put8(fixture() + 0x4c, c.u8(fixture() + 0x4c) + c.u8(fixture() + 0x44));
                        }
                    }
                    c.set(4, fixture());
                    if (compare(c, c.i32(fixture()), -2) != 0) {
                        c.set({{3, c.u32(reports_b)}, {4, fixture()}});
                        c.call(0x103af130, 0x101d4948);
                    }
                    const auto shoot = field(shootout_stage);
                    c.set(4, u(shoot));
                    if (compare(c, shoot, 0) != 0) {
                        // A shoot-out follows: the clock goes back to a multiple of 30 (a multiply-high division).
                        const std::int32_t clock = c.i16(match + clock_at);
                        c.put16(match + clock_at, u(clock / 30 * 30));
                        const auto stage_now = s8(c.u8(match + shootout_stage));
                        c.set(4, u(stage_now));
                        const auto target = c.u32(match + message_target);
                        if (compare(c, stage_now, 1) == 0) event(0x2003, 0, 0x101d49ac, target);
                        else event(0x2002, 0, 0x101d49c8, target);
                        return 4;
                    }
                    c.set(3, match);
                    call(c, 0x101f84e0, 0x101d49d8);
                    publish_reports(Reports::Full);
                    c.set(3, match);
                    call(c, 0x101fd900, 0x101d4ad0);
                    return 2;
                }
                if (upper > 0) unknown = true;
                else return 4;
            } else if (const auto lower = compare(c, ended, 0); lower == 0) {
                // nothing yet
            } else if (lower > 0) {
                return 4;
            } else {
                unknown = true;
            }
            if (unknown) {
                // An unexpected answer from the period routine: reported as an assertion.
                const Assertion assertion{.at = 0x101d4aec, .file = buffer_file + 0x45, .format = buffer_file + 0x53,
                                          .title = buffer_file + 0x60, .name = frame.at(0x4d), .extension = frame.at(0x48),
                                          .line = 0x735};
                report_assertion(c, assertion);
                c.set(4, 0);
                c.set(3, UINT32_MAX);
                c.call(0x1076f7d0, 0x101d4b30);
            }
            c.set(3, match);
            call(c, 0x101d9460, 0x101d4b38);
        }
        // Play goes on: the stopped ball drifts, and the time of the stoppage is drawn.
        if (compare(c, field(play_state), 0) == 0) {
            enter(c, 0x101d4b50, {match});
            drift_ball(c, match);
            const auto stopped_now = c.u8(match + play_state);
            c.set(4, u(s8(stopped_now)));
            if (compare(c, s8(stopped_now), 0) != 0) {
                const auto roll = std::int32_t(draw(c, 10, 0x101d4b68));
                c.put8(frame.at(0x3a), u(s8(roll)));
                const auto clock = c.i16(match + clock_at);
                c.set(4, u(clock));
                c.put16(match + clock_at, u(clock) + u(s8(c.u8(frame.at(0x3a)))));
                const auto extra = scaled_draw(s8(c.u8(frame.at(0x3a))), 0x74, 0x6c, 0x101d4bc0);
                c.put8(match + added_time, c.u8(match + added_time) + u(s8(extra)));
                c.set(4, c.u8(match + added_time));
                return -1;
            }
        }
        // The pending restart's holder and its marker are cleared when somebody else holds the ball.
        c.set({{3, match + 0x10000}, {4, match + 0x10000}});
        if (compare_logical(c, c.u32(match + pending_word_28), c.u32(match + pending_player)) != 0) {
            c.set(3, c.u32(match + pending_player));
            if (compare_logical(c, c.r(3), 0) != 0) {
                c.set(4, 0);
                c.put32(match + pending_word_28, 0);
                c.set({{3, 0}, {4, match + 0x10000}});
                c.put32(match + pending_word_24, 0);
            }
        }
        // The ball's cells are worked out for both sides, and the teams' tactics are applied.
        c.set(3, match);
        c.set(5, byte_7931(c, match));
        c.set(4, c.u8(match + possession_side));
        c.set(3, match);
        call(c, 0x101f4900, 0x101d4c30);
        c.set(3, match);
        call(c, 0x101d93b0, 0x101d4c38);
        c.set(3, match);
        call(c, 0x101e4130, 0x101d4c40);
        c.set(3, match);
        call(c, 0x101f0f20, 0x101d4c48);
        c.set(3, match);
        call(c, 0x101d9080, 0x101d4c50);
        c.set(3, 0);
        c.put32(match + 0x7c68, 0);
        ball_cells(false);
        c.set(3, UINT32_MAX);
        c.put8(match + 2, 0xff);
        c.put8(match + 3, 0xff);
        for (std::uint32_t side = 0; side < 2; ++side) {
            c.set({{3, match}, {4, side}});
            enter(c, side ? 0x101d4d70 : 0x101d4d4c, {match, side});
            const auto flagged = side_flag_1000(c, match, side);
            if (side == 0) c.set(7, u(s8(flagged)));
            else c.set(3, u(s8(c.r(3))));
            if (compare(c, s8(flagged), 0) != 0) {
                c.set({{3, match}, {4, side}});
                call(c, 0x101d8f20, side ? 0x101d4d8c : 0x101d4d64);
            }
        }
        // Every actor starts the tick unmarked.
        const auto slot_at = [&](std::uint32_t index) { return match + (index << 2) + actors_at; };
        for (actor_i = 0;; actor_i = actor_i + 1) {
            c.set(4, u(s8(actor_i)));
            if (compare(c, s8(actor_i), field(actors_count)) >= 0) break;
            const auto slot = u(s8(actor_i)) << 2;
            auto who = c.u32(match + slot + actors_at);
            c.set({{3, who}, {4, actor_i}});
            set_byte_18(c, who, actor_i);
            c.set(7, slot);
            who = c.u32(match + (u(s8(actor_i)) << 2) + actors_at);
            c.set(3, who);
            clear_marking_list(c, who);
            c.set(4, u(s8(actor_i)) << 2);
            who = c.u32(match + (u(s8(actor_i)) << 2) + actors_at);
            c.set({{3, who}, {4, 0}});
            set_marking_target(c, who, 0);
        }
        // Actors without a marker find one (the nearest opponent who can be marked).
        const auto link = [&](std::uint32_t marked, std::uint32_t marker) {
            c.set({{3, marked}, {4, marker}});
            set_marking_target(c, marked, marker);
            c.set({{3, marker}, {4, marked}});
            append_marker(c, marker, marked);
        };
        for (actor_i = 0;; actor_i = actor_i + 1) {
            c.set(3, u(s8(actor_i)));
            if (compare(c, s8(actor_i), field(actors_count)) >= 0) break;
            actor = c.u32(slot_at(u(s8(actor_i))));
            const std::uint32_t who = actor;
            c.set(3, who);
            const auto kind = position(c, who);
            c.set(4, kind);
            const auto side = s8(c.u8(who + 0x27));
            const auto team = match + u(side * std::int32_t(team_tactics_size)) + team_tactics;
            c.set({{5, u(side * std::int32_t(team_tactics_size))}, {3, team}});
            c.call(0x101149f0, 0x101d4e2c);
            c.set(4, u(s8(c.r(3))) & 1);
            if (compare(c, std::int32_t(c.r(4)), 0) != 0) continue;
            c.set(3, who);
            call(c, 0x10285ef0, 0x101d4e48);
            found = c.r(3);
            if (compare_logical(c, found) != 0) {
                link(who, found);
                continue;
            }
            c.set(3, who);
            c.call(0x10289560, 0x101d4e78);
            if (compare(c, std::int16_t(c.r(3)), 1) == 0) continue;
            c.set(3, match + 0x10000);
            if (compare_logical(c, who, c.u32(match + pending_player)) == 0) continue;
            const auto team_now = match + u(s8(c.u8(who + 0x27)) * std::int32_t(team_tactics_size)) + team_tactics;
            c.set(3, team_now);
            const auto flags = tactical_flags(c, team_now);
            if (compare(c, std::int32_t(flags & 0x4000), 0) != 0) {
                c.set({{3, match}, {4, who}});
                call(c, 0x101d70d0, 0x101d4ec4);
                found = c.r(3);
                if (compare_logical(c, found) != 0) link(who, found);
                continue;
            }
            const auto other_end = [&]() {  // 0 for the second side's players, 1 for the first's
                return u(s8(c.u8(who + 0x27)) == 0);
            };
            if (compare(c, s8(c.u8(match + possession_side)), s8(c.u8(who + 0x27))) == 0) {
                c.set(3, match);
                r15 = ball_column(c, match);
                c.set({{3, match}, {4, u(s8(other_end())), }, {5, r15}});
                const auto beyond = beyond_column(c, c.r(4), c.r(5));
                if (compare(c, s8(beyond), 0) == 0) continue;
            }
            const auto wants_link = [&]() -> bool {
                c.set(3, who);
                c.call(0x10289560, 0x101d4f44);
                bool near = compare(c, std::int32_t(std::int16_t(c.r(3))) & 4, 0) != 0;
                if (!near) {
                    c.set(3, who);
                    c.call(0x10289560, 0x101d4f5c);
                    near = compare(c, std::int32_t(std::int16_t(c.r(3))) & 2, 0) != 0;
                }
                if (near) {
                    c.set(3, who);
                    c.call(0x10283620, 0x101d4f74);
                    if (compare(c, s8(c.r(3)), 0) != 0) return true;
                }
                c.set(3, who);
                c.call(0x10289560, 0x101d4f88);
                bool far = compare(c, std::int32_t(std::int16_t(c.r(3))) & 8, 0) != 0;
                if (!far) {
                    c.set(3, who);
                    c.call(0x10289560, 0x101d4fa0);
                    c.set(4, c.r(3));
                    far = compare(c, std::int32_t(std::int16_t(c.r(3))) & 0x10, 0) != 0;
                }
                if (far) {
                    c.set(3, match);
                    const auto column = ball_column(c, match);
                    c.set(5, column);
                    c.set({{3, match}, {4, u(s8(other_end()))}});
                    if (compare(c, s8(beyond_column(c, c.r(4), c.r(5))), 0) != 0) return true;
                }
                c.set(3, match);
                r15 = ball_column(c, match);
                c.set(3, match);
                const auto row = ball_row(c, match);
                c.set({{5, row}, {3, match}, {4, c.u8(who + 0x27)}, {6, r15}});
                return compare(c, s8(inside_end_area(c, c.r(4), c.r(5), c.r(6))), 0) != 0;
            };
            if (wants_link()) {
                c.set({{3, match}, {4, who}});
                call(c, 0x101d70d0, 0x101d5028);
                found = c.r(3);
                if (compare_logical(c, found) != 0) link(who, found);
            }
        }

        // Each actor takes its turn.
        c.put8(frame.at(0x3b), 0);
        const auto current = [&] { return c.u32(slot_at(std::uint32_t(actor_j))); };
        for (actor_j = 0;; actor_j = actor_j + 1) {
            const auto limit = field(actors_count);
            c.set(3, u(limit));
            if (compare(c, std::int32_t(actor_j), limit) >= 0) break;
            c.set(3, current());
            const auto target = marking_target(c, current());
            if (compare_logical(c, target) != 0) {
                const auto last = field(possession_side);
                const auto who = current();
                c.set({{3, u(s8(c.u8(who + 0x27)))}, {4, c.u8(who + 0x27)}});
                if (compare(c, last, s8(c.u8(who + 0x27))) != 0) continue;
            }
            // After a stopping action, the actor may be told off for a hard challenge.
            bool act_now = true;
            if (compare(c, s8(c.u8(frame.at(0x3b))), 0) != 0) {
                const auto roll = std::int32_t(draw(c, 10, 0x101d50c8));
                r15 = u(roll);
                c.set(3, current());
                const auto rating = primary_rating(c, current());
                act_now = compare(c, s8(rating), roll) > 0;
            }
            if (act_now) {
                c.set(3, current());
                c.call(0x102775e0, 0x101d50f8);
            }
            // Shots (0x11) and headers (0x12) can stop play: the chance is taken from the player's rating.
            c.set(3, current());
            const auto act = std::int16_t(action_code(c, current()));
            if (compare(c, act, 0x11) == 0) {
                const double f2 = c.f32(constants + 0x20), f1 = c.f32(current() + 0x74), f0 = c.f32(constants + 0x1c);
                c.put64(frame.at(0x6c), fpu.fctiwz(0, fpu.multiply_add(0x101d512c, f2, f1, f0)));
                c.set_f(1, f1);
                if (compare(c, draw(c, c.u32(frame.at(0x70)), 0x101d5140), 0) == 0) c.put8(frame.at(0x3b), 1);
            } else {
                c.set(3, current());
                if (compare(c, std::int16_t(action_code(c, current())), 0x12) == 0) {
                    const double f2 = c.f32(constants + 0x20);
                    c.set(4, u(actor_j) << 2);
                    const double f1 = c.f32(current() + 0xc4), f0 = c.f32(constants + 0x1c);
                    c.put64(frame.at(0x6c), fpu.fctiwz(0, fpu.multiply_add(0x101d5188, f2, f1, f0)));
                    c.set_f(1, f1);
                    if (compare(c, draw(c, c.u32(frame.at(0x70)), 0x101d519c), 0) == 0) c.put8(frame.at(0x3b), 1);
                }
            }
            // While play is stopped (or in extra time) the clock runs on by a drawn amount.
            c.set(4, c.u8(match + play_state));
            if (compare(c, field(play_state), 0) != 0 || compare(c, field(extra_time_flag), 0) != 0) {
                const auto roll = std::int32_t(draw(c, 10, 0x101d51d4));
                c.put8(frame.at(0x39), u(s8(roll)));
                const auto clock = c.i16(match + clock_at);
                const auto byte = c.u8(frame.at(0x39));
                c.set({{5, u(clock)}, {4, byte}});
                c.put16(match + clock_at, u(clock) + u(s8(byte)));
                c.set({{4, c.u8(frame.at(0x39))}, {5, u(s8(c.u8(frame.at(0x39))))}});
                const auto extra = scaled_draw(s8(c.u8(frame.at(0x39))), 0x6c, 0x74, 0x101d522c);
                c.set(4, u(s8(extra)));
                c.put8(match + added_time, c.u8(match + added_time) + u(s8(extra)));
                if (compare(c, field(0x792b), 0) != 0) {
                    c.set(3, match);
                    call(c, 0x101f6c60, 0x101d5254);
                }
                return -1;
            }
            // The ball's carrier gets its turn.
            const auto carrier = [&] { return c.u32(match + carrier_at); };
            const auto carrier_turn = [&] {
                if (compare(c, field(carrier_wait), 0) != 0) {
                    c.put8(match + carrier_wait, c.u8(match + carrier_wait) - 1u);
                    c.set(4, c.u8(match + carrier_wait));
                    if (compare(c, s8(c.u8(match + carrier_wait)), 0) == 0 && compare(c, field(shot_state), 4) == 0 &&
                        compare_logical(c, carrier()) == 0) {
                        c.set({{3, match}, {4, c.u8(match + 0x1df)}, {5, c.u8(match + 0x1e0)}});
                        call(c, 0x101ec890, 0x101d52b0);
                        return;
                    }
                }
                c.set(4, u(field(carrier_wait)));
                if (compare(c, field(carrier_wait), 0) != 0) return;
                c.set(3, carrier());
                if (compare_logical(c, carrier()) == 0) return;
                // The carrier stands on the ball: the pass or shot goes ahead.
                const auto on_ball = [&] {
                    c.set(3, carrier());
                    const auto r = row(c, carrier());
                    c.set(4, u(s8(r)));
                    if (compare(c, field(ball_row_at), s8(r)) != 0) return false;
                    c.set(3, carrier());
                    const auto col = column(c, carrier());
                    c.set(4, col);
                    c.set(4, u(s8(col)));
                    return compare(c, field(ball_column_at), s8(col)) == 0;
                };
                const auto go = [&](std::uint32_t back) {
                    c.set({{3, carrier()}, {4, u(c.i16(match + 0x794e))}, {5, c.u8(match + 0x1e1)}, {6, c.u8(match + 0x1e2)}});
                    c.call(0x1023cd20, back);
                };
                if (on_ball()) {
                    go(0x101d5320);
                    return;
                }
                c.set(4, carrier());
                c.set(3, c.u32(match + ball_holder));
                if (compare_logical(c, carrier(), c.u32(match + ball_holder)) != 0) return;
                c.set(3, carrier());
                if (compare(c, s8(c.u8(carrier() + 0x2c)), 0) != 0) return;
                c.set(3, carrier());
                const auto rating = secondary_rating(c, carrier());
                c.set(4, u(s8(rating)));
                if (compare(c, s8(rating), 0) == 0) return;
                c.set(3, carrier());
                const auto trait = trait_10a(c, carrier());
                c.set(4, u(s8(trait)));
                if (compare(c, s8(trait), 0) == 0) return;
                c.set(3, carrier());
                const auto saved = carrier();
                c.put32(frame.at(0x44), saved);
                c.set(4, 1);
                c.put8(match + carrier_wait, 1);
                c.set({{3, carrier()}, {4, 1}});
                c.call(0x10281af0, 0x101d5394);
                c.set(4, 0);
                c.put8(match + carrier_wait, 0);
                c.set(3, carrier());
                c.set(5, c.u32(frame.at(0x44)));
                if (compare_logical(c, carrier(), c.r(5)) != 0) return;
                if (on_ball()) go(0x101d53fc);
            };
            carrier_turn();
            // The stopped clock, once more, then the normal turn.
            c.set(4, c.u8(match + play_state));
            if (compare(c, field(play_state), 0) != 0 || compare(c, field(extra_time_flag), 0) != 0) {
                const auto roll = std::int32_t(draw(c, 10, 0x101d5424));
                c.put8(frame.at(0x38), u(s8(roll)));
                c.put16(match + clock_at, u(c.i16(match + clock_at)) + u(s8(c.u8(frame.at(0x38)))));
                c.set(4, u(s8(c.u8(frame.at(0x38)))));
                const auto extra = scaled_draw(s8(c.u8(frame.at(0x38))), 0x6c, 0x74, 0x101d547c);
                c.set(4, c.r(3));
                c.put8(match + added_time, c.u8(match + added_time) + u(s8(extra)));
                return -1;
            }
            if (compare_logical(c, c.u32(match + 0x7c68)) != 0) return -1;
            // A player who made a challenge (0x104, 0x76) joins the actors, if there is room.
            c.set(4, u(actor_j) << 2);
            c.set(3, current());
            const auto kind1 = std::int16_t(action_code(c, current()));
            c.set(4, u(std::int32_t(kind1)));
            bool joins = compare(c, kind1, 0x104) == 0;
            if (!joins) {
                c.set(5, u(actor_j) << 2);
                c.set(3, current());
                const auto kind2 = std::int16_t(action_code(c, current()));
                c.set(4, u(std::int32_t(kind2)));
                joins = compare(c, kind2, 0x76) == 0;
            }
            if (joins && compare(c, std::int32_t(actor_j), 0x16) < 0) {
                const auto joiner = current();
                c.set(7, joiner);
                const auto count_now = c.u8(match + actors_count);
                c.set(4, count_now);
                c.put8(match + actors_count, count_now + 1u);
                c.put32(slot_at(u(s8(count_now))), joiner);
            }
            c.set(3, match);
            call(c, 0x101e4130, 0x101d5514);
            c.set(7, c.u8(frame.at(0x3d)));
            if (compare(c, s8(c.u8(frame.at(0x3d))), field(ball_row_at)) != 0 ||
                compare(c, s8(c.u8(frame.at(0x3c))), field(ball_column_at)) != 0)
                ball_cells(true);
        }
        return -1;
    };
    for (;;) {
        c.set(4, u(s8(remaining)));
        if (compare(c, s8(remaining), 0) == 0) return result(c, 1);
        const auto outcome = pass();
        if (outcome >= 0) return result(c, std::uint32_t(outcome));
    }
}
}  // namespace cm::aot::match_eng
