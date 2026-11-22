// PLAYER_REGEN.CPP, block `regen_pending_processor`: cases for 0x102dabb0 (the queue of regenerations, then the spare staff's abilities).
// The captures reach it (season-day: once, same-save-continue: seven times) but always with an empty queue and a pool the routine leaves alone,
// so every case starts from the career's real state (season-day) and edits what the routine reads.
//
// The queue (list B): entries whose id, day (halfword +4) and year (halfword +6) are before, at and after the game's date (every comparison
// outcome, negative days and the extremes of a halfword), the player pool (list D) empty or not, ids outside the people table (-1, the count,
// one above it), several entries (the loop is bounded by the count at the start, the entries shift down at each pop). The persons are real ones
// with the regen marker clear (the newgen routine only clears it: 0x102e4530's own block checks the creation), plus a few with the marker set
// and no club, which run the creation for real (the callee 0x10186ea0 that opens a file is answered).
//
// The pool (list C): the routine picks the best candidate of the nation the first entry names (0x102dbf00). The people table's base cell and
// count are rewritten to a window of real people (as the candidate-score block does), so the pick is one person the case edits: a pure player
// of age 40 with large ratings and the nation's key; the others of the window are under 31 (not candidates). Each case then sets the person's
// player record or none, his club (or none), the club's reputation, the player's halfwords, the position byte, the staff record's two bytes,
// the coach slots and the club's manager word, and answers the routines outside the module (the human-club test 0x1014c2e0, the busy count
// 0x100c9440, the free post 0x100c9bb0, the database routine 0x100c3870) so each outcome of each test is taken, in both orders. The draws are
// placed by the pool's position: scripted (the cursor far from the end, the samples given: the draw below 4 zero or not, the draw below the
// byte +0x63 zero or not) or every draw refilling (the refill sites paired by the call trace). The abilities are swept over the role byte (the
// square against the halfwords and the caps, both sides of every threshold of the three variants) and the three halfwords, with the
// floating-point status cleared in half of the cases (the snapshot's FPSCR has the sticky inexact bit set).
// Labels start with "regen_pending:" (CM_PLAYER_REGEN_BLOCK=regen_pending).
#include "player_regen_checks.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <stdexcept>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t entry_point = 0x102dabb0, build_person = 0x100c3870, club_human = 0x1014c2e0, club_busy = 0x100c9440, club_free = 0x100c9bb0,
                        forget_history = 0x10186ea0, retire_check = 0x1014b690;
constexpr std::int32_t regen_slot = -0x7b28, date_slot = -0x7b80, people_slot = -0x7be0, count_slot = -0x7be4, staff_slot = -0x79f8, kind_slot = -0x7b9c,
                       nations_slot = -0x7b98, players_slot = -0x7b38, cursor_slot = -0x7b84, end_slot = -0x7b88, salt_slot = -0x7b90,
                       start_slot = -0x7b8c, nation_count_slot = -0x7b04;

std::string number(std::int64_t n) { return std::to_string(n); }
bool full() { return std::getenv("CM_REGEN_PENDING_FULL") != nullptr; }

struct Ring {
    std::uint32_t cursor_cell, end_cell, salt_cell, start, end;
    explicit Ring(const Scenario& s)
        : cursor_cell(s.toc(cursor_slot)), end_cell(s.toc(end_slot)), salt_cell(s.toc(salt_slot)), start(s.toc(start_slot)), end(s.r32(end_cell)) {}
    // Every draw refills: the pool's end is its start; the sample and salt are the generator's seed for each draw.
    std::vector<Write> every_draw(std::uint32_t sample, std::uint32_t salt = 0) const {
        return {w32(end_cell, start), w32(cursor_cell, start), w32(salt_cell, salt), w16(start, sample)};
    }
    // The cursor `before` draws short of the end; draw n reads samples[n] (the draw `before` refills and reads the pool's start).
    std::vector<Write> scripted(std::uint32_t before, std::uint32_t salt, const std::vector<std::uint32_t>& samples) const {
        const auto position = end - 2 * before;
        std::vector<Write> memory{w32(cursor_cell, position), w32(salt_cell, salt)};
        for (std::size_t n = 0; n < samples.size(); ++n)
            memory.push_back(w16(n < before ? position + 2 * std::uint32_t(n + 1) : start + 2 * std::uint32_t(n - before), samples[n]));
        return memory;
    }
};

struct World {
    const Scenario& s;
    std::uint32_t regen, date, people_cell, count_cell, staff, kinds, nations, players, queue, pool_c, pool_d;
    std::int32_t day, nation_count;
    std::uint32_t year;
    std::vector<std::uint32_t> attached, free_players, with_nation, unmarked;
    explicit World(const Scenario& scenario)
        : s(scenario), regen(scenario.global(regen_slot)), date(scenario.toc(date_slot)), people_cell(scenario.toc(people_slot)), count_cell(scenario.toc(count_slot)),
          staff(scenario.global(staff_slot)), kinds(scenario.global(kind_slot)), nations(scenario.global(nations_slot)), players(scenario.global(players_slot)),
          queue(0), pool_c(0), pool_d(0), day(0), nation_count(std::int32_t(scenario.global(nation_count_slot))), year(0) {
        if (regen == 0) throw std::runtime_error("regen_pending: the career has no regen object");
        queue = s.r32(regen + 0x10); pool_c = s.r32(regen + 0x1c); pool_d = s.r32(regen + 0x3c);
        if (queue == 0 || pool_c == 0 || pool_d == 0) throw std::runtime_error("regen_pending: the regen object lacks a list block");
        day = std::int16_t(s.u16(date)); year = s.u16(date + 2);
        for (std::uint32_t n = 0; n + 16 < s.people(); ++n) {
            const auto p = s.person(n);
            if (s.r32(p + 0x1c) == 0 && s.r32(p + 0x14) != 0 && s.r32(p + 0x24) != 0) attached.push_back(n);
            if (s.r32(p + 0x1c) == 0 && s.r32(p + 0x14) != 0 && s.r32(p + 0x24) == 0) free_players.push_back(n);
            if (s.r32(p + 0xc) != 0) with_nation.push_back(n);
            if (((s.u8(kinds + s.r32(p) * 100u + 0xc) >> 2) & 3) == 0) unmarked.push_back(n);
        }
        if (attached.size() < 100 || with_nation.size() < 100 || unmarked.size() < 100) throw std::runtime_error("regen_pending: the career has too few people");
        std::fprintf(stderr, "regen_pending: regen %x queue %x pool C %x D %x, date day %d year %u, %zu attached players, %zu unmarked\n", regen, queue,
                     pool_c, pool_d, day, year, attached.size(), unmarked.size());
    }
    std::uint32_t nation(std::uint32_t index) const { return nations + (index % std::uint32_t(nation_count)) * 0x128; }
    std::uint32_t key_of(std::uint32_t nation_record) const { return s.r32(nation_record + 0x68); }
    std::uint32_t row(std::uint32_t person) const { return kinds + s.r32(person) * 100u + 0xc; }
};

// ---- the queue ----------------------------------------------------------------------------------------------------------------------
struct Due { std::int32_t id; std::int32_t day; std::uint32_t year; };
std::vector<Write> queue_writes(const World& w, const std::vector<Due>& entries, std::uint32_t d_count) {
    std::vector<Write> m{w32(w.regen + 0x14, std::uint32_t(entries.size())), w32(w.regen + 0x40, d_count)};
    for (std::size_t n = 0; n < entries.size(); ++n) {
        const auto at = w.queue + std::uint32_t(n) * 0xc;
        append(m, {w32(at, std::uint32_t(entries[n].id)), w16(at + 4, std::uint32_t(entries[n].day)), w16(at + 6, entries[n].year), w8(at + 8, 0x40 + std::uint32_t(n))});
    }
    return m;
}
Case queue_case(const World& w, const std::string& label, const std::vector<Due>& entries, std::uint32_t d_count, std::vector<Write> more = {}) {
    auto m = queue_writes(w, entries, d_count);
    append(m, more);
    return {.entry = entry_point, .registers = {{3, w.regen}}, .memory = std::move(m), .label = "regen_pending: queue, " + label};
}
Due due_at(const World& w, std::int32_t id, int day_offset = -1, int year_offset = 0) {
    return {id, w.day + day_offset, std::uint32_t(std::int32_t(w.year) + year_offset)};
}

// ---- the pool -----------------------------------------------------------------------------------------------------------------------
struct Pick {
    bool player = true;
    bool attached = true;
    std::uint32_t club_index = 3;
    std::int32_t value_a = 5000, value_8 = 4000, value_c = 3000, reputation = 8000;
    std::uint32_t position = 0, role = 100;
    std::int32_t rating63 = 0;   // the signed byte +0x63
    std::int32_t cube_byte = -1; // the player record's byte +0x2f (the candidate score's cubed term), when not -1
    std::uint32_t b30 = 5, b31 = 5;
    std::uint32_t slots = 0;     // bit n: coach slot n filled
    std::uint32_t manager = 0;   // the club's +0x94
    std::uint32_t person_job = 0;
    // The answers of the routines outside the module (the pick draws nothing of them).
    std::uint32_t human = 0, busy = 0, free_post = 0;
    bool real_build = false;
    // Draw outcomes (scripted): the draw below 4 (swap) is zero when `swap_zero`, the draw below the byte +0x63 is a multiple of it when `rating_zero`.
    bool swap_zero = false, rating_zero = false;
    std::uint32_t after = 17;    // the sample of a draw below 50 / 0x5dc / 500 (and every other draw of no consequence)
};

// The number of draws the candidate's score takes for the pick (age 40: a draw below 1350; a draw below the cube for a player record with a nonzero
// halfpotential; one for an unattached person; the last below the score).
std::uint32_t candidate_draws(const World& w, std::uint32_t person, const Pick& pick) {
    std::uint32_t draws = 1;  // the age term
    if (pick.player) {
        const auto record = w.s.r32(person + 0x14);
        const auto half = std::int8_t(w.s.u8(record + 0x2f)) / 2;
        if (half != 0) ++draws;
    }
    if (!pick.attached) ++draws;
    return draws + 1;  // the last draw
}

struct Setup {
    std::uint32_t start_index = 0;       // the window's first person (the pick)
    std::vector<Pick> picks{Pick{}};     // the pick of each iteration (the window's first persons, one nation each)
    std::uint32_t pool_count = 3;
    bool no_candidate = false;
};

// The writes that make person `index` of the window the best candidate of nation `nation` with the pick's data, and the staff record the pool's
// entry `first` names.
std::vector<Write> make_pick(const World& w, std::uint32_t person, std::uint32_t nation_record, std::uint32_t first, const Pick& p, const std::vector<std::uint32_t>& donors, std::size_t which) {
    std::vector<Write> m;
    m.push_back(w8(w.row(person), w.s.u8(w.row(person)) & ~0x0cu));
    append(m, {w32(person + 0x1c, 0), w32(person + 0xc, nation_record), w8(person + 0x5d, 40), w8(person + 0x5e, p.role), w8(person + 0x63, std::uint32_t(p.rating63)),
               w8(person + 0x69, 100), w8(person + 0x66, 100), w8(person + 0x62, p.person_job)});
    const auto club = p.attached ? w.s.club(p.club_index + std::uint32_t(which)) : 0u;
    m.push_back(w32(person + 0x24, club));
    if (p.attached) {
        append(m, {w16(club + 0x1ec, std::uint32_t(p.reputation)), w32(club + 0x94, p.manager)});
        for (std::uint32_t slot = 0; slot < 5; ++slot) m.push_back(w32(club + 0x160 + 4 * slot, (p.slots >> slot & 1) ? 0x60000000 + slot : 0));
    }
    if (!p.player) {
        m.push_back(w32(person + 0x14, 0));
    } else {
        // A real player record (the person's own when he has one, else a donor's).
        auto record = w.s.r32(person + 0x14);
        if (record == 0) {
            record = w.s.r32(w.s.person(donors[which % donors.size()]) + 0x14);
            m.push_back(w32(person + 0x14, record));
        }
        append(m, {w8(record + 0xf, p.position), w16(record + 0xa, std::uint32_t(p.value_a)), w16(record + 8, std::uint32_t(p.value_8)),
                   w16(record + 0xc, std::uint32_t(p.value_c))});
        if (p.cube_byte >= 0) m.push_back(w8(record + 0x2f, std::uint32_t(p.cube_byte)));
    }
    const auto staff_record = w.staff + first * 0x44;
    append(m, {w8(staff_record + 0x30, p.b30), w8(staff_record + 0x31, p.b31)});
    return m;
}

Case pool_case(const World& w, [[maybe_unused]] const Ring& ring, const std::string& label, const Setup& setup, std::vector<Write> pool_writes, std::vector<Write> more = {},
               bool clear_fpscr = false) {
    std::vector<Write> m{w32(w.regen + 0x14, 0), w32(w.regen + 0x20, setup.pool_count)};
    const auto windows = std::max<std::size_t>(setup.picks.size(), 1);
    // The window: the people table starts at the pick, `windows` persons plus sixteen.
    const auto first_person = w.s.person(setup.start_index);
    append(m, {w32(w.people_cell, first_person), w32(w.count_cell, std::uint32_t(windows + 16 + 2))});
    // Donors of player records: the real attached players.
    std::vector<std::uint32_t> donors(w.attached.begin(), w.attached.begin() + 8);
    // The window's persons: each pick, then the fillers (under 31).
    for (std::uint32_t n = 0; n < windows + 2; ++n) {
        const auto person = first_person + n * 0x70;
        if (setup.no_candidate || n >= setup.picks.size()) {
            append(m, {w8(person + 0x5d, 20), w32(person + 0x1c, 0)});
            continue;
        }
        const auto nation_record = w.nation(100 + 13 * n + setup.start_index);
        const auto first = 50 + 11 * n;
        append(m, make_pick(w, person, nation_record, first, setup.picks[n], donors, n));
        // The pool entry of this pick: its staff record index and the nation's key.
        const auto at = w.pool_c + n * 0x10;
        append(m, {w32(at, first), w32(at + 4, w.key_of(nation_record)), w32(at + 8, 0), w32(at + 12, n)});
    }
    // Entries beyond the picks (the count is larger): staff indexes, keys of a nation no window person has.
    for (std::uint32_t n = std::uint32_t(setup.picks.size()); n < setup.pool_count; ++n) {
        const auto at = w.pool_c + n * 0x10;
        append(m, {w32(at, 400 + n), w32(at + 4, 0x7e7e7e00 + n), w32(at + 8, 0), w32(at + 12, n)});
    }
    if (setup.no_candidate && setup.pool_count > 0) append(m, {w32(w.pool_c + 4, 0x7e7e7e00)});
    append(m, pool_writes);
    append(m, more);
    Case item{.entry = entry_point, .registers = {{3, w.regen}}, .memory = std::move(m), .label = "regen_pending: pool, " + label};
    const auto& pick = setup.picks.empty() ? Pick{} : setup.picks[0];
    item.answers = {{club_human, pick.human}, {club_busy, pick.busy}, {club_free, pick.free_post}};
    if (!pick.real_build) item.answers.push_back({build_person, 0});
    item.clear_fpscr = clear_fpscr;
    item.pokes = {{build_person, {}}};  // every player's case calls it: the poke makes the checker trace the case
    return item;
}

// The samples the case's draws read, after the candidate's: the swap draw below 4 (and the one below 50 when it is zero), the draw below the byte.
std::vector<std::uint32_t> script_for(const World& w, const Pick& pick, std::uint32_t person) {
    std::vector<std::uint32_t> samples;
    for (std::uint32_t n = 0, count = candidate_draws(w, person, pick); n < count; ++n) samples.push_back(1);
    if (pick.player && pick.position == 0x14 && std::int8_t(pick.b30) > std::int8_t(pick.b31)) {
        samples.push_back(pick.swap_zero ? 4 : 5);
        if (pick.swap_zero) samples.push_back(pick.after);
    }
    if (pick.player && pick.value_a <= 0x1d4c && std::int8_t(pick.rating63) != 0) {
        const std::int32_t bound = std::int8_t(pick.rating63);
        samples.push_back(pick.rating_zero ? std::uint32_t(bound < 0 ? -bound : bound) * 3 : 7);
    }
    if (!pick.player) { samples.push_back(pick.after); samples.push_back(pick.after + 1); samples.push_back(pick.after + 2); }
    return samples;
}
}

// The first cases: every draw refilling (a traced case pairs the refill sites of its whole path: the candidate's draws, then the routine's), over
// the person kinds. The routine's own sites: the swap draw 0x102dadc0, the draw below 50 0x102dae0c, the draw below the byte +0x63 0x102db400 and
// the three draws of a person with no player record 0x102dbd48, 0x102dbdac, 0x102dbe10.
PLAYER_REGEN_CASES("regen_pending") {
    const World w(scenario);
    const Ring ring(scenario);
    for (std::uint32_t variant = 0; variant < 24; ++variant) {
        Setup setup;
        setup.start_index = w.attached[(variant * 37) % w.attached.size()];
        Pick pick;
        pick.player = variant % 3 != 0;
        pick.position = variant % 2 ? 0x14 : 0;
        pick.b30 = 9; pick.b31 = 2;
        pick.rating63 = 20 + std::int32_t(variant);
        pick.value_a = 3000;
        pick.after = 3 + variant;
        setup.picks = {pick};
        cases.push_back(pool_case(w, ring, "every draw refills " + number(variant), setup, ring.every_draw((0x1111u + 977u * variant) & 0xffff, variant * 19)));
    }
    // A pick whose draws refill at each position in turn (the draw after `before` samples), for the three kinds.
    for (std::uint32_t kind = 0; kind < 3; ++kind)
        for (std::uint32_t before = 0; before < 9; ++before) {
            Setup setup;
            setup.start_index = w.attached[(kind * 101 + before * 7) % w.attached.size()];
            Pick pick;
            pick.player = kind != 0;
            pick.position = kind == 2 ? 0x14 : 0;
            pick.b30 = 9; pick.b31 = 2;
            pick.rating63 = 30;
            pick.value_a = 3000;
            setup.picks = {pick};
            const auto person = scenario.person(setup.start_index);
            const auto base = candidate_draws(w, person, pick);
            auto script = script_for(w, pick, person);
            for (auto& sample : script) sample = 1 + sample;
            cases.push_back(pool_case(w, ring, "refill at draw " + number(base + before) + " kind " + number(kind), setup, ring.scripted(base + before, 0, script)));
        }
}

PLAYER_REGEN_CASES("regen_pending") {
    // The queue: every comparison of the date, the pool of players empty or not, ids outside the table, several entries. Real people with the
    // regen marker clear (the newgen routine clears the marker and returns).
    const World w(scenario);
    const auto id = [&](std::size_t n) { return std::int32_t(w.unmarked[(n * 97 + 5) % w.unmarked.size()] ); };
    cases.push_back(queue_case(w, "no entries", {}, 1));
    cases.push_back(queue_case(w, "one entry due, pool of players empty", {due_at(w, id(0))}, 0));
    cases.push_back(queue_case(w, "one entry due, year before", {due_at(w, id(1), 5, -1)}, 1));
    cases.push_back(queue_case(w, "one entry not due, year after", {due_at(w, id(2), -5, 1)}, 1));
    cases.push_back(queue_case(w, "one entry due, same year, earlier day", {due_at(w, id(3), -1, 0)}, 1));
    cases.push_back(queue_case(w, "one entry not due, same day", {due_at(w, id(4), 0, 0)}, 1));
    cases.push_back(queue_case(w, "one entry not due, same year, later day", {due_at(w, id(5), 1, 0)}, 1));
    for (const std::int32_t today_edit : {0, 1}) {
        // The game's date moved to the halfword extremes: the day compare is signed.
        for (const auto& [today, entry_day] : std::vector<std::pair<std::int32_t, std::int32_t>>{{-1, -2}, {-2, -1}, {-1, -1}, {0, -1}, {-1, 0}, {32767, -32768}, {-32768, 32767},
                                                                                                    {32767, 32766}, {32766, 32767}, {-32768, -32768}, {0x7fff, 0x7fff}, {100, 99}, {99, 100}}) {
            std::vector<Write> more{w16(w.date, std::uint32_t(today)), w16(w.date + 2, 2001 + std::uint32_t(today_edit))};
            cases.push_back(queue_case(w, "day " + number(today) + " against " + number(entry_day) + " year " + number(2001 + today_edit),
                                       {{id(6), entry_day, 2001 + std::uint32_t(today_edit)}}, 1, more));
        }
    }
    // The year compare is unsigned: years above 0x8000 and the extremes.
    for (const auto& [today_year, entry_year] : std::vector<std::pair<std::uint32_t, std::uint32_t>>{{0x8000, 0x7fff}, {0x7fff, 0x8000}, {0xffff, 0}, {0, 0xffff}, {0xffff, 0xfffe}, {1, 0}, {0, 0}}) {
        std::vector<Write> more{w16(w.date + 2, today_year)};
        cases.push_back(queue_case(w, "year " + number(today_year) + " against " + number(entry_year), {{id(7), w.day - 1, entry_year}}, 1, more));
    }
    // Several entries: the first due and the second not, all due, the pool of players, ids at the table's ends.
    cases.push_back(queue_case(w, "two entries, the first due", {due_at(w, id(8)), due_at(w, id(9), 5, 0)}, 1));
    cases.push_back(queue_case(w, "two entries, both due", {due_at(w, id(10)), due_at(w, id(11), -3, -1)}, 1));
    cases.push_back(queue_case(w, "two entries, the first not due, the second due", {due_at(w, id(12), 4, 0), due_at(w, id(13))}, 1));
    cases.push_back(queue_case(w, "five entries all due", {due_at(w, id(14)), due_at(w, id(15)), due_at(w, id(16)), due_at(w, id(17)), due_at(w, id(18))}, 1));
    cases.push_back(queue_case(w, "five entries, the third not due", {due_at(w, id(19)), due_at(w, id(20)), due_at(w, id(21), 9, 0), due_at(w, id(22)), due_at(w, id(23))}, 1));
    // The same person twice (the second pass finds the marker clear), the first and the last real people, a person of a club.
    cases.push_back(queue_case(w, "the same person twice", {due_at(w, id(24)), due_at(w, id(24))}, 1));
    cases.push_back(queue_case(w, "person 0 and the last person", {due_at(w, 0), due_at(w, std::int32_t(scenario.people()) - 1)}, 1));
    cases.push_back(queue_case(w, "a person with a club (marker set)", {due_at(w, std::int32_t(w.attached[3]))}, 1,
                               {w8(w.row(scenario.person(w.attached[3])), 2 << 2)}));
    // The id test: below 0, the count itself (accepted), one above it, the largest.
    const auto count = std::int32_t(scenario.people());
    for (const std::int32_t bad : {-1, -2, count + 1, count + 2, 0x7fffffff, std::int32_t(0x80000000u)}) {
        cases.push_back(queue_case(w, "id " + number(bad) + " (assertion)", {due_at(w, bad)}, 1));
        cases.back().pokes = {{0x1077b720, {}}};  // traced: the report's calls are paired
    }
    cases.push_back(queue_case(w, "id -1 after a due entry", {due_at(w, id(25)), due_at(w, -1)}, 1));
    cases.back().pokes = {{0x1077b720, {}}};
    cases.push_back(queue_case(w, "id above the count after a due entry", {due_at(w, id(26)), due_at(w, count + 1)}, 1));
    cases.back().pokes = {{0x1077b720, {}}};
    cases.push_back(queue_case(w, "id = the count", {due_at(w, count)}, 1));
    cases.push_back(queue_case(w, "id = the count minus one", {due_at(w, count - 1)}, 1));
    // The pool of players empty: nothing after the first due entry's test.
    cases.push_back(queue_case(w, "two due entries, the pool of players empty", {due_at(w, id(27)), due_at(w, id(28))}, 0));
}

PLAYER_REGEN_CASES("regen_pending") {
    // The queue's creation for real: a person with the marker set and no club (the newgen routine's own checks cover its paths); the plan
    // (list D's first entry: a player record slot and two nations) names a real record. Both sides run the creation and the pop.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(77);
    std::vector<std::uint32_t> candidates;
    for (const auto n : w.free_players) candidates.push_back(n);
    if (candidates.size() < 12) candidates.assign(w.attached.begin(), w.attached.begin() + 12);
    for (std::uint32_t n = 0; n < 8; ++n) {
        const auto person_index = candidates[(n * 13 + 1) % candidates.size()];
        const auto person = scenario.person(person_index);
        const auto record = scenario.r32(person + 0x14);
        if (record < w.players || (record - w.players) % 0x48 != 0) continue;
        const auto slot = (record - w.players) / 0x48;
        std::vector<Write> more{w8(w.row(person), 1 << 2), w32(person + 0x24, 0), w32(w.pool_d, slot), w32(w.pool_d + 4, 20 + n), w32(w.pool_d + 8, 30 + n)};
        auto item = queue_case(w, "creation for real " + number(n), {due_at(w, std::int32_t(person_index))}, 1, more);
        append(item.memory, ring.scripted(n * 5, random() & 0xff, [&] { std::vector<std::uint32_t> samples; for (unsigned k = 0; k < 130; ++k) samples.push_back(random() & 0xffff); return samples; }()));
        item.answers = {{forget_history, 0}};
        item.pokes = {{retire_check, {}}};
        cases.push_back(std::move(item));
    }
    // Two creations and the loop's bound.
    {
        const auto first = scenario.person(candidates[2]), second = scenario.person(candidates[5]);
        const auto record = scenario.r32(first + 0x14);
        if (record >= w.players && (record - w.players) % 0x48 == 0) {
            std::vector<Write> more{w8(w.row(first), 1 << 2), w32(first + 0x24, 0), w8(w.row(second), 2 << 2), w32(second + 0x24, 0), w32(w.pool_d, (record - w.players) / 0x48),
                                    w32(w.pool_d + 4, 21), w32(w.pool_d + 8, 31)};
            auto item = queue_case(w, "two creations", {due_at(w, std::int32_t(candidates[2])), due_at(w, std::int32_t(candidates[5]))}, 1, more);
            item.answers = {{forget_history, 0}};
            item.pokes = {{retire_check, {}}};
            append(item.memory, ring.every_draw(0x2222, 3));
            cases.push_back(std::move(item));
        }
    }
}

PLAYER_REGEN_CASES("regen_pending") {
    // The pool: the exits (a count of 0, 1, 2; three with no candidate in the window), the pick of one, two and three iterations (the loop is bounded
    // by the count less 2), the pick missing at the second iteration.
    const World w(scenario);
    const Ring ring(scenario);
    for (const std::uint32_t count : {0u, 1u, 2u}) {
        Setup setup;
        setup.start_index = w.attached[11];
        setup.pool_count = count;
        cases.push_back(pool_case(w, ring, "count " + number(count) + " (nothing to do)", setup, ring.scripted(100, 0, {1, 2, 3})));
    }
    {
        Setup setup;
        setup.start_index = w.attached[12];
        setup.no_candidate = true;
        setup.picks = {Pick{}};
        cases.push_back(pool_case(w, ring, "no candidate in the window", setup, ring.scripted(100, 0, {1, 2, 3})));
    }
    for (std::uint32_t count = 3; count <= 6; ++count)
        for (std::uint32_t picks = 1; picks <= 4; ++picks) {
            Setup setup;
            setup.start_index = w.attached[20 + count * 5 + picks];
            setup.pool_count = count;
            setup.picks.clear();
            for (std::uint32_t n = 0; n < picks; ++n) {
                Pick pick;
                pick.player = (n + count) % 3 != 0;
                pick.position = n % 2 ? 0x14 : 0;
                pick.b30 = 9; pick.b31 = 2;
                pick.rating63 = 10 + std::int32_t(n);
                pick.value_a = (n == 1) ? 9000 : 3000;
                pick.real_build = false;
                setup.picks.push_back(pick);
            }
            auto m = ring.scripted(500, 0, std::vector<std::uint32_t>(120, 7));
            cases.push_back(pool_case(w, ring, "count " + number(count) + ", " + number(picks) + " pick(s)", setup, m));
        }
    // The queue and the pool in one run (the date's compare then the window's pick): the loop's registers start again.
    for (std::uint32_t variant = 0; variant < 3; ++variant) {
        Setup setup;
        setup.start_index = w.attached[300 + variant];
        Pick pick;
        pick.player = variant != 0;
        pick.rating63 = 5;
        setup.picks = {pick};
        auto item = pool_case(w, ring, "after a queue of " + number(variant + 1), setup, ring.scripted(500, 0, std::vector<std::uint32_t>(60, 9)));
        // Person 0 of the window is the pick: give the queue ids of the window's fillers (their markers are clear).
        std::vector<Write> more;
        for (std::uint32_t n = 0; n <= variant; ++n) {
            const auto at = w.queue + n * 0xc;
            append(more, {w32(at, 1 + n), w16(at + 4, std::uint32_t(w.day - 1)), w16(at + 6, w.year), w8(at + 8, 0x40)});
            append(more, {w8(w.row(scenario.person(setup.start_index + 1 + n)), 0)});
        }
        append(more, {w32(w.regen + 0x14, variant + 1), w32(w.regen + 0x40, 1)});
        append(item.memory, more);
        cases.push_back(std::move(item));
    }
}

PLAYER_REGEN_CASES("regen_pending") {
    // A person with no player record: three draws below 1500 / 1500 / 500, each plus 1, at the pool's positions; and one with a record.
    const World w(scenario);
    const Ring ring(scenario);
    for (std::uint32_t before = 0; before < 8; ++before) {
        Setup setup;
        setup.start_index = w.attached[40 + before];
        Pick pick;
        pick.player = false;
        pick.after = 100 + 400 * before;
        setup.picks = {pick};
        const auto person = scenario.person(setup.start_index);
        const auto base = candidate_draws(w, person, pick);
        cases.push_back(pool_case(w, ring, "no player record, refill at " + number(before), setup, ring.scripted(base + before, 0, script_for(w, pick, person))));
    }
    for (const std::uint32_t sample : {0u, 1u, 0x5dbu, 0x5dcu, 0x5ddu, 0x1f3u, 0x1f4u, 0x1f5u, 0xffffu, 0x8000u}) {
        Setup setup;
        setup.start_index = w.attached[60];
        Pick pick;
        pick.player = false;
        pick.after = sample;
        setup.picks = {pick};
        const auto person = scenario.person(setup.start_index);
        auto script = script_for(w, pick, person);
        cases.push_back(pool_case(w, ring, "no player record, samples " + hex(sample), setup, ring.scripted(500, 0, script)));
        // With a salt: negative sums reach the remainders with their sign.
        cases.push_back(pool_case(w, ring, "no player record, samples " + hex(sample) + " salt -40000", setup, ring.scripted(500, 0xffff6000u, script)));
    }
    // A person with no player record and no club cannot be scored (the score reads the null record's halfword: 0x0c): run with the null page, where
    // the generator reads it; here the person has a club.
}

PLAYER_REGEN_CASES("regen_pending") {
    // The swap and its draws: the position byte 20 or not, the staff record's bytes +0x30 and +0x31 (signed: more, equal, less, the signs), the draw
    // below 4 zero or not, the draw below 50, each refilling or not.
    const World w(scenario);
    const Ring ring(scenario);
    std::uint32_t n = 0;
    const std::array<std::pair<std::uint32_t, std::uint32_t>, 10> pairs{{{9, 2}, {2, 9}, {5, 5}, {0x80, 0x7f}, {0x7f, 0x80}, {0xff, 0x01}, {0x01, 0xff}, {0x00, 0xff}, {0xff, 0x00}, {0x7f, 0x7e}}};
    for (const auto position : {0x14u, 0x13u, 0x15u, 0u, 0x94u, 0x114u & 0xffu}) {
        for (const auto& [low, high] : pairs) {
            for (const bool zero : {false, true}) {
                Setup setup;
                setup.start_index = w.attached[80 + (++n % 400)];
                Pick pick;
                pick.position = position;
                pick.b30 = low; pick.b31 = high;
                pick.swap_zero = zero;
                pick.rating63 = 0;
                pick.value_a = 3000;
                pick.after = 20 + n;
                setup.picks = {pick};
                const auto person = scenario.person(setup.start_index);
                cases.push_back(pool_case(w, ring, "swap position " + hex(position) + " bytes " + hex(low) + "/" + hex(high) + (zero ? " zero" : " nonzero"), setup,
                                          ring.scripted(500, 0, script_for(w, pick, person))));
            }
        }
    }
    // The swap's draws with a sum divisible by 4 and others, negative sums, the sample of the draw below 50 over its range.
    for (const std::uint32_t sample : {0u, 1u, 2u, 3u, 4u, 5u, 8u, 0xfffcu, 0xfffdu, 0x8000u, 0x7fffu}) {
        for (const std::uint32_t salt : {0u, 0xffff0000u, 0xffff8001u, 3u, 0x7fffu}) {
            Setup setup;
            setup.start_index = w.attached[500 + n++ % 100];
            Pick pick;
            pick.position = 0x14;
            pick.b30 = 9; pick.b31 = 2;
            pick.rating63 = 0;
            pick.value_a = 3000;
            setup.picks = {pick};
            const auto person = scenario.person(setup.start_index);
            auto script = script_for(w, pick, person);
            const auto draws = candidate_draws(w, person, pick);
            // The swap draw's sample is the case's; the draw below 50 reads the next.
            script.resize(draws);
            script.push_back(sample); script.push_back(sample ^ 0x35);
            cases.push_back(pool_case(w, ring, "swap draw sample " + hex(sample) + " salt " + hex(salt), setup, ring.scripted(500, salt, script)));
        }
    }
    // Refills at the swap's draws.
    for (std::uint32_t refill = 0; refill < 3; ++refill)
        for (std::uint32_t variant = 0; variant < 8; ++variant) {
            Setup setup;
            setup.start_index = w.attached[700 + refill * 8 + variant];
            Pick pick;
            pick.position = 0x14;
            pick.b30 = 9; pick.b31 = 2;
            pick.rating63 = variant % 2 ? 40 : 0;
            pick.value_a = 3000;
            setup.picks = {pick};
            const auto person = scenario.person(setup.start_index);
            const auto draws = candidate_draws(w, person, pick);
            std::vector<std::uint32_t> script(draws, 1);
            for (unsigned k = 0; k < 6; ++k) script.push_back(4 * variant + k);
            cases.push_back(pool_case(w, ring, "swap draws refill at " + number(refill) + " variant " + number(variant), setup,
                                      ring.scripted(draws + refill, variant * 777, script)));
        }
}

PLAYER_REGEN_CASES("regen_pending") {
    // The club: none, a human's (the answer's low byte zero is false), the reputation against the player's halfword +0xa at 0.5 and 0.75 (both
    // sides of each, negative reputations), the busy count below, at and above 5, the coach slots (all empty, some filled, all filled), the
    // post taken (job byte 0xf) or not, the free-post answer, and the manager word.
    const World w(scenario);
    const Ring ring(scenario);
    std::uint32_t n = 0;
    const auto run = [&](const std::string& label, Pick pick, bool real = false) {
        Setup setup;
        setup.start_index = w.attached[900 + (++n % 300)];
        pick.real_build = real;
        setup.picks = {pick};
        const auto person = scenario.person(setup.start_index);
        auto item = pool_case(w, ring, label, setup, ring.scripted(500, 0, script_for(w, pick, person)));
        cases.push_back(std::move(item));
    };
    Pick base;
    base.value_a = 3000;
    base.value_8 = 3000;
    base.value_c = 3000;
    {
        Pick pick = base;
        pick.attached = false;
        run("club: none (unattached)", pick);
    }
    for (const std::uint32_t human : {0u, 1u, 0x100u, 0x80u, 0xffu, 0x1ff00u}) {
        Pick pick = base;
        pick.human = human;
        pick.reputation = 20000;
        run("club: human answer " + hex(human), pick);
    }
    // Value against reputation x 0.5 (first test) and x 0.75 (second): value = x - 1, x, x + 1.
    for (const std::int32_t reputation : {8000, 20000, 1000, 0, 1, -1000, 32767, -32768, 3, 4, 7}) {
        for (const double factor : {0.5, 0.75}) {
            for (const int delta : {-1, 0, 1}) {
                for (const std::uint32_t busy : {0u, 4u}) {
                    Pick pick = base;
                    pick.reputation = reputation;
                    const auto threshold = std::int32_t(factor * reputation);
                    pick.value_a = std::clamp(threshold + delta, -32768, 32767);
                    pick.busy = busy;
                    pick.free_post = 0;
                    run("club: reputation " + number(reputation) + " factor " + number(int(factor * 100)) + " delta " + number(delta) + " busy " + number(busy), pick);
                }
            }
        }
    }
    // The busy count and the coach slots / manager post.
    for (const std::uint32_t busy : {0u, 3u, 4u, 5u, 6u, 0x105u, 0x80u, 0xffu, 0x104u, 0x7fu}) {
        for (const std::uint32_t slots : {0u, 1u, 0x10u, 0x1fu, 0x15u, 0xau}) {
            for (const std::uint32_t free_post : {0u, 1u}) {
                Pick pick = base;
                pick.reputation = 20000;
                pick.value_a = 3000;      // below half the reputation: the coach arm when the busy count is under 5
                pick.busy = busy;
                pick.slots = slots;
                pick.free_post = free_post;
                run("club: coach arm busy " + hex(busy) + " slots " + hex(slots) + " free " + hex(free_post), pick);
            }
        }
    }
    // A person who already has the job byte 0xf: the post-taken test (`lbz` against 0xf) when no slot is empty.
    for (const std::uint32_t job : {0xfu, 0xdu, 0u, 0x10f & 0xffu}) {
        Pick pick = base;
        pick.reputation = 20000; pick.value_a = 3000; pick.busy = 0; pick.slots = 0x1f; pick.person_job = job; pick.free_post = 0;
        run("club: all slots filled, job byte " + hex(job), pick);
    }
    // The manager post: the free-post answer zero or nonzero (also 0x100: a word with a zero low byte) in the second test.
    for (const std::uint32_t free_post : {0u, 1u, 0x100u, 0x10000u, 0xffffffffu}) {
        for (const std::uint32_t manager : {0u, 0x1234u}) {
            Pick pick = base;
            pick.reputation = 1000; pick.value_a = 700; pick.free_post = free_post; pick.manager = manager; pick.busy = 9;
            run("club: manager post, free answer " + hex(free_post) + " manager " + hex(manager), pick);
            pick.value_a = 600; pick.busy = 0;
            run("club: manager post by the second test, free answer " + hex(free_post) + " manager " + hex(manager), pick);
        }
    }
    // The database routine for real, on a few of them.
    for (const std::uint32_t variant : {0u, 1u, 2u, 3u}) {
        Pick pick = base;
        pick.reputation = 20000; pick.value_a = 3000; pick.busy = variant;
        run("club: database routine for real " + number(variant), pick, true);
    }
}

PLAYER_REGEN_CASES("regen_pending") {
    // The abilities of the staff record: the three variants (the halfword +0xa above 7500; else a draw below the byte +0x63, nonzero or zero (or no
    // draw for the byte 0)), the role byte against the thresholds where its square (or 0.75 of it) crosses a halfword and the caps, and each halfword
    // across its own thresholds. The status register cleared in half of the cases.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(4242);
    // The halfwords: the square's neighbours and the caps' (7500 / 0.9 = 8333.3, 8000, 9000), the extremes and small values.
    const auto values_for = [&](std::uint32_t role, std::int32_t cap_value) {
        const std::int32_t sq = std::int32_t(role * role), weighted = std::int32_t(role * role * 3 / 4);
        std::vector<std::int32_t> values{-32768, -1000, -1, 0, 1, 100, 5000, 7500, 7501, 32767, cap_value - 1, cap_value, cap_value + 1, cap_value + 2};
        for (const auto v : {sq, weighted}) for (const int d : {-1, 0, 1}) values.push_back(std::clamp(v + d, -32768, 32767));
        for (auto& v : values) v = std::clamp(v, -32768, 32767);
        return values;
    };
    const std::array<std::uint32_t, 22> roles{0, 1, 2, 10, 50, 89, 90, 91, 92, 94, 95, 100, 103, 104, 105, 106, 109, 110, 128, 200, 254, 255};
    std::vector<std::uint32_t> role_list(roles.begin(), roles.end());
    if (full()) { role_list.clear(); for (std::uint32_t role = 0; role < 256; ++role) role_list.push_back(role); }  // every role byte
    std::uint32_t n = 0;
    for (const auto variant : {0, 1, 2}) {
        const std::int32_t cap_value = variant == 0 ? 8334 : variant == 1 ? 8000 : 9000;
        for (const auto role : role_list) {
            for (unsigned k = 0; k < (full() ? 8u : 4u); ++k) {
                const auto values = values_for(role, cap_value);
                const auto choose = [&] { return values[random() % values.size()]; };
                Pick pick;
                pick.role = role;
                pick.value_8 = choose();
                pick.value_c = choose();
                pick.attached = false;
                pick.player = true;
                pick.position = 0;
                pick.reputation = 0;
                if (variant == 0) {
                    pick.value_a = std::int32_t(7501 + random() % (full() ? 2000 : 600));
                    if (k == 0) pick.value_a = 7501;
                    if (k == 1) pick.value_a = 32767;
                } else {
                    pick.value_a = choose();
                    if (pick.value_a > 0x1d4c) pick.value_a = 0x1d4c - std::int32_t(random() % 50);
                    pick.rating63 = k % 4 == 3 ? 0 : (k % 4 == 2 ? -7 : 100);
                    pick.rating_zero = variant == 2;
                    if (pick.rating63 == 0) pick.rating_zero = true;
                }
                Setup setup;
                setup.start_index = w.free_players[(n * 5 + 3) % w.free_players.size()];
                setup.picks = {pick};
                // An unattached pick: the person must keep a player record (the real person's own).
                const auto person = scenario.person(setup.start_index);
                std::vector<Write> more;
                const auto expected_variant = variant == 0 ? "high" : (pick.rating63 == 0 || pick.rating_zero) ? "low" : "mid";
                const bool clear = (n & 1) != 0;
                cases.push_back(pool_case(w, ring, std::string("abilities ") + expected_variant + " role " + number(role) + " v" + number(k) + " n" + number(n), setup,
                                          ring.scripted(500, 0, script_for(w, pick, person)), more, clear));
                ++n;
            }
        }
    }
}
PLAYER_REGEN_CASES("regen_pending") {
    // The registers r7 and r8 the pick leaves for the caller's caller (player_regen_volatile_read.inc): the candidate score's scaled draws (a cubed term of
    // 0x10000 or more: byte +0x2f of 82 or more; a score of 0x10000 or more: a club reputation of 1 against a large halfword, or a large role byte for an
    // unattached person) leave the sum (and the sample) in them, the removal of the pool entry leaves its counter in r7. Over pool positions and samples.
    const World w(scenario);
    const Ring ring(scenario);
    std::uint32_t n = 0;
    for (const bool nation_scaled : {false, true})
        for (const bool final_scaled : {false, true})
            for (const bool attached : {true, false})
                for (std::uint32_t variant = 0; variant < 6; ++variant) {
                    Pick pick;
                    pick.attached = attached;
                    pick.cube_byte = nation_scaled ? 82 + std::int32_t(variant) * 7 : 4;
                    pick.value_a = 3000;
                    pick.reputation = final_scaled ? 1 : 8000;
                    pick.role = final_scaled ? 255 : 10;
                    pick.rating63 = 0;
                    Setup setup;
                    setup.start_index = attached ? w.attached[1300 + (n++ % 100)] : w.free_players[(n++ * 7) % w.free_players.size()];
                    setup.picks = {pick};
                    const auto person = scenario.person(setup.start_index);
                    std::vector<std::uint32_t> samples(40, 0);
                    for (std::size_t k = 0; k < samples.size(); ++k) samples[k] = (variant * 977 + k * 4099 + 13) & 0xffff;
                    const auto before = variant % 3 == 0 ? 500u : variant % 3 == 1 ? 1u : 2u;
                    std::string label = std::string("scaled draws, nation ") + (nation_scaled ? "scaled" : "small") + ", last " + (final_scaled ? "scaled" : "small") +
                                        (attached ? ", attached" : ", unattached") + " " + number(variant);
                    cases.push_back(pool_case(w, ring, label, setup, ring.scripted(before, variant * 31, samples)));
                    (void)person;
                }
    // Several picks (the pick at each iteration leaves its registers for the next), and the pick that finds nobody after a scaled one.
    for (std::uint32_t variant = 0; variant < 8; ++variant) {
        Setup setup;
        setup.start_index = w.attached[1500 + variant];
        setup.pool_count = 3 + variant % 3;
        setup.picks.clear();
        for (std::uint32_t k = 0; k < 1 + variant % 3; ++k) {
            Pick pick;
            pick.cube_byte = (k + variant) % 2 ? 90 : 4;
            pick.reputation = (k + variant) % 3 ? 1 : 8000;
            pick.rating63 = 0;
            setup.picks.push_back(pick);
        }
        cases.push_back(pool_case(w, ring, "scaled draws, picks " + number(variant), setup, ring.scripted(variant % 2 ? 1u : 500u, variant, std::vector<std::uint32_t>(60, 0x4321 + variant))));
    }
}
}
