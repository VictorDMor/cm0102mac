// PLAYER_REGEN.CPP, block `lifecycle_and_drivers`: cases for the regen object's constructor (0x102d8230) and destructor (0x102d85c0) and the two scans
// that fill its retirement list (0x102d86c0 daily, 0x102d8830 for a new game). No capture reaches any of them, so the cases start from the career's
// real state (season-day) and edit what the routines read.
//
// The scans walk the people table with a retirement entry on the frame: the people table's base cell and count are rewritten to a window of the first
// real people (ids 0..n-1 pass the id test: the window's count is n + 16), so each case is a few hundred persons, and edited over every test the scans
// make: the age byte (under and over 30, the signed byte's extremes), the club and its link (+0x30, the link table's word), the id against the count, the
// kind-table class, a retirement already queued in list A, the contract record's flag byte (answered: none, flag zero, a flag, a flag whose low byte is
// zero), the decisions' outcomes (an old player with a club retires: dates by the competition's rows; the unattached decision draws), the caller's
// r19, and several appended entries so the sort is exercised. The destructor runs on the real regen object (its blocks and the per-nation club lists
// are real heap objects), on copies of it with each block missing, with every flag and nation count. The constructor runs on an object in scratch
// memory: the data file's lookup answered (found; refused once or twice before it is found), the new game and the load of retire.dat (an in-memory
// archive), each allocation refused in turn (the assertion and the end of the game), with the window for the initial scan.
// Labels start with "lifecycle:" (CM_PLAYER_REGEN_BLOCK=lifecycle).
#include "player_regen_checks.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t construct = 0x102d8230, destruct = 0x102d85c0, scan_daily = 0x102d86c0, scan_initial = 0x102d8830;
constexpr std::uint32_t contract_of = 0x10082f20, allocator = 0x10775130, competition_of_club = 0x10191fd0,
                        competition_of_nation = 0x10191f80, days_between = 0x10718130, protect_list = 0x10362d70;
constexpr std::int32_t regen_slot = -0x7b28, people_slot = -0x7be0, count_slot = -0x7be4, link_table_slot = -0x7b5c, nation_count_slot = -0x7b04, date_slot = -0x7b80,
                       table_slot = -0x7b44, kind_slot = -0x7b9c, cursor_slot = -0x7b84, end_slot = -0x7b88, salt_slot = -0x7b90, start_slot = -0x7b8c;
constexpr std::uint32_t area = scratch_base + 0x30000, object_at = area, record_at = area + 0x400, link_at = area + 0x500;

std::string number(std::int64_t n) { return std::to_string(n); }

struct Ring {
    std::uint32_t cursor_cell, end_cell, salt_cell, start, end;
    explicit Ring(const Scenario& s)
        : cursor_cell(s.toc(cursor_slot)), end_cell(s.toc(end_slot)), salt_cell(s.toc(salt_slot)), start(s.toc(start_slot)), end(s.r32(end_cell)) {}
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
    std::uint32_t regen, people_cell, count_cell, link_table, kinds, date, rows;
    std::vector<std::uint32_t> players_club, staff_club, staff_free, players_free, with_link;
    explicit World(const Scenario& scenario)
        : s(scenario), regen(scenario.global(regen_slot)), people_cell(scenario.toc(people_slot)), count_cell(scenario.toc(count_slot)),
          link_table(scenario.global(link_table_slot)), kinds(scenario.global(kind_slot)), date(scenario.toc(date_slot)), rows(scenario.toc(table_slot)) {
        if (regen == 0) throw std::runtime_error("lifecycle: the career has no regen object");
        for (std::uint32_t n = 0; n < 700 && n + 16 < s.people(); ++n) {
            const auto p = s.person(n);
            const bool player = s.r32(p + 0x14) != 0, staff = s.r32(p + 0x1c) != 0, club = s.r32(p + 0x24) != 0;
            if (player && club) players_club.push_back(n);
            if (player && !club) players_free.push_back(n);
            if (!player && staff && club) staff_club.push_back(n);
            if (!player && staff && !club) staff_free.push_back(n);
            if (club && s.r32(s.r32(p + 0x24) + 0x30) != 0) with_link.push_back(n);
        }
        std::fprintf(stderr, "lifecycle: regen %x, %zu players with a club, %zu staff with a club, %zu unattached staff, %zu unattached players, %zu with a link in the first 700\n",
                     regen, players_club.size(), staff_club.size(), staff_free.size(), players_free.size(), with_link.size());
        if (players_club.size() < 20 || staff_club.size() < 5 || with_link.size() < 3)
            throw std::runtime_error("lifecycle: the first people of the career are too few of a kind");
    }
    // The window: the first `n` people (their ids are below n + 16 - 16).
    std::vector<Write> window(std::uint32_t n) const { return {w32(people_cell, s.person(0)), w32(count_cell, n + 16)}; }
    std::uint32_t person(std::uint32_t n) const { return s.person(n); }
    std::uint32_t kind_row(std::uint32_t person) const { return kinds + s.r32(person) * 100u + 0xc; }
};

// A retirement entry of list A for person `id` (the bytes of a pending entry).
std::vector<Write> queued(const World& w, std::uint32_t slot, std::uint32_t id) {
    const auto at = w.s.r32(w.regen + 4) + slot * 0x1c;
    return {w32(at, id), w32(at + 4, 0xffffffffu), w16(at + 8, 100), w16(at + 0xa, 2001), w16(at + 0x10, 200), w16(at + 0x12, 2001), w8(at + 0x18, 1), w8(at + 0x19, 0)};
}
std::vector<Write> counted_a(const World& w, std::uint32_t count) { return {w32(w.regen + 8, count)}; }

// The answers that steer a decision to retire a player of age 46 with a club (the competition's row, the date distance).
std::vector<std::pair<std::uint32_t, std::uint32_t>> retiring_answers(std::uint32_t competition = 5) {
    return {{competition_of_club, competition}, {competition_of_nation, competition}, {days_between, 100}};
}
}

// ---- the scans ------------------------------------------------------------------------------------------------------------------------
PLAYER_REGEN_CASES("lifecycle") {
    // The scans first (the call trace pairs an entry's first 32 calls): real people unedited over several windows, then each filter.
    const World w(scenario);
    const Ring ring(scenario);
    for (const auto entry : {scan_daily, scan_initial})
        for (const std::uint32_t size : {0u, 1u, 3u, 40u, 120u}) {
            auto memory = w.window(size);
            cases.push_back({.entry = entry, .registers = {{3, w.regen}, {19, size}}, .memory = memory,
                             .label = std::string("lifecycle: ") + (entry == scan_daily ? "daily" : "initial") + " scan, real people 0.." + number(size)});
        }
}

PLAYER_REGEN_CASES("lifecycle") {
    const World w(scenario);
    std::mt19937 random(9001);
    const std::array<std::uint32_t, 10> ages{20, 29, 30, 31, 40, 46, 0x80, 0xff, 0x1d, 0x1e};
    // The filters of the daily scan, one person at a time among 10 neighbours: age, link mark, id, class, queued, contract flag.
    for (std::uint32_t variant = 0; variant < 80; ++variant) {
        const auto entry = (variant & 1) ? scan_initial : scan_daily;
        const auto size = 12u;
        auto memory = w.window(size);
        const auto pick = w.with_link[variant % w.with_link.size()];
        // Window persons 0..11: the person under test is index 3 (the others keep their real data: some retire, some are skipped).
        const auto person = w.person(3);
        const auto club = w.s.r32(w.person(pick) + 0x24);
        const auto age = ages[(variant / 2) % ages.size()];
        append(memory, {w8(person + 0x5d, age), w32(person + 0x24, club), w32(club + 0x30, link_at), w32(link_at, 5 + variant % 3),
                        w32(w.link_table + (5 + variant % 3) * 4, (variant / 20) % 2 ? 1u : 0u)});
        if (variant % 5 == 1) append(memory, {w32(person, size + 2)});                    // the id test
        if (variant % 5 == 2) append(memory, {w8(w.kind_row(person), 1 << 2)});            // the class
        if (variant % 5 == 3) { append(memory, queued(w, 0, w.s.r32(person))); append(memory, counted_a(w, 1)); }  // already queued
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
        if (variant % 7 == 4) {                                                            // the contract record's flag
            append(memory, {w8(record_at + 0x54, (variant / 14) % 2 ? 0x80u : 0u), w8(record_at + 0x44, 10), w8(record_at + 0x45, 0), w32(record_at + 0x50, 0)});
            answers.push_back({contract_of, record_at});
        }
        cases.push_back({.entry = entry, .registers = {{3, w.regen}, {19, variant % 3 ? 0u : 5u}}, .memory = memory,
                         .label = std::string("lifecycle: ") + (entry == scan_daily ? "daily" : "initial") + " scan, filters " + number(variant), .answers = answers});
    }
    // The age byte at its boundaries and the signed extremes, the link table's word set and clear, each against the club's link missing.
    for (const std::uint32_t age : {0u, 29u, 30u, 31u, 0x7fu, 0x80u, 0xffu})
        for (const std::uint32_t link_state : {0u, 1u, 2u, 3u}) {
            auto memory = w.window(8);
            const auto person = w.person(2);
            const auto club = w.s.r32(w.person(w.with_link[1]) + 0x24);
            append(memory, {w8(person + 0x5d, age), w32(person + 0x24, link_state == 0 ? 0u : club), w32(club + 0x30, link_state == 2 ? 0u : link_at), w32(link_at, 9),
                            w32(w.link_table + 36, link_state == 3 ? 0u : 7u)});
            cases.push_back({.entry = scan_daily, .registers = {{3, w.regen}, {19, 0}}, .memory = memory,
                             .label = "lifecycle: daily scan, age " + hex(age) + " link state " + number(link_state)});
        }
}

PLAYER_REGEN_CASES("lifecycle") {
    // The caller's r19: a staff member with no job byte (+0x62 and +0x60 zero) takes the decisions' job from it. A person of the window made a staff
    // member of a real staff member's club (no player record, his staff record), over every job value and two draws.
    const World w(scenario);
    const Ring ring(scenario);
    const auto donor = w.person(w.staff_club[0]);
    for (const auto entry : {scan_daily, scan_initial})
        for (const std::uint32_t r19 : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 15u, 0x80u, 0xffu, 0x105u})
            for (const std::uint32_t sample : {1u, 40u}) {
                auto memory = w.window(8);
                const auto person = w.person(3);
                append(memory, {w32(person + 0x14, 0), w32(person + 0x1c, w.s.r32(donor + 0x1c)), w32(person + 0x24, w.s.r32(donor + 0x24)), w8(person + 0x62, 0),
                                w8(person + 0x60, 0), w8(person + 0x5d, 62), w8(w.kind_row(person), 0)});
                append(memory, ring.scripted(400, 0, std::vector<std::uint32_t>(30, sample)));
                cases.push_back({.entry = entry, .registers = {{3, w.regen}, {19, r19}}, .memory = memory,
                                 .label = std::string("lifecycle: ") + (entry == scan_daily ? "daily" : "initial") + " scan, a staff member with no job byte, r19 " + hex(r19) +
                                          " sample " + number(sample)});
            }
}

PLAYER_REGEN_CASES("lifecycle") {
    // Retirements: players of 46 with a club retire (the competition's answers), several of them (the sort), the unattached decision's draws, a mix.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(31);
    const auto date = w.date;
    for (const auto entry : {scan_daily, scan_initial})
        for (std::uint32_t variant = 0; variant < 24; ++variant) {
            const auto size = 10 + variant % 7;
            auto memory = w.window(size);
            std::uint32_t retiring = 0;
            for (std::uint32_t slot = 0; slot < size && retiring < 2 + variant % 4; ++slot) {
                const auto person = w.person(slot);
                if (w.s.r32(person + 0x14) == 0 || w.s.r32(person + 0x24) == 0) continue;
                append(memory, {w8(person + 0x5d, 46 + (variant + slot) % 3), w8(w.kind_row(person), 0)});
                ++retiring;
            }
            // Some entries already queued (a later one sorts among them), and the game's date moved.
            if (variant % 3 == 1) { append(memory, queued(w, 0, 50)); append(memory, queued(w, 1, 4)); append(memory, counted_a(w, 2)); }
            if (variant % 4 == 2) append(memory, {w16(date, 40 + variant * 13), w16(date + 2, 2001)});
            auto answers = retiring_answers(variant % 5 == 3 ? 0xffu : 5u);
            std::vector<Case::NthAnswer> nth;
            std::vector<std::uint32_t> samples;
            for (unsigned k = 0; k < 40; ++k) samples.push_back(random() & 0xffff);
            append(memory, ring.scripted(variant % 4 == 0 ? 3u : 400u, variant, samples));
            cases.push_back({.entry = entry, .registers = {{3, w.regen}, {19, variant % 4}}, .memory = memory,
                             .label = std::string("lifecycle: ") + (entry == scan_daily ? "daily" : "initial") + " scan, retirements " + number(variant), .answers = answers});
        }
    // The unattached decision (the daily scan's second question): free players and staff of the window, the pool giving draws below and above.
    for (std::uint32_t variant = 0; variant < 12 && !w.players_free.empty(); ++variant) {
        auto memory = w.window(30);
        for (std::uint32_t slot = 0; slot < 30; ++slot) {
            const auto person = w.person(slot);
            if (slot % 3 == 0) append(memory, {w32(person + 0x24, 0), w32(person + 0x20, 0), w8(person + 0x5d, 25 + (slot + variant) % 10)});
        }
        std::vector<std::uint32_t> samples;
        for (unsigned k = 0; k < 60; ++k) samples.push_back(variant % 2 ? 3 : (random() & 0xffff));
        append(memory, ring.scripted(variant % 3 == 0 ? 2u : 300u, 0, samples));
        append(memory, {w16(date, 10), w16(date + 2, 2003)});
        cases.push_back({.entry = scan_daily, .registers = {{3, w.regen}, {19, 0}}, .memory = memory,
                         .label = "lifecycle: daily scan, unattached persons " + number(variant), .nth_answers = {{protect_list, 1, 0}}});
    }
}

// ---- the destructor -------------------------------------------------------------------------------------------------------------------
PLAYER_REGEN_CASES("lifecycle") {
    const World w(scenario);
    const auto nation_count_cell = scenario.toc(nation_count_slot);
    const auto real_count = scenario.r32(nation_count_cell);
    cases.push_back({.entry = destruct, .registers = {{3, 0}, {4, 1}}, .label = "lifecycle: destructor, no object"});
    cases.push_back({.entry = destruct, .registers = {{3, 0}, {4, 0}}, .label = "lifecycle: destructor, no object, no flag"});
    cases.push_back({.entry = destruct, .registers = {{3, w.regen}, {4, 0}}, .label = "lifecycle: destructor, the real object, flag 0"});
    for (const std::uint32_t flag : {1u, 0x7fffu, 0x8000u, 0x8001u, 0xffffu, 0x10000u, 0x10001u, 0xffff0001u, 0x80000000u})
        cases.push_back({.entry = destruct, .registers = {{3, w.regen}, {4, flag}}, .label = "lifecycle: destructor, the real object, flag " + hex(flag)});
    // A copy of the object in scratch with each block missing and the array missing (the real blocks are freed through the copy).
    for (std::uint32_t missing = 0; missing < 16; ++missing) {
        std::vector<Write> memory;
        for (std::uint32_t at = 0; at < 0x48; at += 4) memory.push_back(w32(object_at + at, scenario.r32(w.regen + at)));
        for (std::uint32_t list = 0; list < 4; ++list)
            if (missing >> list & 1) memory.push_back(w32(object_at + (list == 0 ? 4u : list == 1 ? 0x10u : list == 2 ? 0x1cu : 0x3cu), 0));
        cases.push_back({.entry = destruct, .registers = {{3, object_at}, {4, 0}}, .memory = memory, .label = "lifecycle: destructor, copy, blocks missing " + number(missing)});
    }
    {
        std::vector<Write> memory;
        for (std::uint32_t at = 0; at < 0x48; at += 4) memory.push_back(w32(object_at + at, scenario.r32(w.regen + at)));
        memory.push_back(w32(object_at, 0));
        cases.push_back({.entry = destruct, .registers = {{3, object_at}, {4, 0}}, .memory = memory, .label = "lifecycle: destructor, copy, no per-nation array"});
    }
    // The nation count (the loop's bound is read each time): 0, 1, 2, a few, the real one, a negative count.
    for (const std::uint32_t count : {0u, 1u, 2u, 5u, real_count, real_count + 1, 0xffffffffu, 0x80000000u}) {
        std::vector<Write> memory{w32(nation_count_cell, count)};
        for (std::uint32_t at = 0; at < 0x48; at += 4) memory.push_back(w32(object_at + at, scenario.r32(w.regen + at)));
        cases.push_back({.entry = destruct, .registers = {{3, object_at}, {4, 0}}, .memory = memory, .label = "lifecycle: destructor, copy, nation count " + number(count)});
    }
}

// ---- the constructor ------------------------------------------------------------------------------------------------------------------
namespace {
Case::Archive archive_with(bool fail_open = false) {
    Case::Archive a;
    a.enabled = true; a.fail_open = fail_open;
    return a;
}
// retire.dat of four empty lists (the four count words, zero).
std::vector<Write> empty_file() { return {w32(archive_state + 0x18, 16)}; }
}

PLAYER_REGEN_CASES("lifecycle") {
    const World w(scenario);
    const auto window = w.window(10);
    // The data file is found at once, after one refusal, after two; the new game (the initial scan over the window, ten sentinel entries of list C)
    // and the load of retire.dat (the in-memory archive).
    const std::vector<Case::NthAnswer> none;
    const auto base = [&](const std::string& label, std::uint32_t archive, std::vector<Write> memory) {
        Case item{.entry = construct, .registers = {{3, object_at}, {4, archive}}, .memory = std::move(memory), .label = "lifecycle: constructor, " + label};
        item.lookups = {0};
        return item;
    };
    cases.push_back(base("new game", 0, window));
    {
        auto item = base("new game, file refused once", 0, window);
        item.lookups = {0xffffffd5u, 0};
        cases.push_back(item);
    }
    {
        auto item = base("new game, file refused twice", 0, window);
        item.lookups = {0xffffffd5u, 0x100u, 0};
        cases.push_back(item);
    }
    {
        auto item = base("new game, file answer with a zero low halfword is found", 0, window);
        item.lookups = {0x10000u};
        cases.push_back(item);
    }
    for (const std::uint32_t r19 : {0u, 5u, 0xfu}) {
        auto item = base("new game, r19 " + number(r19), 0, window);
        item.registers.push_back({19, r19});
        cases.push_back(item);
    }
    {
        auto memory = window;
        append(memory, empty_file());
        auto item = base("load retire.dat", archive_handle, memory);
        item.archive = archive_with();
        cases.push_back(item);
        auto failing = base("load retire.dat, the open fails", archive_handle, memory);
        failing.archive = archive_with(true);
        cases.push_back(failing);
        auto none_given = base("load, archive pointer 1", 1, memory);
        none_given.archive = archive_with();
        cases.push_back(none_given);
    }
    // Each allocation refused in turn (the first four allocations are the lists' blocks): the assertion of its line and the end of the game.
    for (std::uint32_t call = 1; call <= 4; ++call) {
        auto item = base("allocation " + number(call) + " refused", 0, window);
        item.nth_answers = {{allocator, call, 0}};
        item.original_owners = {allocator};
        item.pokes = {{0x10769b60, {}}};  // the report's dialog: traced, so the report's calls are paired
        cases.push_back(item);
        auto loading = base("allocation " + number(call) + " refused, loading", archive_handle, [&] { auto m = window; append(m, empty_file()); return m; }());
        loading.nth_answers = {{allocator, call, 0}};
        loading.original_owners = {allocator};
        loading.pokes = {{0x10769b60, {}}};
        loading.archive = archive_with();
        cases.push_back(loading);
    }
    // The staff row count the sentinels use (TOC -0x79f4): 1, 12, a large one.
    for (const std::uint32_t count : {1u, 12u, 0x10000u}) {
        auto memory = window;
        append(memory, {w32(scenario.toc(-0x79f4), count)});
        cases.push_back(base("new game, sentinel count " + number(count), 0, memory));
    }
    // The object memory holds leftovers.
    {
        auto memory = window;
        for (std::uint32_t at = 0; at < 0x60; at += 4) memory.push_back(w32(object_at + at, 0xa5a5a500u + at));
        cases.push_back(base("new game, the object's memory dirty", 0, memory));
    }
}
}
