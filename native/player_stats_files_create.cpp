#include "player_stats_detail.hpp"
#include "player_stats_files.hpp"
#include <bit>

// The player-stats object's constructor: the CD, the history archive, then
// the saved game or a new game (the real-world history import).
namespace cm::aot::player_stats {
using namespace detail;
namespace {
// The relations group's routines the new game calls (player_stats_relations.hpp).
constexpr std::int32_t first_history_year = 1900;
constexpr std::int32_t progress_step = 500;  // history rows between progress updates
constexpr std::int32_t season_capacity = 100;  // a new game's season records
constexpr std::uint32_t group_count = 64;

// The relation threshold of a person: 200 - 10 x adaptability (int16, held in r21).
std::uint32_t threshold(const Person& person) {
    return extended(std::int16_t(200 - std::int32_t(person.adaptability()) * 10));
}
// A relation strength from a score at or above the threshold: (score -
// threshold) / 50, at most 10 (`cmpwi r0,10` on the quotient), as an int8.
std::uint32_t strength(Cpu& c, std::int32_t score, std::int32_t threshold) {
    const std::int32_t quotient = (score - threshold) / 50;
    const std::int32_t value = compare(c, quotient, 10) > 0 ? 10 : quotient;
    return std::uint32_t(std::int32_t(std::int8_t(value)));
}

// Asks for the CD until its volume is there: FSMakeFSSpec of the volume's
// name, a Pascal string of the two halves' characters interleaved (their
// lengths added), built afresh on each try.
void insert_cd(Cpu& c, const Frame& frame, std::uint32_t pool) {
    Program program{c};
    const Kept<13> half_a{c, c.toc(slot::cd_name_a)};
    const Kept<17> half_b{c, c.toc(slot::cd_name_b)};
    Kept<23> mounted{c, 0};
    const auto copy_a = frame.at(0x138), copy_b = frame.at(0x68), name = frame.at(0x244);
    do {
        const Kept<24> first_word{c, c.u32(half_a)};
        for (std::uint32_t at = 0; at < 0x10; at += 4) c.put32(copy_a + at, c.u32(half_a + at));
        c.put8(copy_a + 0x10, c.u8(half_a + 0x10));
        for (std::uint32_t at = 0; at < 0x10; at += 4) c.put32(copy_b + at, c.u32(half_b + at));
        mounted = 0;
        std::uint32_t at = 1;
        for (; nonzero(c, c.i8(copy_a + at)); ++at) {
            c.put8(name + 2 * at - 1, c.u8(copy_a + at));
            c.put8(name + 2 * at, c.u8(copy_b + at));
        }
        c.put8(name, c.u8(copy_a) + c.u8(copy_b));
        c.set({{7, c.u8(copy_a)}, {8, c.u32(half_b)}, {9, c.u8(half_a + 0x10)}, {10, c.u32(half_a + 0xc)}});
        const auto error = program.toolbox(routine::fs_make_fs_spec, 0x10301634, {0, 0, name, frame.at(0x17c)});
        if (!nonzero(c, std::int16_t(error))) mounted = 1;
        if (compare(c, mounted, 0) == 0) {
            program.localize(frame.at(0x40c), pool + text::insert_cd_title, 0x10301658);
            program.localize(frame.at(0x344), pool + text::insert_cd, 0x10301664);
            program.alert(frame.at(0x40c), frame.at(0x344), 1, 0x10301674);
        }
    } while (compare(c, mounted, 0) == 0);
}

// A new game's relations: each person's score for each group (1-64) at or
// above his threshold, and for a non-player at a club abroad, his club's
// nation's group by his years there (as an int8, 0-10, from the start
// date's year less the year he joined his club).
void seed_relations(Cpu& c, const Frame& frame, const Stats& stats) {
    Program program{c};
    const auto person_count = c.toc(slot::person_count);  // r18
    const Date start{c, c.toc(slot::start_date)};
    Kept<13> index{c, 0};
    stats.relations() = 0;
    stats.relation_capacity() = 0;
    stats.relation_count() = 0;
    Kept<17> person_address{c, c.u32(c.toc(slot::persons))};
    for (; compare(c, index, c.i32(person_count)) < 0; index = index + 1, person_address = person_address + Person::size) {
        const Person person{c, person_address};
        Kept<23> group{c, 1};
        const Kept<21> limit{c, threshold(person)};
        for (; compare(c, std::int8_t(group), std::int32_t(group_count)) <= 0; group = group + 1) {
            enter(c, 0x10301990, {stats.address, person.address, group});
            const auto score = std::int16_t(result(c, group_score(c, person.address, group)));
            if (!nonzero(c, score) || compare(c, score, std::int32_t(std::uint32_t(limit))) < 0) continue;
            const auto amount = strength(c, score, std::int32_t(std::uint32_t(limit)));
            enter(c, 0x103019e8, {stats.address, person.address, group, amount, 0});
            add_relation(c, stats.address, person.address, group, amount, 0);
        }
        if (nonzero_logical(c, person.player()) || !nonzero_logical(c, person.non_player())) continue;
        const std::uint32_t club = person.club();
        if (!nonzero_logical(c, club)) continue;
        const std::uint32_t nation = Club{c, club}.nation();
        if (!nonzero_logical(c, nation) || compare_logical(c, nation, person.nation()) == 0) continue;
        std::int32_t years = std::int8_t(std::uint32_t(start.year()) - person.joined().year());
        if (compare(c, years) < 0) years = 0;
        if (compare(c, years, 10) > 0) years = 10;
        if (!nonzero(c, years)) continue;
        // constants[2] x float(years) x float(adaptability), truncated.
        Fpu fpu{c};
        const auto constants = c.toc(slot::constants);
        const double bias = c.f64(constants + 0x20), scale = c.f64(constants + 0x10);
        const double years_value = fpu.fsubs(stacked_integer(c, frame.at(0x5b4), signed_word(years)), bias);
        const double adaptability_value =
            fpu.fsubs(stacked_integer(c, frame.at(0x5ac), signed_word(person.adaptability())), bias);
        const double product = fpu.fmuls(years_value, adaptability_value);
        c.put64(frame.at(0x5a4), fpu.fctiwz(0, fpu.fmul(scale, product)));
        c.set_f(1, years_value);
        c.set_f(2, bias);
        c.set_f(3, scale);
        const std::uint32_t nation_group = std::uint8_t(Nation{c, nation}.group().get()), amount = c.u32(frame.at(0x5a8));
        enter(c, 0x10301ad0, {stats.address, person.address, nation_group, amount, 1});
        add_to_relation(c, stats.address, person.address, nation_group, amount, 1);
    }
}

// The history rows by club: each player's appearances and goals at each club
// go to club_histories. With the "check goalkeepers" option a goalkeeper's
// season of six goals or more is reported. A row's goals without
// appearances are dropped first.
void total_clubs(Cpu& c, const Frame& frame, std::uint32_t pool) {
    Program program{c};
    const auto rows = c.toc(slot::history_rows), count = c.toc(slot::history_count);
    const auto options = c.toc(slot::options), title = frame.at(0x4d4);
    Kept<23> player{c, 0};
    Kept<18> index{c, 0};
    Kept<25> club{c, 0};
    Kept<17> offset{c, 0};
    keep(c, {{13, 0x10624dd3}});  // the original's multiplier for index / 500
    Kept<26> appearances{c, 0};
    Kept<27> goals{c, 0};
    const auto add_totals = [&](std::uint32_t back) {
        program.call(routine::add_club_history, back,
                     {c.u32(c.toc(slot::club_histories)), player, club, appearances, goals});
    };
    for (;; index = index + 1, offset = offset + HistoryRow::size) {
        const std::int32_t total = c.i32(count);
        c.set({{3, count}, {6, std::uint32_t(total)}});
        if (compare(c, std::int32_t(std::uint32_t(index)), total - 1) > 0) break;
        if (compare(c, std::int32_t(std::uint32_t(index)) % progress_step) == 0)
            program.call(routine::progress, 0x10301c10,
                         {c.toc(slot::game), title, index + 1, std::uint32_t(total) << 1, std::uint32_t(-1)});
        const HistoryRow row{c, c.u32(rows) + offset};
        const Kept<24> kept_row{c, row.address};
        if (nonzero_logical(c, row.goals()) && !nonzero_logical(c, row.appearances())) row.goals() = 0;
        const std::uint32_t person = row.player();
        if (!nonzero_logical(c, result(c, person)) || !nonzero_logical(c, Person{c, person}.player()) ||
            !nonzero_logical(c, row.club()))
            continue;
        if (compare_logical(c, person, player) != 0 || compare_logical(c, row.club(), club) != 0) {
            if (nonzero_logical(c, player) && nonzero_logical(c, club)) add_totals(0x10301c98);
            player = row.player();
            appearances = 0;
            club = row.club();
            goals = 0;
        }
        if (nonzero_logical(c, program.strstr(options, pool + text::check_goalkeepers, 0x10301cb4)) &&
            nonzero_logical(c, row.player()) && nonzero_logical(c, Person{c, row.player()}.player()) &&
            compare(c, std::int16_t(program.call(routine::roles, 0x10301cd8, {Person{c, row.player()}.player()})), 1) == 0 &&
            compare_logical(c, row.goals(), 6) >= 0) {
            const auto warning = c.toc(slot::scratch_text);
            program.sprintf(0x10301cfc, {warning, pool + text::too_many_goals});
            const auto length = program.strlen(warning, 0x10301d04);
            program.call(routine::append_name, 0x10301d1c, {row.player(), warning + length, 0x7d0, 1});
            program.strcat(warning, pool + text::newline, 0x10301d28);
            report(c, pool, 0x10301d3c, frame.at(0x98), frame.at(0x40), 0x139, false);
            c.set(3, c.toc(slot::success));
        }
        c.set(3, row.appearances());
        appearances = appearances + row.appearances();
        goals = goals + row.goals();
    }
    if (nonzero_logical(c, player) && nonzero_logical(c, club)) add_totals(0x10301dd0);
}

// When the history import moves on to the next player: the previous one's
// group scores (the stack table, int16 per group) become relations. A group
// of his club's nation but none of his own gets at least his age first.
void score_groups(Cpu& c, const Frame& frame, const Stats& stats, const Person& person) {
    Program program{c};
    const auto groups = frame.at(0x1c4), start_date = c.toc(slot::start_date);
    keep(c, {{31, groups}});
    Kept<16> group{c, 1};
    Kept<27> end{c, 2};  // the byte offset past the group's int16
    for (; compare(c, std::int8_t(group), std::int32_t(group_count)) <= 0; end = end + 2, group = group + 1) {
        const std::uint32_t nation = person.nation();
        if (!nonzero_logical(c, nation)) break;
        if (const std::uint32_t club = person.club(); nonzero_logical(c, club)) {
            const std::int32_t number = std::int8_t(group);
            if (compare(c, Nation{c, Club{c, club}.nation()}.group(), number) == 0 &&
                compare(c, Nation{c, nation}.group(), number) != 0) {
                const auto joined = person.joined().address;
                const auto days_at_club = [&](std::uint32_t back) {
                    return std::int16_t(program.call(routine::days_between, back,
                                                     {start_date, c.u32(joined), c.u32(joined + 4)}));
                };
                const std::int16_t days = days_at_club(0x10301f90);
                keep(c, {{30, groups}, {26, end - 2}});
                const Field<std::int16_t> score{c, groups + end - 2};
                std::int16_t value = score;
                c.set(4, extended(value));
                if (compare(c, value, days) <= 0) c.set(4, extended(value = days_at_club(0x10301fbc)));
                score = value;
            }
        }
        const std::int16_t score = Field<std::int16_t>{c, groups + end - 2};
        c.set(4, extended(score));
        const auto limit = std::int32_t(std::int16_t(c.r(21)));
        if (nonzero(c, score) && compare(c, score, limit) >= 0) {
            const auto amount = strength(c, score, limit);
            enter(c, 0x10302024, {stats.address, person.address, group, amount, 1});
            add_to_relation(c, stats.address, person.address, group, amount, 1);
        }
    }
}

// The history rows by player and year (1900 up to the year before the
// start): each player's seasons become two CareerRows each (senior club and
// league) in the history archive, where his PlayerEntry says, and score his
// groups (the clubs' nations other than his own: appearances and recency, at
// least his age in days scaled at his current club); his seasons at his
// current club add to his real-world appearances and goals.
void import_careers(Cpu& c, const Frame& frame, const Stats& stats, std::uint32_t pool) {
    Program program{c};
    const auto rows = c.toc(slot::history_rows), count = c.toc(slot::history_count);
    const auto title = frame.at(0x4d4), groups = frame.at(0x1c4);
    const Date start{c, c.toc(slot::start_date)}, today{c, c.toc(slot::today)};
    const CareerRow record{c, frame.at(0x14c)};
    Kept<25> index{c, 0};
    Kept<18> list{c, 0};  // the player's CareerRows
    c.put32(frame.at(0x5bc), 0x10624dd3);  // the multiplier for index / 500, kept on the stack
    Kept<13> offset{c, 0};
    Kept<17> previous{c, 0};
    const auto store = [&](std::uint32_t write_back, std::uint32_t destroy_back) {
        const PlayerEntry entry{c, c.r(22)};
        program.call(history_file::write, write_back,
                     {stats.archive(), entry.history_offset().address(), entry.history_size().address(), list});
        program.call(list::destroy, destroy_back, {list, 1});
    };
    for (;; index = index + 1, offset = offset + HistoryRow::size) {
        const std::int32_t total = c.i32(count);
        c.set({{3, count}, {7, std::uint32_t(total)}});
        if (compare(c, std::int32_t(std::uint32_t(index)), total) >= 0) break;
        if (compare(c, std::int32_t(std::uint32_t(index)) % progress_step) == 0)
            program.call(routine::progress, 0x10301e4c,
                         {c.toc(slot::game), title, std::uint32_t(total) + index + 1, std::uint32_t(total) << 1,
                          std::uint32_t(-1)});
        const HistoryRow row{c, c.u32(rows) + offset};
        const Kept<23> kept_row{c, row.address};
        const std::int32_t year = row.year();
        if (compare(c, year, first_history_year) < 0 || compare(c, year, start.year()) >= 0) continue;
        const Person person{c, row.player()};
        const Kept<24> kept_person{c, person.address};
        if (!nonzero_logical(c, person.address)) {
            if (nonzero_logical(c, previous) && nonzero_logical(c, list)) {
                store(0x10301ea0, 0x10301eac);
                // r19 and r20 keep pointing into the destroyed list (see below).
                list = 0;
                keep(c, {{14, 0}});
                previous = 0;
            }
            continue;
        }
        const Kept<16> player_record{c, person.player()};
        if (!nonzero_logical(c, player_record)) {
            previous = person.address;
            continue;
        }
        if (compare_logical(c, person.address, previous) != 0 && nonzero_logical(c, list)) {
            store(0x10301ef0, 0x10301efc);
            keep(c, {{19, 0}, {20, 0}, {14, 0}});
            list = 0;
        }
        if (compare_logical(c, person.address, previous) != 0) {
            keep(c, {{22, stats.players() + c.u32(player_record) * PlayerEntry::size}});
            if (nonzero_logical(c, previous)) score_groups(c, frame, stats, Person{c, previous});
            program.memset(groups, 0, 2 * group_count, 0x10302048);
            keep(c, {{21, threshold(person)}});
        }
        // The season's record: senior club, then the same as league.
        program.memset(record.address, 0, CareerRow::size, 0x1030206c);
        record.year() = std::uint16_t(row.year());
        const std::uint32_t row_club = row.club();
        record.club() = nonzero_logical(c, row_club) ? Club{c, row_club}.id().get() : -2;
        record.kind() = (record.kind() & 0xe0) | category::senior_club;
        record.record().rating_total() = -1;
        record.record().appearances() = row.appearances().get();
        record.record().tally(2) = row.goals().get();
        record.other_club() = -1;
        c.set(3, std::uint32_t(-1));
        // The previous season's two rows get this season's loan flag as
        // their status (career_table moves status one season up). After the
        // player's list was stored for a row without a person, r19 and r20
        // still point into the freed list; the original writes there too.
        if (nonzero_logical(c, c.r(20)) && nonzero_logical(c, c.r(19))) {
            const std::uint32_t loan = nonzero(c, row.loan()) ? 1 : 0;
            c.set(3, loan);
            for (const unsigned n : {20u, 19u}) c.put8(c.r(n) + 0x2c, (c.u8(c.r(n) + 0x2c) & 0x1f) | loan << 5);
        }
        if (compare_logical(c, record.year(), c.r(14) & 0xffff) < 0) {
            report(c, pool, 0x1030213c, frame.at(0x78), frame.at(0x38), 0x1e0, false);
            c.set(3, c.toc(slot::success));
        }
        keep(c, {{14, record.year()}});
        if (!nonzero_logical(c, list)) {
            const Kept<16> created{c, program.operator_new(0x10, 0x10302190)};
            if (nonzero(c, std::int32_t(std::uint32_t(created)))) program.call(list::create, 0x1030219c, {created});
            list = created;
        }
        record.kind() = (record.kind() & 0xe0) | category::senior_club;
        keep(c, {{20, program.call(list::append, 0x103021c0, {list, record.address, CareerRow::size})}});
        record.kind() = (record.kind() & 0xe0) | category::league;
        keep(c, {{19, program.call(list::append, 0x103021e4, {list, record.address, CareerRow::size})}});

        const std::int32_t club_index = record.club();
        if (compare(c, club_index, -1) > 0) {
            const auto clubs = c.u32(c.toc(slot::clubs));
            const auto nation_at = std::uint32_t(club_index) * Club::size + 0x2c;
            const std::uint32_t nation = c.u32(clubs + nation_at);
            c.set({{3, c.toc(slot::clubs)}, {4, clubs}, {5, nation}, {6, nation_at}});
            const std::uint32_t second = person.second_nation();
            if (nonzero_logical(c, nation) && compare_logical(c, nation, result(c, person.nation())) != 0 &&
                (!nonzero_logical(c, result(c, second)) || compare_logical(c, nation, second) != 0)) {
                const std::uint8_t group_byte = Nation{c, nation}.group();
                const std::int32_t group = std::int8_t(group_byte);
                c.set({{5, group_byte}, {3, std::uint32_t(group)}});
                if (compare(c, group) > 0 && compare(c, group, std::int32_t(group_count)) <= 0) {
                    // apps x 6 + 180 - 2 x (years ago), added as an int16.
                    const auto at = std::uint32_t(group) * 2 - 2;
                    const Field<std::int16_t> score{c, groups + at};
                    const std::int32_t old = score;
                    const std::uint32_t ago = (std::uint32_t(today.year()) - record.year()) << 1;
                    const std::uint32_t sum =
                        std::uint32_t(old) +
                        extended(std::int16_t(std::uint32_t(record.record().appearances()) * 6 + 0xb4 - ago));
                    score = std::int16_t(sum);
                    c.set({{3, groups}, {5, today.year()}, {7, sum}, {8, ago}, {9, std::uint32_t(old)}, {10, at}});
                    const std::uint32_t current = person.club();
                    c.set(7, current);
                    const auto at_current = [&] {
                        if (!nonzero_logical(c, current)) return false;
                        const std::int32_t id = Club{c, current}.id();
                        c.set(7, std::uint32_t(id));
                        return compare(c, id, club_index) == 0;
                    };
                    if (at_current()) {
                        // At his current club: at least his days there x constants[3].
                        Fpu fpu{c};
                        const Date joined = person.joined();
                        const std::uint32_t days =
                            (std::uint32_t(today.year()) - joined.year()) * 365 + extended(today.day()) - extended(joined.day());
                        const auto constants = c.toc(slot::constants);
                        const double bias = c.f64(constants + 0x20), scale = c.f64(constants + 0x18);
                        const std::int32_t current_score = score;
                        double value = fpu.fmul(scale, fpu.fsub(stacked_integer(c, frame.at(0x5a4), signed_word(days)), bias));
                        const double kept = fpu.fsub(stacked_integer(c, frame.at(0x5ac), signed_word(current_score)), bias);
                        c.set_f(0, kept);
                        fpu.fcmp(0, value, kept);
                        if (!(c.cr(0) & 4)) {
                            value = fpu.fsub(stacked_integer(c, frame.at(0x5b4), signed_word(current_score)), bias);
                        }
                        c.put64(frame.at(0x59c), fpu.fctiwz(0, value));
                        score = std::int16_t(c.u32(frame.at(0x5a0)));
                        c.set_f(1, value);
                        c.set_f(2, bias);
                        c.set_f(3, scale);
                        c.set({{3, groups}, {4, constants}, {5, at}, {6, extended(joined.day())}, {7, 0x43300000}});
                    }
                }
            }
        }
        if (const std::uint32_t current = person.club(); nonzero_logical(c, result(c, current)) &&
                                                         compare(c, result(c, Club{c, current}.id()), record.club()) == 0) {
            const PlayerEntry entry{c, c.r(22)};
            entry.real_appearances() = std::int16_t(entry.real_appearances() + record.record().appearances());
            c.set(3, extended(entry.real_goals()));
            entry.real_goals() = std::int16_t(entry.real_goals() + record.record().tally(2));
        }
        previous = person.address;
    }
    if (nonzero_logical(c, list)) store(0x103023b4, 0x103023c0);
}

// A new game: empty season records, a PlayerEntry per player, the rating
// sums from constants, the relations, then (unless the options say
// "no real history" or the import is off) the real-world career histories.
void new_game(Cpu& c, const Frame& frame, const Stats& stats, std::uint32_t pool) {
    Program program{c};
    stats.season_capacity() = season_capacity;
    stats.season_count() = 0;
    stats.free_seasons() = 0;
    stats.seasons() = program.malloc(std::uint32_t(stats.season_capacity().get()) << 5, 0x10301760);
    if (!nonzero_logical(c, stats.seasons())) {
        report(c, pool, 0x10301784, frame.at(0xf8), frame.at(0x58), 0x79);
        program.fatal_exit(std::uint32_t(-1), 0x103017cc);
        stats.season_capacity() = 0;
        return;
    }
    stats.player_count() = c.i32(c.toc(slot::stats_players));
    stats.players() = program.malloc(std::uint32_t(stats.player_count().get()) * PlayerEntry::size, 0x103017f4);
    if (!nonzero_logical(c, stats.players())) {
        report(c, pool, 0x10301818, frame.at(0xd8), frame.at(0x50), 0x85);
        program.fatal_exit(std::uint32_t(-1), 0x10301860);
        stats.player_count() = 0;
        return;
    }
    const auto constants = c.toc(slot::constants);
    const auto sum = c.u64(constants), count = c.u64(constants + 8);
    c.set_f(1, std::bit_cast<double>(sum));
    c.set_f(0, std::bit_cast<double>(count));
    for (const bool rated : {true, false}) {
        c.put64(stats.rating_sums(rated), sum);
        c.put64(stats.rating_sums(rated) + 8, count);
    }
    c.set({{3, 0}, {4, constants}, {6, 0}});
    for (std::int32_t index = 0; compare(c, index, stats.player_count()) < 0; ++index) {
        const PlayerEntry entry{c, stats.players() + std::uint32_t(index) * PlayerEntry::size};
        keep(c, {{22, entry.address}});
        for (std::int32_t category = 1; category <= 6; ++category) entry.season(category) = -1;
        entry.history_offset() = -1;
        entry.history_size() = -1;
        entry.real_appearances() = 0;
        entry.real_goals() = 0;
        // The original copies the ratings' nibbles one from another, the first 0: all three bytes end 0.
        for (std::uint32_t byte = 3; byte-- > 0;) entry.ratings(byte) = 0;
        c.set({{3, std::uint32_t(index + 1)}, {4, 0}, {5, stats.players()}, {6, std::uint32_t(index + 1) * PlayerEntry::size}});
    }
    seed_relations(c, frame, stats);
    program.qsort(stats.relations(), stats.relation_count(), Relation::size, c.toc(slot::compare_relations_tv), 0x10301af8);
    if (nonzero_logical(c, program.strstr(c.toc(slot::options), pool + text::no_real_history, 0x10301b04))) return;
    if (nonzero(c, c.i8(result(c, c.toc(slot::history_import_off))))) return;
    if (!nonzero(c, c.i8(result(c, c.toc(slot::history_import_on))))) return;
    if (!nonzero_logical(c, c.u32(result(c, c.toc(slot::history_rows))))) {
        report(c, pool, 0x10301b50, frame.at(0xb8), frame.at(0x48), 0xef);
        return;
    }
    program.localize(frame.at(0x4d4), pool + text::player_histories, 0x10301ba4);
    const auto sort_rows = [&](std::int32_t comparator, std::uint32_t back) {
        program.qsort(c.u32(c.toc(slot::history_rows)), c.u32(c.toc(slot::history_count)), HistoryRow::size,
                      c.toc(comparator), back);
    };
    sort_rows(slot::compare_by_club_tv, 0x10301bc0);
    total_clubs(c, frame, pool);
    sort_rows(slot::compare_by_player_tv, 0x10301dec);
    enter(c, 0x10301df0);
    merge_history_rows(c);
    import_careers(c, frame, stats, pool);
    program.call(routine::free_history_rows, 0x103023c4);
}
}

std::uint32_t create_stats(Cpu& c, std::uint32_t stats_address, std::uint32_t archive) {
    Frame frame(c, 0x10301540, 0x610, {.first_gpr = 13});
    Program program{c};
    const Stats stats{c, stats_address};
    const Kept<15> object{c, stats_address};
    const Kept<29> pool{c, c.toc(slot::pool)};
    // What the original loads into its registers first (the history import uses them).
    keep(c, {{16, c.toc(slot::scratch_text)}, {30, c.toc(slot::club_histories)}, {31, c.toc(slot::options)},
             {18, c.toc(slot::person_count)}, {28, c.toc(slot::start_date)}, {25, archive}, {14, 0}, {19, 0},
             {20, 0}, {26, 0}, {27, 0}});
    insert_cd(c, frame, pool);
    stats.archive() = new_history_file(c, 13, pool, history_file::mode, 0x10301680);
    if (!nonzero_logical(c, stats.archive())) {
        report(c, pool, 0x103016cc, frame.at(0x118), frame.at(0x60), 0x5e);
        program.fatal_exit(std::uint32_t(-1), 0x10301714);
        return result(c, stats_address);
    }
    // A saved game that fails to load starts a new one.
    if (nonzero_logical(c, archive)) {
        enter(c, 0x10301730, {stats_address, archive});
        if (nonzero(c, std::int8_t(load_stats(c, stats_address, archive)))) return result(c, stats_address);
    }
    new_game(c, frame, stats, pool);
    return result(c, stats_address);
}
}
