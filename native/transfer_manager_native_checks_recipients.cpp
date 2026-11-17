// Cases for transfer_manager_native_recipients*.cpp: the news recipients (0x103fc710) and the routines around them.
#include "transfer_manager_native_checks.hpp"

namespace cm::aot::transfer_manager_native_checks {
namespace {
constexpr std::uint32_t table_of_contents = 0x108bba90;
constexpr std::uint32_t news_accepts = 0x102cdbf0;
std::string label(const char* what, unsigned a, unsigned b = 0, unsigned c = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c);
}
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;
std::vector<Write> operator+(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }
Registers with_toc(Registers registers) {
    registers.push_back({2, table_of_contents});
    return registers;
}
// The halfwords of a person's reputation record (+8, +0xa, +0xc) as the routine reads them: the player profile
// (person +0x14) when there is one, otherwise the staff record (+0x1c) at +0x28.
std::vector<Write> reputation(const Scenario& scenario, std::uint32_t person, std::uint32_t home, std::uint32_t current,
                              std::uint32_t world) {
    if (const auto profile = scenario.r32(person + 0x14))
        return {{2, profile + 8, home}, {2, profile + 0xa, current}, {2, profile + 0xc, world}};
    if (const auto staff = scenario.r32(person + 0x1c))
        return {{2, staff + 0x28, home}, {2, staff + 0x2a, current}, {2, staff + 0x2c, world}};
    return {};
}
}

TRANSFER_MANAGER_NATIVE_CASES("recipients") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto people = scenario.people();
    const auto out = scratch_base + 0x4000;
    const auto buyer = scratch_base + 0x5000, offer = scratch_base + 0x5100;
    const std::vector<Write> fixtures = {{4, buyer, 0x1234}, {4, offer + 0x24, 0x4321}};
    const std::uint32_t kinds_list[] = {0x2, 0x8, 0x10, 0x1a, 0x12, 0x210, 0x90, 0x290, 0x50, 0x250, 0xd0, 0x2d0, 0x1fa, 0x3fa};
    // 0x103fc710 over people spread over the table: every kind, with and without the news desk taking the clubs.
    unsigned n = 0;
    for (unsigned index = 0; index < people; index += people / 37 + 1, ++n)
        for (const auto kinds : kinds_list)
            for (const bool accepts : {true, false}) {
                if (!accepts && (n + kinds) % 3) continue;
                cases.push_back({.entry = 0x103fc710,
                                 .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, kinds}, {6, out}, {7, 0x14}}),
                                 .label = label("recipients", index, kinds, accepts)});
                if (accepts) cases.back().answers.push_back({news_accepts, 1});
            }
    // Capacities that fill up (the assertion report), a negative one and zero.
    for (unsigned index = 0; index < people; index += people / 23 + 1)
        for (const std::uint32_t capacity : {0u, 1u, 2u, 3u, 0xffu})
            for (const auto kinds : {0x1au, 0x10u, 0x210u, 0x8u}) {
                Case one{.entry = 0x103fc710,
                         .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, kinds}, {6, out}, {7, capacity}}),
                         .label = label("recipients small", index, kinds, capacity)};
                one.answers.push_back({news_accepts, 1});
                cases.push_back(one);
            }
    // Reputation tiers: the thresholds of both tiers, around and across them.
    struct Triple { std::uint32_t home, current, world; };
    const Triple triples[] = {{0x100, 0x100, 0x100},   {0x100, 0x1000, 0x100},  {0x1d00, 0x100, 0x100}, {0x100, 0x1d00, 0x100},
                              {0x2100, 0x100, 0x100},  {0x100, 0x2100, 0x100},  {0x2100, 0x2100, 0x100}, {0x100, 0x100, 0x2100},
                              {0x100, 0x100, 0x2329},  {0x100, 0x100, 0x2328},  {0x100, 0x100, 0x23f1},  {0x100, 0x100, 0x23f0},
                              {0x100, 0x100, 0x251d},  {0x100, 0x100, 0x251c},  {0x100, 0xc00, 0x100},   {0x100, 0xbb9, 0x100},
                              {0x100, 0x1a00, 0x100},  {0x1c21, 0x1c20, 0x100}, {0x1c20, 0x1c21, 0x100}, {0x206d, 0x206c, 0x100},
                              {0x206c, 0x206d, 0x100}, {0x1c20, 0x1c20, 0x206d}, {0x1c20, 0x1c20, 0x206c}, {0x100, 0x1388, 0x100},
                              {0x100, 0x1389, 0x100},  {0x100, 0x9c5, 0x100},   {0x100, 0x9c4, 0x100},   {0x100, 0x2edf, 0x100},
                              {0x100, 0xffff, 0x100},  {0x100, 0x7000, 0x100}};
    for (unsigned index = 0, done = 0; index < people && done < 14; index += people / 15 + 1, ++done)
        for (std::size_t t = 0; t < sizeof triples / sizeof *triples; ++t) {
            const auto person = scenario.person(index);
            const auto memory = reputation(scenario, person, triples[t].home, triples[t].current, triples[t].world);
            if (memory.empty()) continue;
            for (const auto kinds : {0x10u, 0x210u, 0xd0u, 0x2d0u, 0x50u, 0x250u, 0x90u, 0x290u}) {
                if ((t + kinds + index) % 2) continue;
                Case one{.entry = 0x103fc710,
                         .registers = with_toc({{3, manager}, {4, person}, {5, kinds}, {6, out}, {7, t % 5 == 4 ? 2u : 0x14u}}),
                         .memory = memory,
                         .label = label("recipients tier", index, unsigned(t), kinds)};
                cases.push_back(one);
            }
        }
    // The last 16 people of the table (the ones the derived and listed recipients scan): made players of clubs
    // (byte +0x6b, the club record at +0x24, the nation at +0xc), with distinct or repeated clubs and the nations and
    // nation groups of other real people, so every selection finds recipients and repeated clubs are removed.
    const auto tail_people = [&](unsigned variant, std::uint32_t nation_source, std::uint32_t other_source) {
        std::vector<Write> writes;
        for (unsigned i = 0; i < 16; ++i) {
            const auto p = scenario.person(people - 1 - i);
            const auto club_slot = variant == 1 ? (i / 2) : i;
            const auto source = (i % 3 == 0) ? nation_source : (i % 3 == 1 ? other_source : scenario.person((i * 977) % people));
            writes.push_back({1, p + 0x6b, variant == 2 && i % 4 == 3 ? 0u : 1u});
            const auto own_club = scenario.r32(nation_source + 0x24);
            writes.push_back({4, p + 0x24, (i % 5 == 4 && variant != 0) ? 0u : (i % 4 == 0 && own_club) ? own_club : scenario.club((club_slot * 11 + 3 + variant) % 200)});
            writes.push_back({4, p + 0xc, (variant == 2 && i % 4 == 1) ? 0u : scenario.r32(source + 0xc)});
        }
        return writes;
    };
    for (unsigned k = 0, index = 5; k < 18; ++k, index = (index + people / 17 + 3) % people) {
        const auto person = scenario.person(index);
        const auto other = scenario.person((index * 31 + 7) % people);
        const auto nation = scenario.r32(person + 0xc), other_nation = scenario.r32(other + 0xc);
        for (unsigned variant = 0; variant < 3; ++variant) {
            auto memory = tail_people(variant, person, other);
            if (nation && other_nation) {
                const bool swapped = k % 2;
                const auto club_nation = scenario.r32(person + 0x24) ? scenario.r32(scenario.r32(person + 0x24) + 0x2c) : 0;
                auto group_of = [&](std::uint32_t nation_record) { return nation_record ? scenario.r32(nation_record + 0x68) : 0u; };
                if (k % 4 >= 2 && club_nation) {
                    memory.push_back({4, scenario.toc(-0x7a10), group_of(swapped ? club_nation : other_nation)});
                    memory.push_back({4, scenario.toc(-0x7a1c), group_of(swapped ? other_nation : club_nation)});
                } else {
                    memory.push_back({4, scenario.toc(-0x7a10), scenario.r32((swapped ? other_nation : nation) + 0x68)});
                    memory.push_back({4, scenario.toc(-0x7a1c), scenario.r32((swapped ? nation : other_nation) + 0x68)});
                }
            }
            for (const auto kinds : {0x10u, 0x210u, 0x1au, 0x18u, 0x8u, 0xd0u, 0x2d0u, 0x1fau, 0x50u, 0x250u, 0x90u, 0x290u}) {
                for (const auto tier : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u}) {
                    if ((tier + kinds + k) % 3 == 0) continue;
                    // The tier is set through the person's reputation record (the thresholds of both tiers).
                    static const std::uint32_t worlds[] = {0x100, 0x2300, 0x2400, 0x2600, 0x100, 0x100, 0x100, 0x100};
                    static const std::uint32_t currents[] = {0x100, 0x100, 0x100, 0x100, 0x2100, 0xe00, 0x1800, 0x1b00};
                    auto written = memory;
                    // The class value of the person's club's league (+0x18 of the record at club +0x30), small, so the class test can pass.
                    if (const auto own_club = scenario.r32(person + 0x24))
                        if (const auto league = scenario.r32(own_club + 0x30)) written.push_back({2, league + 0x18, 1});
                    const auto rep = reputation(scenario, person, tier == 4 ? 0x2100 : 0x100, currents[tier], worlds[tier]);
                    written.insert(written.end(), rep.begin(), rep.end());
                    Case one{.entry = 0x103fc710,
                             .registers = with_toc({{3, manager}, {4, person}, {5, kinds}, {6, out}, {7, (k + tier) % 4 == 3 ? 2u : (k + tier) % 4 == 1 ? 1u : (k + tier) % 7 == 2 ? 0u : 0x14u}}),
                             .memory = written, .label = label("recipients tail", index, variant * 1000 + kinds, tier)};
                    one.answers.push_back({news_accepts, 1});
                    cases.push_back(one);
                }
            }
        }
    }
    // A fabricated contract found for the person (kind byte at +0x38: 0x14 is kind 5, 0x08 kind 2, 0x2c kind 11), its
    // club at +4: the member club lookups, the staff-contract fee and the news recipients behind them.
    const auto contract = scratch_base + 0x300;
    for (unsigned k = 0, index = 9; k < 10; ++k, index = (index + people / 9 + 5) % people)
        for (const std::uint32_t kind_byte : {0x14u, 0x08u, 0x2cu}) {
            const std::vector<Write> record = {{4, contract + 4, (k * 13 + 5) % 100}, {1, contract + 0x38, kind_byte}, {4, contract + 0xc, 12000 + k}};
            const auto person = scenario.person(index);
            for (const auto kinds : {0x10u, 0x1au, 0x210u, 0xd0u}) {
                Case one{.entry = 0x103fc710, .registers = with_toc({{3, manager}, {4, person}, {5, kinds}, {6, out}, {7, 0x14}}),
                         .memory = record, .label = label("recipients contract", index, kind_byte, kinds), .answers = {{0x10082f20, contract}}};
                one.answers.push_back({news_accepts, 1});
                cases.push_back(one);
            }
            for (const std::uint32_t fee : {0xffffffffu, 9000u}) {
                cases.push_back({.entry = 0x103e68c0,
                                 .registers = with_toc({{3, manager}, {4, person}, {5, buyer}, {6, 1}, {7, fee}, {8, 0}}),
                                 .memory = record + fixtures, .label = label("record contract", index, kind_byte, fee & 0xff),
                                 .answers = {{0x10082f20, contract}}});
            }
        }
    // The value tests: the pool's constants (doubles at +0x180, +0x188, +0x190, +0x198) set so that the player value
    // beats every bound (limit -1e9, scales 0), and the nation / staff-record nulls of the high reputation branch.
    {
        const auto pool = scenario.toc(-0x4ba0);
        const std::vector<Write> generous = {{4, pool + 0x180, 0xc1cdcd65}, {4, pool + 0x184, 0}, {4, pool + 0x190, 0xc1cdcd65},
                                             {4, pool + 0x194, 0}, {4, pool + 0x188, 0}, {4, pool + 0x18c, 0},
                                             {4, pool + 0x198, 0}, {4, pool + 0x19c, 0}};
        for (unsigned k = 0, index = 11; k < 12; ++k, index = (index + people / 11 + 7) % people)
            for (const auto current : {3500u, 5000u, 7000u})
                for (const auto kinds : {0x90u, 0xd0u, 0x290u, 0x2d0u, 0x50u, 0x250u}) {
                    const auto person = scenario.person(index);
                    auto memory = generous + reputation(scenario, person, 0x100, current, 0x100);
                    if (const auto own_club = scenario.r32(person + 0x24))
                        if (const auto league = scenario.r32(own_club + 0x30)) memory.push_back({2, league + 0x18, 1});
                    Case one{.entry = 0x103fc710, .registers = with_toc({{3, manager}, {4, person}, {5, kinds}, {6, out}, {7, 0x14}}),
                             .memory = memory, .label = label("recipients value", index, current, kinds)};
                    one.answers.push_back({news_accepts, 1});
                    cases.push_back(one);
                }
        for (unsigned k = 0, index = 13; k < 8; ++k, index = (index + people / 7 + 3) % people) {
            const auto person = scenario.person(index);
            const auto record = scenario.r32(person + 0x24);
            for (const auto kinds : {0x10u, 0x210u}) {
                auto base = reputation(scenario, person, 0x100, 0x100, kinds == 0x10 ? 0x2100u : 0x2400u);
                std::vector<Write> nation_null = base + std::vector<Write>{{4, person + 0xc, 0}};
                std::vector<Write> staff_null = base;
                if (record) staff_null.push_back({4, record + 0x2c, 0});
                for (const auto& memory : {nation_null, staff_null, base + std::vector<Write>{{4, person + 0x24, 0}}})
                    cases.push_back({.entry = 0x103fc710, .registers = with_toc({{3, manager}, {4, person}, {5, kinds}, {6, out}, {7, 0x14}}),
                                     .memory = memory, .label = label("recipients nulls", index, kinds)});
            }
            cases.push_back({.entry = 0x103e68c0, .registers = with_toc({{3, manager}, {4, person}, {5, buyer}, {6, 1}, {7, 100}, {8, 0}}),
                             .memory = std::vector<Write>{{4, person + 0x24, 0}} + fixtures, .label = label("record no link", index)});
            cases.push_back({.entry = 0x10410000, .registers = with_toc({{3, manager}, {4, person}, {5, buyer}, {6, 1}, {7, offer}}),
                             .memory = fixtures, .label = label("signing refused", index), .answers = {{news_accepts, 0}}});
            cases.push_back({.entry = 0x1040b470, .registers = with_toc({{3, manager}, {4, person}}),
                             .label = label("close no contract", index), .answers = {{0x10082f20, 0}, {news_accepts, 1}}});
            cases.push_back({.entry = 0x1040b470, .registers = with_toc({{3, manager}, {4, person}}),
                             .memory = std::vector<Write>{{4, scratch_base + 0x304, 5}, {1, scratch_base + 0x338, 0x14}},
                             .label = label("close kind5", index), .answers = {{0x10082f20, scratch_base + 0x300}, {news_accepts, 1}}});
        }
    }
    // ---- Coverage closing pass (gaps list). ----
    // Persons with and without a contract, from the real tables.
    std::vector<unsigned> contracted, uncontracted;
    for (unsigned index = 1; index < people && (contracted.size() < 12 || uncontracted.size() < 6); index += people / 200 + 1)
        (scenario.contract_of(scenario.person(index)) ? contracted : uncontracted).push_back(index);
    // 0x103fc710: the shortlist fills the buffer (capacity 0 and 1 on the tail fixture) and the news desk refuses a club.
    for (unsigned k = 0, index = 7; k < 10; ++k, index = (index + people / 9 + 3) % people) {
        const auto person = scenario.person(index);
        const auto other = scenario.person((index * 31 + 7) % people);
        for (unsigned variant = 0; variant < 3; ++variant)
            for (const std::uint32_t capacity : {0u, 1u, 2u})
                for (const bool accepts : {true, false}) {
                    Case one{.entry = 0x103fc710, .registers = with_toc({{3, manager}, {4, person}, {5, 0x18u + (k % 2) * 0x2}, {6, out}, {7, capacity}}),
                             .memory = tail_people(variant, person, other), .label = label("recipients listed", index, variant * 10 + capacity, accepts)};
                    if (accepts) one.answers.push_back({news_accepts, 1});
                    cases.push_back(one);
                    // The shortlist's staff service qualifies every person and no club is reserved: all of the tail is listed.
                    one.answers.push_back({0x1030fbf0, 1});
                    one.answers.push_back({0x1014c490, 0});
                    one.label += " qualified";
                    cases.push_back(one);
                }
    }
    // The nation-region and nation-group selections with people of no nation, a person of no nation, and the person's
    // own club among the tail (its rival record's region matching).
    for (unsigned k = 0, index = 15; k < 14; ++k, index = (index + people / 13 + 5) % people) {
        const auto person = scenario.person(index);
        const auto other = scenario.person((index * 17 + 3) % people);
        for (const auto kinds : {0x10u, 0x210u})
            for (const auto tier : {1u, 2u, 4u}) {
                static const std::uint32_t worlds[] = {0x100, 0x2300, 0x2400, 0x2600, 0x100};
                static const std::uint32_t currents[] = {0x100, 0x100, 0x100, 0x100, 0x2100};
                auto memory = tail_people(2, person, other);
                const auto rep = reputation(scenario, person, tier == 4 ? 0x2100 : 0x100, currents[tier], worlds[tier]);
                memory.insert(memory.end(), rep.begin(), rep.end());
                if (k % 2) memory.push_back({4, person + 0xc, 0});
                if (const auto club = scenario.r32(person + 0x24)) {
                    // k % 4 == 3: the tail's nation group in the first cell and the club's nation group in the second, with no
                    // person nation (r21 = 0), so the group test is passed through the rival club's nation (r20).
                    const bool rival_group = k % 4 == 3;
                    memory.push_back({4, scenario.toc(-0x7a10), scenario.r32(scenario.r32(rival_group ? other + 0xc : club + 0x2c) + 0x68)});
                    memory.push_back({4, scenario.toc(-0x7a1c), scenario.r32(scenario.r32(rival_group ? club + 0x2c : other + 0xc) + 0x68)});
                }
                Case one{.entry = 0x103fc710, .registers = with_toc({{3, manager}, {4, person}, {5, kinds}, {6, out}, {7, 0x14}}),
                         .memory = memory, .label = label("recipients nations", index, kinds, tier)};
                cases.push_back(one);
            }
    }
    // The value test passing with a current reputation of at most 3000.
    {
        const auto pool = scenario.toc(-0x4ba0);
        const std::vector<Write> generous = {{4, pool + 0x180, 0xc1cdcd65}, {4, pool + 0x184, 0}, {4, pool + 0x190, 0xc1cdcd65},
                                             {4, pool + 0x194, 0}, {4, pool + 0x188, 0}, {4, pool + 0x18c, 0}, {4, pool + 0x198, 0}, {4, pool + 0x19c, 0}};
        for (unsigned k = 0, index = 21; k < 6; ++k, index = (index + people / 5 + 9) % people)
            for (const auto current : {2000u, 3000u})
                for (const auto kinds : {0x90u, 0x290u}) {
                    const auto person = scenario.person(index);
                    Case one{.entry = 0x103fc710, .registers = with_toc({{3, manager}, {4, person}, {5, kinds}, {6, out}, {7, 0x14}}),
                             .memory = generous + reputation(scenario, person, 0x100, current, 0x100), .label = label("recipients low value", index, current, kinds)};
                    one.answers.push_back({news_accepts, 1});
                    cases.push_back(one);
                }
    }
    // 0x103e68c0: a history so large that the allocator is asked for 0xf0000000 bytes (0x5fffff9c + 100 rows of 0x28).
    for (const std::uint32_t rows_null : {0u, 1u})
        for (unsigned index = 5; index < people; index += people / 3 + 1) {
            std::vector<Write> memory = {{4, manager + 0x998, 0x05ffff9c}, {4, manager + 0x99c, 0x05ffff9c},
                                         {4, manager + 0x994, rows_null ? 0u : scenario.r32(manager + 0x994)}};
            cases.push_back({.entry = 0x103e68c0,
                             .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, buyer}, {6, 1}, {7, 100}, {8, 0}}),
                             .memory = memory + fixtures, .label = label("record huge", index, rows_null)});
        }
    // 0x1040b470: an offer whose person has no contract, a person with none, and a listed person with a contract at
    // the offer person's club (the offer is pointed at the listed person).
    {
        const auto listed = scratch_base + 0x7100, entry_cell = scratch_base + 0x7200, listed_cell = scratch_base + 0x7220, list = scratch_base + 0x7000;
        const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
        for (std::uint32_t k = 0, step = size / 14 + 1; size && size < 100000 && k < size; k += step) {
            const auto real = scenario.r32(array + k * 4);
            if (!real) continue;
            for (std::size_t v = 0; v < 3; ++v) {
                const auto holder = v == 0 && !uncontracted.empty() ? uncontracted[k % uncontracted.size()] : contracted.empty() ? 0 : contracted[k % contracted.size()];
                const auto subject = v == 2 && !uncontracted.empty() ? uncontracted[(k + 1) % uncontracted.size()] : holder;
                const std::vector<Write> fixture = {{4, real + 0xc, holder}, {4, real + 4, list}, {4, list + 4, listed}, {4, list + 0xc, 0},
                                                    {4, entry_cell, k}, {4, listed_cell, holder}, {1, listed_cell + 4, 1}, {4, real + 0x28, 0x555}};
                // The contract manager's pending list holds a record for the listed person at the offer person's club (found
                // for real by 0x1007f720): its kind byte is rewritten for the listed person.
                std::vector<Write> pending_record;
                if (const auto held = scenario.contract_of(scenario.person(holder))) {
                    const auto cm = scenario.global(-0x7b7c);
                    const auto pending_list = scratch_base + 0x7400, node = scratch_base + 0x7440, record = scratch_base + 0x7500;
                    pending_record = {{4, cm + 0x34, pending_list}, {2, pending_list, 1}, {4, pending_list + 4, node}, {4, pending_list + 0xc, 0}, {4, node, record},
                                      {4, node + 8, 0}, {4, record, scenario.r32(scenario.person(holder))},
                                      {4, record + 4, scenario.r32(scenario.club(scenario.r32(held + 4)))}};
                }
                // Dispatches of the iterator step inside the callees shift the numbering of the answered ones, so the places of
                // the loops' terminating calls are tried at several distances.
                for (unsigned shift_a = 0; shift_a < (pending_record.empty() ? 2u : 3u); ++shift_a)
                    for (unsigned gap = 0; gap < (pending_record.empty() ? 1u : 3u); ++gap)
                        for (unsigned second = 0; second < (pending_record.empty() ? 1u : 3u); ++second) {
                            Case first{.entry = 0x1040b470, .registers = with_toc({{3, manager}, {4, scenario.person(subject)}}),
                                       .memory = fixture + pending_record,
                                       .label = label("close closing", k, unsigned(v), shift_a * 100 + gap * 10 + second),
                                       .answers = {{0x103eaea0, 1}, {0x103eb020, 0}, {news_accepts, 1}}};
                            const unsigned a = 2 + shift_a, stop = a + 1 + gap, b = stop + 1 + second;
                            first.nth_answers = {{0x10738e00, 1, entry_cell}, {0x10738e00, a, listed_cell}, {0x10738e00, stop, 0},
                                                 {0x10738e00, b, listed_cell}, {0x10738e00, b + 1, 0}, {0x10738e00, b + 2, 0}, {0x10738e00, b + 3, 0}};
                            cases.push_back(first);
                        }
            }
        }
    }
    // 0x10410000: people spread over the table, the buyer word and the offer in the scratch area.
    for (unsigned index = 0; index < people; index += people / 41 + 1)
        for (const std::uint32_t kind : {0u, 3u, 0xffu, 0x80u}) {
            Case one{.entry = 0x10410000,
                     .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, buyer}, {6, kind}, {7, offer}}),
                     .memory = fixtures,
                     .label = label("signing", index, kind)};
            one.answers.push_back({news_accepts, 1});
            cases.push_back(one);
        }
    cases.push_back({.entry = 0x10410000, .registers = with_toc({{3, manager}, {4, 0}, {5, buyer}, {6, 1}, {7, offer}}), .memory = fixtures,
                     .label = "signing no person"});
    cases.push_back({.entry = 0x10410000, .registers = with_toc({{3, manager}, {4, scenario.person(3)}, {5, 0}, {6, 1}, {7, offer}}),
                     .memory = fixtures, .label = "signing no buyer"});
    // ---- 0x103e64e0: a pending list of people (the manager's header at +0xa5c; nodes in the scratch area). ----
    // Entries: person index word at +0, flag byte at +4, word at +8; nodes: data pointer at +0, next at +8.
    const auto nodes = scratch_base + 0x6000, entries = scratch_base + 0x6400;
    const auto pending = [&](const std::vector<std::pair<std::uint32_t, std::uint32_t>>& list) {
        std::vector<Write> writes = {{2, manager + 0xa5c, std::uint32_t(list.size())}, {4, manager + 0xa60, list.empty() ? 0 : nodes},
                                     {4, manager + 0xa64, 0}, {4, manager + 0xa68, 0}};
        for (std::size_t k = 0; k < list.size(); ++k) {
            const auto node = nodes + std::uint32_t(k) * 0x10, entry = entries + std::uint32_t(k) * 0x10;
            writes.push_back({4, node, entry});
            writes.push_back({4, node + 8, k + 1 < list.size() ? node + 0x10 : 0});
            writes.push_back({4, entry, list[k].first});
            writes.push_back({1, entry + 4, list[k].second});
            writes.push_back({4, entry + 8, 0x777 + std::uint32_t(k)});
        }
        return writes;
    };
    const std::uint32_t forms[] = {0x00, 0x05, 0x80, 0xff, 0x03, 0x7f, 0x14};
    unsigned m = 0;
    for (unsigned index = 0; index < people; index += people / 29 + 1, ++m)
        for (const auto form : forms)
            for (const std::uint32_t flag : {0u, 1u}) {
                if ((m + form + flag) % 2) continue;
                auto memory = pending({{index, flag}, {(index * 7 + 3) % people, flag ^ 1}});
                memory.push_back({1, scenario.person(index) + 0x67, form});
                for (const bool accepts : {true, false}) {
                    Case one{.entry = 0x103e64e0, .registers = with_toc({{3, manager}}), .memory = memory,
                             .label = label("moves", index, form, flag * 2 + accepts)};
                    if (accepts) one.answers.push_back({news_accepts, 1});
                    cases.push_back(one);
                }
            }
    cases.push_back({.entry = 0x103e64e0, .registers = with_toc({{3, manager}}), .memory = pending({}), .label = "moves empty"});
    // ---- 0x103e68c0: the history row, the fee and the settlement. ----
    const auto history_full = [&](std::uint32_t rows_null) {
        return std::vector<Write>{{4, manager + 0x998, scenario.r32(manager + 0x99c)}, {4, manager + 0x994, rows_null ? 0u : scenario.r32(manager + 0x994)}};
    };
    for (unsigned index = 0; index < people; index += people / 31 + 1)
        for (const std::uint32_t fee : {0xffffffffu, 25000u})
            for (const std::uint32_t loan : {0u, 1u, 0x100u}) {
                Case one{.entry = 0x103e68c0,
                         .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, buyer}, {6, 2}, {7, fee}, {8, loan}}),
                         .memory = fixtures, .label = label("record", index, fee & 0xff, loan)};
                cases.push_back(one);
                one.registers[2] = {5, 0};
                one.label = label("record no buyer", index, fee & 0xff, loan);
                cases.push_back(one);
                if (index % 3 == 0) {
                    one.registers[2] = {5, scenario.club(index % 40)};
                    one.label = label("record club buyer", index, fee & 0xff, loan);
                    cases.push_back(one);
                }
            }
    for (unsigned index = 0; index < people; index += people / 7 + 1)
        for (const std::uint32_t rows_null : {0u, 1u}) {
            auto memory = history_full(rows_null);
            memory.insert(memory.end(), fixtures.begin(), fixtures.end());
            Case one{.entry = 0x103e68c0,
                     .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, buyer}, {6, 1}, {7, 100}, {8, 0}}), .memory = memory,
                     .label = label("record grow", index, rows_null)};
            cases.push_back(one);
        }
    for (unsigned index = 0; index < people; index += people / 5 + 1)
        cases.push_back({.entry = 0x103e68c0,
                         .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, buyer}, {6, 1}, {7, 100}, {8, 0}}),
                         .memory = fixtures, .label = label("record declined", index), .answers = {{0x10083110, 1}}});
    // ---- 0x1040b470: the offers of the manager, each with its person. ----
    const auto offers_array = scenario.r32(manager), offers_size = scenario.r32(manager + 4);
    if (offers_size && offers_size < 100000)
        for (std::uint32_t k = 0, step = offers_size / 150 + 1; k < offers_size; k += step) {
            const auto offer = scenario.r32(offers_array + k * 4);
            if (!offer) continue;
            const auto person_index = scenario.r32(offer + 0xc);
            if (person_index >= people) continue;
            for (const bool accepts : {true, false})
                for (const std::uint32_t status : {0x13u, 0x05u}) {
                    Case one{.entry = 0x1040b470, .registers = with_toc({{3, manager}, {4, scenario.person(person_index)}}),
                             .memory = {{1, offer + 0x34, status}}, .label = label("close offers", k, accepts, status)};
                    if (accepts) one.answers.push_back({news_accepts, 1});
                    cases.push_back(one);
                }
        }
    for (unsigned index = 0; index < people; index += people / 25 + 1)
        for (const bool accepts : {true, false}) {
            Case one{.entry = 0x1040b470, .registers = with_toc({{3, manager}, {4, scenario.person(index)}}), .label = label("close spread", index, accepts)};
            if (accepts) one.answers.push_back({news_accepts, 1});
            cases.push_back(one);
        }
    for (unsigned index = 3; index < people; index += people / 9 + 1) {
        const Case::NthAnswer none{0x10738e00, 1, 0};
        (void)none;
        cases.push_back({.entry = 0x10410000, .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, buyer}, {6, 1}, {7, offer}}),
                         .memory = fixtures, .label = label("signing no club", index), .answers = {{0x10082f20, 0}}});
        cases.push_back({.entry = 0x10410000, .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, buyer}, {6, 1}, {7, offer}}),
                         .memory = fixtures, .label = label("signing listed", index), .answers = {{0x102de3c0, 1}}});
        cases.push_back({.entry = 0x10410000, .registers = with_toc({{3, manager}, {4, scenario.person(index)}, {5, buyer}, {6, 1}, {7, offer}}),
                         .memory = fixtures + std::vector<Write>{{1, scenario.person(people - 1) + 0x6b, 0}}, .label = label("signing none", index),
                         .answers = {{0x103f43b0, 1}}});
    }
    // 0x1040b470 with offer lists: iterator answers hand the offer (by index) and the listed people to the loops.
    {
        const auto listed = scratch_base + 0x7100, entry_cell = scratch_base + 0x7200, listed_cell = scratch_base + 0x7220, list = scratch_base + 0x7000;
        const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
        for (std::uint32_t k = 0, step = size / 12 + 1; size && size < 100000 && k < size; k += step) {
            const auto real = scenario.r32(array + k * 4);
            if (!real || scenario.r32(real + 0xc) >= people) continue;
            const auto person = scenario.person(scenario.r32(real + 0xc));
            const std::vector<Write> fixture = {{4, real + 4, list}, {4, list + 4, listed}, {4, list + 0xc, 0}, {4, entry_cell, k},
                                                {4, listed_cell, (k * 7 + 1) % people}, {1, listed_cell + 4, 1}, {4, real + 0x28, 0x555}};
            for (unsigned shift_a = 0; shift_a < 3; ++shift_a)
                for (unsigned shift_b = 0; shift_b < 3; ++shift_b) {
                    if (shift_a && shift_b && shift_a != shift_b) continue;
                    Case first{.entry = 0x1040b470, .registers = with_toc({{3, manager}, {4, person}}), .memory = fixture,
                               .label = label("close lists", k, shift_a, shift_b), .answers = {{0x103eaea0, 1}, {0x103eb020, 0}, {news_accepts, 1}}};
                    const unsigned a = 2 + shift_a, b = 4 + shift_a + shift_b;
                    first.nth_answers = {{0x10738e00, 1, entry_cell}, {0x10738e00, a, listed_cell}, {0x10738e00, a + 1, 0},
                                         {0x10738e00, b, listed_cell}, {0x10738e00, b + 1, 0}, {0x10738e00, b + 2, 0}};
                    cases.push_back(first);
                }
            Case second{.entry = 0x1040b470, .registers = with_toc({{3, manager}, {4, scenario.person((k * 3 + 1) % people)}}), .memory = fixture,
                        .label = label("close rivals", k), .answers = {{0x103eaea0, 0}, {0x103eb020, 1}, {news_accepts, 1}}};
            second.nth_answers = {{0x10738e00, 1, entry_cell}, {0x10738e00, 2, 0}};
            cases.push_back(second);
            second.answers = {{0x103eaea0, 0}, {0x103eb020, 1}, {news_accepts, 0}};
            second.label = label("close rivals refused", k);
            cases.push_back(second);
        }
    }
}
}
