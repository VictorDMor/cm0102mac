// PLAYER_REGEN.CPP, block `retirement_processor`: cases for 0x102d8920, the daily walk of the regen object's retirement list (list A). Everything
// starts from the career's real people and real regen object (season-day), with list A (and list B, which the routine sorts last) rewritten by
// each case: entries with chosen ids, clubs, reasons and dates (relative to a date the case sets), the people edited over every field the walk
// branches on (age byte, club, class nibble of the kind-table row, player and staff records, the job byte, the nation record's halfword +0xac, the
// player record's halfwords and bytes, the staff pool's count), the pooled generator positioned and its samples chosen so that each draw takes a
// wanted remainder, and the outside routines that decide (the contract manager's hold 0x10083110, the transfer manager's 0x103f5e50, the date
// distance 0x10718130, the contract holder 0x10082f20 with a record built in scratch memory, 0x100d3150, the human-club test 0x1014c2e0) answered
// where the snapshot has no person for the outcome. Labels start with "retirement_processor:".
//
// The draws of the pure-player conversion, in order (a script of samples gives each its remainder): a draw below the byte +0x67 (none for 0),
// the % 100 test, the bound draw (the cube of the ability byte plus the player record's +0xa; none for 0, wide at 0x10000 or more), the % 50000
// test, the second chance's scaled draw and its draw below 30, the swap's % 4 and % 50, the variant's draw below the byte +0x63; then the tail's:
// a random nation (a person with no nation), % 65 and % 35 (age 30 or more), % 4 (a staff person with a club).
#include "player_regen_checks.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <stdexcept>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t processor = 0x102d8920, trace_callee = 0x10715880, contract_hold = 0x10083110, transfer_hold = 0x103f5e50,
                        days_between = 0x10718130, contract_of = 0x10082f20, contract_guard = 0x100d3150, human_club = 0x1014c2e0,
                        contract_apply = 0x103727c0, contract_end = 0x10374ba0, shortlist_remove = 0x10365590;
// The staff detach's callees reach the file system (HGetVol, FSMakeFSSpec), which the checker has no host for (the detach block answers the same).
const std::vector<std::pair<std::uint32_t, std::uint32_t>> detach_callees{{0x101aa680, 0}, {0x101aaa90, 0}, {0x103e9ac0, 0}, {0x103d2b30, 0}, {0x1030f950, 0}};
constexpr std::int32_t regen_slot = -0x7b28, date_slot = -0x7b80, kind_slot = -0x7b9c, cursor_slot = -0x7b84, end_slot = -0x7b88, salt_slot = -0x7b90,
                       start_slot = -0x7b8c, nation_count_slot = -0x7b04, staff_records_slot = -0x79f8;
// A contract record built in scratch memory (the answer of 0x10082f20): +0x50 is the word the walk tests.
constexpr std::uint32_t scratch_record = scratch_base + 0x58000;

struct Ring {
    std::uint32_t cursor_cell, end_cell, salt_cell, start, end;
    explicit Ring(const Scenario& s)
        : cursor_cell(s.toc(cursor_slot)), end_cell(s.toc(end_slot)), salt_cell(s.toc(salt_slot)), start(s.toc(start_slot)), end(s.r32(end_cell)) {}
    std::vector<Write> every_draw(std::uint32_t sample, std::uint32_t salt = 0) const {
        return {w32(end_cell, start), w32(cursor_cell, start), w32(salt_cell, salt), w16(start, sample)};
    }
    // The cursor `before` draws short of the end; draw n (from 0) reads samples[n] (before the wrap at the cells after the cursor, after it at the start).
    std::vector<Write> scripted(std::uint32_t before, std::uint32_t salt, const std::vector<std::uint32_t>& samples) const {
        const auto position = end - 2 * before;
        std::vector<Write> memory{w32(cursor_cell, position), w32(salt_cell, salt)};
        for (std::size_t n = 0; n < samples.size(); ++n)
            memory.push_back(w16(n < before ? position + 2 * std::uint32_t(n + 1) : start + 2 * std::uint32_t(n - before), samples[n]));
        return memory;
    }
};

// The people by kind.
struct World {
    const Scenario& s;
    std::uint32_t regen;
    std::vector<std::uint32_t> players_club, players_free, staff_club, staff_free, both;
    explicit World(const Scenario& scenario) : s(scenario), regen(scenario.global(regen_slot)) {
        if (regen == 0) throw std::runtime_error("retirement_processor: the career has no regen object");
        for (std::uint32_t n = 0; n + 16 < s.people(); ++n) {
            const auto p = s.person(n);
            if (s.r32(p + 0x0c) == 0) continue;  // every case's person has a nation (the conversion reads its halfword +0xac)
            const bool player = s.r32(p + 0x14) != 0, staff = s.r32(p + 0x1c) != 0, club = s.r32(p + 0x24) != 0;
            if (player && staff) both.push_back(p);
            else if (player) (club ? players_club : players_free).push_back(p);
            else if (staff) (club ? staff_club : staff_free).push_back(p);
        }
    }
    static std::uint32_t at(const std::vector<std::uint32_t>& list, std::size_t n) { return list[(n * 2654435761u + 17) % list.size()]; }
};

std::string number(std::int64_t n) { return std::to_string(n); }

// An entry of list A: the person, the entry's club id (-1: none), the two dates (the announcement a, the retirement b), the reason byte and the
// announced byte.
struct Listed {
    std::uint32_t person;
    std::uint32_t club = 0xffffffffu;
    std::uint32_t day_a = 0, year_a = 0, day_b = 0, year_b = 0;
    std::uint32_t reason = 1, announced = 0;
};
// Today (the date cell's halfwords): each case sets it, so that the entries' dates are relative to known values.
constexpr std::uint32_t today_day = 200, today_year = 2005;
enum class When { past_year, past_day, today, next_day, next_year };
void date_of(When when, std::uint32_t& day, std::uint32_t& year) {
    switch (when) {
    case When::past_year: day = 300; year = today_year - 1; break;
    case When::past_day: day = today_day - 1; year = today_year; break;
    case When::today: day = today_day; year = today_year; break;
    case When::next_day: day = today_day + 1; year = today_year; break;
    case When::next_year: day = 10; year = today_year + 1; break;
    }
}
Listed listed(const Scenario& s, std::uint32_t person, When a, When b, std::uint32_t reason = 1, std::uint32_t announced = 0, std::uint32_t club = 0xffffffffu) {
    Listed e{.person = s.r32(person), .club = club, .reason = reason, .announced = announced};
    date_of(a, e.day_a, e.year_a);
    date_of(b, e.day_b, e.year_b);
    return e;
}
// List A's block (the regen object's word +4 points at it) rewritten with the entries and the count, list B with the queue.
std::vector<Write> lists(const Scenario& s, std::uint32_t regen, const std::vector<Listed>& entries, const std::vector<std::array<std::uint32_t, 3>>& queue = {}) {
    std::vector<Write> memory;
    const auto base = s.r32(regen + 4);
    for (std::size_t n = 0; n < entries.size(); ++n) {
        const auto at = base + std::uint32_t(n) * 0x1c;
        const auto& e = entries[n];
        append(memory, {w32(at, e.person), w32(at + 4, e.club), w16(at + 8, e.day_a), w16(at + 0xa, e.year_a), w8(at + 0xc, 0), w16(at + 0x10, e.day_b),
                        w16(at + 0x12, e.year_b), w8(at + 0x14, 0), w8(at + 0x18, e.reason), w8(at + 0x19, e.announced)});
    }
    memory.push_back(w32(regen + 8, std::uint32_t(entries.size())));
    const auto queue_base = s.r32(regen + 0x10);
    for (std::size_t n = 0; n < queue.size(); ++n) {
        const auto at = queue_base + std::uint32_t(n) * 0xc;
        append(memory, {w32(at, queue[n][0]), w16(at + 4, queue[n][1]), w16(at + 6, queue[n][2]), w8(at + 8, 0)});
    }
    memory.push_back(w32(regen + 0x14, std::uint32_t(queue.size())));
    return memory;
}
std::vector<Write> today(const Scenario& s) {
    const auto date = s.toc(date_slot);
    return {w16(date, today_day), w16(date + 2, today_year)};
}
// The kind-table row's class byte of a person.
std::uint32_t row_of(const Scenario& s, std::uint32_t person) { return s.global(kind_slot) + s.r32(person) * 100 + 0xc; }

// A sample whose (sample + salt) % modulus is `wanted` (the salt below the modulus), the sample's multiple chosen by `k`.
std::uint32_t pick(std::uint32_t modulus, std::uint32_t wanted, std::uint32_t salt, std::uint32_t k) {
    const auto base = (wanted % modulus + modulus - salt % modulus) % modulus;
    const auto room = (0xffffu - base) / modulus + 1;
    return base + modulus * (k % room);
}

struct Builder {
    std::vector<Case>& cases;
    const Scenario& s;
    std::uint32_t regen;
    bool trace = true;
    std::vector<Case::Poke> traced() const { return trace ? std::vector<Case::Poke>{{trace_callee, {}}} : std::vector<Case::Poke>{}; }
    Case& add(const std::string& label, std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
        cases.push_back({.entry = processor, .registers = {{3, regen}}, .memory = std::move(memory), .label = "retirement_processor: " + label,
                         .answers = std::move(answers), .pokes = traced()});
        return cases.back();
    }
};
}

// The walk's dates: entries that are not yet due (announced or not, announcement date past, today, or to come), one and two entries, every class
// of the row; then entries that are due, with a person that retires at once (age 46, a player with a club) so the loop continues past removals.
PLAYER_REGEN_CASES("retirement_processor") {
    const World w(scenario);
    Builder b{cases, scenario, w.regen};
    const Ring ring(scenario);
    if (w.players_club.size() < 50 || w.players_free.size() < 5 || w.staff_club.size() < 20 || w.staff_free.size() < 20)
        throw std::runtime_error("retirement_processor: the career has too few people of a kind");
    std::size_t n = 0;
    // The refill of the pool at the first draw of a retiring staff person: it is the first retirement case (the call trace pairs the first 32).
    for (const bool announced : {false, true})
        for (const When a : {When::past_year, When::past_day, When::today, When::next_day, When::next_year})
            for (const When due : {When::today, When::next_day, When::next_year})
                for (const std::uint32_t cls : {0u, 1u, 2u}) {
                    const auto person = World::at(w.players_club, ++n);
                    auto memory = today(scenario);
                    append(memory, lists(scenario, w.regen, {listed(scenario, person, a, due, 1, announced ? 1 : 0)}));
                    append(memory, {w8(row_of(scenario, person), cls << 2 | (cls == 2 ? 0x10 : 0))});
                    b.add(std::string("not due: announced ") + number(announced) + " a " + number(int(a)) + " due " + number(int(due)) + " class " + number(cls), memory);
                }
    // Several entries: the first announced (the walk goes on), the second not, and so on; the second's dates and the third's.
    for (const bool first_announced : {true, false})
        for (const When a2 : {When::past_day, When::next_day})
            for (const When a3 : {When::past_day, When::today}) {
                std::vector<Listed> entries;
                for (std::uint32_t k = 0; k < 3; ++k) {
                    const auto person = World::at(w.players_club, ++n + 500);
                    entries.push_back(listed(scenario, person, k == 0 ? When::past_year : k == 1 ? a2 : a3, When::next_year, 1, k == 0 && first_announced ? 1 : 0));
                }
                auto memory = today(scenario);
                append(memory, lists(scenario, w.regen, entries));
                b.add(std::string("three entries not due: first announced ") + number(first_announced) + " a2 " + number(int(a2)) + " a3 " + number(int(a3)), memory);
            }
    // No entries; and the queue (list B) sorted: counts 0, 1 and several, with ties in the halfwords.
    for (const std::size_t count : {0u, 1u, 2u, 5u})
        for (const std::size_t queue : {0u, 1u, 4u, 9u}) {
            std::vector<Listed> entries;
            for (std::size_t k = 0; k < count; ++k) entries.push_back(listed(scenario, World::at(w.players_club, k + 900 + n), When::past_year, When::next_year));
            std::vector<std::array<std::uint32_t, 3>> q;
            for (std::size_t k = 0; k < queue; ++k) q.push_back({scenario.r32(World::at(w.players_club, k * 3 + 40)), std::uint32_t((k * 7) % 4), std::uint32_t(3 - (k % 3))});
            auto memory = today(scenario);
            append(memory, lists(scenario, w.regen, entries, q));
            b.add("lists of " + number(std::int64_t(count)) + " entries and a queue of " + number(std::int64_t(queue)), memory);
        }
}

// The filters and holds before the retirement: a young person (under 30, not an injury entry) with a club or in a blocking class is dropped; the
// contract manager's and the transfer manager's holds cancel (the class bits cleared); the entry's club against the person's, the days to the
// contract's end, the age, the contract record and its word +0x50 and the guard decide whether a short contract spares him (answered: the snapshot
// has no person at each combination).
PLAYER_REGEN_CASES("retirement_processor") {
    const World w(scenario);
    Builder b{cases, scenario, w.regen};
    std::size_t n = 0;
    for (const std::uint32_t age : {20u, 29u, 30u, 37u, 38u, 45u, 255u, 127u})
        for (const std::uint32_t reason : {1u, 8u})
            for (const bool club : {true, false})
                for (const std::uint32_t nibble : {0u, 1u}) {
                    const auto person = World::at(club ? w.players_club : w.players_free, ++n + 3000);
                    auto memory = today(scenario);
                    append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year, reason)}));
                    append(memory, {w8(person + 0x5d, age), w8(row_of(scenario, person), nibble << 4)});
                    b.add("filter age " + number(age) + " reason " + number(reason) + (club ? " club" : " free") + " nibble " + number(nibble), memory);
                }
    for (const std::uint32_t hold : {0u, 1u, 0x100u, 0x101u})
        for (const std::uint32_t transfer : {0u, 1u, 0x100u, 0x80u})
            for (const bool player : {true, false}) {
                const auto person = World::at(player ? w.players_club : w.staff_club, ++n + 3100);
                auto memory = today(scenario);
                append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year)}));
                append(memory, {w8(person + 0x5d, 40), w8(row_of(scenario, person), 4)});
                b.add("holds contract " + number(hold) + " transfer " + number(transfer) + (player ? " player" : " staff"), memory,
                      {{contract_hold, hold}, {transfer_hold, transfer}});
            }
    // The club fix-up and the short contract: every combination of the entry's club (none, the person's, another), the person's club (has or not),
    // the days to the end (0, 209, 210, 400, negative), the age (37, 38), the contract record (none, +0x50 zero, nonzero) and the guard (0, 1, 0x100).
    for (const std::uint32_t entry_club : {0u, 1u, 2u})
        for (const bool club : {true, false})
            for (const std::int32_t days : {0, 209, 210, 400, -5})
                for (const std::uint32_t age : {37u, 38u})
                    for (const std::uint32_t record : {0u, 1u, 2u})
                        for (const std::uint32_t guard : {0u, 1u, 0x100u}) {
                            if (record == 0 && guard != 0) continue;  // the guard is not asked without a record
                            const auto person = World::at(club ? w.players_club : w.players_free, ++n + 3200);
                            const auto person_club = scenario.r32(person + 0x24);
                            const auto entry_club_id = entry_club == 0 ? 0xffffffffu : entry_club == 1 ? (person_club ? scenario.r32(person_club) : 7u) : (person_club ? scenario.r32(person_club) + 1 : 9u);
                            auto memory = today(scenario);
                            append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year, 1, 0, entry_club_id)}));
                            append(memory, {w8(person + 0x5d, age), w8(row_of(scenario, person), 0), w32(scratch_record + 0x50, record == 2 ? 5 : 0)});
                            // Only the first lookup of the contract record (the short-contract test's) is answered: the fake record's fields are empty, so the later
                            // lookups (the announcement's) are the real ones.
                            auto& item = b.add("short contract entry club " + number(entry_club) + (club ? " club" : " free") + " days " + number(days) + " age " + number(age) + " record " + number(record) +
                                                   " guard " + number(guard),
                                               memory, {{contract_hold, 0}, {transfer_hold, 0}, {days_between, std::uint32_t(days)}, {contract_guard, guard}});
                            item.nth_answers = {{contract_of, 1, record == 0 ? 0 : scratch_record}};
                        }
}

// The pure player's conversion (the draws above), controlled: the first bound and the first two remainders, the bound's kind (none, small, wide),
// the % 50000 test (direct assignment or the second chance and its product test against the nation's halfword), the staff pool's count, the swap, the
// variants of the derived halfwords, the tail by the person's nation. The pool is positioned 40 draws short of its end, the salt 0 (a sample is its
// own remainder); every case is traced.
PLAYER_REGEN_CASES("retirement_processor") {
    const World w(scenario);
    Builder b{cases, scenario, w.regen};
    const Ring ring(scenario);
    std::mt19937 random(2024);
    const auto base = scenario.global(staff_records_slot);
    // Real staff records' indices for the pool.
    std::vector<std::uint32_t> indices;
    for (const auto staff : w.staff_club) {
        const auto record = scenario.r32(staff + 0x1c);
        if (indices.size() < 8 && record >= base && (record - base) % 0x44 == 0 && (record - base) / 0x44 < 100000) indices.push_back((record - base) / 0x44);
    }
    if (indices.size() < 2) throw std::runtime_error("retirement_processor: no staff record indices");
    const auto pool_c = [&](std::uint32_t count) {
        std::vector<Write> memory{w32(w.regen + 0x20, count)};
        const auto entries = scenario.r32(w.regen + 0x1c);
        for (std::uint32_t k = 0; k < count; ++k) append(memory, {w32(entries + k * 0x10, indices[k % indices.size()]), w32(entries + k * 0x10 + 4, 3), w32(entries + k * 0x10 + 8, 4)});
        return memory;
    };
    // The person's fields every conversion case sets: age 40, no blocking class, the byte +0x67 100 (the first draw's bound), the ability byte, the
    // player record's potential halfword +0xa and the others, the position byte, the nation's halfword +0xac.
    const auto person_memory = [&](std::uint32_t person, std::uint32_t ability, std::int32_t potential, std::uint32_t byte_63, std::uint32_t position, std::uint32_t nation_half) {
        const auto record = scenario.r32(person + 0x14);
        const auto nation = scenario.r32(person + 0x0c);
        return std::vector<Write>{w8(person + 0x5d, 40), w8(row_of(scenario, person), 0), w8(person + 0x67, 100), w8(person + 0x5e, ability), w8(person + 0x63, byte_63),
                                  w16(record + 0xa, std::uint32_t(potential)), w16(record + 8, 2600), w16(record + 0xc, 1800), w8(record + 0xf, position), w16(nation + 0xac, nation_half)};
    };
    std::size_t n = 0;
    for (const std::int32_t potential : {8000, 3000, 0})                       // above or below 7500 (the variants), 0 for no bound with a zero byte
        for (const std::uint32_t ability : {0u, 10u, 100u, 250u})              // the cube's base: none (with potential 0), small, wide
            for (const std::uint32_t d4 : {0u, 49999u})                        // % 50000: below r15 (assign directly) or the second chance
                for (const std::uint32_t product : {0u, 1u})                   // the second chance's product test: fails (half 0) or passes (a large halfword)
                    for (const std::uint32_t count : {2u, 5u}) {               // the staff pool's count: 2 skips the assignment
                        if (d4 == 0 && product == 1) continue;                 // the product test is only on the second-chance path
                        const auto person = World::at(n % 3 == 2 ? w.players_free : w.players_club, n + 4000);
                        ++n;
                        const std::int32_t cube = std::int32_t(ability * ability * ability) + potential;
                        std::vector<std::uint32_t> samples;
                        samples.push_back(pick(100, 60, 0, random()));         // d1: below the byte +0x67 (100): remainder 60
                        samples.push_back(pick(100, 10, 0, random()));         // d2: % 100 -> 10 < 60: proceed
                        if (cube != 0) {
                            samples.push_back(cube < 0x10000 ? pick(std::uint32_t(cube), 9, 0, random()) : std::uint32_t(random() & 0xffff));
                            if (cube >= 0x10000) samples.push_back(random() & 0xffff);  // the wide form's second draw (a call of 0x102da630)
                        }
                        samples.push_back(pick(50000, d4, 0, 0));              // d4
                        if (d4 != 0) { samples.push_back(random() & 0xffff); samples.push_back(pick(30, random() % 30, 0, random())); }
                        samples.push_back(pick(5, n % 2 ? 0u : 3u, 0, random()));  // the variant's draw below the byte +0x63 (5): 0 gives the weak variant
                        for (unsigned k = 0; k < 4; ++k) samples.push_back(random() & 0xffff);
                        auto memory = today(scenario);
                        append(memory, ring.scripted(40, 0, samples));
                        append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year)}));
                        append(memory, pool_c(count));
                        append(memory, person_memory(person, ability, potential, 5, 0, product ? 5000u : 0u));
                        b.add("convert potential " + number(potential) + " ability " + number(ability) + " d4 " + number(d4) + " product " + number(product) + " pool " + number(count), memory);
                    }
    // The swap of the staff record's bytes +0x30/+0x31 and its % 4 and % 50 draws, with the position byte +0xf of 20 and not.
    for (const std::uint32_t position : {20u, 19u})
        for (const std::uint32_t first : {5u, 90u, 0xf0u})
            for (const std::uint32_t second : {5u, 90u, 0xf0u})
                for (const std::uint32_t quarter : {0u, 1u}) {
                    const auto person = World::at(w.players_club, ++n + 5000);
                    std::vector<std::uint32_t> samples{pick(100, 60, 0, random()), pick(100, 10, 0, random()), std::uint32_t(random() & 0xffff), std::uint32_t(random() & 0xffff), pick(50000, 0, 0, 0)};
                    samples.push_back(pick(4, quarter, 0, random()));
                    samples.push_back(pick(50, random() % 50, 0, random()));
                    for (unsigned k = 0; k < 5; ++k) samples.push_back(random() & 0xffff);
                    auto memory = today(scenario);
                    append(memory, ring.scripted(40, 0, samples));
                    append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year)}));
                    append(memory, pool_c(4));
                    append(memory, person_memory(person, 100, 3000, 5, position, 0));
                    const auto staff_record = base + indices[0] * 0x44;  // the record the pool's first entry names
                    append(memory, {w8(staff_record + 0x30, first), w8(staff_record + 0x31, second)});
                    // Half of them with the database's finishing routine 0x100c3870 (which follows the swap and rewrites these bytes) answered, so the
                    // draw's stored byte is what the comparison sees.
                    b.add("swap position " + number(position) + " bytes " + number(first) + "/" + number(second) + " quarter " + number(quarter) + (n % 2 ? " finish answered" : ""), memory,
                          n % 2 ? std::vector<std::pair<std::uint32_t, std::uint32_t>>{{0x100c3870, 0}} : std::vector<std::pair<std::uint32_t, std::uint32_t>>{});
                }
}

// The tails by kind of person, real and edited: players with and without a club (the conversion not taken: the byte +0x67 is 0), staff with a club
// by every job byte and the human-club answer, staff without a club, people with both records (a real one, or a player given a staff record). The date
// draws: age under 30 (today), 30 or more (1..65 days on, or the next year from day 330 on).
PLAYER_REGEN_CASES("retirement_processor") {
    const World w(scenario);
    Builder b{cases, scenario, w.regen};
    const Ring ring(scenario);
    std::mt19937 random(777);
    std::size_t n = 0;
    const auto date = scenario.toc(date_slot);
    // Real people unedited (their own ages, clubs, classes) but for the entry; some young ones are dropped, the others retire.
    for (std::size_t k = 0; k < 40; ++k)
        for (const auto* list : {&w.players_club, &w.players_free, &w.staff_club, &w.staff_free, &w.both}) {
            if (list->empty()) continue;
            const auto person = World::at(*list, ++n + 6000);
            auto memory = today(scenario);
            append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year, k % 5 == 0 ? 8 : 1)}));
            if (k % 2) append(memory, ring.scripted(k % 12 + 1, random() & 0xffff, {random() & 0xffff, random() & 0xffff, random() & 0xffff, random() & 0xffff, random() & 0xffff, random() & 0xffff}));
            b.trace = k < 4;
            b.add("real person " + number(std::int64_t(n)) + (list == &w.players_club ? " player club" : list == &w.players_free ? " player free" : list == &w.staff_club ? " staff club" :
                                                                list == &w.staff_free ? " staff free" : " both"), memory);
        }
    b.trace = true;
    // The date draws: ages 29 and 30+ (today, or the draw below 65 and, at day 330 on, the wrap), for staff and for players without the conversion.
    for (const std::uint32_t age : {29u, 30u, 40u})
        for (const std::uint32_t day : {200u, 330u, 364u})
            for (const std::uint32_t rem65 : {0u, 14u, 15u, 34u, 35u, 64u})
                for (const bool staff : {true, false}) {
                    const auto person = World::at(staff ? w.staff_free : w.players_free, ++n + 7000);
                    auto memory = today(scenario);
                    append(memory, {w16(date, day)});
                    append(memory, ring.scripted(40, 0, {pick(65, rem65, 0, random()), pick(35, random() % 35, 0, random()), random() & 0xffff, random() & 0xffff}));
                    append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year)}));
                    append(memory, {w8(person + 0x5d, age), w8(row_of(scenario, person), 0), w32(person + 0x24, 0)});
                    if (!staff) append(memory, {w8(person + 0x67, 0)});
                    b.add(std::string("date draws ") + (staff ? "staff" : "player") + " age " + number(age) + " day " + number(day) + " rem65 " + number(rem65), memory,
                          [&] { auto answered = detach_callees; answered.push_back({shortlist_remove, 0}); return answered; }());
                }
    // Staff with a club: the job byte over every kind, the human answer, and the % 4 draw (0 and not).
    for (const std::uint32_t job : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 0x80u, 0xffu})
        for (const int human : {-1, 0, 1})
            for (const std::uint32_t quarter : {0u, 1u, 3u})
                for (const std::uint32_t age : {29u, 40u}) {
                    const auto person = World::at(w.staff_club, ++n + 8000);
                    // The staff detach's callee 0x101aaa90 reaches the file system (HGetVol), which the checker has no host for: answered.
                    auto answers = detach_callees;
                    if (human >= 0) answers.push_back({human_club, std::uint32_t(human)});
                    auto memory = today(scenario);
                    append(memory, ring.scripted(40, 0, {pick(65, 20, 0, random()), pick(35, 4, 0, random()), pick(4, quarter, 0, random()), random() & 0xffff, random() & 0xffff}));
                    append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year)}));
                    append(memory, {w8(person + 0x5d, age), w8(row_of(scenario, person), 0), w8(person + 0x62, job)});
                    if (age == 29) append(memory, {w32(person + 0x24, scenario.r32(person + 0x24))});
                    b.add("staff club job " + number(job) + " human " + number(human) + " quarter " + number(quarter) + " age " + number(age), memory, answers);
                }
    // A player given a staff record (both records), with and without the second nation and the first (the random nation index), a club or not, and
    // the contract record answered (the apply step).
    for (const bool nation : {true, false})
        for (const bool second : {true, false})
            for (const bool club : {true, false})
                for (const std::uint32_t record : {0u, 1u})
                    for (const std::uint32_t count : {0u, 1u, 40u, 0x10000u, 0x20000u, 0xffff0000u, 0xfffe0000u, 0xffff0001u, 0x7fffffffu}) {
                        if (nation && count != 1) continue;  // the nation count's draw only without a nation
                        const auto person = World::at(club ? w.players_club : w.players_free, ++n + 9000);
                        const auto staff = World::at(w.staff_club, n);
                        const std::int32_t signed_count = std::int32_t(count);
                        const bool wide = !nation && (signed_count >= 0x10000 || signed_count <= -0x10000);
                        const auto count_cell = scenario.toc(nation_count_slot);
                        auto memory = today(scenario);
                        append(memory, ring.scripted(40, 0, {random() & 0xffff, random() & 0xffff, random() & 0xffff, random() & 0xffff}));
                        append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year)}));
                        append(memory, {w8(person + 0x5d, 40), w8(row_of(scenario, person), 0), w32(person + 0x1c, scenario.r32(staff + 0x1c)), w8(person + 0x67, 0)});
                        if (!nation && !wide) append(memory, {w32(count_cell, count)});
                        if (!nation) append(memory, {w32(person + 0x0c, 0), w32(person + 0x10, 0)});
                        else if (!second) append(memory, {w32(person + 0x10, 0)});
                        else append(memory, {w32(person + 0x10, scenario.r32(person + 0x0c))});
                        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
                        if (record) answers = {{contract_of, scratch_record}, {contract_apply, 0}, {contract_end, 0}};
                        auto& item = b.add(std::string("both records ") + (nation ? "nation" : "no nation") + (second ? " second" : " single") + (club ? " club" : " free") + " record " + number(record) +
                                               " count " + number(count),
                                           memory, answers);
                        if (wide) {
                            // The callees loop over the nations' count: it is made wide where the draw reads it (the last callee before it is answered and
                            // the cell set at its dispatch), and the real value is put back at the first callee after.
                            const auto real = scenario.global(nation_count_slot);
                            item.answers.push_back({0x1018fdf0, 0});
                            item.pokes.push_back({0x1018fdf0, {w32(count_cell, count)}});
                            item.pokes.push_back({contract_of, {w32(count_cell, real)}});
                            item.pokes.push_back({0x10768300, {w32(count_cell, real)}});
                        }
                    }
}

// The conversion's comparisons at their equalities (one below, equal, one above): the first remainder against the % 100 test, the bound's remainder
// against the % 50000 test, the second chance's product against the draw plus 200000, the potential against 7500, the pool's count against 2.
PLAYER_REGEN_CASES("retirement_processor") {
    const World w(scenario);
    Builder b{cases, scenario, w.regen};
    const Ring ring(scenario);
    std::mt19937 random(2718);
    const auto base = scenario.global(staff_records_slot);
    std::vector<std::uint32_t> indices;
    for (const auto staff : w.staff_club) {
        const auto record = scenario.r32(staff + 0x1c);
        if (indices.size() < 8 && record >= base && (record - base) % 0x44 == 0 && (record - base) / 0x44 < 100000) indices.push_back((record - base) / 0x44);
    }
    std::size_t n = 0;
    struct Edge { const char* what; std::uint32_t r1, rem2, ability; std::int32_t potential; std::uint32_t d3, rem4, half, count; bool second_chance_zero; };
    // The bound of a small cube: ability 10 and potential 3000 give 4000, so the draw d3 below it has the remainder d3. For the product edge the second
    // chance's scaled draw has sample 0 (remainder 0) and its draw below 30 gives 0 (a tenth of 5: 0), so r15 = 0 and the threshold is 200000.
    const std::vector<Edge> edges{
        {"d2 below", 60, 59, 10, 3000, 9, 0, 0, 5, false},      {"d2 equal", 60, 60, 10, 3000, 9, 0, 0, 5, false},      {"d2 above", 60, 61, 10, 3000, 9, 0, 0, 5, false},
        {"d4 below", 60, 10, 10, 3000, 9, 8, 0, 5, false},      {"d4 equal", 60, 10, 10, 3000, 9, 9, 0, 5, false},      {"d4 above", 60, 10, 10, 3000, 9, 10, 0, 5, false},
        {"product below", 60, 10, 100, 3000, 0, 49999, 1999, 5, true}, {"product equal", 60, 10, 100, 3000, 0, 49999, 2000, 5, true}, {"product above", 60, 10, 100, 3000, 0, 49999, 2001, 5, true},
        {"potential 7499", 60, 10, 10, 7499, 9, 0, 0, 5, false}, {"potential 7500", 60, 10, 10, 7500, 9, 0, 0, 5, false}, {"potential 7501", 60, 10, 10, 7501, 9, 0, 0, 5, false},
        {"pool 2", 60, 10, 10, 3000, 9, 0, 0, 2, false}, {"pool 3", 60, 10, 10, 3000, 9, 0, 0, 3, false}, {"pool 1", 60, 10, 10, 3000, 9, 0, 0, 1, false},
    };
    for (const auto& e : edges)
        for (const std::uint32_t byte_63 : {0u, 3u}) {
            const auto person = World::at(w.players_club, 15000 + ++n);
            const auto record = scenario.r32(person + 0x14);
            const auto nation = scenario.r32(person + 0x0c);
            const std::int32_t cube = std::int32_t(e.ability * e.ability * e.ability) + e.potential;
            std::vector<std::uint32_t> samples{pick(100, e.r1, 0, 1), pick(100, e.rem2, 0, 2)};
            if (cube != 0) {
                if (cube < 0x10000) samples.push_back(pick(std::uint32_t(cube), e.d3, 0, 3));
                else { samples.push_back(0); samples.push_back(1); }  // the wide form: its scaled sample (remainder 0) and its second draw
            }
            samples.push_back(pick(50000, e.rem4, 0, 0));
            if (e.second_chance_zero) { samples.push_back(0); samples.push_back(pick(30, 0, 0, 1)); }
            samples.push_back(pick(5, byte_63, 0, 2));
            for (unsigned k = 0; k < 4; ++k) samples.push_back(random() & 0xffff);
            auto memory = today(scenario);
            append(memory, ring.scripted(40, 0, samples));
            append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year)}));
            std::vector<Write> pool{w32(w.regen + 0x20, e.count)};
            const auto entries = scenario.r32(w.regen + 0x1c);
            for (std::uint32_t k = 0; k < e.count; ++k) append(pool, {w32(entries + k * 0x10, indices[k % indices.size()]), w32(entries + k * 0x10 + 4, 3), w32(entries + k * 0x10 + 8, 4)});
            append(memory, pool);
            append(memory, {w8(person + 0x5d, 40), w8(row_of(scenario, person), 0), w8(person + 0x67, 100), w8(person + 0x5e, e.ability), w8(person + 0x63, 5),
                            w16(record + 0xa, std::uint32_t(e.potential)), w8(record + 0xf, 19), w16(nation + 0xac, e.half)});
            b.add(std::string("edge ") + e.what + " variant " + number(byte_63), memory);
        }
}

// A negative salt (the pool's salt cell below -65535, which the checker's mutated runs also produce): a sum below 0 gives negative remainders (C's `%`, the
// bounds' signs), which the tests of the conversion (% 100, % 50000, % 4, the signed draws below the bytes) and the date draws (% 65, % 35) take.
PLAYER_REGEN_CASES("retirement_processor") {
    const World w(scenario);
    Builder b{cases, scenario, w.regen};
    const Ring ring(scenario);
    std::mt19937 random(4242);
    const auto base = scenario.global(staff_records_slot);
    std::vector<std::uint32_t> indices;
    for (const auto staff : w.staff_club) {
        const auto record = scenario.r32(staff + 0x1c);
        if (indices.size() < 8 && record >= base && (record - base) % 0x44 == 0 && (record - base) / 0x44 < 100000) indices.push_back((record - base) / 0x44);
    }
    const auto date = scenario.toc(date_slot);
    for (std::size_t n = 0; n < 90; ++n) {
        const auto person = World::at(n % 3 == 2 ? w.players_free : w.players_club, 16000 + n);
        const auto record = scenario.r32(person + 0x14);
        std::vector<std::uint32_t> samples;
        for (unsigned k = 0; k < 30; ++k) samples.push_back(random() % 4 ? random() & 0xffff : random() % 200);
        const std::uint32_t salt = std::array<std::uint32_t, 5>{0xffff0000u, 0xfffe0000u, 0x80000000u, 0xffff8000u, 0xfffffff0u}[n % 5];
        auto memory = today(scenario);
        append(memory, ring.scripted(40, salt, samples));
        append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year)}));
        std::vector<Write> pool{w32(w.regen + 0x20, 5)};
        const auto entries = scenario.r32(w.regen + 0x1c);
        for (std::uint32_t k = 0; k < 5; ++k) append(pool, {w32(entries + k * 0x10, indices[k % indices.size()]), w32(entries + k * 0x10 + 4, 3), w32(entries + k * 0x10 + 8, 4)});
        append(memory, pool);
        append(memory, {w8(person + 0x5d, n % 2 ? 40u : 29u), w8(row_of(scenario, person), 0), w8(person + 0x67, n % 4 == 0 ? 0xf0u : 100u), w8(person + 0x5e, n % 3 == 0 ? 10u : 100u),
                        w8(person + 0x63, n % 5 == 0 ? 0xfbu : 5u), w16(record + 0xa, n % 2 ? 3000u : 8000u), w8(record + 0xf, 20), w16(date, n % 7 == 0 ? 330u : 200u)});
        const auto staff_record = base + indices[0] * 0x44;
        append(memory, {w8(staff_record + 0x30, 90), w8(staff_record + 0x31, 5)});
        b.add("negative salt " + number(std::int64_t(n)), memory, n % 2 ? std::vector<std::pair<std::uint32_t, std::uint32_t>>{{0x100c3870, 0}} : std::vector<std::pair<std::uint32_t, std::uint32_t>>{});
    }
}

// The transfer manager's shortlist object (the word at +0x988 of the object at TOC -0x7b40) absent: the shortlist release is skipped.
PLAYER_REGEN_CASES("retirement_processor") {
    const World w(scenario);
    Builder b{cases, scenario, w.regen};
    const auto managers = scenario.global(-0x7b40);
    for (std::size_t n = 0; n < 8; ++n) {
        const auto& list = n % 4 == 0 ? w.players_club : n % 4 == 1 ? w.players_free : n % 4 == 2 ? w.staff_club : w.staff_free;
        const auto person = World::at(list, 17000 + n);
        auto memory = today(scenario);
        append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year)}));
        append(memory, {w32(managers + 0x988, 0), w8(person + 0x5d, 40), w8(row_of(scenario, person), 0)});
        b.add("shortlist none " + number(std::int64_t(n)), memory, n >= 2 ? detach_callees : std::vector<std::pair<std::uint32_t, std::uint32_t>>{});
    }
}

// A random sweep over everything above together: one to four entries (any person kind, dates on both sides of both thresholds, reasons, announced
// bytes, entry clubs), the people edited over every field the walk reads, the staff pool, the pool's position and samples, and a random subset of the
// deciding routines answered. 600 cases (CM_RETIREMENT_FULL=N: N cases; the sweep is untraced, the whole generator state and the memory are compared).
PLAYER_REGEN_CASES("retirement_processor") {
    const World w(scenario);
    Builder b{cases, scenario, w.regen};
    b.trace = false;
    const Ring ring(scenario);
    std::mt19937 random(31415);
    const auto base = scenario.global(staff_records_slot);
    std::vector<std::uint32_t> indices;
    for (const auto staff : w.staff_club) {
        const auto record = scenario.r32(staff + 0x1c);
        if (indices.size() < 8 && record >= base && (record - base) % 0x44 == 0 && (record - base) / 0x44 < 100000) indices.push_back((record - base) / 0x44);
    }
    const char* full = std::getenv("CM_RETIREMENT_FULL");
    const std::size_t total = full ? std::size_t(std::strtoul(full, nullptr, 10)) : 600;
    const auto date = scenario.toc(date_slot);
    const std::array<std::uint32_t, 10> ages{0, 20, 25, 29, 30, 37, 38, 45, 127, 255};
    const std::array<std::uint32_t, 6> bytes{0, 1, 5, 100, 127, 255};
    const std::array<std::uint32_t, 5> abilities{0, 10, 40, 100, 255};
    const std::array<std::int32_t, 7> potentials{-100, 0, 3000, 7500, 7501, 8000, 20000};
    const std::array<When, 5> whens{When::past_year, When::past_day, When::today, When::next_day, When::next_year};
    for (std::size_t n = 0; n < total; ++n) {
        std::vector<Listed> entries;
        auto memory = today(scenario);
        const auto count = 1 + random() % 4;
        for (std::size_t k = 0; k < count; ++k) {
            const auto kind = random() % 5;
            const auto& list = kind == 0 ? w.players_club : kind == 1 ? w.players_free : kind == 2 ? w.staff_club : kind == 3 ? w.staff_free : w.both;
            if (list.empty()) continue;
            const auto person = World::at(list, random() % 100000);
            const auto person_club = scenario.r32(person + 0x24);
            const std::uint32_t entry_club = random() % 3 == 0 ? 0xffffffffu : random() % 3 == 0 ? (person_club ? scenario.r32(person_club) : 5u) : 3u;
            entries.push_back(listed(scenario, person, whens[random() % 5], random() % 4 ? When::past_year : whens[random() % 5],
                                     std::array<std::uint32_t, 3>{1, 8, 0x20}[random() % 3], random() % 5 == 0 ? 1 : 0, entry_club));
            append(memory, {w8(person + 0x5d, ages[random() % ages.size()]), w8(row_of(scenario, person), random() % 6 == 0 ? random() & 0xff : 0u),
                            w8(person + 0x67, bytes[random() % bytes.size()]), w8(person + 0x63, bytes[random() % bytes.size()]), w8(person + 0x5e, abilities[random() % abilities.size()]),
                            w8(person + 0x62, random() % 3 == 0 ? random() & 0xff : 1u)});
            if (random() % 5 == 0) append(memory, {w32(person + 0x24, 0)});
            if (random() % 9 == 0) append(memory, {w32(person + 0x0c, scenario.r32(w.players_club[0] + 0x0c))});
            if (const auto record = scenario.r32(person + 0x14); record != 0) {
                append(memory, {w16(record + 0xa, std::uint32_t(potentials[random() % potentials.size()])), w16(record + 8, random() % 9000), w16(record + 0xc, random() % 9000),
                                w8(record + 0xf, random() % 4 == 0 ? 20u : random() & 0xff)});
                if (const auto nation = scenario.r32(person + 0x0c); nation != 0 && random() % 2) append(memory, {w16(nation + 0xac, random() % 3 ? random() % 30000 : 0u)});
            }
            if (random() % 6 == 0 && !indices.empty()) append(memory, {w32(person + 0x1c, base + indices[random() % indices.size()] * 0x44)});
        }
        if (entries.empty()) continue;
        append(memory, lists(scenario, w.regen, entries));
        // The staff pool: count 0..6, the first entries naming real staff records.
        const auto pool = random() % 7;
        append(memory, {w32(w.regen + 0x20, pool)});
        const auto entries_c = scenario.r32(w.regen + 0x1c);
        for (std::size_t k = 0; k < pool && !indices.empty(); ++k) append(memory, {w32(entries_c + k * 0x10, indices[random() % indices.size()]), w32(entries_c + k * 0x10 + 4, 3)});
        if (random() % 4 == 0) append(memory, {w16(date, std::uint32_t(random() % 3 ? 330 + random() % 36 : random() % 400))});
        std::vector<std::uint32_t> samples;
        for (unsigned k = 0; k < 40; ++k) samples.push_back(random() % 3 ? random() & 0xffff : random() % 120);
        const auto salt = random() % 4 == 0 ? random() & 0xffff : 0u;
        if (random() % 5 == 0) append(memory, ring.every_draw(random() & 0xffff, salt));
        else append(memory, ring.scripted(random() % 45, salt, samples));
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
        if (random() % 7 == 0) answers.push_back({contract_hold, random() % 2});
        if (random() % 7 == 0) answers.push_back({transfer_hold, random() % 2});
        if (random() % 6 == 0) answers.push_back({days_between, std::array<std::uint32_t, 4>{0, 209, 210, 0xfffffff0u}[random() % 4]});
        if (random() % 5 == 0) { answers.push_back({contract_of, random() % 3 ? scratch_record : 0}); answers.push_back({contract_apply, 0}); answers.push_back({contract_end, 0}); }
        if (random() % 6 == 0) answers.push_back({contract_guard, random() % 2});
        if (random() % 5 == 0) answers.push_back({human_club, random() % 2});
        if (random() % 3 == 0) answers.insert(answers.end(), detach_callees.begin(), detach_callees.end());  // the callees that reach the file system have no host in the checker
        if (random() % 3 == 0) answers.push_back({shortlist_remove, 0});
        append(memory, {w32(scratch_record + 0x50, random() % 2)});
        b.add("random " + number(std::int64_t(n)), memory, answers);
    }
}

// A refill at each draw position in turn (the call trace pairs only the first 32 calls of an entry and the cases that carry a poke; a refill is paired only
// when the pool is at its end at that draw): for each path of the walk, `before` 0..draws-1 draws short of the end, so the draw number `before` refills
// (the remainders after it are not controlled, which does not matter for pairing that draw's site). Draw numbers: the conversion's d1 (below the byte
// +0x67), d2 (% 100), d3 (the bound), d4 (% 50000), the second chance's scaled draw and its draw below 30, the swap's % 4 and % 50, the variant's draw
// below the byte +0x63, the tail's nation draw; the date's % 65 and % 35; the % 4 of a staff person with a club.
PLAYER_REGEN_CASES("retirement_processor") {
    const World w(scenario);
    Builder b{cases, scenario, w.regen};
    const Ring ring(scenario);
    std::mt19937 random(99);
    const auto base = scenario.global(staff_records_slot);
    std::vector<std::uint32_t> indices;
    for (const auto staff : w.staff_club) {
        const auto record = scenario.r32(staff + 0x1c);
        if (indices.size() < 8 && record >= base && (record - base) % 0x44 == 0 && (record - base) / 0x44 < 100000) indices.push_back((record - base) / 0x44);
    }
    const auto date = scenario.toc(date_slot);
    const auto pool_c = [&](std::uint32_t count) {
        std::vector<Write> memory{w32(w.regen + 0x20, count)};
        const auto entries = scenario.r32(w.regen + 0x1c);
        for (std::uint32_t k = 0; k < count; ++k) append(memory, {w32(entries + k * 0x10, indices[k % indices.size()]), w32(entries + k * 0x10 + 4, 3), w32(entries + k * 0x10 + 8, 4)});
        return memory;
    };
    struct Config { const char* name; std::vector<std::uint32_t> samples; std::uint32_t ability, potential, byte_63, position, half; bool no_nation; };
    // The staff record the pool's first entry names holds bytes +0x30 = 90, +0x31 = 5 (swapped when the position byte is 20).
    const std::vector<Config> configs{
        {"direct strong swap", {pick(100, 60, 0, 1), pick(100, 10, 0, 2), 7, 8, pick(50000, 0, 0, 0), pick(4, 0, 0, 3), pick(50, 7, 0, 5)}, 100, 8000, 5, 20, 0, false},
        {"direct medium", {pick(100, 60, 0, 1), pick(100, 10, 0, 2), 7, 8, pick(50000, 0, 0, 0), pick(5, 3, 0, 4)}, 100, 3000, 5, 19, 0, false},
        {"direct weak", {pick(100, 60, 0, 1), pick(100, 10, 0, 2), 7, 8, pick(50000, 0, 0, 0), pick(5, 0, 0, 4)}, 100, 3000, 5, 19, 0, false},
        {"second chance", {pick(100, 60, 0, 1), pick(100, 10, 0, 2), 7, 8, pick(50000, 49999, 0, 0), 12345, pick(30, 11, 0, 3), pick(4, 0, 0, 3), pick(50, 7, 0, 5), pick(5, 0, 0, 4)}, 100, 3000, 5, 20, 5000, false},
        {"small bound no nation", {pick(100, 60, 0, 1), pick(100, 10, 0, 2), 7, pick(50000, 0, 0, 0), pick(5, 3, 0, 4), 9}, 10, 3000, 5, 19, 0, true},
    };
    for (const auto& config : configs)
        for (std::uint32_t before = 0; before < config.samples.size() + 1; ++before) {
            const auto person = World::at(w.players_club, 12000 + cases.size());
            const auto record = scenario.r32(person + 0x14);
            const auto nation = scenario.r32(person + 0x0c);
            auto memory = today(scenario);
            append(memory, ring.scripted(before, 0, config.samples));
            append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year)}));
            append(memory, pool_c(4));
            append(memory, {w8(person + 0x5d, 40), w8(row_of(scenario, person), 0), w8(person + 0x67, 100), w8(person + 0x5e, config.ability), w8(person + 0x63, config.byte_63),
                            w16(record + 0xa, std::uint32_t(config.potential)), w8(record + 0xf, config.position), w16(nation + 0xac, config.half)});
            const auto staff_record = base + indices[0] * 0x44;
            append(memory, {w8(staff_record + 0x30, 90), w8(staff_record + 0x31, 5)});
            if (config.no_nation) append(memory, {w32(person + 0x0c, 0), w32(person + 0x10, 0)});
            b.add(std::string("refill at draw ") + number(before) + " " + config.name, memory);
        }
    // The date draws and the staff person's % 4, day 330 so the second draw is taken.
    for (std::uint32_t before = 0; before < 5; ++before)
        for (const bool staff : {true, false}) {
            const auto person = World::at(staff ? w.staff_club : w.players_free, 13000 + before + (staff ? 100 : 0));
            auto memory = today(scenario);
            append(memory, {w16(date, 330)});
            append(memory, ring.scripted(before, 0, {pick(65, 40, 0, 1), pick(35, 4, 0, 2), pick(4, 0, 0, 3), 5, 6}));
            append(memory, lists(scenario, w.regen, {listed(scenario, person, When::past_year, When::past_year)}));
            append(memory, {w8(person + 0x5d, 40), w8(row_of(scenario, person), 0)});
            if (staff) append(memory, {w8(person + 0x62, 1)});
            else append(memory, {w8(person + 0x67, 0)});
            b.add(std::string("refill at draw ") + number(before) + (staff ? " staff club" : " player free") + " date draws", memory, staff ? std::vector<std::pair<std::uint32_t, std::uint32_t>>{{human_club, 0}} : std::vector<std::pair<std::uint32_t, std::uint32_t>>{});
        }
}
}
