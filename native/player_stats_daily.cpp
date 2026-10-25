// PLAYER_STATS.CPP: the match-day updates of the season records (record_match
// and the routines it uses). The archive of finished seasons is in
// player_stats_daily_archive.cpp.
#include "player_stats_detail.hpp"

namespace cm::aot::player_stats {
using namespace detail;

std::uint32_t record_match(Cpu& c, std::uint32_t stats_address, std::uint32_t fixture_address, std::uint32_t match_address) {
    const bool has_fixture = nonzero(c, std::int32_t(fixture_address));  // or. r30,r4,r4
    Frame frame(c, 0x103024a0, 0xf0, {.first_gpr = 13});
    Program program{c};
    // The prologue loads the TOC words it uses into non-volatile registers;
    // most are reused for loop values later, as the original does.
    const Kept<25> contracts{c, c.toc(slot::contracts)};
    const Kept<26> player_records{c, c.toc(slot::player_records)};
    const Kept<29> stats_reg{c, stats_address};
    Kept<15> row_address{c, c.toc(slot::random_salt)};
    const Kept<31> match_reg{c, match_address};
    Kept<14> target{c, c.toc(slot::random_cursor)};  // later the player record, then his PlayerEntry
    const Kept<27> person_count{c, c.toc(slot::person_count)};
    const Kept<13> persons{c, c.toc(slot::persons)};
    const Kept<28> constants{c, c.toc(slot::constants)};
    Kept<18> pass{c, c.toc(slot::success)};
    Kept<17> side_address{c, c.toc(slot::message)};
    Kept<16> other_side{c, c.toc(slot::pool)};  // later the other side's offset in the match
    const Kept<30> fixture_reg{c, fixture_address};
    // The category of the game for the side being recorded; set only when
    // computed (it keeps the caller's r21 until then).
    Kept<21> category{c, c.r(21)};
    const std::uint32_t pool = c.toc(slot::pool);
    if (!has_fixture) {
        report(c, pool, 0x103024fc, frame.at(0x68), frame.at(0x40), 0x28c, false);
        return 0;
    }
    if (!nonzero_logical(c, match_address)) {
        report(c, pool, 0x1030255c, frame.at(0x48), frame.at(0x38), 0x294, false);
        return 0;
    }
    const Stats stats{c, stats_address};
    const Fixture fixture{c, fixture_address};
    const Match match{c, match_address};
    if (compare(c, fixture.id(), -2) == 0) {
        // A game outside any competition: league (2) or, one draw in nine, cup (3).
        const Field<std::uint32_t> cursor{c, c.toc(slot::random_cursor)};
        const Field<std::uint32_t> salt{c, c.toc(slot::random_salt)};
        const std::uint32_t previous = cursor, next = previous + 2;
        cursor = next;
        if (compare_logical(c, next, c.u32(c.toc(slot::random_end))) > 0) {
            cursor = c.toc(slot::random_table);
            salt = program.call(0x1077b770, 0x103025d4, {next, previous}) & 0xffff;
        }
        const std::uint32_t at = cursor;
        const auto draw = std::int32_t(c.u16(at) + salt);
        leave(c, {{5, at}, {4, std::uint32_t(draw)}});
        category = draw % 9 != 0 ? category::league : category::cup;
    }
    c.put32(frame.at(0x90), 0);  // the side's offset in the match
    Kept<20> side{c, 0};
    do {
        if (compare(c, fixture.id(), -2) != 0) {
            const std::uint32_t club = compare(c, side, 0) == 0 ? fixture.home() : fixture.away();
            enter(c, 0x10302654, {fixture_address, club});
            category = result(c, competition_type(c, fixture_address, club));
        }
        Kept<19> index{c, 0};
        Kept<23> row_offset{c, 0};
        side_address = match_address + c.u32(frame.at(0x90)) + 0x120;
        other_side = (1 - side) * Match::side_size + 0x120;
        do {
            row_address = side_address + MatchRow::first + row_offset;
            // One row's tallies into the player's season records.
            const auto record_row = [&] {
                const MatchRow row{c, row_address};
                // A substitute who did not come on, and empty rows, are skipped.
                if (compare(c, index, 10) > 0 && compare(c, row.came_on(), -1) == 0) return;
                if (compare(c, row.person(), -1) == 0) return;
                if (compare(c, row.player(), -1) == 0) return;
                {
                    const Kept<22> person{c, c.u32(persons) + std::uint32_t(row.person().get()) * Person::size};
                    if (nonzero(c, std::int32_t(person)) && compare(c, Person{c, person}.id(), c.i32(person_count)) >= 0)
                        return;
                    target = c.u32(player_records) + std::uint32_t(row.player().get()) * 0x48;
                    {
                        // The rating goes into the sums of rated or unrated games.
                        const std::uint32_t sums = stats.rating_sums(nonzero(c, match.rated()));
                        Fpu fpu{c};
                        const double rating = fpu.fsub(stacked_integer(c, frame.at(0x88), row.rating()), unsigned_bias);
                        c.put_f64(sums, fpu.fadd(c.f64(sums), rating));
                        c.put_f64(sums + 8, fpu.fadd(c.f64(sums + 8), c.f64(constants + 0x28)));
                    }
                    // Into the game's category, then into senior club (6) unless
                    // the game is international or non-competitive.
                    pass = 1;
                    do {
                        const Kept<24> into{c, compare(c, pass, 1) == 0 ? std::uint32_t(category) : 6};
                        if (compare(c, std::int8_t(into), category::senior_club) == 0) {
                            const auto game = std::int8_t(category);
                            if (compare(c, game, category::international) == 0 || compare(c, game, category::non_competitive) == 0)
                                continue;
                        }
                        enter(c, 0x103027a4, {stats_address, target, into});
                        std::uint32_t address = season_record(c, stats_address, target, std::int8_t(std::uint32_t(into)));
                        if (!nonzero_logical(c, address)) {
                            enter(c, 0x103027bc, {stats_address, target, into});
                            address = result(c, add_season_record(c, stats_address, target, into));
                        }
                        const SeasonRecord record{c, address};
                        const auto add = [](const Field<std::uint8_t>& field, std::uint32_t value) {
                            field = std::uint8_t(field + value);
                        };
                        const auto add16 = [](const Field<std::int16_t>& field, std::uint32_t value) {
                            field = std::int16_t(std::uint32_t(field.get()) + value);
                        };
                        add(record.appearances(), 1);
                        if (compare(c, index, 10) > 0) add(record.substitute_appearances(), 1);
                        add(record.tally(2), row.goals());
                        add(record.tally(3), row.conceded());
                        add(record.tally(4), row.penalties());
                        add(record.penalties_bracketed(), row.penalties_bracketed());
                        add(record.tally(5), row.assists());
                        const std::uint8_t scored = c.u8(side_address), conceded = c.u8(match_address + other_side);
                        add(record.tally(6), scored);
                        add(record.tally(7), conceded);
                        if (compare_logical(c, scored, conceded) > 0) add(record.games_won(), 1);
                        if (compare_logical(c, scored, conceded) < 0) add(record.games_lost(), 1);
                        add(record.tally(10), row.yellow_cards());
                        add(record.tally(11), row.red_cards());
                        if (compare(c, row.player(), match.man_of_match()) == 0) add(record.tally(12), 1);
                        const bool starter = compare(c, index, 11) < 0;
                        add16(record.rating_total(), row.rating());
                        add16(record.passes(), row.passes());
                        add16(record.passes_completed(), row.passes_completed());
                        add16(record.tackles(), row.tackles());
                        add16(record.dribbles(), row.dribbles());
                        add16(record.shots(), row.shots());
                        const std::int16_t shots_on_target = record.shots_on_target();
                        c.set(4, extended(shots_on_target));
                        record.shots_on_target() = std::int16_t(std::uint32_t(shots_on_target) + row.shots_on_target());
                        // Minutes played: to the minute he went off (substituted,
                        // sent off, or the end), from the minute he came on.
                        const Field<std::int32_t> minutes = record.figure();
                        const auto until = [&]() -> std::int32_t {
                            std::int32_t minute = row.substituted();
                            if (compare(c, minute, -1) > 0) return minute;
                            minute = row.sent_off();
                            if (compare(c, minute, -1) > 0) return minute;
                            return match.length();
                        };
                        if (starter) {
                            const std::int32_t off = row.substituted(), sent_off = row.sent_off();
                            c.set(4, std::uint32_t(off));
                            if (compare(c, off, -1) > 0) minutes = std::int32_t(std::uint32_t(minutes.get()) + std::uint32_t(off));
                            else {
                                c.set(4, std::uint32_t(sent_off));
                                if (compare(c, sent_off, -1) > 0)
                                    minutes = std::int32_t(std::uint32_t(minutes.get()) + std::uint32_t(sent_off));
                                else {
                                    c.set(4, std::uint32_t(minutes.get()));
                                    minutes = std::int32_t(std::uint32_t(minutes.get()) + std::uint32_t(std::int32_t(match.length())));
                                }
                            }
                        } else {
                            const std::int32_t on = row.came_on();
                            c.set(5, std::uint32_t(on));
                            if (compare(c, on, -1) != 0) {
                                const std::int32_t off = until();
                                c.set(4, std::uint32_t(minutes.get()));
                                minutes = std::int32_t(std::uint32_t(minutes.get()) + std::uint32_t(off - on));
                            }
                        }
                    } while (pass = pass + 1, compare(c, pass, 2) <= 0);
                    // The player's entry: his games and goals for his club in the
                    // league (or a league-like competition), and his last five ratings.
                    const auto game = std::int8_t(category);
                    const bool league = compare(c, game, category::league) == 0;
                    c.set(4, stats.players());
                    target = stats.players() + std::uint32_t(row.player().get()) * PlayerEntry::size;
                    const PlayerEntry entry{c, target};
                    bool counts = league;
                    if (!counts) {
                        const std::int32_t id = fixture.id();
                        counts = compare(c, id, -1) > 0 && nonzero(c, std::int8_t(program.call(0x104646d0, 0x10302a30, {std::uint32_t(id)})));
                    }
                    if (counts) {
                        entry.figure(1) = std::int16_t(entry.figure(1) + 1);
                        c.set(4, person);
                        entry.figure(2) = std::int16_t(std::uint32_t(entry.figure(2).get()) + row.goals());
                        if (!nonzero_logical(c, program.call(0x10083110, 0x10302a60, {c.u32(contracts), person})) &&
                            nonzero_logical(c, program.call(0x10082f20, 0x10302a74, {c.u32(contracts), person}))) {
                            const std::uint32_t contract = program.call(0x10082f20, 0x10302a88, {c.u32(contracts), person});
                            const std::int16_t games = c.i16(contract + 0x42);
                            c.set(4, extended(games));
                            c.put16(contract + 0x42, std::uint32_t(games) + 1);
                        }
                    }
                    // Shift the 4-bit ratings one place and add this game's.
                    const auto r0 = entry.ratings(0), r1 = entry.ratings(1), r2 = entry.ratings(2);
                    r0 = std::uint8_t((r0 << 4) | (r1 >> 4));
                    r1 = std::uint8_t((r1 << 4) | (r2 >> 4));
                    r2 = std::uint8_t((r2 & 0x0f) | ((row.rating() << 4) & 0xf0));
                }
            };
            record_row();
            index = index + 1;
            row_offset = row_offset + MatchRow::size;
        } while (compare(c, index, int(Match::rows)) < 0);
        c.put32(frame.at(0x90), c.u32(frame.at(0x90)) + Match::side_size);
        side = side + 1;
    } while (compare(c, side, 2) < 0);
    return 1;
}

std::uint32_t add_season_record(Cpu& c, std::uint32_t stats_address, std::uint32_t player, std::uint32_t category_value) {
    const bool has_player = nonzero_logical(c, player);
    Frame frame(c, 0x10308090, 0xd0, {.first_gpr = 24});
    Program program{c};
    const Kept<28> alert_text{c, c.toc(slot::alert_text)};
    const Kept<29> alert_title{c, c.toc(slot::alert_title)};
    const Kept<26> stats_reg{c, stats_address};
    Kept<27> index_address{c, c.toc(slot::success)};
    const Kept<30> message{c, c.toc(slot::message)};
    const Kept<31> pool{c, c.toc(slot::pool)};
    if (!has_player) {
        report(c, pool, 0x103080d4, frame.at(0x90), frame.at(0x48), 0xe92, false);
        return 0;
    }
    const Stats stats{c, stats_address};
    const PlayerEntry entry{c, stats.players() + c.u32(player) * PlayerEntry::size};
    c.set(4, stats.players());
    const auto wanted = std::int8_t(category_value);
    if (wanted < category::non_competitive || wanted > category::senior_club) {
        report(c, pool, 0x103081a4, frame.at(0x70), frame.at(0x40), 0xeca, false);
        return 0;
    }
    index_address = entry.season(wanted).address();
    const Field<std::int32_t> index{c, index_address};
    if (compare(c, index, -1) == 0) {
        if (nonzero(c, stats.free_seasons())) {
            // Reuse a freed record (rating_total -1). The search registers
            // keep their last values when none is found.
            Kept<24> address{c, stats.seasons()};
            Kept<25> n{c, 0};
            while (compare(c, n, stats.season_count()) < 0) {
                if (compare(c, SeasonRecord{c, address}.rating_total(), -1) == 0) {
                    program.memset(address, 0, SeasonRecord::size, 0x1030822c);
                    index = std::int32_t(n);
                    const std::int32_t free = stats.free_seasons();
                    c.set(4, std::uint32_t(free));
                    stats.free_seasons() = free - 1;
                    return address;
                }
                n = n + 1;
                address = address + SeasonRecord::size;
            }
        }
        const std::int32_t capacity = stats.season_capacity();
        if (compare(c, stats.season_count(), capacity) >= 0) {
            stats.season_capacity() = capacity + 100;
            stats.seasons() =
                program.realloc(stats.seasons(), std::uint32_t(stats.season_capacity().get()) << 5, 0x10308280);
            if (!nonzero_logical(c, stats.seasons())) {
                // Out of memory: a detailed report and an alert; the success
                // flag stays as it is.
                const auto name = frame.at(0x50), extension = frame.at(0x38);
                program.call(0x10739890, 0x103082a8, {pool + text::file_name, 0, 0, name, extension});
                const std::uint32_t version = program.call(0x10768810, 0x103082ac);
                program.sprintf(0x103082cc, {message, pool + text::assert_format_detail, version, name, extension, 0xef4,
                                             pool + text::no_memory_detail});
                program.localize(alert_title, pool + text::no_memory_title, 0x103082d8);
                program.call(0x10727890, 0x103082e8, {alert_text, pool + text::no_memory, message});
                program.alert(alert_title, alert_text, 0, 0x103082f8);
                return 0;
            }
        }
        const std::int32_t n = stats.season_count();
        stats.season_count() = n + 1;
        index = n;
        program.memset(stats.seasons() + (std::uint32_t(index.get()) << 5), 0, SeasonRecord::size, 0x1030832c);
    }
    return stats.seasons() + (std::uint32_t(index.get()) << 5);
}

std::uint32_t competition_type(Cpu& c, std::uint32_t fixture_address, std::uint32_t club) {
    const bool has_fixture = nonzero(c, std::int32_t(fixture_address));  // or. r27,r3,r3
    Frame frame(c, 0x10308de0, 0xd0, {.first_gpr = 27});
    Program program{c};
    const Kept<27> fixture_reg{c, fixture_address};
    const Kept<29> success{c, c.toc(slot::success)};
    const Kept<30> message{c, c.toc(slot::message)};
    const Kept<28> club_reg{c, club};
    const Kept<31> pool{c, c.toc(slot::pool)};
    if (!has_fixture) {
        report(c, pool, 0x10308e1c, frame.at(0x90), frame.at(0x48), 0x1014, false);
        return 2;
    }
    const Fixture fixture{c, fixture_address};
    if (nonzero(c, std::int8_t(program.call(0x10494d20, 0x10308e70, {c.u32(c.toc(slot::fixture_rules)), fixture.competition()})))) {
        // Non-competitive, or international for a national side.
        const bool national = nonzero(c, std::int8_t(program.call(0x100c7d00, 0x10308e80, {fixture.home()})));
        return national ? category::international : category::non_competitive;
    }
    const std::int32_t id = c.i32(fixture.competition());
    c.set(4, std::uint32_t(id));
    for (const auto friendly : {slot::friendly_competition_1, slot::friendly_competition_2, slot::friendly_competition_3})
        if (compare(c, id, c.i32(c.toc(friendly))) == 0) return category::non_competitive;
    if (compare_logical(c, club, fixture.home()) == 0 && !nonzero(c, fixture.home_counts()))
        return category::non_competitive;
    if (compare_logical(c, club, fixture.away()) == 0 && !nonzero(c, fixture.away_counts()))
        return category::non_competitive;
    const std::uint32_t competition = c.u32(c.u32(c.toc(slot::competitions)) + (std::uint32_t(fixture.id().get()) << 2));
    c.set(4, competition);
    if (!nonzero_logical(c, competition)) {
        report(c, pool, 0x10308f3c, frame.at(0x70), frame.at(0x40), 0x103a, false);
        return 2;
    }
    const std::int32_t kind = Competition{c, competition}.kind();
    if (compare(c, kind, 4) == 0) return category::international;
    if (compare(c, kind, 2) == 0) return category::continental;
    if (compare(c, kind, 1) == 0) {
        const auto league = std::int8_t(program.call(0x104646d0, 0x10308fb4, {std::uint32_t(fixture.id().get()), competition}));
        return league != 0 ? category::league : category::cup;
    }
    report(c, pool, 0x10308fe0, frame.at(0x50), frame.at(0x38), 0x105d, false);
    return 2;
}

void clear_seasons(Cpu& c, std::uint32_t stats_address, std::uint32_t person) {
    const bool has_person = nonzero(c, std::int32_t(person));  // or. r27,r4,r4
    Frame frame(c, 0x10309040, 0xa0, {.first_gpr = 27});
    Program program{c};
    const Kept<27> person_reg{c, person};
    const Kept<28> success{c, c.toc(slot::success)};
    const Kept<29> message{c, c.toc(slot::message)};
    const Kept<31> stats_reg{c, stats_address};
    const Kept<30> pool{c, c.toc(slot::pool)};
    // Both reports end the program (fatal_exit -1); the original goes on
    // after the call regardless.
    if (!has_person) {
        report(c, pool, 0x1030907c, frame.at(0x68), frame.at(0x40), 0x1071, false);
        program.fatal_exit(std::uint32_t(-1), 0x103090c0);
    }
    const Person who{c, person};
    if (!nonzero_logical(c, who.player())) {
        report(c, pool, 0x103090e0, frame.at(0x48), frame.at(0x38), 0x1075, false);
        program.fatal_exit(std::uint32_t(-1), 0x10309124);
    }
    const Stats stats{c, stats_address};
    const PlayerEntry entry{c, stats.players() + c.u32(who.player()) * PlayerEntry::size};
    for (std::int32_t category = 1; category <= 6; ++category) {
        const std::int32_t index = entry.season(category);
        if (compare(c, index, -1) == 0) continue;
        SeasonRecord{c, stats.seasons() + (std::uint32_t(index) << 5)}.rating_total() = -1;  // freed
        stats.free_seasons() = stats.free_seasons() + 1;
        entry.season(category) = -1;
    }
    entry.figure(1) = 0;
    entry.figure(2) = 0;
    for (std::uint32_t byte = 0; byte < 3; ++byte) entry.ratings(byte) = 0;
    leave(c, {{4, entry.history_offset().address()}, {5, entry.history_size().address()}, {6, 0}});
    // 0x10749fc0 marks the archive's history as none (size -1).
    program.call(0x10749fc0, 0x103092d0, {stats.archive()});
}
}
