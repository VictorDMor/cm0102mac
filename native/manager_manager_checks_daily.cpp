// MANAGER_MANAGER.CPP checker cases for the daily update block (manager_manager_daily.hpp):
// 0x10196390, the module's day, and its helpers 0x10197650, 0x101976b0, 0x10197960 and 0x101b5090.
//
// The real day of the career snapshot is one case. The randomized ones keep the real object, list
// of job offers, people and clubs, cut the list after a few offers and write, over the records
// those offers point at, the values the rules read (dates around today's, posts, statuses, flags,
// reputations, slots), undone afterwards; a fixed seed. Half of them also answer the other modules'
// routines (their results steer the rules the real records rarely reach). Forced cases follow for the
// outcomes those draws miss; some write a record while another module's routine is dispatched
// (`Case::pokes`), for the tests the original repeats after a call. The statics the day
// keeps beside the table of contents (its month check) take every combination, and the slot at
// sp+0xb4 of the day's frame, which the day reads before it ever writes a word there, takes values
// of its own. The helpers get direct cases over the same kinds of records.
#include "manager_manager_checks.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t daily = 0x10196390, add_applicant = 0x10197650, end_post = 0x101976b0,
                        clear_post = 0x10197960, announce = 0x101b5090;
constexpr std::int32_t manager_object = -0x79f0, club_total = -0x7b50, national_slot = -0x7b04,
                       date_slot = -0x7b80, scope_slot = -0x7a14, leagues_slot = -0x7b5c, gate_slot = -0x7a9c;
struct Rng {
    std::uint64_t state;
    std::uint32_t next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return std::uint32_t(state >> 33);
    }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
    bool chance(unsigned percent) { return below(100) < percent; }
    template <class T> T pick(std::initializer_list<T> values) { return *(values.begin() + below(unsigned(values.size()))); }
};
// The scratch area: a deal record the end-of-post helper flags.
constexpr std::uint32_t deal_record = scratch_base + 0x1000;
// The other modules' routines the day and its helpers call.
namespace other {
constexpr std::uint32_t days_between = 0x10718130, employer_record = 0x10103510, level_of = 0x100f55b0,
                        link_b = 0x104ae570, deal_of = 0x10082f20, link_a = 0x104ade40, link_c = 0x104ad840,
                        human_a = 0x1030f950, human_b = 0x103d2860, month_of = 0x10718360,
                        news_sink = 0x10388860, log_open = 0x10748ce0, log_write = 0x10739de0;
}

struct Draft {
    Case item;
    void put(unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); }
    void answer(std::uint32_t entry, std::uint32_t value) { item.answers.push_back({entry, value}); }
};

// What the snapshot holds that the cases draw from.
struct World {
    const Scenario& sc;
    std::uint32_t object = 0, clubs = 0, people = 0, limit = 0, date = 0, today_day = 0, today_year = 0, rows = 0,
                  sp = 0, national_cell = 0, club_count_cell = 0, scope_cell = 0;
    std::vector<std::uint32_t> careers, staffs, places_a, places_b, places_c, regions, leagues;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> nodes;  // list node, offer
    std::vector<std::pair<std::uint32_t, std::uint32_t>> pairs;  // the real offers' person and club indices
    explicit World(const Scenario& scenario) : sc(scenario) {
        object = sc.global(manager_object);
        clubs = sc.global(club_total);
        people = sc.people();
        const auto national = sc.global(national_slot);
        limit = clubs - 2 * national;
        date = sc.toc(date_slot);
        today_day = unsigned(sc.s.memory.read<2>(date));
        today_year = unsigned(sc.s.memory.read<2>(date + 2));
        rows = sc.r32(object);
        sp = std::uint32_t(sc.s.r<4>(4));
        national_cell = sc.toc(national_slot);
        club_count_cell = sc.toc(club_total);
        scope_cell = sc.toc(scope_slot);
        for (unsigned k = 0; k < people && k < 30000; ++k) {
            const auto who = sc.person(k);
            if (sc.r32(who + 0x14)) careers.push_back(sc.r32(who + 0x14));
            if (sc.r32(who + 0x1c)) staffs.push_back(sc.r32(who + 0x1c));
            if (sc.r32(who + 0x28)) places_a.push_back(sc.r32(who + 0x28));
            if (sc.r32(who + 0x2c)) places_b.push_back(sc.r32(who + 0x2c));
            if (sc.r32(who + 0x30)) places_c.push_back(sc.r32(who + 0x30));
        }
        for (unsigned k = 0; k < clubs && k < 3000; ++k) {
            if (sc.r32(sc.club(k) + 0x2c)) regions.push_back(sc.r32(sc.club(k) + 0x2c));
            if (sc.r32(sc.club(k) + 0x30)) leagues.push_back(sc.r32(sc.club(k) + 0x30));
        }
        const auto list = sc.r32(object + 8);
        unsigned seen = 0;
        for (auto node = sc.r32(list + 4); node && seen < 600; node = sc.r32(node + 8), ++seen) {
            const auto job = sc.r32(node);
            if (nodes.size() < 12) nodes.emplace_back(node, job);
            pairs.emplace_back(sc.r32(job + 8), sc.r32(job + 0x10));
        }
    }
    std::uint32_t any(const std::vector<std::uint32_t>& list, Rng& rng) const { return list.empty() ? 0 : list[rng.below(unsigned(list.size()))]; }
    std::uint32_t normal_club(Rng& rng) const { return sc.club(rng.below(limit)); }
    std::uint32_t flagged_club(Rng& rng) const { return sc.club(limit + rng.below(clubs - limit)); }
    std::uint32_t normal_person(Rng& rng) const { return sc.person(rng.below(people - 16)); }
    std::uint32_t human(Rng& rng) const { return sc.person(people - 16 + rng.below(16)); }
    std::uint32_t id_of(std::uint32_t record) const { return sc.r32(record); }
};

// A date's two halfwords near today's: the year before, the same, after; the day before, equal, after,
// and the signed extremes.
struct Stamp { std::uint32_t day, year; };
Stamp near_today(const World& w, Rng& rng, int lean) {
    // lean < 0: the stamp is past today's; > 0: ahead; 0: anything
    const auto year_move = rng.chance(70) ? 0 : rng.pick<int>({-1, 1, -3, 2});
    std::uint32_t year = w.today_year + year_move;
    std::uint32_t day = w.today_day;
    const auto kind = rng.below(100);
    if (kind < 35) day = w.today_day - 1 - rng.below(20);
    else if (kind < 55) day = w.today_day;
    else if (kind < 85) day = w.today_day + 1 + rng.below(20);
    else day = rng.pick<std::uint32_t>({0, 0xffff, 0xfffe, 0x7fff, 0x8000, 0x8001, 0x1, 0x16d});
    if (lean < 0 && rng.chance(80)) { year = w.today_year; day = w.today_day - 1 - rng.below(30); }
    if (lean > 0 && rng.chance(80)) { year = w.today_year; day = w.today_day + 1 + rng.below(30); }
    return {day & 0xffff, year & 0xffff};
}

std::uint32_t status_value(Rng& rng) {
    return rng.chance(85) ? rng.below(17) : rng.pick<std::uint32_t>({0x7f, 0x80, 0xff, 0xfb, 0x11, 0x20});
}

void put_date(Draft& d, std::uint32_t at, const Stamp& stamp, std::uint32_t extra) {
    d.put(2, at, stamp.day);
    d.put(2, at + 2, stamp.year);
    d.put(1, at + 4, extra);
}

// A person's fields the rules read. `club` is the club he is being moved to; `other` another one.
struct Posts { std::uint32_t club_post, side_post; };
void write_person(Draft& d, const World& w, Rng& rng, std::uint32_t who, std::uint32_t club, std::uint32_t other, unsigned career_pct,
                  unsigned staff_pct, bool human) {
    d.put(4, who + 0x14, rng.chance(career_pct) ? w.any(w.careers, rng) : 0);
    d.put(4, who + 0x1c, rng.chance(staff_pct) ? w.any(w.staffs, rng) : 0);
    const auto pick_club = [&] { return rng.chance(35) ? club : rng.chance(50) ? other : rng.chance(50) ? w.normal_club(rng) : w.flagged_club(rng); };
    if (rng.chance(50)) d.put(4, who + 0x20, rng.chance(60) ? pick_club() : 0);
    if (rng.chance(50)) d.put(4, who + 0x24, rng.chance(75) ? pick_club() : 0);
    d.put(1, who + 0x60, status_value(rng));
    d.put(1, who + 0x62, status_value(rng));
    d.put(1, who + 0x6b, human ? rng.pick<std::uint32_t>({1, 1, 1, 3, 0, 0x80}) : rng.pick<std::uint32_t>({0, 1}));
    put_date(d, who + 0x4c, near_today(w, rng, 0), rng.below(3));
    put_date(d, who + 0x3c, near_today(w, rng, 0), rng.below(3));
}

// A club's fields the rules read.
void write_club(Draft& d, const World& w, Rng& rng, std::uint32_t club, std::uint32_t person) {
    d.put(4, club + 0x90, rng.pick<std::uint32_t>({0, person, person, w.normal_person(rng)}));
    const auto region = rng.chance(97) ? w.any(w.regions, rng) : 0;
    d.put(4, club + 0x2c, region);
    const auto league = rng.chance(97) ? w.any(w.leagues, rng) : 0;
    d.put(4, club + 0x30, league);
    // The leagues table's entry for it (read by the news gate).
    if (league && rng.chance(25)) d.put(4, w.sc.global(leagues_slot) + (w.sc.r32(league) << 2), 0);
    d.put(2, club + 0x1ec, rng.pick<std::uint32_t>({0x176f, 0x1770, 0x1771, 0x1c51, 0x1c52, 0x1c53, 0, 0x7fff, 0x8000, 0xffff, 6001, 3000, 500}));
    const auto slot = [&](std::uint32_t at) { d.put(4, at, rng.chance(40) ? person : rng.chance(30) ? 0 : w.normal_person(rng)); };
    for (const std::uint32_t at : {0x80u, 0x90u, 0x94u}) if (rng.chance(25)) d.put(4, club + at, rng.chance(60) ? person : 0);
    for (unsigned k = 0; k < 3; ++k) slot(club + 0x84 + 4 * k);
    for (unsigned k = 0; k < 5; ++k) slot(club + 0x160 + 4 * k);
    for (unsigned k = 0; k < 7; ++k) slot(club + 0x174 + 4 * k);
    for (unsigned k = 0; k < 3; ++k) slot(club + 0x190 + 4 * k);
    // The applicant slots: full, empty or in between.
    const auto fill = rng.pick<unsigned>({0, 0, 10, 49, 50, 25});
    for (unsigned k = 0; k < 50; ++k) d.put(4, club + 0x98 + 4 * k, k < fill ? (rng.chance(30) ? person : w.normal_person(rng)) : 0);
    if (region) {
        // The scope the day compares with the games' own (the cell at TOC -0x7a14), and the
        // monthly link (a pointer, only tested here).
        if (rng.chance(40)) d.put(4, w.scope_cell, w.sc.r32(region + 0x68));
        if (rng.chance(30)) d.put(4, region + 0x74, 0);
        d.put(1, region + 0x120, rng.pick<std::uint32_t>({0, 1, 1, 0x80, 0xff}));
    }
}

void write_career(Draft& d, const World& w, Rng& rng, std::uint32_t who) {
    const auto career = w.sc.r32(who + 0x14);
    if (career) d.put(2, career + 0xa, rng.pick<std::uint32_t>({0, 100, 1000, 0x1000, 0x1400, 0x1500, 0x1800, 0x7fff, 0x8000, 0xfff0}));
}

// The news item writer (0x10388860, reached through the news owners' generated page) ends in file
// system imports the checker does not implement; its result is not read by anything compared.
void sink_answer(Draft& d) { d.answer(other::news_sink, 0); d.answer(other::log_open, 0); d.answer(other::log_write, 0); }

// The answers a case gives for the other modules' routines (all calls of one routine answer alike).
// A routine can be answered (or poked) when both sides reach it through the dispatcher. Other
// modules' owners decline under this checker, so they do; the tactics owners, which run in place
// whatever the checker says, and leaves run in place do not. The ones not answered here run for
// real, steered through the records they read.
void write_answers(Draft& d, Rng& rng) {
    sink_answer(d);
    d.answer(other::days_between, rng.pick<std::uint32_t>({0, 0x59, 0x5a, 0x5b, 0xffffffffu, 1000}));
    d.answer(other::employer_record, rng.chance(50) ? 1 : 0);
    d.answer(other::level_of, rng.below(256));
    d.answer(other::link_b, rng.chance(50) ? 1 : 0);
    d.answer(other::deal_of, rng.chance(70) ? deal_record : 0);
    for (const auto entry : {other::link_a, other::link_c, other::human_a, other::human_b})
        d.answer(entry, rng.below(4));
}

// One offer of the list, rewritten.
void write_offer(Draft& d, const World& w, Rng& rng, std::uint32_t job, unsigned due_pct) {
    // A real offer's person and club (so the records the other modules read stay coherent), now
    // and then a human manager or a national side.
    const auto& pair = w.pairs[rng.below(unsigned(w.pairs.size()))];
    const bool human = rng.chance(20);
    const auto person_at = human ? w.human(rng) : w.sc.person(pair.first);
    const auto club = rng.chance(20) ? w.flagged_club(rng) : w.sc.club(pair.second);
    const auto other_club = rng.chance(30) ? w.flagged_club(rng) : w.sc.club(w.pairs[rng.below(unsigned(w.pairs.size()))].second);
    const bool due = rng.chance(due_pct);
    d.put(1, job + 0, rng.chance(80) ? 1 : 0);
    d.put(1, job + 1, rng.chance(25) ? 1 : 0);
    d.put(4, job + 4, rng.chance(30) ? 0xffffffffu : w.id_of(w.normal_person(rng)));
    d.put(4, job + 8, w.id_of(person_at));
    d.put(4, job + 0x10, w.id_of(club));
    const auto decide = near_today(w, rng, due ? -1 : 0), reminder = near_today(w, rng, rng.chance(50) ? -1 : 0);
    d.put(2, job + 0x18, decide.day);
    d.put(2, job + 0x1a, decide.year);
    d.put(2, job + 0x20, reminder.day);
    d.put(2, job + 0x22, reminder.year);
    write_person(d, w, rng, person_at, club, other_club, 55, 85, human);
    write_career(d, w, rng, person_at);
    write_club(d, w, rng, club, person_at);
    // The person's row of the table at TOC -0x7b9c (0x64 bytes by id): the staff gate answers
    // non-zero when its club index (+4) is not negative; bits 2-3 of the byte at +0xc are tested
    // by the day. The board gate reads bit 0 of byte 10 * id of the transfer object's table.
    const auto row = w.sc.global(-0x7b9c) + w.id_of(person_at) * 0x64;
    d.put(4, row + 4, rng.chance(55) ? w.id_of(club) : 0xffffffffu);
    d.put(1, row + 0xc, rng.below(16));
    const auto transfers = w.sc.global(-0x7b40);
    d.put(1, w.sc.r32(transfers + 0x9a0) + w.id_of(person_at) * 10, rng.below(2));
    if (rng.chance(60)) write_club(d, w, rng, other_club, person_at);
    // The candidate and the club's current manager, for the late reminder.
    if (rng.chance(30)) d.put(4, club + 0x90, 0);
}

Draft base_daily(const World& w, Rng& rng, const std::string& label) {
    Draft d;
    d.item.entry = daily;
    d.item.registers = {{3, w.object}};
    d.item.label = label;
    // The day's frame: the slot at sp+0xb4 is read as a word before the day writes a byte of it.
    const auto slot = w.sp - 0x170 + 0xb4;
    if (rng.chance(70)) {
        d.put(4, slot, rng.next());
        d.put(4, slot - 4, rng.next());
    }
    return d;
}

// The statics beside the table of contents: 0 = not run yet, 1 = the first month check of a run,
// 2 = the same month as stored, 3 = another month (the clubs' monthly work).
void write_statics(Draft& d, const World& w, Rng& rng, unsigned mode) {
    const std::uint32_t base = std::uint32_t(w.sc.s.r<4>(8));
    d.put(1, base - 0x3940, mode == 0 ? 0 : 1);
    d.put(1, base - 0x393f, mode == 1 ? 1 : 0);
    // The month the callee answers, and the month the day stored last time: equal in mode 2.
    const auto month = rng.below(13);
    d.answer(other::month_of, month);
    d.put(1, base - 0x393e, mode == 2 ? month : mode == 3 ? (month + 1 + rng.below(12)) % 13 : 0);
}

// The monthly loop: a handful of clubs, flagged or not by the national count.
void write_monthly(Draft& d, const World& w, Rng& rng) {
    const auto clubs = 1 + rng.below(6);
    d.put(4, w.club_count_cell, clubs);
    d.put(4, w.national_cell, rng.below(3));
    for (unsigned k = 0; k < clubs; ++k) {
        const auto club = w.sc.club(k);
        const auto region = rng.chance(70) ? w.any(w.regions, rng) : 0;
        d.put(4, club + 0x2c, region);
        if (region && rng.chance(50)) d.put(4, region + 0x74, 0);
    }
}

// The human managers: a post, a flag byte and a date around today's; the club's board row flag.
void write_humans(Draft& d, const World& w, Rng& rng) {
    for (unsigned k = 0; k < 16; ++k) {
        if (rng.chance(30)) continue;
        const auto who = w.sc.person(w.people - 16 + k);
        const auto club = rng.chance(25) ? 0 : (rng.chance(80) ? w.normal_club(rng) : w.flagged_club(rng));
        d.put(4, who + 0x24, club);
        d.put(1, who + 0x6b, rng.pick<std::uint32_t>({0, 1, 3, 0x80, 0xff}));
        put_date(d, who + 0x4c, near_today(w, rng, rng.chance(50) ? -1 : 0), rng.below(3));
        if (club) d.put(1, w.rows + w.id_of(club) * 0x4c + 2, rng.pick<std::uint32_t>({0x80, 0xff, 0, 0x7f, 0x81}));
    }
}
}  // namespace

MANAGER_MANAGER_CASES("daily") {
    if (scenario.match) return;
    const World w(scenario);
    if (!w.clubs || !w.object || w.nodes.empty()) return;
    Rng rng{0x6461696c79ULL};
    // The real day.
    cases.push_back({.entry = daily, .registers = {{3, w.object}}, .label = "daily real"});
    for (unsigned mode = 0; mode < 4; ++mode) {
        auto d = base_daily(w, rng, "daily statics " + std::to_string(mode));
        write_statics(d, w, rng, mode);
        if (mode == 3) write_monthly(d, w, rng);
        cases.push_back(std::move(d.item));
    }
    for (unsigned n = 0; n < 700; ++n) {
        const bool answered = rng.chance(50);
        auto d = base_daily(w, rng, std::string("daily random ") + std::to_string(n) + (answered ? " answered" : " real"));
        const auto mode = rng.chance(60) ? 2u : rng.below(4);
        write_statics(d, w, rng, mode);
        if (mode == 3) write_monthly(d, w, rng);
        if (rng.chance(30)) {
            d.put(2, w.date, rng.pick<std::uint32_t>({0x162, 0x15e, 0x15f, 0, 7, 0xfff9, 0xffff, 0x7fff, 0x8000}));
        }
        sink_answer(d);
        write_humans(d, w, rng);
        // Rewrite the first few offers (the rest of the real list is as the snapshot has it).
        const auto count = 1 + rng.below(unsigned(w.nodes.size()) < 4 ? unsigned(w.nodes.size()) : 4);
        for (unsigned k = 0; k < count; ++k) write_offer(d, w, rng, w.nodes[k].second, 70);
        if (answered) write_answers(d, rng);
        cases.push_back(std::move(d.item));
    }
    // The two status switches, every case in turn (the side status at +0x60 picks it). The first
    // offer is due; its person is not a human manager.
    //  - side posts (0x10196e24): an ordinary club; the person manages club C with both posts there,
    //    so the employer is C, the bad-run test (0x100d3c40) answers 1 and the old post is retired
    //    (0x101b5b20) before the side post is looked at;
    //  - flagged clubs (0x10197014): the offer's club is flagged and the person's side post is it.
    const std::uint32_t statuses[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 0x7f, 0x80, 0xff};
    for (unsigned n = 0; n < 2 * 3 * std::size(statuses); ++n) {
        const bool flagged_offer = n % 2 == 1;
        const auto status = statuses[(n / 2) % std::size(statuses)];
        auto d = base_daily(w, rng, std::string(flagged_offer ? "daily flagged status " : "daily side status ") + std::to_string(status));
        write_statics(d, w, rng, 2);
        sink_answer(d);
        const auto job = w.nodes[0].second;
        write_offer(d, w, rng, job, 100);
        const auto& pair = w.pairs[rng.below(unsigned(w.pairs.size()))];
        const auto person_at = w.sc.person(pair.first);
        d.put(4, job + 8, w.id_of(person_at));
        if (flagged_offer) {
            const auto club = w.flagged_club(rng);
            d.put(4, job + 0x10, w.id_of(club));
            d.put(4, person_at + 0x20, club);
            write_club(d, w, rng, club, person_at);
        } else {
            const auto club = w.normal_club(rng), employer = w.normal_club(rng);
            d.put(4, job + 0x10, w.id_of(club));
            d.put(4, person_at + 0x24, employer);
            d.put(4, person_at + 0x20, employer);
            d.put(4, employer + 0x90, person_at);
            d.answer(0x100d3c40, 1);
            write_club(d, w, rng, club, person_at);
        }
        d.put(1, person_at + 0x60, status);
        d.put(4, person_at + 0x14, rng.chance(60) ? w.any(w.careers, rng) : 0);
        d.put(4, person_at + 0x1c, rng.chance(70) ? w.any(w.staffs, rng) : 0);
        cases.push_back(std::move(d.item));
    }
}

// Forced cases for the outcomes the draws above do not reach. The first offer is due, its person is
// an ordinary one, and the gates before the move are open: the staff gate (the person's row has no
// club), the row's bits (unless `bits`), the board gate (the transfer table's byte).
namespace {
constexpr std::uint32_t make_date = 0x107156f0, retire_old = 0x101b5b20, bad_run = 0x100d3c40, staff_gate = 0x10083110,
                        club_gate = 0x100c96e0;
constexpr std::int32_t people_count_slot = -0x7be4;
struct Forced {
    bool flagged_offer = false;    // the offer's club is flagged: the side post's rules
    std::uint32_t status = 5;      // the status byte of the post the rules look at
    bool career = true;            // the person has a career record
    bool staff_gone = false;       // the staff record goes while the offer's row is stamped
    bool posts_move = false;       // the person's posts change while the old club lets him go
    bool side_elsewhere = false;   // flagged offer: the side post is an ordinary club
    bool no_places = false;        // none of the person's three places
    bool no_region = false;        // the offer's club has no region
    bool full_applicants = false;  // the club's applicant slots are all taken, by records the club's count leaves out
    bool no_side = false;          // he has no side post
    bool club_post_counts = false; // flagged offer: his club post is the employer too, and the club's reputation lets it count
    unsigned flagged_index = ~0u;  // which flagged club (the club gate lets few through)
    bool hire = false;             // the figures that let the club take him as an applicant
    bool count_shrinks = false;    // the people count drops below his id while the row is stamped
    bool staff_gone_early = false; // the staff record goes during the staff gate, right after the first test of it
    bool count_shrinks_early = false;  // the people count drops during the club gate, before the employer is chosen
    unsigned bits = 0;             // bits 2-3 of the row byte
};
Case forced_daily(const World& w, Rng& rng, const Forced& f, const std::string& label) {
    auto d = base_daily(w, rng, label);
    write_statics(d, w, rng, 2);
    sink_answer(d);
    const auto job = w.nodes[0].second;
    write_offer(d, w, rng, job, 100);
    const auto& pair = w.pairs[rng.below(unsigned(w.pairs.size()))];
    const auto person_at = w.sc.person(pair.first);
    const auto club = !f.flagged_offer ? w.normal_club(rng)
                    : f.flagged_index == ~0u ? w.flagged_club(rng) : w.sc.club(w.limit + f.flagged_index % (w.clubs - w.limit));
    auto employer = club;
    for (unsigned tries = 0; employer == club && tries < 50; ++tries)
        employer = f.flagged_offer && !f.side_elsewhere ? w.flagged_club(rng) : w.normal_club(rng);
    d.put(1, job + 0, 1);
    d.put(4, job + 8, w.id_of(person_at));
    d.put(4, job + 0x10, w.id_of(club));
    d.put(2, job + 0x18, (w.today_day - 3) & 0xffff);
    d.put(2, job + 0x1a, w.today_year);
    write_club(d, w, rng, club, person_at);
    d.put(4, club + 0x90, 0);
    // The old employer holds his posts. Where the posts must move, he is also its manager, so the
    // bad-run test (answered) has him let go (0x101b5b20) before the posts are looked at.
    d.put(4, person_at + 0x24, f.flagged_offer && !f.club_post_counts ? 0 : employer);
    if (f.club_post_counts) d.put(2, club + 0x1ec, 0x2000);
    d.put(4, person_at + 0x20, f.no_side ? 0 : employer);
    d.put(4, employer + 0x90, f.posts_move ? person_at : 0);
    d.answer(bad_run, 1);
    d.put(1, person_at + 0x60, f.status);
    d.put(1, person_at + 0x62, f.status);
    d.put(1, person_at + 0x6b, 0);
    const auto career = w.any(w.careers, rng);
    d.put(4, person_at + 0x14, f.career ? career : 0);
    d.put(4, person_at + 0x1c, w.any(w.staffs, rng));
    if (f.no_places) for (const std::uint32_t at : {0x28u, 0x2cu, 0x30u}) d.put(4, person_at + at, 0);
    const auto row = w.sc.global(-0x7b9c) + w.id_of(person_at) * 0x64;
    d.put(4, row + 4, 0xffffffffu);
    d.put(1, row + 0xc, f.bits << 2);
    d.put(1, w.sc.r32(w.sc.global(-0x7b40) + 0x9a0) + w.id_of(person_at) * 10, 0);
    if (f.hire || f.no_region || f.full_applicants) {
        // His career figure beats the club's reputation; the club's region is not the games' own.
        if (f.career) d.put(2, career + 0xa, 0x7000);
        d.put(2, club + 0x1ec, 0);
        const auto region = w.any(w.regions, rng);
        d.put(4, club + 0x2c, f.no_region ? 0 : region);
        if (region) d.put(4, w.scope_cell, w.sc.r32(region + 0x68) + 1);
        // A full list of applicants the club's own count (0x100c9070) leaves out: records whose
        // first word is not below the people count.
        const auto outsider = scratch_base + 0x2000;
        d.put(4, outsider, 0x7fffffff);
        for (unsigned k = 0; k < 50; ++k) d.put(4, club + 0x98 + 4 * k, f.full_applicants ? outsider : k < 3 ? w.normal_person(rng) : 0);
    }
    Case::Poke stamped{make_date, {}}, retired{retire_old, {}};
    if (f.staff_gone) stamped.writes.push_back({4, person_at + 0x1c, 0});
    if (f.count_shrinks) stamped.writes.push_back({4, w.sc.toc(people_count_slot), w.id_of(person_at)});
    if (f.posts_move) {
        retired.writes.push_back({4, person_at + 0x24, f.flagged_offer ? 0 : club});
        if (f.flagged_offer) retired.writes.push_back({4, person_at + 0x20, 0});
    }
    if (!stamped.writes.empty()) d.item.pokes.push_back(std::move(stamped));
    if (f.staff_gone_early) d.item.pokes.push_back({staff_gate, {{4, person_at + 0x1c, 0}}});
    if (f.count_shrinks_early) d.item.pokes.push_back({club_gate, {{4, w.sc.toc(people_count_slot), w.id_of(person_at)}}});
    // With no old employer he then counts as a human manager with none, which the two routines
    // for those do not survive: they are answered, and their arguments compared. The count is back
    // as the first of them is called, for the routines that close the day.
    if (f.count_shrinks_early && f.status == 11) {
        for (const auto entry : {other::human_a, other::human_b}) d.answer(entry, 0);
        d.item.pokes.push_back({other::human_a, {{4, w.sc.toc(people_count_slot), w.sc.r32(w.sc.toc(people_count_slot))}}});
    }
    if (!retired.writes.empty()) d.item.pokes.push_back(std::move(retired));
    return std::move(d.item);
}
}  // namespace

MANAGER_MANAGER_CASES("daily forced") {
    if (scenario.match) return;
    const World w(scenario);
    if (!w.clubs || !w.object || w.nodes.empty() || w.limit >= w.clubs || w.limit < 2) return;  // flagged and ordinary clubs both
    Rng rng{0x666f72636564ULL};
    const std::uint32_t statuses[] = {5, 6, 8, 11, 12, 15, 0, 9, 0x80};  // each switch case, and defaults
    const auto name = [](const Forced& f, const char* what) {
        return std::string("daily forced ") + what + (f.flagged_offer ? " flagged" : " club") + " status " + std::to_string(f.status) +
               (f.career ? "" : " no career") + (f.staff_gone ? " staff gone" : "") + (f.posts_move ? " posts move" : "");
    };
    // Every status of the three switches, with and without the career record, the staff record
    // going, and the posts moving (which is what sends an ordinary club to the side post's rules).
    for (const auto status : statuses)
        for (unsigned mask = 0; mask < 16; ++mask) {
            Forced f{.flagged_offer = (mask & 1) != 0, .status = status, .career = (mask & 2) == 0,
                     .staff_gone = (mask & 4) != 0, .posts_move = (mask & 8) != 0};
            cases.push_back(forced_daily(w, rng, f, name(f, "switch")));
        }
    for (unsigned n = 0; n < 6; ++n) {
        Forced f;
        // The row's bits send away a person without a career record.
        f = {.career = false, .bits = 1 + n % 3};
        cases.push_back(forced_daily(w, rng, f, name(f, "bits")));
        // A flagged club for a person whose side post is an ordinary club: no event is posted.
        f = {.flagged_offer = true, .status = n % 2 ? 6u : 8u, .side_elsewhere = true};
        cases.push_back(forced_daily(w, rng, f, name(f, "side elsewhere")));
        // The event with none of his places.
        f = {.status = 6, .no_places = true};
        cases.push_back(forced_daily(w, rng, f, name(f, "no places")));
        // The applicant rule: taken, no region, no free slot.
        f = {.status = 6, .hire = true};
        cases.push_back(forced_daily(w, rng, f, name(f, "hire")));
        f = {.status = 6, .no_region = true};
        cases.push_back(forced_daily(w, rng, f, name(f, "no region")));
        f = {.status = 6, .full_applicants = true};
        cases.push_back(forced_daily(w, rng, f, name(f, "full applicants")));
        // The people count drops under his id after the first test of it.
        f = {.flagged_offer = n % 2 == 1, .status = 6, .count_shrinks = true};
        cases.push_back(forced_daily(w, rng, f, name(f, "count shrinks")));
        f = {.status = 6, .posts_move = true, .count_shrinks = true};
        cases.push_back(forced_daily(w, rng, f, name(f, "count shrinks")));
        // The same two, earlier: the staff record goes during the staff gate, before the row's bits
        // are looked at; the count drops during the club gate, before the employer is chosen.
        f = {.staff_gone_early = true, .bits = 1 + n % 3};
        cases.push_back(forced_daily(w, rng, f, name(f, "staff gone early")));
        f = {.flagged_offer = n % 2 == 1, .status = 6, .count_shrinks_early = true};
        cases.push_back(forced_daily(w, rng, f, name(f, "count shrinks early")));
        // The posts move and he has no side post: neither post is the old employer's.
        f = {.status = 6, .posts_move = true, .no_side = true};
        cases.push_back(forced_daily(w, rng, f, name(f, "no side")));
    }
    // The flagged clubs in turn, since the club gate (0x100c96e0) lets few of them through: the
    // statuses whose rule needs the career or the staff record, and a side post at an ordinary club.
    for (unsigned n = 0; n < 60; ++n) {
        Forced f;
        // Status 11 is not an employer by the side post: the club post is, for a club of reputation.
        f = {.flagged_offer = true, .status = 11, .career = n % 2 == 0, .flagged_index = n, .club_post_counts = true};
        cases.push_back(forced_daily(w, rng, f, name(f, "flagged turn club post")));
        f = {.flagged_offer = true, .status = 5, .flagged_index = n};
        cases.push_back(forced_daily(w, rng, f, name(f, "flagged turn")));
        f = {.flagged_offer = true, .status = 8, .side_elsewhere = true, .flagged_index = n};
        cases.push_back(forced_daily(w, rng, f, name(f, "flagged turn side elsewhere")));
        // The people count drops to his id during the club gate, and the side post is not an
        // employer (status 11): the rule for his club post finds him outside the count.
        f = {.flagged_offer = true, .status = 11, .flagged_index = n, .count_shrinks_early = true};
        cases.push_back(forced_daily(w, rng, f, name(f, "flagged turn count shrinks early")));
    }
    // The message for a club of modest reputation whose region goes while the bad-run test runs
    // (the helper, directly): the region is read before and after it.
    for (unsigned n = 0; n < 24; ++n) {
        Draft d;
        const auto& pair = w.pairs[rng.below(unsigned(w.pairs.size()))];
        const auto person_at = w.sc.person(pair.first), club = w.normal_club(rng);
        write_person(d, w, rng, person_at, club, w.normal_club(rng), 60, 85, false);
        write_club(d, w, rng, club, person_at);
        // Reported: an open region and a league with an entry in the leagues table; the gate's
        // date is past; an ordinary person.
        const auto region = w.any(w.regions, rng), league = w.any(w.leagues, rng);
        d.put(4, club + 0x2c, region);
        if (region) d.put(1, region + 0x120, 1);
        d.put(4, club + 0x30, league);
        if (league) d.put(4, w.sc.global(leagues_slot) + (w.sc.r32(league) << 2), 1);
        d.put(2, club + 0x1ec, 0x100);
        put_date(d, w.sc.toc(gate_slot), {(w.today_day - 5) & 0xffff, w.today_year}, 0);
        sink_answer(d);
        d.answer(bad_run, 1);
        d.item.pokes.push_back({bad_run, {{4, club + 0x2c, 0}}});
        d.item.entry = announce;
        d.item.registers = {{3, w.object}, {4, person_at}, {5, n % 3 ? w.normal_club(rng) : 0}, {6, club}, {7, std::uint32_t(n % 5)}};
        d.item.label = "announce forced region goes " + std::to_string(n);
        cases.push_back(std::move(d.item));
    }
}

// The helpers: direct calls over the same kinds of records.
MANAGER_MANAGER_CASES("daily helpers") {
    if (scenario.match) return;
    const World w(scenario);
    if (!w.clubs || !w.object || w.nodes.empty()) return;
    Rng rng{0x68656c70657273ULL};
    const auto record_pair = [&](Draft& d, std::uint32_t& person_at, std::uint32_t& club) {
        const bool human = rng.chance(25);
        const auto& pair = w.pairs[rng.below(unsigned(w.pairs.size()))];
        person_at = human ? w.human(rng) : w.sc.person(pair.first);
        club = rng.chance(20) ? w.flagged_club(rng) : w.sc.club(pair.second);
        const auto other_club = rng.chance(50) ? w.flagged_club(rng) : w.normal_club(rng);
        write_person(d, w, rng, person_at, club, other_club, 60, 85, human);
        write_club(d, w, rng, club, person_at);
        write_career(d, w, rng, person_at);
        // Posts at the club itself, so the helpers' first rule runs often.
        if (rng.chance(55)) d.put(4, person_at + 0x24, club);
        if (rng.chance(45)) d.put(4, person_at + 0x20, club);
    };
    for (unsigned n = 0; n < 150; ++n) {
        Draft d;
        std::uint32_t person_at = 0, club = 0;
        record_pair(d, person_at, club);
        d.item.entry = add_applicant;
        d.item.registers = {{3, w.object}, {4, person_at}, {5, club}};
        d.item.label = "add_applicant " + std::to_string(n);
        cases.push_back(std::move(d.item));
    }
    for (const auto entry : {end_post, clear_post}) {
        for (unsigned n = 0; n < 380; ++n) {
            const bool answered = rng.chance(50);
            Draft d;
            std::uint32_t person_at = 0, club = 0;
            record_pair(d, person_at, club);
            // Every status in turn, for both posts.
            d.put(1, person_at + 0x62, n % 19 == 18 ? 0xff : n % 19);
            d.put(1, person_at + 0x60, (n / 3) % 19 == 18 ? 0x80 : (n / 3) % 19);
            if (answered) write_answers(d, rng);
            else sink_answer(d);
            d.item.entry = entry;
            d.item.registers = {{3, w.object}, {4, person_at}, {5, club}};
            if (entry == clear_post) d.item.registers.push_back({31, rng.pick<std::uint32_t>({0, 1, 0x100, 0xff, 0x5a000000u, 0x80})});
            d.item.label = std::string(entry == end_post ? "end_post " : "clear_post ") + std::to_string(n) + (answered ? " answered" : " real");
            cases.push_back(std::move(d.item));
        }
    }
    for (unsigned n = 0; n < 700; ++n) {
        const bool answered = rng.chance(60);
        Draft d;
        std::uint32_t person_at = 0, club = 0;
        record_pair(d, person_at, club);
        const auto employer = rng.chance(20) ? 0 : rng.chance(50) ? w.flagged_club(rng) : w.normal_club(rng);
        // The message gate: the date at TOC -0x7a9c against today's.
        const auto gate = w.sc.toc(gate_slot);
        put_date(d, gate, near_today(w, rng, rng.chance(70) ? -1 : 0), 0);
        const auto kind = rng.pick<std::uint32_t>({0, 1, 2, 3, 4, 5, 5, 6, 0x105, 0xff, 0x80, 7});
        if (answered) write_answers(d, rng);
        else sink_answer(d);
        d.item.entry = announce;
        d.item.registers = {{3, w.object}, {4, rng.chance(4) ? 0 : person_at}, {5, employer}, {6, rng.chance(4) ? 0 : club}, {7, kind}};
        d.item.label = "announce " + std::to_string(n) + (answered ? " answered" : " real");
        cases.push_back(std::move(d.item));
    }
}
}  // namespace cm::aot::manager_manager_checks
