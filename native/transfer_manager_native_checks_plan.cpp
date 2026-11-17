// Cases for transfer_manager_native_plan*.cpp: concluding an agreed offer and moving the player to the buyer.
#include "transfer_manager_native_checks.hpp"
#include "guest_abi.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>

namespace cm::aot::transfer_manager_native_checks {
namespace {
constexpr std::uint32_t table_of_contents = 0x108bba90;
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;
Registers with_toc(Registers registers) {
    registers.push_back({2, table_of_contents});
    return registers;
}
std::string label(const char* what, unsigned a, unsigned b = 0, unsigned c = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c);
}
struct RealOffer { std::uint32_t index, address; };
std::vector<RealOffer> real_offers(const Scenario& scenario, unsigned wanted) {
    std::vector<RealOffer> offers;
    const auto manager = scenario.transfer_manager();
    const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
    if (size == 0 || size > 100000) return offers;
    const auto step = size / wanted + 1;
    for (std::uint32_t i = 0; i < size; i += step)
        if (const auto offer = scenario.r32(array + i * 4)) offers.push_back({i, offer});
    return offers;
}
std::vector<std::uint32_t> listed_people(const Scenario& scenario, std::uint32_t offer) {
    std::vector<std::uint32_t> indexes;
    const auto list = scenario.r32(offer + 4);
    if (!list) return indexes;
    auto entry = scenario.r32(list + 4);
    for (unsigned guard = 0; entry && guard < 8; ++guard, entry = scenario.r32(entry + 8))
        if (const auto item = scenario.r32(entry)) indexes.push_back(scenario.r32(item));
    return indexes;
}
}

TRANSFER_MANAGER_NATIVE_CASES("plan") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto people = scenario.people();
    const auto offers = real_offers(scenario, 160);
    if (std::getenv("PLAN_DUMP")) {
        std::map<unsigned, unsigned> roles, with_record, with_club, with_staff;
        for (unsigned i = 0; i < people; ++i) {
            const auto p = scenario.person(i);
            const auto role = scenario.r32(p + 0x60) >> 8 & 0xffff;
            (void)role;
            const unsigned r = (scenario.r32(p + 0x60) >> 8) & 0xff;  // byte +0x61? print +0x62 below
            (void)r;
            const unsigned kind = (scenario.r32(p + 0x60) & 0xffff) >> 8;
            ++roles[kind];
            if (scenario.r32(p + 0x14)) ++with_record[kind];
            if (scenario.r32(p + 0x24)) ++with_club[kind];
            if (scenario.r32(p + 0x1c)) ++with_staff[kind];
        }
        for (const auto& [k, n] : roles) std::fprintf(stderr, "kind %u people %u record %u club %u staff %u\n", k, n, with_record[k], with_club[k], with_staff[k]);
    }
    // 0x103f2c00: the club, a person that meets all four tests (written into the person), and other kinds.
    for (unsigned c = 0; c < 8; ++c) {
        const auto club = scenario.club(c * 5 % 60);
        for (unsigned index = 0; index < people; index += people / 5 + 1)
            for (const std::uint32_t kind : {5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 0xfeu}) {
                const auto person = scenario.person(index);
                std::vector<Write> memory;
                if (kind % 3 != 1) {
                    memory.push_back({4, person + 0x24, 0});
                    const std::uint32_t wanted = kind == 13 ? 6 : kind == 15 ? 8 : kind;
                    memory.push_back({1, person + 0x62, wanted & 0xff});
                    memory.push_back({4, person + 0x1c, scenario.r32(person + 0x1c) ? scenario.r32(person + 0x1c) : person});
                    memory.push_back({4, person + 0xc, scenario.r32(club + 0x2c)});
                }
                cases.push_back({.entry = 0x103f2c00, .registers = with_toc({{3, manager}, {4, club}, {5, kind}}),
                                 .memory = memory, .label = label("pass on person", c, index, kind)});
            }
    }
    // A missing club: the assertion report.
    cases.push_back({.entry = 0x103f2c00, .registers = with_toc({{3, manager}, {4, 0}, {5, 8}}), .label = "pass on person, no club"});
    // 0x103edef0: the manager's rows of interest (+0xa6c, 0x10 each, count at +0xa70), the person's flag 0x20 and the club.
    const auto rows = scenario.r32(manager + 0xa6c), count = scenario.r32(manager + 0xa70);
    const auto flags = scenario.r32(manager + 0x9a0);
    std::vector<std::uint32_t> row_people;
    if (rows && count && count < 100000)
        for (std::uint32_t i = 0, step = count / 12 + 1; i < count; i += step) row_people.push_back(scenario.r32(rows + i * 0x10));
    for (unsigned c = 0; c < 6; ++c) {
        const auto club = c == 5 ? 0 : scenario.club(c * 9 % 60);
        for (const auto index : row_people)
            for (const bool flagged : {true, false}) {
                if (index >= people) continue;
                std::vector<Write> memory;
                const auto flag_at = flags + index * 10;
                memory.push_back({1, flag_at, (scenario.r32(flag_at) >> 24 & 0xdf) | (flagged ? 0x20u : 0u)});
                cases.push_back({.entry = 0x103edef0, .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, club}}),
                                 .memory = memory, .label = label("update row", c, index, flagged)});
            }
    }
    for (unsigned index = 0; index < people; index += people / 9 + 1)
        for (unsigned c = 0; c < 4; ++c) {
            const auto club = scenario.club(c * 7 % 60);
            cases.push_back({.entry = 0x103edef0, .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, club}}),
                             .label = label("update row, any person", index, c)});
        }
    // Rows found for a person with the flag set: the row's club differs from the new club's record, the clubs' kind
    // bytes (+0x118) take every combination, and the new club's record is missing.
    for (std::uint32_t i = 0, step = count / 12 + 1; rows && i < count && count < 100000; i += step) {
        const auto row = rows + i * 0x10, index = scenario.r32(row);
        if (index >= people) continue;
        for (const unsigned which : {3u, 7u, 11u, 17u}) {
            const auto club = scenario.club((i + which) % 60);
            const auto row_club = scenario.global(-0x7b98) + (scenario.r32(row + 0xc) + 1) * 0x128;
            for (const std::uint32_t kinds : {0u, 2u}) {
                std::vector<Write> memory{{1, flags + index * 10, 0x20}, {4, row + 0xc, scenario.r32(row + 0xc) + 1}};
                memory.push_back({1, row_club + 0x118, (kinds & 2) ? 0xeu : 0u});
                Case one{.entry = 0x103edef0, .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, club}}),
                         .memory = memory, .label = label("update row, kinds", unsigned(i), which, kinds)};
                cases.push_back(one);
            }
        }
    }
    // An index pointer of zero: the original reads through the null page.
    cases.push_back({.entry = 0x103edef0, .registers = with_toc({{3, manager}, {4, 0}, {5, scenario.club(2)}}), .label = "update row, no index"});
    cases.push_back({.entry = 0x103edef0, .registers = with_toc({{3, manager}, {4, 0}, {5, 0}}), .label = "update row, nothing"});
    // 0x103efaa0 and 0x103efec0: real offers, the offer's person and club, the listed people. The mover's club is
    // the person's own club where it has one (a contract with it exists), else the offer's club.
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto offer = offers[i].address;
        const auto person_index = scenario.r32(offer + 0xc), club_index = scenario.r32(offer + 8);
        if (person_index >= people) continue;
        const auto person = scenario.person(person_index);
        const auto club = scenario.club(club_index % 60);
        cases.push_back({.entry = 0x103efaa0, .registers = with_toc({{3, manager}, {4, offer}}), .label = label("conclude", unsigned(i))});
        if (i % 4 == 1) {
            cases.push_back({.entry = 0x103efaa0, .registers = with_toc({{3, manager}, {4, offer}}),
                             .memory = {{4, person + 0x1c, club}}, .label = label("conclude, staff record", unsigned(i))});
            cases.back().answers.push_back({0x10308350, 0});
            cases.back().answers.push_back({0x10385aa0, 0});
        }
        const auto own = scenario.r32(person + 0x24) ? scenario.r32(person + 0x24) : club;
        for (const std::uint32_t target : {own, club}) {
            if (target == club && own == club) continue;
            for (const std::uint32_t fee : {0u, 1000000u, 3000000u, 6000000u}) {
                std::vector<Write> memory{{4, offer + 0x20, fee}};
                cases.push_back({.entry = 0x103efec0, .registers = with_toc({{3, manager}, {4, person}, {5, target}, {6, offer}}),
                                 .memory = memory, .label = label("move", unsigned(i), fee / 1000000, target == own)});
            }
        }
        for (const auto index : listed_people(scenario, offer))
            if (index < people && scenario.r32(scenario.person(index) + 0x24))
                cases.push_back({.entry = 0x103efec0,
                                 .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, scenario.r32(scenario.person(index) + 0x24)}, {6, offer}}),
                                 .label = label("move listed", unsigned(i), index)});
    }
    // Routines of other modules answered (r3 on every dispatch) to steer the branches the snapshot's state does not take:
    // the player statistics archive (0x10308350, which asserts in this snapshot) is always answered.
    struct Steer { std::uint32_t entry; std::uint32_t values[2]; };
    const Steer steers[] = {{0x100c7a20, {0, 1}}, {0x1014c2e0, {0, 1}}, {0x10362d70, {0, 1}},
                            {0x10083110, {0, 1}}, {0x100ce320, {0, 1}}, {0x100d12d0, {0, 1}}, {0x1030fbf0, {0, 1}}, {0x100d3040, {0, 1}}};
    constexpr unsigned steer_count = sizeof steers / sizeof *steers;
    const auto steered = [&](Case base, unsigned mask, std::uint32_t club_value) {
        base.answers.push_back({0x10308350, 0});
        base.answers.push_back({0x101aa680, 0});
        if (base.entry == 0x103efaa0) base.answers.push_back({0x10385aa0, 0});
        for (unsigned k = 0; k < steer_count; ++k) {
            auto value = steers[k].values[(mask >> k) & 1];
            if (steers[k].entry == 0x100ce320 && value) value = club_value;
            base.answers.push_back({steers[k].entry, value});
        }
        base.label += " steer " + std::to_string(mask);
        return base;
    };
    const unsigned all_steers = (1u << steer_count) - 1;
    std::vector<unsigned> masks{0, all_steers};
    for (unsigned k = 0; k < steer_count; ++k) { masks.push_back(1u << k); masks.push_back(all_steers & ~(1u << k)); }
    const auto cursor_at_limit = Write{4, scenario.toc(-0x7b84), scenario.global(-0x7b88)};
    std::vector<Case> steered_cases;
    for (const auto& base : cases)
        if (base.entry == 0x103efaa0 || base.entry == 0x103efec0) {
            auto twin = base;
            twin.answers.push_back({0x10308350, 0});
            twin.label += " (stats answered)";
            steered_cases.push_back(twin);
        }
    cases.insert(cases.end(), steered_cases.begin(), steered_cases.end());
    // People of every role at their own club (their contract exists), with the first real offer.
    if (!offers.empty()) {
        const auto offer = offers[0].address;
        for (unsigned index = 0; index < people; index += people / 150 + 1) {
            const auto person = scenario.person(index);
            const auto own = scenario.r32(person + 0x24);
            if (!own) continue;
            cases.push_back({.entry = 0x103efec0, .registers = with_toc({{3, manager}, {4, person}, {5, own}, {6, offer}}),
                             .label = label("move any person", index)});
            auto twin = cases.back();
            twin.answers.push_back({0x10308350, 0});
            twin.label += " (stats answered)";
            cases.push_back(twin);
        }
    }
    // The steered moves: players of real offers at their own club, three fees, roles, the answer masks above, an owner
    // for the club (a staff record) so that a big fee draws, and a record in the club's table of open items that
    // names the person.
    std::uint32_t staff_person = 0;
    for (unsigned index = 0; index < people && !staff_person; ++index)
        if (scenario.r32(scenario.person(index) + 0x1c)) staff_person = scenario.person(index);
    const std::uint32_t roles[] = {0, 5, 0xc, 9, 1, 0xb};
    const auto items_table = scenario.r32(manager + 0x988) ? scenario.r32(scenario.r32(manager + 0x988) + 0xdc) : 0;
    const auto fake_one = scratch_base + 0x9000, fake_two = scratch_base + 0x9400, fake_three = scratch_base + 0x9800;
    for (std::size_t i = 0, n = 0; i < offers.size() && i < 30; i += 3) {
        const auto offer = offers[i].address;
        const auto person_index = scenario.r32(offer + 0xc);
        if (person_index >= people) continue;
        const auto person = scenario.person(person_index);
        const auto own = scenario.r32(person + 0x24);
        if (!own) continue;
        for (const std::uint32_t fee : {0u, 3000000u, 6000000u}) {
            std::vector<Write> memory{{4, offer + 0x20, fee}, {4, own + 0x80, staff_person}, {1, staff_person + 0x36, 100}};
            if ((i / 3 + fee / 3000000) % 2) memory.push_back(cursor_at_limit);
            if (fee == 6000000) memory.push_back({1, offer + 0x37, 1});
            // The club's open items: the first names this person (kind 0xb) and links to the person itself.
            if (items_table) {
                const auto items = scenario.r32(items_table + scenario.r32(own) * 0xc);
                if (items) {
                    memory.push_back({1, items, 0xb});
                    memory.push_back({4, items + 0x10, person_index});
                    memory.push_back({4, items + 0x14, person_index});
                }
            }
            for (const auto mask : masks) {
                ++n;
                auto with = memory;
                const auto role = roles[n % 6];
                if (role) with.push_back({1, person + 0x62, role});
                if (n % 4 == 0) { with.push_back({1, person + 0x60, 5}); with.push_back({4, person + 0x20, own}); }
                if (n % 8 == 4) { with.push_back({1, person + 0x60, 5}); with.push_back({4, person + 0x20, 0}); }
                if (n % 5 == 0) with.push_back({1, offer + 0x11, (mask & 4) ? 11u : 9u});
                if (n % 7 == 0) with.push_back({4, person + 0x24, scenario.club(1)});
                if (n % 11 == 0) with.push_back({4, person + 0x24, 0});
                if (n % 6 == 1) with.push_back({4, person + 0xc, scenario.r32(own + 0x2c)});
                if (n % 3 == 0) with.push_back({4, person + 0x1c, own});
                if (n % 9 == 0) with.push_back({1, person + 0x66, 0});
                if (n % 8 == 3) with.push_back({4, own + 0x90, 0});
                if (n % 10 == 7) with.push_back({4, scenario.toc(-0x7440), 0});
                if (n % 13 == 2) { with.push_back({1, offer + 0x37, 1}); with.push_back({4, offer + 0x20, 0}); with.push_back({1, offer + 0x38, (n % 2) ? 0x30u : 0x40u}); }
                if (n % 12 == 5) with.push_back({4, own + 0x80, person});
                if (items_table && n % 3 == 1) {
                    const auto items = scenario.r32(items_table + scenario.r32(own) * 0xc);
                    if (items) with.push_back({4, items + 0x14, n % 2 ? 0xffffffffu : (person_index + 1) % people});
                }
                Case base{.entry = 0x103efec0, .registers = with_toc({{3, manager}, {4, person}, {5, own}, {6, offer}}),
                          .memory = with, .label = label("move steered", unsigned(i), fee / 1000000, role)};
                cases.push_back(steered(base, mask, own));
                // The same with fake contract records (a kind-5 contract or not; grades, the next slot, mood bytes).
                for (const bool fifth : {false, true}) {
                    auto faked = with;
                    faked.push_back({1, fake_one + 0x38, fifth ? 0x14u : 0x04u});
                    faked.push_back({4, fake_one + 4, scenario.r32(own)});
                    faked.push_back({1, fake_two + 0x60, n % 3 == 0 ? 0u : (n % 3 == 1 ? 3u : 7u)});
                    faked.push_back({1, fake_two + 0x78, 3});
                    faked.push_back({4, fake_two + 0x7c, 5});
                    faked.push_back({1, fake_three + 0x44, n % 2 ? 60u : 0xb0u});
                    faked.push_back({1, fake_three + 0x45, n % 3 ? 2u : 9u});
                    Case twin{.entry = 0x103efec0, .registers = base.registers, .memory = faked,
                              .label = label("move fake contracts", unsigned(i), fee / 1000000, role)};
                    twin.answers.push_back({0x10082f20, fake_one});
                    twin.answers.push_back({0x1007f720, fake_two});
                    twin.answers.push_back({0x10082f90, fake_three});
                    cases.push_back(steered(twin, mask, own));
                }
            }
        }
    }
    // Concluding with lists built in the scratch area (a header whose +4 is the first entry; entries hold the
    // person at +0 and the next at +8), the first listed person the offer's own club's.
    for (std::size_t i = 0; i < offers.size() && i < 36; i += 3) {
        const auto offer = offers[i].address;
        const auto person_index = scenario.r32(offer + 0xc);
        if (person_index >= people) continue;
        const auto first = scenario.person((person_index + 1 + i) % people), second = scenario.person((person_index + 101 + i) % people);
        const auto header = scratch_base + 0x8000, entry_a = scratch_base + 0x8100, entry_b = scratch_base + 0x8200;
        std::vector<Write> memory{{4, offer + 4, header}, {4, header + 4, entry_a}, {4, header + 0xc, 0}, {4, entry_a, first},
                                  {4, entry_a + 8, entry_b}, {4, entry_b, second}, {4, entry_b + 8, 0}};
        if (i % 2) memory.push_back(cursor_at_limit);
        for (const auto mask : masks) {
            Case base{.entry = 0x103efaa0, .registers = with_toc({{3, manager}, {4, offer}}), .memory = memory,
                      .label = label("conclude listed", unsigned(i))};
            if (mask % 3 == 0) base.memory.push_back({4, scenario.person(person_index) + 0x1c, first});
            if (mask % 5 == 0) base.memory.push_back({4, offer + 0x20, 0});
            cases.push_back(steered(base, mask, scenario.r32(first + 0x24)));
            if (mask % 2 == 0) {  // the offer's person without a contract, or with a kind-5 one
                auto lost = base;
                lost.label = label("conclude listed, contract", unsigned(i), mask);
                lost.memory.push_back({1, fake_one + 0x38, (mask % 4) ? 0x14u : 0x04u});
                lost.memory.push_back({4, fake_one + 4, scenario.r32(scenario.r32(first + 0x24))});
                if (mask % 8) lost.answers.push_back({0x10082f20, fake_one});
                else lost.answers.push_back({0x10082f20, 0});
                cases.push_back(steered(lost, mask, scenario.r32(first + 0x24)));
            }
        }
    }
    // Gap closing: explicit combinations (every answer mask with 0x100c7a20 true, 0x1014c2e0 false/true) for players whose
    // club's open items name them, role 9, kind-11 offers, a manager record without staff record, a club row table gone.
    for (std::size_t i = 0, shown = 0; i < offers.size() && shown < 4; i += 5) {
        const auto offer = offers[i].address;
        const auto person_index = scenario.r32(offer + 0xc);
        if (person_index >= people) continue;
        const auto person = scenario.person(person_index);
        const auto own = scenario.r32(person + 0x24);
        if (!own || !scenario.r32(person + 0x14)) continue;
        ++shown;
        for (unsigned mask = 0; mask <= all_steers; ++mask) {
            std::vector<Write> memory{{4, offer + 0x20, 0}, {4, offer + 0x37, 0x01000000}};
            if (items_table) {
                const auto items = scenario.r32(items_table + scenario.r32(own) * 0xc);
                if (items) { memory.push_back({1, items, 0xb}); memory.push_back({4, items + 0x10, person_index}); memory.push_back({4, items + 0x14, person_index}); }
            }
            if (mask & 8) memory.push_back({1, person + 0x62, 9});
            if (mask & 16) memory.push_back({1, offer + 0x11, 11});
            if (mask & 32) memory.push_back({4, own + 0x90, scenario.person((person_index + 7) % people)});  // no staff record
            if (mask & 64) memory.push_back({1, offer + 0x38, 0x50});
            if (mask & 128) memory.push_back({1, offer + 0x38, 0x30});
            Case base{.entry = 0x103efec0, .registers = with_toc({{3, manager}, {4, person}, {5, own}, {6, offer}}),
                      .memory = memory, .label = label("move gaps", unsigned(i), mask)};
            cases.push_back(steered(base, (mask & 0x87) | 1, own));
            cases.push_back(steered(base, (mask & 0x87) | 3, own));
            auto faked = base;
            faked.label += " fake";
            faked.memory.push_back({1, fake_one + 0x38, (mask & 1) ? 0x14u : 0x04u});
            faked.memory.push_back({4, fake_one + 4, scenario.r32(own)});
            faked.memory.push_back({1, fake_two + 0x78, 3});
            faked.memory.push_back({4, fake_two + 0x7c, 5});
            faked.answers = {{0x10082f20, fake_one}, {0x1007f720, fake_two}};
            cases.push_back(steered(faked, (mask & 0x87) | 2, own));
            cases.push_back(steered(faked, (mask & 0x87) | 3, own));
        }
        // Big paid amounts (a bonus past the cap), a kind-11 offer with 0x10362d70 true, a person without a contract club.
        for (const std::uint32_t paid : {2000000u, 3000000u, 8000000u, 50000000u, 400000000u, 0x7fffffffu}) {
            Case base{.entry = 0x103efec0, .registers = with_toc({{3, manager}, {4, person}, {5, own}, {6, offer}}),
                      .memory = {{1, offer + 0x11, (paid / 1000) % 2 ? 11u : 5u}}, .label = label("move paid", unsigned(i), paid / 1000)};
            base.answers.push_back({0x10416710, paid});
            base.answers.push_back({0x10084be0, paid});
            for (const unsigned mask : {0x00u, 0x04u, 0x05u, 0x84u, 0xffu}) cases.push_back(steered(base, mask, own));
        }
        for (const unsigned mask : {0x00u, 0x04u, 0x05u, 0x0eu}) {
            Case base{.entry = 0x103efec0, .registers = with_toc({{3, manager}, {4, person}, {5, own}, {6, offer}}),
                      .memory = {{1, person + 0x62, 9}, {1, offer + 0x11, 5}}, .label = label("move no contract", unsigned(i), mask)};
            base.answers.push_back({0x10082f20, 0});
            cases.push_back(steered(base, mask, own));
        }
    }
    {
        const auto club0 = scenario.club(0);
        const auto table = scenario.r32(manager + 0x988) + 0xdc;
        const auto person = scenario.person(1 % people);
        Case base{.entry = 0x103efec0, .registers = with_toc({{3, manager}, {4, person}, {5, club0}, {6, offers.empty() ? 0 : offers[0].address}}),
                  .memory = {{4, table, 0}}, .label = "move, club 0 without item rows"};
        cases.push_back(steered(base, 0, club0));
        cases.push_back({.entry = 0x103f2c00, .registers = with_toc({{3, manager}, {4, scenario.club(2)}, {5, 8}}),
                         .memory = {{4, scenario.toc(-0x7be4), 0}}, .label = "pass on person, no people"});
    }
}
}
