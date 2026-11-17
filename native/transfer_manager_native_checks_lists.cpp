// Cases for transfer_manager_native_lists*.cpp: 0x103f9200, 0x103f3670 and 0x103f6240.
#include "transfer_manager_native_checks.hpp"
#include "guest_abi.hpp"
#include <algorithm>

namespace cm::aot::transfer_manager_native_checks {
namespace {
std::string name(const char* what, unsigned a, unsigned b = 0, unsigned c = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c);
}
std::uint32_t caller_sp(const Scenario& scenario) { return guest::Cpu(scenario.s).sp(); }
struct Lcg {
    std::uint32_t state;
    std::uint32_t next() { state = state * 1664525u + 1013904223u; return state >> 8; }
    std::uint32_t below(std::uint32_t n) { return n ? next() % n : 0; }
};
}

TRANSFER_MANAGER_NATIVE_CASES("lists") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto sp = caller_sp(scenario);
    const std::uint32_t r2 = 0x108bba90;
    const auto with_toc = [&](Case c) { c.registers.push_back({2, r2}); return c; };

    // ---- 0x103f9200: the first day the record's rule row accepts, a year from the start date. ----
    {
        const std::uint32_t record = scratch_base + 0x100, out = scratch_base + 0x140;
        std::vector<std::uint32_t> keys;
        for (unsigned i = 0; i < 27; i += 3) {
            const auto row = scenario.r32(manager + 0x9e0 + i * 4);
            if (row) keys.push_back(scenario.r32(row + 0xc));
        }
        keys.push_back(0xdeadbeef);  // no row matches: the first day is taken
        unsigned n = 0;
        for (const auto key : keys) {
            for (const std::uint32_t stale : {0u, 0x00112233u, 0xffffffffu}) {
                std::vector<Write> memory{{4, record + 0x2c, key}, {4, sp - 0xa0 + 0x5c, stale}};
                cases.push_back(with_toc({.entry = 0x103f9200, .registers = {{3, manager}, {4, record}, {5, out}}, .memory = memory,
                                          .label = name("first accepted day", n, stale & 0xff)}));
                if (key != 0xdeadbeef)  // the rule row refuses every day: the error report
                    cases.push_back(with_toc({.entry = 0x103f9200, .registers = {{3, manager}, {4, record}, {5, out}}, .memory = memory,
                                              .label = name("no accepted day", n, stale & 0xff), .answers = {{0x105c4d10, 0}}}));
            }
            ++n;
        }
    }

    // ---- 0x103f3670: dated entries between two days. ----
    {
        const auto table = scenario.r32(manager + 0x994);
        const auto count = scenario.r32(manager + 0x99c);
        std::vector<std::uint32_t> years;
        for (std::uint32_t i = 0; table && i < count && i < 100000; i += count / 6 + 1) years.push_back(scenario.r32(table + i * 0x28 + 0x20) & 0xffff);
        if (count) years.push_back(scenario.r32(table + (count - 1) * 0x28 + 0x20) & 0xffff);
        const std::uint32_t list_cell = scratch_base + 0x200, size_cell = scratch_base + 0x204, own_list = scratch_base + 0x1000;
        for (const auto year : years) {
            for (const std::uint32_t span : {0u, 1u, 3u}) {
                for (const std::uint32_t days : {1u, 5u, 20u, 0x1fu, 0xffu}) {
                    const auto to_year = year + span;
                    std::vector<Write> memory{{4, list_cell, 0}, {4, size_cell, 0}};
                    cases.push_back(with_toc({.entry = 0x103f3670,
                                              .registers = {{3, manager}, {4, days & 0x7f}, {5, year}, {6, (days * 3) & 0x7f}, {7, to_year}, {8, list_cell}, {9, size_cell}},
                                              .memory = memory, .label = name("dated entries", year, span, days),
                                              .answers = {{0x10739de0, 0}}}));
                }
            }
        }
        // The allocator answering "no" (the failure report, then the copy into the null pointer under --null-page).
        for (const auto year : years)
            cases.push_back(with_toc({.entry = 0x103f3670,
                                      .registers = {{3, manager}, {4, 1}, {5, year}, {6, 0x1c}, {7, year + 2}, {8, list_cell}, {9, size_cell}},
                                      .memory = {{4, list_cell, 0}, {4, size_cell, 0}}, .label = name("allocation fails", year),
                                      .answers = {{0x10739de0, 0}, {0x10775130, 0}}}));
        // A table of the case's own (sorted by date) in the manager's slot, so that the table loop has entries.
        {
            const std::uint32_t fake = scratch_base + 0x40000, fake_list = scratch_base + 0x42000;
            std::vector<Write> base{{4, manager + 0x994, fake}, {4, manager + 0x99c, 16}, {4, list_cell, 0}, {4, size_cell, 0}};
            for (std::uint32_t entry = 0; entry < 16; ++entry) {
                const auto at = fake + entry * 0x28;
                for (std::uint32_t word = 0; word < 0x28; word += 4) base.push_back({4, at + word, 0x11110000u + entry * 0x100 + word});
                base.push_back({2, at + 0x22, 2001 + entry / 6});
                base.push_back({2, at + 0x20, 1 + (entry % 6) * 20});
                base.push_back({1, at + 0x24, 0});
            }
            for (const std::uint32_t from_year : {2000u, 2001u, 2002u})
                for (const std::uint32_t to_year : {2001u, 2002u, 2003u})
                    for (const std::uint32_t from_day : {1u, 41u})
                        for (const std::uint32_t to_day : {1u, 60u, 120u}) {
                            auto memory = base;
                            cases.push_back(with_toc({.entry = 0x103f3670,
                                                      .registers = {{3, manager}, {4, from_day}, {5, from_year}, {6, to_day}, {7, to_year}, {8, list_cell}, {9, size_cell}},
                                                      .memory = memory, .label = name("own table", from_year, to_year, from_day * 1000 + to_day),
                                                      .answers = {{0x10739de0, 0}}}));
                        }
            // The allocator refusing the first allocation (answered); the growth of a full list calls the allocator's native
            // realloc, which a case cannot answer (and which faults on the case's own pointer, alike on both sides).
            cases.push_back(with_toc({.entry = 0x103f3670,
                                      .registers = {{3, manager}, {4, 1}, {5, 2001}, {6, 0x1c}, {7, 2002}, {8, list_cell}, {9, size_cell}},
                                      .memory = base, .label = name("own table allocation fails", 0),
                                      .answers = {{0x10739de0, 0}, {0x10775130, 0}}}));
            auto grown = base;
            grown[2] = {4, list_cell, fake_list};
            grown[3] = {4, size_cell, 2};
            for (std::uint32_t entry = 0; entry < 2; ++entry) {
                grown.push_back({2, fake_list + entry * 0x28 + 0x22, 1990});  // the list is older than the start day: not covered
                grown.push_back({2, fake_list + entry * 0x28 + 0x20, 5});
                grown.push_back({1, fake_list + entry * 0x28 + 0x24, 0});
            }
            cases.push_back(with_toc({.entry = 0x103f3670,
                                      .registers = {{3, manager}, {4, 1}, {5, 2001}, {6, 0x1c}, {7, 2002}, {8, list_cell}, {9, size_cell}},
                                      .memory = grown, .label = name("own table growth", 0),
                                      .answers = {{0x10739de0, 0}}}));
            // A list pointer with no capacity, and a list that starts after the start day (neither covers it).
            auto empty_capacity = grown;
            empty_capacity[3] = {4, size_cell, 0};
            cases.push_back(with_toc({.entry = 0x103f3670,
                                      .registers = {{3, manager}, {4, 1}, {5, 2001}, {6, 0x1c}, {7, 2002}, {8, list_cell}, {9, size_cell}},
                                      .memory = empty_capacity, .label = name("own list without capacity", 0), .answers = {{0x10739de0, 0}}}));
            auto later = grown;
            later[3] = {4, size_cell, 40};
            for (std::uint32_t entry = 0; entry < 2; ++entry) later.push_back({2, fake_list + entry * 0x28 + 0x22, 2005});
            cases.push_back(with_toc({.entry = 0x103f3670,
                                      .registers = {{3, manager}, {4, 1}, {5, 2001}, {6, 0x1c}, {7, 2002}, {8, list_cell}, {9, size_cell}},
                                      .memory = later, .label = name("own list starting later", 0), .answers = {{0x10739de0, 0}}}));
            // A list that covers the days (first entry before the start, last after the end).
            auto covered = grown;
            covered[3] = {4, size_cell, 3};
            for (std::uint32_t entry = 0; entry < 3; ++entry) {
                covered.push_back({2, fake_list + entry * 0x28 + 0x22, entry == 2 ? 2010u : 1990u});
                covered.push_back({2, fake_list + entry * 0x28 + 0x20, 5});
            }
            for (const std::uint32_t to_year : {2001u, 2002u})
                cases.push_back(with_toc({.entry = 0x103f3670,
                                          .registers = {{3, manager}, {4, 1}, {5, 2001}, {6, 0x1c}, {7, to_year}, {8, list_cell}, {9, size_cell}},
                                          .memory = covered, .label = name("own covered list", to_year),
                                          .answers = {{0x10739de0, 0}}}));
            // The yearly file: opened (a handle answered) with a FILE of the case's own (the C library's reads are
            // native routines a case cannot answer): the state and flags a read needs, and a buffer of records, which
            // fread copies from before it asks the OS for more. The records' years run past the end year (a stop), or
            // the buffer runs out (a refill through the FILE's own, unset, procedures).
            const std::uint32_t file = scratch_base + 0x44000, buffer = scratch_base + 0x44400;
            for (const unsigned kind : {0u, 1u, 2u, 3u, 4u}) {
                auto memory = base;
                memory.push_back({2, file + 4, 0x0060});
                memory.push_back({1, file + 8, 0x40});
                memory.push_back({1, file + 0xc, 0});
                memory.push_back({1, file + 0xd, 0});
                memory.push_back({4, file + 0x48, file + 0x100});  // the close procedure: a transition vector to `li r3,0; blr`
                memory.push_back({4, file + 0x100, 0x1077fa68});
                memory.push_back({4, file + 0x104, r2});
                const std::uint32_t records = kind == 0 ? 4 : kind == 1 ? 2 : kind == 4 ? 3 : 3;
                memory.push_back({4, file + 0x28, buffer});
                memory.push_back({4, file + 0x2c, records * 0x28});
                for (std::uint32_t entry = 0; entry < records; ++entry) {
                    const std::uint32_t year = kind == 2 ? (entry == 0 ? 2000u : 2001u) : kind == 4 ? (entry == 2 ? 2002u : 2001u) : 2001 + entry;
                    for (std::uint32_t word = 0; word < 0x28; word += 4) memory.push_back({4, buffer + entry * 0x28 + word, 0x22220000u + entry * 0x100 + word});
                    memory.push_back({2, buffer + entry * 0x28 + 0x22, year});
                    memory.push_back({2, buffer + entry * 0x28 + 0x20, kind == 4 ? 100 + entry * 100 : 1 + entry * 30});
                    memory.push_back({1, buffer + entry * 0x28 + 0x24, 0});
                }
                cases.push_back(with_toc({.entry = 0x103f3670,
                                          .registers = {{3, manager}, {4, 1}, {5, 2001}, {6, kind == 4 ? 1u : 0x1c + 5 * (kind & 1)}, {7, 2002}, {8, list_cell}, {9, size_cell}},
                                          .memory = memory, .label = name("own table yearly file", kind),
                                          .answers = {{0x10739de0, file}}}));
            }
        }
        // A list that is already there: rows copied from the table at its ends (covering it), and a size that stays.
        if (count >= 2) {
            std::vector<Write> memory{{4, list_cell, own_list}, {4, size_cell, 1000}};
            for (std::uint32_t entry = 0; entry < 1000; ++entry) {
                const auto from = table + std::min<std::uint32_t>(entry, count - 1) * 0x28;
                for (std::uint32_t at = 0; at < 0x28; at += 4) memory.push_back({4, own_list + entry * 0x28 + at, scenario.r32(from + at)});
            }
            const auto first_year = scenario.r32(table + 0x20) & 0xffff;
            for (const std::uint32_t span : {0u, 1u})
                for (const std::uint32_t day : {1u, 9u})
                    cases.push_back(with_toc({.entry = 0x103f3670,
                                              .registers = {{3, manager}, {4, day}, {5, first_year + 1}, {6, day + 2}, {7, first_year + 1 + span}, {8, list_cell}, {9, size_cell}},
                                              .memory = memory, .label = name("covered list", span, day)}));
        }
    }

    // ---- 0x103f6240: a contract's stage change. ----
    {
        const std::uint32_t flag_out = scratch_base + 0x300, profile = scratch_base + 0x340, injury = scratch_base + 0x380, finance = scratch_base + 0x400;
        std::vector<unsigned> holders;
        for (unsigned i = 0; i < scenario.people() && holders.size() < 6; i += scenario.people() / 40 + 1)
            if (scenario.contract_of(scenario.person(i))) holders.push_back(i);
        Lcg rng{0x6240};
        for (unsigned n = 0; n < 2400 && !holders.empty(); ++n) {
            const bool biased = n >= 900;
            const auto index = holders[rng.below(unsigned(holders.size()))];
            const auto person = scenario.person(index);
            const auto contract = scenario.contract_of(person);
            const auto club = scenario.club(scenario.r32(contract + 4));
            const std::int32_t stage = biased ? std::int32_t(rng.below(6)) + 1 : std::int32_t(rng.below(11)) - 2;  // -2..8
            const auto nibble = biased ? rng.below(std::uint32_t(stage) + 1) : rng.below(8);
            std::vector<Write> memory;
            const auto put = [&](std::uint32_t at, std::uint32_t value) { memory.push_back({1, at, value & 0xff}); };
            put(contract + 0x60, 0x10 | nibble);
            put(contract + 0x38, rng.below(3) == 0 ? 8 : rng.below(256));
            memory.push_back({4, contract + 0x50, rng.below(2) ? 0x4000u : 0u});
            put(contract + 0x4c, rng.below(256));
            put(contract + 0x44, rng.below(256));
            put(contract + 0x5c, rng.below(256));
            for (const std::uint32_t at : {0x5du, 0x63u, 0x65u, 0x67u})
                put(person + at, biased ? 1 + rng.below(at == 0x63 ? 40 : at == 0x5d ? 80 : 4) : rng.below(3) == 0 ? rng.below(256) : rng.below(24));
            memory.push_back({4, person + 8, rng.below(3) ? 1000u + rng.below(5000) : 0u});
            memory.push_back({4, person + 0x14, rng.below(3) ? profile : 0u});
            put(profile + 0xf, rng.below(32));
            put(profile + 0x15, rng.below(32));
            memory.push_back({2, injury + 0x34, rng.below(300)});
            memory.push_back({1, finance + 0x165, rng.below(2)});
            memory.push_back({4, scenario.toc(-0x7b90), rng.below(6)});
            Case c{.entry = 0x103f6240,
                   .registers = {{3, manager}, {4, person}, {5, club}, {6, rng.below(4000)}, {7, flag_out}, {8, std::uint32_t(stage)},
                                 {9, rng.below(8)}, {10, rng.below(3)}}};
            c.memory = memory;
            c.label = name("contract stage", index, std::uint32_t(stage), nibble);
            // The routine's own calls come first: answering the first dispatch (or two) leaves the shortlist code real.
            if (rng.below(biased ? 8 : 3) == 0) c.nth_answers.push_back({0x10103510, 1, finance});
            if (biased || rng.below(3)) {
                const auto days = rng.below(2) ? 40u : 10u;
                c.nth_answers.push_back({0x10718130, 1, days});
                c.nth_answers.push_back({0x10718130, 2, n % 17 == 0 ? 5u : days});
            }
            if (biased || rng.below(2)) c.nth_answers.push_back({0x1007c140, 1, rng.below(7)});
            if (rng.below(2)) c.nth_answers.push_back({0x1018c340, 1, rng.below(3) ? injury : 0u});
            if (n % 29 == 0) c.registers[2] = {5, scenario.club(1)};  // another club: no contract found
            cases.push_back(with_toc(c));
        }
        // The draws of the late-answer paths: the sample table is game memory, so the case lays out its own (every
        // sample the same value `v`, a seed of 0, cursor just before the first) and the draws follow from `v` and the
        // person's ranges: a draw below a range is v % range, so range 1 gives 0 (the roll fails) and range 4 with v 1
        // gives 1 (it passes).
        if (!holders.empty()) {
            const std::uint32_t samples = scratch_base + 0x50000;
            for (const std::uint32_t answer : {1u, 2u, 3u, 4u, 5u})
                for (const std::uint32_t v : {1u, 2u, 3u, 5u})
                    for (const std::uint32_t greed : {1u, 4u})
                        for (const std::uint32_t pride : {1u, 4u})
                            for (const std::uint32_t leniency : {5u, 20u})
                                for (const std::pair<std::int32_t, std::uint32_t> pair : {std::pair{4, 2u}, std::pair{3, 1u}, std::pair{2, 1u}}) {
                                    const auto index = holders[(answer + v) % holders.size()];
                                    const auto person = scenario.person(index);
                                    const auto contract = scenario.contract_of(person);
                                    const auto contract_club = scenario.club(scenario.r32(contract + 4));
                                    const auto club = (v == 2 && pride == 1 && greed == 4) ? 0u : contract_club;  // some without a club
                                    std::vector<Write> memory;
                                    const auto put = [&](std::uint32_t at, std::uint32_t value) { memory.push_back({1, at, value & 0xff}); };
                                    put(contract + 0x60, 0x10 | pair.second);
                                    put(contract + 0x38, 8);
                                    memory.push_back({4, contract + 0x50, 0x4000u});
                                    put(person + 0x5d, 1);
                                    put(person + 0x63, leniency);
                                    put(person + 0x65, pride);
                                    put(person + 0x67, greed);
                                    memory.push_back({4, person + 0x14, 0});
                                    memory.push_back({2, contract_club + 0x1ec, 0});
                                    memory.push_back({1, finance + 0x165, 0});
                                    for (std::uint32_t i = 0; i < 400; ++i) memory.push_back({2, samples + i * 2, v});
                                    memory.push_back({4, scenario.toc(-0x7b84), samples - 2});
                                    memory.push_back({4, scenario.toc(-0x7b88), samples + 2 * 399});
                                    memory.push_back({4, scenario.toc(-0x7b8c), samples});
                                    memory.push_back({4, scenario.toc(-0x7b90), 0});
                                    Case c{.entry = 0x103f6240,
                                           .registers = {{3, manager}, {4, person}, {5, club}, {6, 100}, {7, flag_out}, {8, std::uint32_t(pair.first)}, {9, 0}, {10, 0}}};
                                    c.memory = memory;
                                    c.label = name("contract stage draws", answer * 100 + v, greed * 10 + pride, leniency * 10 + std::uint32_t(pair.first));
                                    c.nth_answers = {{0x10103510, 1, finance}, {0x10718130, 1, 40}, {0x10718130, 2, 1000}, {0x1007c140, 1, answer}, {0x1018c340, 1, 0}};
                                    cases.push_back(with_toc(c));
                                }
        }
        // A club whose finance record is flagged (byte +0x165 = 1) and the amount test above the contract's worth.
        if (!holders.empty()) {
            const auto person = scenario.person(holders[0]);
            const auto contract = scenario.contract_of(person);
            const auto club = scenario.club(scenario.r32(contract + 4));
            for (const std::uint32_t worth : {0u, 1000000u})
                cases.push_back(with_toc({.entry = 0x103f6240,
                                          .registers = {{3, manager}, {4, person}, {5, club}, {6, 5000}, {7, flag_out}, {8, 2}, {9, 0}, {10, 1}},
                                          .memory = {{1, contract + 0x38, 8}, {4, person + 0x14, profile}, {4, person + 8, 77}, {1, finance + 0x165, 1}},
                                          .label = name("flagged club amount", worth),
                                          .nth_answers = {{0x10103510, 1, finance}, {0x10084be0, 1, worth}}}));
        }
    }
}
}
