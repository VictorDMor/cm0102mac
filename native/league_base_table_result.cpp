// LEAGUE.CPP, block `table`: match results into the standings. 0x104a3b90 applies a fixture to the two clubs'
// rows (home or away group, the totals, the head-to-head entries and the two clubs' new points), 0x104a3930 is
// the virtual (+0xc) that gets a result into a competition and its statistics, 0x104a26b0 (called by
// MATCH_ENG.CPP, so it keeps its frame and the registers its callees save) asks whether a result would decide
// the title. 0x104a3b90 and 0x104a3930 leave r4-r10 as the original does (league_base_volatile_readers.inc), so
// the registers the original writes are written here at the same places; 0x104a4870 is a leaf of 0x104a3b90
// whose registers reach it the same way.
#include "league_base_table_detail.hpp"

namespace cm::aot::league_base {
namespace st = table::row;
namespace fx = table::fixture;
using table::lbz_extsb;
using table::lha;

void count_head_to_head(Cpu& c, std::uint32_t league, std::uint32_t rows, std::uint32_t row, std::uint32_t entry,
                        std::uint32_t scored, std::uint32_t conceded) {
    if (std::int32_t(row) < 0) return;
    if (std::int32_t(entry) < 0) return;
    const auto row_address = rows + row * league::standing_row;  // r4
    const auto at = entry << 4;                                  // r0
    const auto goals = sext8(scored), against = sext8(conceded); // r6, r5
    const auto first_table = c.u32(row_address + st::head_to_head);  // r9
    const auto played = c.u8(first_table + at + table::entry::played);
    c.put8(first_table + at + table::entry::played, played + 1);
    if (std::int32_t(goals) > std::int32_t(against)) {
        auto cell = c.u32(row_address + st::head_to_head) + at;
        c.put8(cell + table::entry::won, c.u8(cell + table::entry::won) + 1);
        cell = c.u32(row_address + st::head_to_head) + at;
        const auto points = lbz_extsb(c, league + table::field::win_points);
        c.put16(cell + table::entry::points, c.u16(cell + table::entry::points) + points);
    } else if (goals == against) {
        auto cell = c.u32(row_address + st::head_to_head) + at;
        c.put8(cell + table::entry::drawn, c.u8(cell + table::entry::drawn) + 1);
        cell = c.u32(row_address + st::head_to_head) + at;
        const auto points = lbz_extsb(c, league + table::field::draw_points);
        c.put16(cell + table::entry::points, c.u16(cell + table::entry::points) + points);
    } else {
        const auto cell = c.u32(row_address + st::head_to_head) + at;
        c.put8(cell + table::entry::lost, c.u8(cell + table::entry::lost) + 1);
    }
    const auto entries = c.u32(row_address + st::head_to_head);  // r6
    const auto entry_address = entries + at;                     // r7
    const auto old_scored = lha(c, entry_address + table::entry::scored);  // r6
    const auto new_scored = old_scored + goals;                  // r5
    c.put16(entry_address + table::entry::scored, new_scored);
    const auto final_entries = c.u32(row_address + st::head_to_head) + at;  // r4
    c.put16(final_entries + table::entry::conceded, c.u16(final_entries + table::entry::conceded) + against);
    // The registers the original leaves: r3 the conceded byte, r4 the entry, r5 the new scored sum, r6 the old
    // scored, r7 the entry (first load), r9 the first table.
    c.set({{3, against}, {4, final_entries}, {5, new_scored}, {6, old_scored}, {7, entry_address}, {9, first_table}});
}

void apply_to_rows(Cpu& c, std::uint32_t league, std::uint32_t fixture, std::uint32_t rows, std::uint32_t count,
                   std::uint32_t decider, std::uint32_t out_home, std::uint32_t out_away) {
    const Frame frame(c, 0x104a3b90, 0x60, {.first_gpr = 22});
    std::uint32_t r22 = c.r(22), r26 = decider;
    std::uint32_t home = 0xffffffffu, away = 0xffffffffu;  // r28, r29: the indexes of the two clubs' rows
    // The saved registers the original holds at its virtual calls (r22 and r26 take row offsets in some branches).
    const auto sync = [&] {
        c.set({{22, r22}, {23, count}, {24, out_away}, {25, out_home}, {26, r26}, {27, league}, {28, home},
               {29, away}, {30, rows}, {31, fixture}});
    };
    sync();
    // The object that answers the result question: the competition of the record when it is a league (kind 2),
    // the league itself otherwise (and when the record or the table is missing: then null, and the league is used).
    const auto record = c.u32(league + competition::record);
    c.set(4, record);
    std::uint32_t object = 0;  // r3
    if (record != 0) {
        const auto competitions = c.u32(c.toc(toc::competitions));
        c.set(3, competitions);
        if (competitions != 0) {
            const auto candidate = c.u32(competitions + (c.u32(record) << 2));
            c.set(3, candidate);
            object = c.u8(candidate + competition::kind) == competition::knockout ? candidate : league;
        }
    }
    c.set(3, object);
    // The two clubs' rows: the first row of the club at home (the fixture's +0x1c) and of the one away (+0x20),
    // searched until a row that is neither of them once both were found.
    const auto limit = sext16(count);
    c.set({{4, rows}, {7, 0}});
    std::uint32_t pointer = rows, index = 0;
    for (;;) {
        const auto tested = sext16(index);
        c.set(5, tested);
        if (!(std::int32_t(tested) < std::int32_t(limit))) break;
        const auto club = c.u32(pointer);
        c.set({{6, club}, {5, c.u32(fixture + fx::home)}});
        if (club == c.u32(fixture + fx::home)) {
            home = index;
            c.set(28, home);
        } else {
            c.set(5, c.u32(fixture + fx::away));
            if (club == c.u32(fixture + fx::away)) {
                away = index;
                c.set(29, away);
            } else {
                c.set(5, sext16(home));
                if (std::int32_t(sext16(home)) != -1) {
                    c.set(5, sext16(away));
                    if (std::int32_t(sext16(away)) != -1) break;
                }
            }
        }
        pointer += league::standing_row;
        ++index;
        c.set({{4, pointer}, {7, index}});
    }
    const auto home_index = std::int32_t(sext16(home)), away_index = std::int32_t(sext16(away));
    const auto home_row = [&] { return rows + std::uint32_t(home_index) * league::standing_row; };
    const auto away_row = [&] { return rows + std::uint32_t(away_index) * league::standing_row; };
    const auto byte_up = [&](std::uint32_t at) { const auto old = c.u8(at); c.put8(at, old + 1); return old; };
    const auto half_up = [&](std::uint32_t at, std::uint32_t delta) {
        const auto old = lha(c, at);
        c.put16(at, old + delta);
        return old;
    };
    const auto byte_of = [&](std::uint32_t at) { return lbz_extsb(c, fixture + at); };
    // The goals go to the home club's group +0x24 and the away club's +0x34: the extra-time goals when the
    // fixture has them (+0x47 is not -1), else the regular ones.
    if (home_index > -1) {
        const auto row = home_row();
        c.set(5, row);
        byte_up(row + st::home + st::played);
        const auto extra = byte_of(fx::extra_home);
        c.set(4, extra);
        std::uint32_t old = 0;
        if (std::int32_t(extra) != -1) {
            half_up(row + st::home + st::scored, extra);
            old = half_up(row + st::home + st::conceded, byte_of(fx::extra_away));
        } else {
            old = half_up(row + st::home + st::scored, byte_of(fx::home_goals));
            old = half_up(row + st::home + st::conceded, byte_of(fx::away_goals));
        }
        c.set(4, old);
    }
    if (away_index > -1) {
        const auto row = away_row();
        c.set(4, row);
        byte_up(row + st::away + st::played);
        std::uint32_t old = 0;
        if (std::int32_t(byte_of(fx::extra_home)) != -1) {
            half_up(row + st::away + st::scored, byte_of(fx::extra_away));
            old = half_up(row + st::away + st::conceded, byte_of(fx::extra_home));
        } else {
            half_up(row + st::away + st::scored, byte_of(fx::away_goals));
            old = half_up(row + st::away + st::conceded, byte_of(fx::home_goals));
        }
        c.set(5, old);
    }
    // The result: who won decides which counters move.
    const auto home_goals = byte_of(fx::home_goals), away_goals = byte_of(fx::away_goals);
    c.set(4, home_goals);
    const auto target = object ? object : league;
    // A club's points for the fixture: the virtual +0x98 on the competition's object (or on the league), a
    // signed byte.
    const auto virtual_points = [&](std::uint32_t club, std::uint32_t back_object, std::uint32_t back_league) {
        sync();
        return sext8(virtual_call(c, 0x98, object ? back_object : back_league, {target, club, fixture}));
    };
    // A counter of the home club and its points from the virtual (`scratch` takes the row offset, as r26 or r22).
    const auto home_scores = [&](std::uint32_t counter, std::uint32_t& scratch, std::uint32_t back_object, std::uint32_t back_league) {
        if (home_index <= -1) return;
        const auto offset = std::uint32_t(home_index) * league::standing_row;
        scratch = offset;
        const auto row = rows + offset;
        c.set(5, row);
        c.set(4, byte_up(row + counter));
        const auto points = virtual_points(c.u32(fixture + fx::home), back_object, back_league);
        c.set(4, row);
        c.set(3, half_up(row + st::home + st::points, points));
    };
    const auto away_scores = [&](std::uint32_t counter, std::uint32_t& scratch, std::uint32_t back_object, std::uint32_t back_league) {
        if (away_index <= -1) return;
        const auto offset = std::uint32_t(away_index) * league::standing_row;
        scratch = offset;
        const auto row = rows + offset;
        c.set(5, row);
        c.set(4, byte_up(row + counter));
        const auto points = virtual_points(c.u32(fixture + fx::away), back_object, back_league);
        c.set(4, row);
        c.set(3, half_up(row + st::away + st::points, points));
    };
    const auto home_counts = [&](std::uint32_t counter) {
        if (home_index <= -1) return;
        const auto row = home_row();
        c.set(5, row);
        c.set(4, byte_up(row + counter));
    };
    const auto away_counts = [&](std::uint32_t counter) {
        if (away_index <= -1) return;
        const auto row = away_row();
        c.set(4, row);
        c.set(3, byte_up(row + counter));
    };
    const auto set_decider = [&](std::uint32_t at, std::uint32_t value) {
        if (sext8(r26) != 0) c.put8(fixture + at, value);
    };
    const auto decider_by_penalties = [&](std::uint32_t at) {
        const auto flags = lha(c, fixture + fx::flags);
        c.set(0, flags);
        c.put8(fixture + at, (flags & 0x20) == 0 ? 4 : 5);
    };
    if (std::int32_t(home_goals) > std::int32_t(away_goals)) {
        home_scores(st::home + st::won, r26, 0x104a3dcc, 0x104a3dec);
        away_counts(st::away + st::lost);
    } else if (std::int32_t(home_goals) < std::int32_t(away_goals)) {
        home_counts(st::home + st::lost);
        away_scores(st::away + st::won, r26, 0x104a3e88, 0x104a3ea8);
    } else {
        const auto flags = lha(c, fixture + fx::flags);  // r0
        std::uint32_t extra_time = flags & 2;            // r6
        c.set(6, extra_time);
        bool done = false;
        if (extra_time != 0) {
            const auto x = byte_of(fx::extra_home), y = byte_of(fx::extra_away);
            c.set({{5, x}, {4, y}});
            if (std::int32_t(x) > std::int32_t(y)) {
                home_scores(st::home + st::won, r22, 0x104a3f24, 0x104a3f44);
                away_counts(st::away + st::lost);
                if (sext8(r26) != 0) c.put8(fixture + fx::decider_home, 3);
                done = true;
            }
        }
        if (!done && extra_time != 0) {
            const auto x = byte_of(fx::extra_home), y = byte_of(fx::extra_away);
            c.set({{5, x}, {4, y}});
            if (std::int32_t(x) < std::int32_t(y)) {
                home_counts(st::home + st::lost);
                away_scores(st::away + st::won, r22, 0x104a400c, 0x104a402c);
                set_decider(fx::decider_away, 3);
                done = true;
            }
        }
        if (!done) {
            std::uint32_t penalties = flags & 0x20;  // r6
            c.set(6, penalties);
            bool shootout = penalties != 0;
            if (!shootout) {
                c.set(4, flags & 1);
                shootout = (flags & 1) != 0;
            }
            if (shootout) {
                const auto x = byte_of(fx::penalties_home), y = byte_of(fx::penalties_away);
                c.set({{5, x}, {4, y}});
                if (std::int32_t(x) > std::int32_t(y)) {
                    home_scores(st::home + st::drawn, r22, 0x104a40bc, 0x104a40dc);
                    away_counts(st::away + st::lost);
                    if (sext8(r26) != 0) decider_by_penalties(fx::decider_home);
                    done = true;
                }
            }
            if (!done) {
                if (penalties == 0) {
                    c.set(4, flags & 1);
                    shootout = (flags & 1) != 0;
                } else {
                    shootout = true;
                }
                if (shootout) {
                    const auto x = byte_of(fx::penalties_home), y = byte_of(fx::penalties_away);
                    c.set({{5, x}, {4, y}});
                    if (std::int32_t(x) < std::int32_t(y)) {
                        home_counts(st::home + st::lost);
                        away_scores(st::away + st::drawn, r22, 0x104a41c4, 0x104a41e4);
                        if (sext8(r26) != 0) decider_by_penalties(fx::decider_away);
                        done = true;
                    }
                }
            }
            // A plain draw: no extra time decided, no shootout flag.
            if (!done && penalties == 0 && (flags & 1) == 0) {
                if (home_index > -1) {
                    const auto row = home_row();
                    c.set(4, row);
                    byte_up(row + st::home + st::drawn);
                    const auto points = lbz_extsb(c, league + table::field::draw_points);
                    c.set(3, half_up(row + st::home + st::points, points));
                }
                if (away_index > -1) {
                    const auto row = away_row();
                    c.set(4, row);
                    byte_up(row + st::away + st::drawn);
                    const auto points = lbz_extsb(c, league + table::field::draw_points);
                    c.set(3, half_up(row + st::away + st::points, points));
                }
            }
        }
    }
    // The totals of both rows, and the clubs' new points through the caller's pointers.
    const auto totals = [&](std::uint32_t row, std::uint32_t out) {
        for (const auto field : {st::played, st::won, st::drawn, st::lost})
            c.put8(row + st::totals + field, c.u8(row + st::home + field) + c.u8(row + st::away + field));
        for (const auto field : {st::scored, st::conceded, st::points})
            c.put16(row + st::totals + field, c.u16(row + st::home + field) + c.u16(row + st::away + field));
        if (out) c.put16(out, c.u16(row + st::totals + st::points));
    };
    if (home_index > -1) {
        const auto row = home_row();
        c.set(3, row);
        totals(row, out_home);
        c.set(4, lha(c, row + st::home + st::points));
    } else if (out_home) {
        c.put16(out_home, 0);
    }
    if (away_index > -1) {
        const auto row = away_row();
        c.set(4, row);
        totals(row, out_away);
        c.set(3, lha(c, row + st::home + st::points));
    } else if (out_away) {
        c.put16(out_away, 0);
    }
    // The head-to-head entries (the rows have them when the first row's table pointer is set): the entry of the
    // other club in each row's table, found by scanning the entries of both rows together.
    if (c.u32(rows + st::head_to_head) == 0) return;
    const auto clubs = sext16(count);
    c.set({{3, clubs}, {10, 0}, {8, 0}, {7, sext16(home)}, {5, sext16(away)}});
    const auto last = std::int32_t(clubs) - 1;
    std::uint32_t found_home = 0xffffffffu, found_away = 0xffffffffu;  // r6, r23
    std::uint32_t at = 0, k = 0;  // r8, r10
    c.set({{6, found_home}});
    for (;;) {
        const auto tested = sext16(k);
        c.set(9, tested);
        if (!(std::int32_t(tested) < last)) break;
        if (std::int32_t(sext16(home)) > -1) {
            const auto entries = c.u32(rows + std::uint32_t(sext16(home)) * league::standing_row + st::head_to_head);
            if (c.u32(entries + at) == c.u32(fixture + fx::away)) {
                found_home = tested;
                c.set(6, found_home);
                if (std::int32_t(found_away) > -1 && std::int32_t(tested) > -1) break;
            }
        }
        if (std::int32_t(sext16(away)) > -1) {
            const auto entries = c.u32(rows + std::uint32_t(sext16(away)) * league::standing_row + st::head_to_head);
            if (c.u32(entries + at) == c.u32(fixture + fx::home)) {
                found_away = tested;
                if (std::int32_t(tested) > -1 && std::int32_t(found_home) > -1) break;
            }
        }
        at += table::entry::size;
        ++k;
        c.set({{8, at}, {10, k}});
    }
    const auto raw = [&](std::uint32_t at_) { return std::uint32_t(c.u8(fixture + at_)); };
    c.set({{7, raw(fx::home_goals)}, {3, league}, {8, raw(fx::away_goals)}, {4, rows}, {5, sext16(home)}, {6, found_home}});
    c.link(0x104a44bc);
    count_head_to_head(c, league, rows, sext16(home), found_home, raw(fx::home_goals), raw(fx::away_goals));
    c.set({{7, raw(fx::away_goals)}, {3, league}, {8, raw(fx::home_goals)}, {4, rows}, {6, found_away}, {5, sext16(away)}});
    c.link(0x104a44d8);
    count_head_to_head(c, league, rows, sext16(away), found_away, raw(fx::away_goals), raw(fx::home_goals));
}

std::uint32_t apply_result(Cpu& c, std::uint32_t league, std::uint32_t fixture, std::uint32_t extra) {
    const Frame frame(c, 0x104a3930, 0xd0, {.first_gpr = 26});
    // The saved registers: the message buffer, the pool, the league, the fixture, the extra argument and the
    // success flag; r6 holds the pointer to the current date (TOC -0x7b80).
    const auto date = c.toc(-0x7b80), flag = c.toc(toc::success), message = c.toc(toc::message), pool = c.toc(toc::pool);
    c.set({{6, date}, {31, flag}, {28, league}, {26, message}, {30, extra}, {27, pool}, {29, fixture}});
    if (fixture == 0) {
        report(c, frame, {0x104a3974, 0x94, 0x4c, 0xc7f});
        c.set(3, 0xffffffffu);
        return 0xffffffffu;
    }
    // The league's current date (+0xcc: day, +0xce: year, +0xd0: a byte) from the game's.
    c.put16(league + 0xce, c.u16(date + 2));
    c.put16(league + 0xcc, c.u16(date));
    c.put8(league + 0xd0, c.u8(date + 4));
    const auto owner_byte = c.u8(league + competition::owner);
    c.set(7, owner_byte);
    const auto owner = std::int32_t(sext8(owner_byte));
    if (owner == -1) {
        const auto raw = c.u8(fixture + fx::competition);
        c.set(3, raw);
        const auto stage_index = std::int32_t(sext8(raw));
        c.set(6, std::uint32_t(stage_index));
        if (stage_index != -1) {
            if (stage_index <= c.i32(league + competition::last_stage) && stage_index >= 0) {
                // The result goes to that stage: its own virtual +0xc with the same arguments.
                const auto stages = c.u32(league + competition::stages);
                const auto stage = c.u32(stages + (std::uint32_t(stage_index) << 2));
                c.set(3, stage);
                const auto result = virtual_call(c, 0xc, 0x104a3ac4, {stage, fixture, extra});
                return result;
            }
            report(c, frame, {0x104a3a18, 0x74, 0x44, 0xc8e});
            report(c, frame, {0x104a3a68, 0x54, 0x3c, 0xc8f});
            c.set(3, 0);
            return 0;
        }
    }
    const auto fixture_stage = std::int32_t(sext8(c.u8(fixture + fx::competition)));
    c.set(3, std::uint32_t(owner));
    if (owner != fixture_stage && owner != -2) {
        c.set(3, 0);
        return 0;
    }
    const auto rows = c.u32(league + league::standings);
    const auto clubs = lha(c, league + competition::club_count);
    c.set({{5, rows}, {3, league}, {6, clubs}, {4, fixture}, {8, frame.at(0x3a)}, {9, frame.at(0x38)}, {7, 1}});
    c.link(0x104a3b10);
    apply_to_rows(c, league, fixture, rows, clubs, 1, frame.at(0x3a), frame.at(0x38));
    if (std::int32_t(sext8(c.u8(league + competition::owner))) != -2) {
        const auto cell = c.toc(toc::competitions);
        const auto table = c.u32(cell);
        c.set({{3, cell}, {4, table}});
        if (table != 0) {
            const auto record = c.u32(league + competition::record);
            c.set(3, record);
            const auto competition_object = c.u32(table + (c.u32(record) << 2));
            c.set(3, competition_object);
            if (competition_object != 0) {
                const auto stats = c.u32(competition_object + competition::stats);
                c.set(3, stats);
                if (stats != 0 && extra != 0) {
                    const auto away_points = lha(c, frame.at(0x3a)), home_points = lha(c, frame.at(0x38));
                    c.set({{6, away_points}, {4, fixture}, {7, home_points}, {5, extra}});
                    invoke(c, 0x104537b0, 0x104a3b70, {stats, fixture, extra, away_points, home_points});
                }
            }
        }
    }
    c.set(3, 1);
    return 1;
}

std::uint32_t result_decides_title(Cpu& c, std::uint32_t league, std::uint32_t club, std::uint32_t fixture) {
    // The match engine calls this: the full original image (the frame, r28-r31 at each call).
    const Frame frame(c, 0x104a26b0, 0x50, {.first_gpr = 28});
    c.set({{28, club}, {29, fixture}, {30, league}});
    const auto done = [&](std::uint32_t value) { c.set(3, value); return value; };
    if (std::int32_t(sext8(c.u8(fixture + fx::competition))) != -1) return done(0);
    const auto id = c.u32(c.u32(league + competition::record));
    c.set(3, id);
    if (sext8(invoke(c, 0x104646d0, 0x104a26fc, {id})) == 0) return done(0);
    if ((c.u16(league + league::flags) & 1) == 0) return done(0);
    if (c.i16(league + table::field::rounds_played) < 0x18) return done(0);
    c.set({{3, league}, {4, club}, {5, 0}});
    c.link(0x104a2744);
    const auto own_row = find_standing(c, league, club, 0);
    if (own_row == 0) return done(0);
    const auto status = c.u8(own_row + st::status);
    c.set(4, status);
    const auto signed_status = std::int32_t(sext8(status));
    if (signed_status == 5 || ((status - 2u) & 0xff) <= 1 || signed_status == -2 || signed_status == -4 || signed_status == -5)
        return done(0);
    const auto copy = invoke(c, 0x104a6440, 0x104a279c, {league});
    c.set(31, copy);
    if (copy == 0) return done(0);
    const auto clubs = lha(c, league + competition::club_count);
    c.set({{6, clubs}, {3, league}, {4, fixture}, {5, copy}, {7, 0}, {8, 0}, {9, 0}});
    c.link(0x104a27cc);
    apply_to_rows(c, league, fixture, copy, clubs, 0, 0, 0);
    c.set({{3, league}, {4, copy}});
    c.link(0x104a27d8);
    sort_standings(c, league, copy);
    c.set({{3, league}, {4, club}, {5, copy}});
    c.link(0x104a27e8);
    const auto copy_row = find_standing(c, league, club, copy);
    if (sext8(c.u8(copy_row + st::position)) != 0) {
        invoke(c, 0x104a6660, 0x104a2800, {league, copy});
        return done(0);
    }
    std::uint32_t offset = league::standing_row;  // r6
    for (std::uint32_t counter = 1; std::int32_t(sext16(counter)) < c.i16(league + competition::club_count); ++counter, offset += league::standing_row) {
        const auto row = copy + offset;
        const auto win_points = lbz_extsb(c, league + table::field::win_points);
        const auto played = lbz_extsb(c, row + st::totals + st::played);
        const auto limit = lha(c, league + table::field::played_limit);
        const auto remaining = limit - played;
        const auto possible = std::int16_t(lha(c, row + st::totals + st::points) + win_points * remaining);
        const auto leader_points = lha(c, copy + st::totals + st::points);
        if (std::int32_t(std::int32_t(possible)) < std::int32_t(leader_points)) continue;
        // Level or ahead: still catching the leader unless both have played every match.
        const auto leader_played = lbz_extsb(c, copy + st::totals + st::played);
        if (leader_played == limit && limit == played) continue;
        invoke(c, 0x104a6660, 0x104a2870, {league, copy});
        return done(0);
    }
    invoke(c, 0x104a6660, 0x104a28a0, {league, copy});
    return done(1);
}

}
