// PLAYER_REGEN.CPP, block `retire_decision`: cases for the two scans' decisions (0x102dcb20, 0x102dd910), the unattached one (0x102de6e0)
// and the injury retirement (0x102de470). No capture reaches the first three, so everything starts from the career's real people
// (season-day: staff and players of every class, with clubs and without, in contracts) and edits their fields over the original's
// thresholds: the age byte (29..46, 127, -1), the job bytes (every case of the dispatch), the player record's position ratings (+0xf,
// +0x14, +0x15), the values (+4, +6, +0xa) across 1500, 3500, 5500 and the 0.85 ratio, the byte +0x3c (bound of the last draw), the
// club, national and nation links, the nation's flag (+0x120), the game's date, the competition rows, and the pool's position and
// samples (so each draw refills in turn and each remainder takes every outcome). The contract manager's holder and record
// (0x10082f20, 0x10082f90), the contract count (0x10374d10), the competition of a club or nation (0x10191fd0, 0x10191f80), the
// date distance (0x10718130) and the human manager's protect list (0x10362d70) are answered where the path needs a value the snapshot
// has no person for; the rest runs on the real state. The floating-point cases are the ones found by an exhaustive search over the
// inputs (the .local/rewrites/player_regen notes): single against double precision at 1500, 3500 and 5500, and the exact ratio.
// r19 takes several values (the job byte of a staff member with no job bytes). Labels start with "retire_decision:".
#include "player_regen_checks.hpp"
#include <array>
#include <cstdlib>
#include <cstdio>
#include <random>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t daily = 0x102dcb20, initial = 0x102dd910, unattached = 0x102de6e0, injury = 0x102de470;
constexpr std::int32_t regen_slot = -0x7b28, date_slot = -0x7b80, table_slot = -0x7b44, cursor_slot = -0x7b84, end_slot = -0x7b88,
                       salt_slot = -0x7b90, start_slot = -0x7b8c, link_table_slot = -0x7b5c;
constexpr std::uint32_t area = scratch_base + 0x50000, entry_at = area, holder_at = area + 0x100, contract_at = area + 0x200;
constexpr std::uint32_t manager_holder = 0x10082f20, manager_contract = 0x10082f90, contracts_left = 0x10374d10, days_between = 0x10718130,
                        competition_of_club = 0x10191fd0, competition_of_nation = 0x10191f80, protect_list = 0x10362d70;

struct Pool {
    std::uint32_t cursor_cell, end_cell, salt_cell, start, cursor, end;
    explicit Pool(const Scenario& s)
        : cursor_cell(s.toc(cursor_slot)), end_cell(s.toc(end_slot)), salt_cell(s.toc(salt_slot)), start(s.toc(start_slot)),
          cursor(s.r32(cursor_cell)), end(s.r32(end_cell)) {}
    // The cursor `before` draws short of the pool's end (0: the next draw refills), the salt, and the samples the next draws read.
    std::vector<Write> at(std::uint32_t before, std::uint32_t salt, const std::vector<std::uint32_t>& samples = {}) const {
        const auto position = end - 2 * before;
        std::vector<Write> memory{w32(cursor_cell, position), w32(salt_cell, salt)};
        std::uint32_t next = position;
        for (const auto sample : samples) {
            next += 2;
            memory.push_back(w16(next > end ? start : next, sample));
        }
        return memory;
    }
};

struct World {
    const Scenario& s;
    std::vector<std::uint32_t> staff_club, staff_free, players_club, players_free;
    explicit World(const Scenario& scenario) : s(scenario) {
        for (std::uint32_t n = 0; n + 16 < s.people(); ++n) {
            const auto p = s.person(n);
            const bool player = s.r32(p + 0x14) != 0, staff = s.r32(p + 0x1c) != 0, club = s.r32(p + 0x24) != 0;
            if (!player && staff) (club ? staff_club : staff_free).push_back(p);
            else if (player) (club ? players_club : players_free).push_back(p);
        }
    }
    static std::uint32_t at(const std::vector<std::uint32_t>& list, std::size_t n) { return list[(n * 2654435761u + 17) % list.size()]; }
};

std::vector<Write> entry_pattern() {
    std::vector<Write> memory;
    for (std::uint32_t k = 0; k < 0x1c; k += 4) memory.push_back(w32(entry_at + k, 0xa5a5a500u + k));
    return memory;
}
std::string number(std::size_t n) { return std::to_string(n); }
}

// The traced refill of every draw, first (only the first 32 calls of an entry are traced): each path takes its draws with the cursor
// short of the pool's end by the number of draws before the one whose refill is traced; the dates' draws come from a person who retires
// at once (age 46).
PLAYER_REGEN_CASES("retire_decision") {
    const World w(scenario);
    const Pool pool(scenario);
    if (w.staff_club.size() < 20 || w.staff_free.size() < 20 || w.players_club.size() < 50 || w.players_free.size() < 5)
        throw std::runtime_error("retire_decision: the career has too few people of a kind");
    const auto date = scenario.toc(date_slot);
    const auto table = scenario.toc(table_slot);
    const auto link_table = scenario.r32(scenario.toc(link_table_slot));
    const auto link = area + 0x300;
    std::size_t n = 0;
    for (const auto entry : {daily, initial}) {
        const std::string scan = entry == daily ? " daily" : " initial";
        // The job decision's four draws: jobs 4 (a), 8 (b), 5 with a club (c), 5 with none (d).
        for (const std::uint32_t job : {4u, 8u, 5u, 6u}) {
            const bool free = job == 6u;
            const auto person = World::at(free ? w.staff_free : w.staff_club, ++n);
            auto memory = pool.at(0, 7, {3, 9});
            append(memory, entry_pattern());
            append(memory, {w8(person + 0x62, job), w8(person + 0x60, 0), w8(person + 0x5d, 60), w32(person + 0x20, 0)});
            if (free) append(memory, {w32(person + 0x24, 0)});
            cases.push_back({.entry = entry, .registers = {{4, person}, {5, entry_at}, {19, 0}}, .memory = memory,
                             .label = "retire_decision: traced job draw " + number(job) + scan});
        }
        // The dates: a player of 46 with a club retires, the competition 5 (or none: 0xff).
        struct Fill { const char* what; std::uint32_t day, row_day; std::uint32_t before; bool row_flag; std::uint32_t comp; std::vector<std::uint32_t> samples; };
        const std::vector<Fill> fills{
            {"rem95", 100, 10, 0, false, 5, {4, 4, 4, 4}},
            {"rem60", 364, 10, 1, false, 5, {4, 8, 4, 4}},
            {"rem30", 100, 10, 1, false, 5, {4, 8, 4, 4}},
            {"rem30 second", 100, 364, 2, true, 5, {4, 8, 17, 4}},
            {"swap", 364, 10, 3, false, 5, {4, 8, 5, 4}},
            {"rem20", 100, 10, 0, false, 0xff, {4, 4}},
            {"rem10", 360, 10, 1, false, 0xff, {9, 4}}};
        for (const auto& fill : fills) {
            if (entry == initial && fill.comp == 0xff) continue;  // the initial scan dates both tomorrow
            const auto person = World::at(w.players_club, ++n + 40);
            auto memory = pool.at(fill.before, 0, fill.samples);
            append(memory, entry_pattern());
            append(memory, {w8(person + 0x5d, 46), w16(date, fill.day), w16(table + 5 * 0x4c + 0x16, fill.row_day), w8(table + 5 * 0x4c + 0x1c, fill.row_flag)});
            cases.push_back({.entry = entry, .registers = {{4, person}, {5, entry_at}, {19, 0}}, .memory = memory,
                             .answers = {{competition_of_club, fill.comp}}, .nth_answers = {{days_between, 1, 100}},
                             .label = std::string("retire_decision: traced dates ") + fill.what + scan});
        }
        // The unsettled path's mod-3 draw, the contract count path's rating draw and the club-free bound's draw.
        {
            const auto person = World::at(w.players_club, ++n + 90);
            const auto record = scenario.r32(person + 0x14);
            auto memory = pool.at(0, 1, {40001});
            append(memory, entry_pattern());
            append(memory, {w8(person + 0x5d, 36), w8(record + 0x0f, 20), w16(record + 0x0a, 3000), w16(record + 4, 50), w32(person + 0x1c, 0),
                            w32(scenario.r32(person + 0x24) + 0x30, 0), w8(contract_at + 0x44, 10), w8(contract_at + 0x45, 0), w32(contract_at + 0x50, 0)});
            cases.push_back({.entry = entry, .registers = {{4, person}, {5, entry_at}, {19, 0}}, .memory = memory,
                             .answers = {{manager_holder, holder_at}, {manager_contract, contract_at}, {contracts_left, 400}, {days_between, 10}},
                             .label = "retire_decision: traced mod 3 draw" + scan});
        }
        if (entry == daily) {
            const auto person = World::at(w.players_club, ++n + 140);
            const auto record = scenario.r32(person + 0x14);
            auto memory = pool.at(0, 1, {17});
            append(memory, entry_pattern());
            append(memory, {w8(person + 0x5d, 40), w8(record + 0x0f, 20), w16(record + 0x0a, 3000), w16(record + 4, 50), w32(person + 0x1c, 0), w8(person + 0x63, 100),
                            w8(contract_at + 0x44, 10), w8(contract_at + 0x45, 0), w32(contract_at + 0x50, 0)});
            cases.push_back({.entry = entry, .registers = {{4, person}, {5, entry_at}, {19, 0}}, .memory = memory,
                             .answers = {{manager_holder, holder_at}, {manager_contract, contract_at}, {contracts_left, 100}, {days_between, 10}},
                             .label = "retire_decision: traced rating draw"});
            const auto free_person = World::at(w.players_free, ++n);
            const auto free_record = scenario.r32(free_person + 0x14);
            auto bound_memory = pool.at(0, 0, {2000});
            append(bound_memory, entry_pattern());
            append(bound_memory, {w8(free_person + 0x5d, 36), w8(free_record + 0x0f, 20), w16(free_record + 0x0a, 3000), w16(free_record + 4, 50), w8(free_record + 0x3c, 40),
                                  w32(free_person + 0x0c, 0), w32(free_person + 0x1c, 0)});
            cases.push_back({.entry = entry, .registers = {{4, free_person}, {5, entry_at}, {19, 0}}, .memory = bound_memory,
                             .answers = {{manager_holder, holder_at}, {manager_contract, 0}, {days_between, 10}},
                             .label = "retire_decision: traced bound draw"});
        }
    }
    // 0x102dd910 returns at once for a person with no club; one whose club a callee clears (a poke at the contract record's lookup) reaches
    // the club-free branches the original also has: the job decision's free draw and the nation's competition.
    {
        const auto person = World::at(w.staff_club, 61);
        auto memory = pool.at(0, 7, {3, 9});
        append(memory, entry_pattern());
        append(memory, {w8(person + 0x62, 0), w8(person + 0x60, 0), w8(person + 0x5d, 60), w32(person + 0x20, 0)});
        cases.push_back({.entry = initial, .registers = {{4, person}, {5, entry_at}, {19, 5}}, .memory = memory,
                         .pokes = {{manager_contract, {w32(person + 0x24, 0)}}}, .label = "retire_decision: traced free job draw, club cleared by a callee"});
        const auto player = World::at(w.players_club, 62);
        auto fill = pool.at(0, 0, {4});
        append(fill, entry_pattern());
        append(fill, {w8(player + 0x5d, 46)});
        cases.push_back({.entry = initial, .registers = {{4, player}, {5, entry_at}, {19, 0}}, .memory = fill,
                         .answers = {{competition_of_nation, 5}}, .pokes = {{manager_contract, {w32(player + 0x24, 0)}}},
                         .label = "retire_decision: traced nation competition, club cleared by a callee"});
    }
    // 0x102de6e0: the two protect-list tests and its two draws.
    {
        const auto staff_person = World::at(w.staff_free, 5);
        const auto staff_record = scenario.r32(staff_person + 0x1c);
        auto memory = pool.at(0, 3, {5});
        append(memory, entry_pattern());
        append(memory, {w32(staff_person + 0x24, 0), w32(staff_person + 0x20, 0), w16(staff_record + 0x2a, 10)});
        cases.push_back({.entry = unattached, .registers = {{4, staff_person}, {5, entry_at}}, .memory = memory, .nth_answers = {{protect_list, 1, 0}},
                         .label = "retire_decision: traced unattached staff draw"});
        const auto person = World::at(w.players_free, 5);
        const auto record = scenario.r32(person + 0x14);
        auto player_memory = pool.at(0, 3, {5});
        append(player_memory, entry_pattern());
        append(player_memory, {w32(person + 0x24, 0), w32(person + 0x20, 0), w16(date, 100), w16(date + 2, 2001), w16(person + 0x4c, 1), w16(person + 0x4e, 1999), w16(record + 0x0a, 3000)});
        cases.push_back({.entry = unattached, .registers = {{4, person}, {5, entry_at}}, .memory = player_memory, .nth_answers = {{protect_list, 1, 0}, {protect_list, 2, 0}},
                         .label = "retire_decision: traced unattached player draw"});
    }
    (void)link_table; (void)link;
}

PLAYER_REGEN_CASES("retire_decision") {
    // Real people unedited, through both scans (the contract manager and the date routines really run).
    const World w(scenario);
    for (std::size_t n = 0; n < 60; ++n)
        for (const auto entry : {daily, initial}) {
            const auto& list = n % 4 == 0 ? w.staff_club : n % 4 == 1 ? w.players_club : n % 4 == 2 ? w.staff_free : w.players_free;
            const auto person = World::at(list, n);
            std::vector<Write> memory = entry_pattern();
            cases.push_back({.entry = entry, .registers = {{4, person}, {5, entry_at}, {19, std::uint32_t(n * 3)}}, .memory = memory,
                             .label = std::string("retire_decision: real person ") + number(n) + (entry == daily ? " daily" : " initial")});
        }
    for (std::size_t n = 0; n < 40; ++n) {
        const auto person = World::at(w.players_free, n);
        cases.push_back({.entry = unattached, .registers = {{4, person}, {5, entry_at}}, .memory = entry_pattern(), .label = "retire_decision: unattached real player " + number(n)});
    }
    for (std::size_t n = 0; n < 40; ++n) {
        const auto person = World::at(w.staff_free, n);
        cases.push_back({.entry = unattached, .registers = {{4, person}, {5, entry_at}}, .memory = entry_pattern(), .label = "retire_decision: unattached real staff " + number(n)});
    }
}

PLAYER_REGEN_CASES("retire_decision") {
    // The floating-point cases of 0x102dcb20 (found by exhaustive search): a contract record gives the age limit 27 (base 30 from the
    // position byte +0x15, minus one for a contract with word +0x50 and the +0x60 low nibble 7, minus two for a wage above
    // level * 750 + 7500 with the expiry year past the current one and level < 6); the value is the halfword +0xa. The single-precision
    // result straddles a threshold the double-precision one does not: -27505 at age 43 gives 5501 against 5500 (> 0x157c: no retirement,
    // no draw, against the draws below), -17500 at 43 (and -28000 at 42) gives 3500 against 3499 (the unsettled path's < 0xdac) and
    // -7500 at 43 gives 1500 against 1499 (the settled path's < 0x5dc). With the checker's mutations of the thresholds these also run.
    const World w(scenario);
    const Pool pool(scenario);
    const auto date = scenario.toc(date_slot);
    const auto year = scenario.u16(date + 2);
    struct Float { int age; int value; bool unsettled; const char* what; };
    const std::array<Float, 7> floats{{{43, -27505, true, "5501/5500 unsettled"}, {43, -27505, false, "5501/5500"}, {42, -28000, true, "3500/3499"}, {43, -17500, true, "3500/3499 b"},
                                       {43, -7500, false, "1500/1499"}, {41, -30000, false, "1500/1499 b"}, {42, -12000, false, "1500/1499 c"}}};
    for (std::size_t n = 0; n < floats.size(); ++n)
        for (const bool with_staff_flag : {false, true}) {
            (void)with_staff_flag;
            const auto person = World::at(w.players_club, n * 5 + 1);
            const auto record = scenario.r32(person + 0x14);
            const auto club = scenario.r32(person + 0x24);
            std::vector<Write> memory = entry_pattern();
            auto more = pool.at(3, 11, {40001, 5, 7});
            append(memory, more);
            append(memory, {w8(person + 0x5d, std::uint32_t(floats[n].age)), w8(record + 0x0f, 10), w8(record + 0x15, 15), w8(record + 0x14, 5),
                            w16(record + 0x0a, std::uint32_t(floats[n].value)), w16(record + 4, 50), w8(person + 0x65, 3), w32(person + 0x1c, 0),
                            w32(contract_at + 0x50, 1), w8(contract_at + 0x60, 7), w32(contract_at + 0x0c, 99999), w16(contract_at + 0x32, year + 5),
                            w8(contract_at + 0x44, 10), w8(contract_at + 0x45, 0)});
            // Unsettled: the club has no link (+0x30), so a draw modulo 3 follows (sample 40001 + salt 11: not a multiple of 3).
            if (floats[n].unsettled) append(memory, {w32(club + 0x30, 0)});
            else {
                // Settled: the club's link names a table entry that exists.
                const auto link = area + 0x300;
                append(memory, {w32(club + 0x30, link), w32(link, 0), w32(scenario.global(link_table_slot), scenario.r32(scenario.global(link_table_slot)) | 1)});
            }
            cases.push_back({.entry = daily, .registers = {{4, person}, {5, entry_at}, {19, 0}}, .memory = memory,
                             .answers = {{manager_holder, holder_at}, {manager_contract, contract_at}, {contracts_left, 400}, {days_between, 10},
                                         {competition_of_club, 5}},
                             .clear_fpscr = true, .label = std::string("retire_decision: single-precision value ") + floats[n].what + " " + number(n)});
        }
    // The exact ratio 0.85 of 0x102dcb20's comparison (+4 against 0.85 * +6, doubles): the cases around the equal and one off.
    for (const int six : {20, 40, 100, 7, 1000, 3333, -20, 0})
        for (const int delta : {-1, 0, 1}) {
            const auto person = World::at(w.players_club, six + 100);
            const auto record = scenario.r32(person + 0x14);
            const int four = six * 85 / 100 + delta;
            std::vector<Write> memory = entry_pattern();
            append(memory, pool.at(2, 3, {1, 2, 3}));
            append(memory, {w8(person + 0x5d, 36), w8(record + 0x0f, 10), w8(record + 0x15, 10), w8(record + 0x14, 10), w16(record + 6, std::uint32_t(six)),
                            w16(record + 4, std::uint32_t(four)), w16(record + 0x0a, 6000), w8(person + 0x63, 100), w32(person + 0x1c, 0), w8(contract_at + 0x44, 1),
                            w8(contract_at + 0x45, 0), w32(contract_at + 0x50, 0)});
            cases.push_back({.entry = daily, .registers = {{4, person}, {5, entry_at}, {19, 0}}, .memory = memory,
                             .answers = {{manager_holder, holder_at}, {manager_contract, contract_at}, {contracts_left, 100}, {days_between, 10}},
                             .label = "retire_decision: 0.85 ratio six " + number(std::size_t(std::abs(six))) + " delta " + std::to_string(delta)});
        }
    // 0x102dd910: age - limit = 10 (limit 31 from three position bytes under 15): 1.0 - 0.1 * 10 is exactly zero separately and
    // -5.55e-17 fused, so the truncation is inexact only fused (FPSCR XX); and the rest of the gaps.
    for (int gap = 1; gap <= 16; ++gap) {
        const auto person = World::at(w.players_club, 200 + std::size_t(gap));
        const auto record = scenario.r32(person + 0x14);
        std::vector<Write> memory = entry_pattern();
        append(memory, pool.at(3, 5, {7, 11, 13}));
        append(memory, {w8(person + 0x5d, std::uint32_t(31 + gap)), w8(record + 0x0f, 10), w8(record + 0x15, 10), w8(record + 0x14, 10),
                        w16(record + 0x0a, std::uint32_t(2000 + 777 * gap)), w16(record + 4, 40), w8(person + 0x1c, 0)});
        cases.push_back({.entry = initial, .registers = {{4, person}, {5, entry_at}, {19, 0}}, .memory = memory,
                         .answers = {{manager_holder, holder_at}, {manager_contract, contract_at}, {days_between, 10}, {competition_of_club, 5}},
                         .clear_fpscr = true, .label = "retire_decision: initial scan gap " + std::to_string(gap)});
    }
}

PLAYER_REGEN_CASES("retire_decision") {
    // 0x102dcb20 paths the random edits reach rarely: a player with no club on the settled path (a nation record absent) whose last draw is
    // below 25 times his byte +0x3c (a zero bound draws nothing; a sample that is a multiple of the bound gives 0; negative bounds); a player
    // with a club and a contract whose byte +0x44 is negative on the settled path; a person who is a player and staff (the contract-count
    // path retires him without a draw).
    const World w(scenario);
    const Pool pool(scenario);
    for (const std::int32_t byte : {0, 1, -1, 40, -40, 127, -128})
        for (const std::uint32_t sample : {2000u, 2001u, 0u}) {
            const auto person = World::at(w.players_free, std::size_t(byte + 200) + sample);
            const auto record = scenario.r32(person + 0x14);
            std::vector<Write> memory = entry_pattern();
            append(memory, pool.at(2, 0, {sample, sample, sample}));
            append(memory, {w8(person + 0x5d, 36), w8(record + 0x0f, 20), w16(record + 0x0a, 3000), w16(record + 4, 50), w8(record + 0x3c, std::uint32_t(byte)),
                            w32(person + 0x0c, 0), w32(person + 0x24, 0), w32(person + 0x1c, 0)});
            cases.push_back({.entry = daily, .registers = {{4, person}, {5, entry_at}, {19, 0}}, .memory = memory,
                             .answers = {{manager_holder, holder_at}, {manager_contract, 0}, {days_between, 10}, {competition_of_nation, 0xffffffffu}},
                             .label = "retire_decision: no club, settled, byte 3c " + std::to_string(byte) + " sample " + number(sample)});
        }
    for (const std::uint32_t byte_44 : {0x80u, 0xffu, 0x10u}) {
        const auto person = World::at(w.players_club, 300 + byte_44);
        const auto record = scenario.r32(person + 0x14);
        const auto club = scenario.r32(person + 0x24);
        const auto link = area + 0x300;
        std::vector<Write> memory = entry_pattern();
        append(memory, pool.at(2, 0, {1, 2, 3}));
        append(memory, {w8(person + 0x5d, 35), w8(record + 0x0f, 20), w16(record + 0x0a, 3000), w16(record + 4, 50), w32(person + 0x1c, 0), w32(club + 0x30, link),
                        w32(link, 0), w32(scenario.r32(scenario.toc(link_table_slot)), 1), w8(contract_at + 0x44, byte_44), w8(contract_at + 0x45, 0), w32(contract_at + 0x50, 0)});
        cases.push_back({.entry = daily, .registers = {{4, person}, {5, entry_at}, {19, 0}}, .memory = memory,
                         .answers = {{manager_holder, holder_at}, {manager_contract, contract_at}, {contracts_left, 400}, {days_between, 10}},
                         .label = "retire_decision: club, settled, contract byte 44 " + number(byte_44)});
    }
    for (const std::uint32_t staff_word : {0x2000u, 0u}) {
        const auto person = World::at(w.players_club, 321 + staff_word);
        const auto record = scenario.r32(person + 0x14);
        std::vector<Write> memory = entry_pattern();
        append(memory, pool.at(3, 5, {7, 7, 7}));
        append(memory, {w8(person + 0x5d, 40), w8(record + 0x0f, 20), w16(record + 0x0a, 3000), w32(person + 0x1c, staff_word), w8(contract_at + 0x44, 10),
                        w8(contract_at + 0x45, 0), w32(contract_at + 0x50, 0)});
        cases.push_back({.entry = daily, .registers = {{4, person}, {5, entry_at}, {19, 0}}, .memory = memory,
                         .answers = {{manager_holder, holder_at}, {manager_contract, contract_at}, {contracts_left, 100}, {days_between, 10}},
                         .label = "retire_decision: player with a staff record " + number(staff_word)});
    }
}

PLAYER_REGEN_CASES("retire_decision") {
    // Randomised edits over the branches of the two scans.
    const World w(scenario);
    const Pool pool(scenario);
    const auto date = scenario.toc(date_slot);
    const auto table = scenario.toc(table_slot);
    const auto link_table = scenario.global(link_table_slot);
    std::mt19937 random(20261122);
    const auto pick = [&](auto&& values) { return values[random() % values.size()]; };
    const std::array<std::uint32_t, 12> ages{29, 30, 31, 32, 33, 34, 35, 36, 40, 44, 45, 46};
    const std::array<std::uint32_t, 14> big_ages{20, 28, 29, 30, 33, 34, 35, 36, 37, 44, 45, 46, 127, 0xff};
    const std::array<std::uint32_t, 14> jobs{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0x80, 0xff};
    const std::array<std::uint32_t, 10> ratings{0, 10, 14, 15, 20, 0x14, 0x13, 0x7f, 0x80, 0xff};
    const std::array<std::int32_t, 16> values{0, 1, 9, 10, 11, 50, 80, 81, 100, 101, 1000, 5500, 6000, 3500, -1, -30000};
    const std::array<std::int32_t, 9> bytes_3c{0, 1, -1, 5, -5, 40, 127, -128, 100};
    const std::array<std::uint32_t, 8> days{1, 100, 300, 340, 346, 360, 364, 365};
    const std::array<std::uint32_t, 9> competitions{0xffffffffu, 0, 1, 5, 0x20, 0x21, 0x22, 0x80, 0x1ff};
    const std::array<std::uint32_t, 6> contract_counts{0, 100, 299, 300, 400, 0xffff8000u};
    const std::array<std::uint32_t, 8> distances{0, 10, 44, 45, 46, 65, 66, 400};
    std::size_t made = 0;
    for (std::size_t n = 0; n < 1400; ++n) {
        const bool initial_scan = n % 2 == 1;
        const auto kind = (n / 2) % 6;  // staff with club, player with club, staff free, player free, neither, player with national link only
        const auto& list = kind == 0 || kind == 4 ? w.staff_club : kind == 1 ? w.players_club : kind == 2 ? w.staff_free : kind == 3 ? w.players_free : w.players_club;
        const auto person = World::at(list, n);
        const auto record = scenario.r32(person + 0x14);
        std::vector<Write> memory = entry_pattern();
        append(memory, pool.at(random() % 6, random() % 65536, {std::uint32_t(random() % 65536), std::uint32_t(random() % 65536), std::uint32_t(random() % 65536),
                                                                std::uint32_t(random() % 65536), std::uint32_t(random() % 65536)}));
        std::string what = "kind " + number(kind);
        append(memory, {w8(person + 0x5d, kind == 0 || kind == 2 ? pick(big_ages) : pick(ages))});
        if (kind == 4) append(memory, {w32(person + 0x14, 0), w32(person + 0x1c, 0)});
        if (kind == 5) append(memory, {w32(person + 0x24, 0), w32(person + 0x20, scenario.r32(scenario.global(-0x7be8) + 0x10))});
        if (kind == 0 || kind == 2) append(memory, {w8(person + 0x62, pick(jobs)), w8(person + 0x60, pick(jobs)), w32(person + 0x20, random() % 3 == 0 ? 0x1000 : 0)});
        if (record != 0) {
            append(memory, {w8(record + 0x0f, pick(ratings)), w8(record + 0x14, pick(ratings)), w8(record + 0x15, pick(ratings)),
                            w16(record + 4, std::uint32_t(pick(values))), w16(record + 6, std::uint32_t(pick(values))), w16(record + 0x0a, std::uint32_t(pick(values))),
                            w8(record + 0x3c, std::uint32_t(pick(bytes_3c)))});
        }
        append(memory, {w8(person + 0x63, pick(ratings)), w8(person + 0x65, random() % 12), w32(person + 0x4c, (pick(days) << 16) | (2000 + random() % 3))});
        // The game's date and the nation's flag.
        append(memory, {w16(date, pick(days)), w16(date + 2, 2000 + random() % 3)});
        const auto nation = scenario.r32(person + 0x0c);
        if (nation != 0) append(memory, {w8(nation + 0x120, random() % 2)});
        if (random() % 9 == 0) append(memory, {w32(person + 0x0c, 0)});
        if (random() % 5 == 0) append(memory, {w32(person + 0x24, 0)});
        // The link table: the club's link names a table entry that exists or not.
        const auto club = scenario.r32(person + 0x24);
        if (club != 0) {
            const auto link = area + 0x300;
            if (random() % 3 == 0) append(memory, {w32(club + 0x30, 0)});
            else append(memory, {w32(club + 0x30, link), w32(link, 0), w32(link_table, random() % 2)});
        }
        // The competition rows the dates are drawn from.
        for (std::uint32_t row = 0; row < 3; ++row)
            append(memory, {w16(table + row * 0x4c + 0x16, pick(days)), w8(table + row * 0x4c + 0x1c, random() % 2)});
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
        std::vector<Case::NthAnswer> nth;
        if (random() % 4 != 0) {
            // A contract record of scratch memory (or none).
            const bool none = random() % 6 == 0;
            append(memory, {w32(holder_at + 4, 0), w32(contract_at + 0x0c, random() % 2 ? 99999 : random() % 20000), w16(contract_at + 0x32, 2000 + random() % 6),
                            w8(contract_at + 0x44, pick(ratings)), w8(contract_at + 0x45, pick(ratings)), w32(contract_at + 0x50, random() % 2),
                            w8(contract_at + 0x60, random() % 3 == 0 ? 7 : random() % 16)});
            answers.push_back({manager_holder, random() % 8 == 0 ? 0u : holder_at});
            answers.push_back({manager_contract, none ? 0u : contract_at});
            answers.push_back({contracts_left, pick(contract_counts)});
            what += none ? ", no contract" : ", contract";
        }
        if (random() % 3 == 0) {
            answers.push_back({competition_of_club, pick(competitions)});
            answers.push_back({competition_of_nation, pick(competitions)});
        }
        if (random() % 2 == 0) {
            nth.push_back({days_between, 1, pick(distances)});
            nth.push_back({days_between, 2, pick(distances)});
        }
        cases.push_back({.entry = initial_scan ? initial : daily, .registers = {{4, person}, {5, entry_at}, {19, pick(jobs) | (random() % 2 ? 0x100u : 0u)}},
                         .memory = memory, .nth_answers = nth, .answers = answers,
                         .label = "retire_decision: random " + number(n) + (initial_scan ? " initial " : " daily ") + what});
        ++made;
    }
    // r19 on its own: a staff member with no job bytes at all takes the caller's register (every dispatch case).
    for (const std::uint32_t r19 : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 0x7fu, 0x80u, 0xffu, 0x104u, 0x13u, 0x5a131313u, 0xffffff04u, 0x00000408u})
        for (const auto entry : {daily, initial}) {
            const auto person = World::at(w.staff_club, r19 + 3);
            std::vector<Write> memory = entry_pattern();
            append(memory, pool.at(r19 % 3, 100 + r19, {50000, 3, 9, 27}));
            append(memory, {w8(person + 0x62, 0), w8(person + 0x60, 0), w8(person + 0x5d, 50), w32(person + 0x20, r19 % 2 ? 0x2000 : 0)});
            cases.push_back({.entry = entry, .registers = {{4, person}, {5, entry_at}, {19, r19}}, .memory = memory,
                             .label = "retire_decision: r19 " + std::to_string(r19) + (entry == daily ? " daily" : " initial")});
        }
    (void)made;
}

PLAYER_REGEN_CASES("retire_decision") {
    // 0x102de6e0: the unattached.
    const World w(scenario);
    const Pool pool(scenario);
    const auto date = scenario.toc(date_slot);
    std::mt19937 random(7771);
    const auto pick = [&](auto&& values) { return values[random() % values.size()]; };
    const std::array<std::int32_t, 10> abilities{0, 1, 749, 750, 751, 10000, -5, 0x7fff, -0x8000, 300};
    const std::array<std::int32_t, 6> staff_values{0, 49, 50, 51, -1, 25};
    for (std::size_t n = 0; n < 500; ++n) {
        const auto kind = n % 5;  // free player, free staff, player with club, staff with club, neither
        const auto& list = kind == 0 ? w.players_free : kind == 1 ? w.staff_free : kind == 2 ? w.players_club : w.staff_club;
        const auto person = World::at(list, n);
        const auto record = scenario.r32(person + 0x14);
        const auto staff = scenario.r32(person + 0x1c);
        std::vector<Write> memory = entry_pattern();
        append(memory, pool.at(random() % 4, random() % 65536, {std::uint32_t(random() % 65536), std::uint32_t(random() % 65536)}));
        append(memory, {w16(date, 100 + random() % 250), w16(date + 2, 2000 + random() % 3), w16(person + 0x4c, random() % 366), w16(person + 0x4e, 1999 + random() % 4)});
        if (kind == 4) append(memory, {w32(person + 0x14, 0), w32(person + 0x1c, 0)});
        if (kind == 0) append(memory, {w32(person + 0x20, random() % 3 == 0 ? 0x1000 : 0)});
        if (kind == 1 || kind == 3) append(memory, {w32(person + 0x20, random() % 3 == 0 ? 0x1000 : 0)});
        if (record != 0) append(memory, {w16(record + 0x0a, std::uint32_t(pick(abilities)))});
        if (staff != 0) append(memory, {w16(staff + 0x2a, std::uint32_t(pick(staff_values)))});
        std::vector<Case::NthAnswer> nth;
        nth.push_back({protect_list, 1, random() % 8 == 0});
        nth.push_back({protect_list, 2, random() % 8 == 0});
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
        cases.push_back({.entry = unattached, .registers = {{4, person}, {5, entry_at}}, .memory = memory, .nth_answers = nth,
                         .label = "retire_decision: unattached random " + number(n) + " kind " + number(kind)});
    }
}

PLAYER_REGEN_CASES("retire_decision") {
    // 0x102de470: the injury retirement of a person (the regen object is the career's own). The days: halves of every size and sign; an
    // existing list A entry of the person (removed first) or none; a full list (the append grows it, 0x107751b0 answered once with a
    // block and once with none).
    const World w(scenario);
    const Pool pool(scenario);
    const auto regen = scenario.global(regen_slot);
    const auto list = scenario.r32(regen + 4);
    std::mt19937 random(311);
    const std::array<std::uint32_t, 16> day_values{0, 1, 2, 3, 4, 5, 40, 100, 32767, 0xffff, 0xfffe, 0x8000, 0x18000, 0x10001, 7, 0xfffffff0u};
    std::size_t n = 0;
    for (const auto days : day_values)
        for (const std::uint32_t before : {0u, 1u, 3u}) {
            const auto person = World::at(random() % 2 ? w.players_club : w.staff_club, n);
            std::vector<Write> memory = pool.at(before, random() % 65536, {std::uint32_t(random() % 65536), std::uint32_t(random() % 65536)});
            if (n % 3 == 0) {
                // An entry of the person in list A (the first slot), so it is removed and the appended one replaces it.
                append(memory, {w32(regen + 8, 1), w32(list, scenario.r32(person)), w32(list + 4, 0xffffffffu), w16(list + 0x18, 0x0800)});
            }
            cases.push_back({.entry = injury, .registers = {{3, regen}, {4, person}, {5, days}}, .memory = memory,
                             .label = "retire_decision: injury days " + std::to_string(days) + " position " + number(before) + (n % 3 == 0 ? " replaced" : "")});
            ++n;
        }
    const auto person = World::at(w.players_club, 77);
    const auto capacity = scenario.r32(regen + 0xc);
    cases.push_back({.entry = injury, .registers = {{3, regen}, {4, person}, {5, 40}}, .memory = {w32(regen + 8, capacity - 1)},
                     .label = "retire_decision: injury, list A full (growth)"});
    cases.push_back({.entry = injury, .registers = {{3, regen}, {4, person}, {5, 40}}, .memory = {w32(regen + 8, capacity - 1)},
                     .answers = {{0x107751b0, 0}}, .original_owners = {0x107751b0}, .label = "retire_decision: injury, list A full, the allocator gives no block"});
    cases.push_back({.entry = injury, .registers = {{3, regen}, {4, w.players_free.empty() ? person : World::at(w.players_free, 3)}, {5, 40}},
                     .label = "retire_decision: injury, person with no club"});
}
}
