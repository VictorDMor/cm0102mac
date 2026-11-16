// Cases for transfer_manager_native_status*.cpp.
#include "transfer_manager_native_checks.hpp"
#include "guest_abi.hpp"
#include <tuple>

namespace cm::aot::transfer_manager_native_checks {
namespace {
constexpr std::uint32_t status_rows = 0x9a0, status_row_size = 10;
std::string describe(const char* what, unsigned a, unsigned b = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b);
}
// Real offers of the manager (an array of pointers at +0, its size at +4), spread over the array.
std::vector<std::uint32_t> real_offers(const Scenario& scenario, unsigned wanted) {
    std::vector<std::uint32_t> offers;
    const auto manager = scenario.transfer_manager();
    const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
    if (size == 0 || size > 100000) return offers;
    const auto step = size / wanted + 1;
    for (std::uint32_t i = 0; i < size; i += step)
        if (const auto offer = scenario.r32(array + i * 4)) offers.push_back(offer);
    return offers;
}
// The people of an offer's list (+4: head at +4, entries with the item at +0 and the next at +8).
std::vector<std::uint32_t> listed_people(const Scenario& scenario, std::uint32_t offer) {
    std::vector<std::uint32_t> indexes;
    const auto list = scenario.r32(offer + 4);
    if (!list) return indexes;
    auto entry = scenario.r32(list + 4);
    for (unsigned guard = 0; entry && guard < 8; ++guard, entry = scenario.r32(entry + 8))
        if (const auto item = scenario.r32(entry)) indexes.push_back(scenario.r32(item));
    return indexes;
}
Write flags_write(const Scenario& scenario, std::uint32_t person_index, std::uint32_t flags) {
    return {1, scenario.r32(scenario.transfer_manager() + status_rows) + person_index * status_row_size, flags};
}
// Entries of one of the manager's sorted row tables (pointer at +table, count at +count; 0x10 per row).
std::vector<std::uint32_t> table_rows(const Scenario& scenario, std::uint32_t table, std::uint32_t count, unsigned wanted) {
    std::vector<std::uint32_t> rows;
    const auto manager = scenario.transfer_manager();
    const auto base = scenario.r32(manager + table), size = scenario.r32(manager + count);
    if (!base || size == 0 || size > 1000000) return rows;
    const auto step = size / wanted + 1;
    for (std::uint32_t i = 0; i < size; i += step) rows.push_back(base + i * 0x10);
    return rows;
}
}

TRANSFER_MANAGER_NATIVE_CASES("status") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto people = scenario.people();
    // 0x103e9220: people spread over the table: with and without contracts, players and staff.
    for (unsigned index = 0; index < people; index += people / 23 + 1)
        for (const std::uint32_t status : {1u, 2u, 4u, 0x84u})
            cases.push_back({.entry = 0x103e9220, .registers = {{3, manager}, {4, scenario.person(index)}, {5, status}},
                             .label = describe("remaining term", index, status)});

    const auto offers = real_offers(scenario, 20);
    // 0x103ed090 and 0x103ef8a0: real offers; the person's status flags (2, 4, 8 and combinations) are set.
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto offer = offers[i];
        cases.push_back({.entry = 0x103ed090, .registers = {{3, manager}, {4, offer}}, .label = describe("weekday", unsigned(i))});
        const auto person_index = scenario.r32(offer + 0xc);
        const auto listed = listed_people(scenario, offer);
        for (const std::uint32_t flags : {0u, 2u, 4u, 8u, 0xeu, 0x2eu}) {
            std::vector<Write> memory;
            if (person_index < people) memory.push_back(flags_write(scenario, person_index, flags));
            for (const auto index : listed)
                if (index < people) memory.push_back(flags_write(scenario, index, flags ^ ((index & 1) ? 0xeu : 0u)));
            cases.push_back({.entry = 0x103ef8a0, .registers = {{3, manager}, {4, offer}}, .memory = memory,
                             .label = describe("offer statuses", unsigned(i), flags)});
            if (person_index < people) {
                cases.push_back({.entry = 0x103f2560, .registers = {{3, manager}, {4, scenario.person(person_index)}, {5, offer}},
                                 .memory = memory, .label = describe("status fee", unsigned(i), flags)});
                if (!listed.empty() && listed[0] < people)
                    cases.push_back({.entry = 0x103f2560, .registers = {{3, manager}, {4, scenario.person(listed[0])}, {5, offer}},
                                     .memory = memory, .label = describe("status fee listed", unsigned(i), flags)});
            }
        }
    }
    // 0x103f2560 over the people the fee rows (+0x9b4) belong to is reached through the offers above; here
    // the people spread over the table, against a real offer whose person is theirs and another's.
    for (unsigned index = 0; index < people && !offers.empty(); index += people / 31 + 1)
        cases.push_back({.entry = 0x103f2560, .registers = {{3, manager}, {4, scenario.person(index)}, {5, offers[index % offers.size()]}},
                         .label = describe("status fee spread", index)});
    // 0x103f2800: the people of real rows of both tables (+0x9c0 kind 2, +0x9cc kind 5), the row's byte +8 or -1.
    for (const auto& [table, count, kind] : {std::tuple{0x9c0u, 0x9c8u, 2u}, std::tuple{0x9ccu, 0x9d4u, 5u}}) {
        unsigned n = 0;
        for (const auto row : table_rows(scenario, table, count, 16)) {
            const auto index = scenario.r32(row);
            if (index >= people) continue;
            const auto offer = offers.empty() ? 0 : offers[n % offers.size()];
            const std::uint32_t target = scenario.r32(row + 8) & 0xff;
            for (const std::uint32_t wanted : {target, 0xffffffffu, target + 1, 0u})
                for (const std::uint32_t kind_word : {kind, kind ^ 7u, kind | 0x100u})
                    cases.push_back({.entry = 0x103f2800,
                                     .registers = {{3, manager}, {4, scenario.person(index)}, {5, wanted}, {6, kind_word}, {7, offer}},
                                     .label = describe("row fee", index, wanted * 8 + kind_word)});
            ++n;
        }
    }
    // 0x103ed520: people spread over the table against no offer and real offers, with and without a row, both noise flags.
    const auto rows = table_rows(scenario, 0xa6c, 0xa70, 12);
    for (unsigned index = 0; index < people; index += people / 53 + 1) {
        const auto person = scenario.person(index);
        for (const std::uint32_t noisy : {0u, 1u, 0x100u})
            for (unsigned variant = 0; variant < 4; ++variant) {
                const auto offer = (variant & 1) && !offers.empty() ? offers[(index + variant) % offers.size()] : 0;
                const auto row = (variant & 2) && !rows.empty() ? rows[(index + variant) % rows.size()] : 0;
                cases.push_back({.entry = 0x103ed520, .registers = {{3, manager}, {4, person}, {6, offer}, {7, row}, {8, noisy}},
                                 .label = describe("refused", index, noisy * 4 + variant)});
            }
    }
    // The rows' own people, with real offers: the rows decide the 3x/four-game tests.
    for (std::size_t k = 0; k < rows.size(); ++k) {
        const auto index = scenario.r32(rows[k]);
        if (index >= people) continue;
        for (const std::uint32_t noisy : {0u, 1u})
            for (std::size_t o = 0; o < offers.size(); o += 4)
                cases.push_back({.entry = 0x103ed520,
                                 .registers = {{3, manager}, {4, scenario.person(index)}, {6, offers[o]}, {7, 0}, {8, noisy}},
                                 .label = describe("refused row person", unsigned(k), unsigned(o * 2 + noisy))});
    }
    // 0x103e8d30: a fixture and two sides' lineups built in the scratch area from real people and clubs.
    const auto fixture = scratch_base + 0x1000, lineups = scratch_base + 0x2000;
    const std::uint32_t club_choices[] = {0, 1, 5, 17, 40, 77, 120, 200};
    const std::uint32_t flag_choices[] = {0x2, 0x4, 0x20, 0x22, 0x24, 0, 0x26, 0x2e};
    for (unsigned v = 0; v < 48; ++v) {
        std::vector<Write> memory;
        const bool by_id = v % 5 == 0;
        memory.push_back({4, fixture, by_id ? 0xfffffffeu : 7u + v});
        memory.push_back({4, fixture + 0x14, v % 3 == 0 ? 0u : scenario.competition((v / 3) % 4)});
        memory.push_back({4, fixture + 0x1c, scenario.club(club_choices[v % 8])});
        memory.push_back({4, fixture + 0x20, scenario.club(club_choices[(v / 2 + 3) % 8])});
        memory.push_back({1, fixture + 0x40, v & 1});
        memory.push_back({1, fixture + 0x41, (v >> 1) & 1});
        for (unsigned side = 0; side < 2; ++side)
            for (unsigned i = 0; i < 20; ++i) {
                const auto at = lineups + side * 0x664 + i * 0x4c;
                const auto k = v * 40 + side * 20 + i;
                auto index = (k * 2711u + 5u) % people;
                if (k % 3 != 0 && !offers.empty() && scenario.r32(offers[k % offers.size()] + 0xc) < people)
                    index = scenario.r32(offers[k % offers.size()] + 0xc);
                memory.push_back({4, at + 0x194, k % 29 == 7 ? people + 3 : index});
                memory.push_back({1, at + 0x1b1, k % 17 == 3 ? 0xfbu : 0u});
                memory.push_back({1, at + 0x1c6, (i >= 11 && k % 5 == 0) ? 0xffu : 0u});
                memory.push_back(flags_write(scenario, index, flag_choices[(k + v) % 8]));
            }
        // People collected as on the move are on the first side's club or the second's: spread the unlined ones.
        cases.push_back({.entry = 0x103e8d30, .registers = {{3, manager}, {4, fixture}, {5, lineups}}, .memory = memory,
                         .label = describe("match statuses", v)});
    }
    // Fabricated fee rows (the career has none yet) with the contract lookup answered by a record in the scratch
    // area: 0x103f2800 takes the row of the person's id (+0x9c0 kind 2, +0x9cc kind 5), 0x103f2560 the one of the
    // person's club id (+0x9b4).
    const auto contract = scratch_base + 0x300, row = scratch_base + 0x400, club_row = scratch_base + 0x440;
    constexpr std::uint32_t contract_club = 5, fee_club = 9;
    for (unsigned k = 0; k < 24; ++k) {
        const auto person_index = (k * (people / 24 + 1) + 3) % people;
        const auto person = scenario.person(person_index);
        std::vector<Write> rows = {{4, contract + 4, contract_club}, {1, contract + 0x38, k % 2 ? 8u : 0x2c},
                                   {4, row, scenario.r32(person)}, {4, row + 4, 5000 + k * 777}, {4, row + 8, (1 + k % 3) << 24},
                                   {4, row + 0xc, fee_club},
                                   {4, club_row, (k % 4 == 3 ? 1u : 0x32u + k) << 24}, {4, club_row + 4, scenario.r32(scenario.club(contract_club))},
                                   {4, club_row + 8, fee_club}, {4, club_row + 0xc, scenario.r32(person)},
                                   {4, manager + 0x9c0, row}, {4, manager + 0x9c8, 1}, {4, manager + 0x9cc, row}, {4, manager + 0x9d4, 1},
                                   {4, manager + 0x9b4, club_row}, {4, manager + 0x9bc, 1}};
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {{0x10082f20, contract}};
        const auto offer = offers.empty() ? 0 : offers[k % offers.size()];
        for (const std::uint32_t target : {1 + k % 3, 0xffffffffu, 2 + k % 3, 0u})
            for (const std::uint32_t kind : {2u, 5u, 0x105u})
                cases.push_back({.entry = 0x103f2800,
                                 .registers = {{3, manager}, {4, person}, {5, target}, {6, kind}, {7, offer}}, .memory = rows,
                                 .label = describe("row fee fab", k, target * 4 + kind), .answers = answers});
        // 0x103f2560: the offer's own person (branch with the list sum) and another person; amounts above and below 1000.
        auto with_offer = rows;
        if (offer) {
            with_offer.push_back({4, offer + 0xc, scenario.r32(person)});
            with_offer.push_back({4, offer + 0x20, k % 5 == 4 ? 300u : 70000u + k});
            cases.push_back({.entry = 0x103f2560, .registers = {{3, manager}, {4, person}, {5, offer}}, .memory = with_offer,
                             .label = describe("fee fab own", k), .answers = answers});
            cases.push_back({.entry = 0x103f2560, .registers = {{3, manager}, {4, person}, {5, offer}}, .memory = rows,
                             .label = describe("fee fab other", k), .answers = answers});
        }
    }
    // ---- Coverage pass: lists, deeper match paths and the refusal's remaining tests. ----
    const auto status_row = [&](std::uint32_t person_index) {
        return scenario.r32(manager + status_rows) + person_index * status_row_size;
    };
    // A list of two people in the scratch area (head at +4, entries: item at +0, next at +8; items are words).
    const auto list_at = scratch_base + 0x600;
    const auto list_writes = [&](std::uint32_t first, std::uint32_t second) {
        return std::vector<Write>{{4, list_at + 4, list_at + 0x20}, {4, list_at + 0x20, list_at + 0x60}, {4, list_at + 0x28, list_at + 0x40},
                                  {4, list_at + 0x40, list_at + 0x70}, {4, list_at + 0x48, 0}, {4, list_at + 0x60, first}, {4, list_at + 0x70, second}};
    };
    for (std::size_t i = 0; i < offers.size() && i < 8; ++i) {
        const auto offer = offers[i];
        const auto a = (unsigned(i) * 977u + 11u) % people, b = (unsigned(i) * 1493u + 5u) % people;
        for (const std::uint32_t flags : {0u, 2u, 4u, 8u, 0xeu}) {
            auto memory = list_writes(a, b);
            memory.push_back({4, offer + 4, list_at});
            memory.push_back(flags_write(scenario, a, flags));
            memory.push_back(flags_write(scenario, b, flags ^ 0xe));
            cases.push_back({.entry = 0x103ef8a0, .registers = {{3, manager}, {4, offer}}, .memory = memory,
                             .label = describe("offer statuses list", unsigned(i), flags)});
        }
    }
    // 0x103f2560, the offer's own person with a list summed into the amount (small and large), rows as above.
    for (unsigned k = 0; k < 8 && !offers.empty(); ++k) {
        const auto person_index = (k * (people / 8 + 1) + 3) % people;
        const auto person = scenario.person(person_index);
        const auto offer = offers[k % offers.size()];
        auto memory = list_writes((k * 331u + 7) % people, (k * 71u + 13) % people);
        for (const Write& w : std::vector<Write>{{4, contract + 4, contract_club}, {1, contract + 0x38, 0x2c},
                   {4, club_row, (k % 2 ? 1u : 0x32u) << 24}, {4, club_row + 4, scenario.r32(scenario.club(contract_club))},
                   {4, club_row + 8, fee_club}, {4, club_row + 0xc, scenario.r32(person)}, {4, manager + 0x9b4, club_row},
                   {4, manager + 0x9bc, 1}, {4, offer + 4, list_at}, {4, offer + 0xc, scenario.r32(person)},
                   {4, offer + 0x20, k % 4 == 3 ? 0u : 40000u}})
            memory.push_back(w);
        cases.push_back({.entry = 0x103f2560, .registers = {{3, manager}, {4, person}, {5, offer}}, .memory = memory,
                         .label = describe("fee list", k), .answers = {{0x10082f20, contract}}});
    }

    // 0x103e8d30 on a club's real squad (club record +0x98: person pointers).
    const auto people_base = scenario.global(-0x7be0);
    std::vector<unsigned> squad_clubs;
    for (unsigned k = 0; k < 400 && squad_clubs.size() < 6; ++k)
        if (scenario.r32(scenario.club(k) + 0x98) && scenario.r32(scenario.club(k) + 0x9c)) squad_clubs.push_back(k);
    struct Variant { const char* name; std::vector<std::pair<std::uint32_t, std::uint32_t>> answers; bool competition; bool fee_rows; std::uint32_t term; std::uint32_t limit; bool staff; };
    const auto other_club_contract = scratch_base + 0x700;
    const std::vector<Variant> variants = {
        {"plain", {}, false, false, 0, 0, false},
        {"injured", {{0x10187680, 1}}, true, false, 0, 0, false},
        {"suspended", {{0x100e7920, 1}}, true, false, 0, 0, false},
        {"national", {{0x102b7c70, 1}}, true, false, 0, 0, false},
        {"blocked", {{0x103f93e0, 1}}, true, false, 0, 0, false},
        {"held here", {{0x10083110, 1}, {0x10082f20, contract}}, true, false, 0, 0, false},
        {"held there", {{0x10083110, 1}, {0x10082f20, other_club_contract}}, true, false, 0, 0, false},
        {"no contract", {{0x10083110, 1}, {0x10082f20, 0}}, true, false, 0, 0, false},
        {"friendly", {{0x10494d20, 1}}, true, false, 0, 0, false},
        {"low limit", {}, true, false, 0, 6, false},
        {"term 10", {{0x10083110, 0}, {0x10082f20, contract}}, false, true, 10, 0, false},
        {"term 1", {{0x10083110, 0}, {0x10082f20, contract}}, false, true, 1, 0, false},
        {"term 70", {{0x10083110, 0}, {0x10082f20, contract}}, false, true, 70, 0, false},
        {"staff 10", {{0x100c7d00, 1}, {0x10082f20, contract}}, true, true, 10, 0, true},
        {"staff 70", {{0x100c7d00, 1}, {0x10082f20, contract}}, true, true, 70, 0, true},
    };
    for (const auto k : squad_clubs)
        for (std::size_t v = 0; v < variants.size(); ++v) {
            const auto& variant = variants[v];
            std::vector<std::uint32_t> squad;
            for (unsigned i = 0; i < 50; ++i)
                if (const auto pointer = scenario.r32(scenario.club(k) + 0x98 + 4 * i)) squad.push_back((pointer - people_base) / 0x70);
            std::vector<Write> memory;
            memory.push_back({4, fixture, 7});
            memory.push_back({4, fixture + 0x14, variant.competition ? scenario.competition(1) : 0u});
            memory.push_back({4, fixture + 0x1c, scenario.club(k)});
            memory.push_back({4, fixture + 0x20, scenario.club(k)});
            memory.push_back({1, fixture + 0x40, 1});
            memory.push_back({1, fixture + 0x41, 1});
            if (!variant.competition || variant.name == std::string("plain") || variant.name[0] == 'f' || variant.name[0] == 'l')
                memory.push_back({4, fixture, 0xfffffffeu});
            memory.push_back({4, contract + 4, k});
            memory.push_back({1, contract + 0x38, 0x2c});
            memory.push_back({2, contract + 0x42, variant.term});
            memory.push_back({4, other_club_contract + 4, k + 1});
            memory.push_back({1, other_club_contract + 0x38, 0x2c});
            const std::uint32_t flag = variant.staff ? 0x24 : 0x22;
            for (unsigned side = 0; side < 2; ++side)
                for (unsigned i = 0; i < 20; ++i) {
                    const auto at = lineups + side * 0x664 + i * 0x4c;
                    const bool listed = side == 0 && i < 11 && i < squad.size() && squad.size() > 11;
                    memory.push_back({4, at + 0x194, listed ? squad[i] : 0u});
                    memory.push_back({1, at + 0x1b1, listed ? 0u : 0xfbu});
                    memory.push_back({1, at + 0x1c6, 0});
                }
            for (const auto index : squad) {
                memory.push_back(flags_write(scenario, index, flag));
                if (variant.staff) {
                    memory.push_back({1, scenario.person(index) + 0x5e, 40});
                    memory.push_back({1, status_row(index) + 8, 40 - variant.term});
                }
            }
            if (variant.limit) memory.push_back({4, scenario.toc(-0x7be4), variant.limit});
            if (variant.fee_rows && !squad.empty()) {
                const auto key = scenario.r32(scenario.person(squad[0]));
                memory.push_back({4, row, key});
                memory.push_back({4, row + 4, 5000});
                memory.push_back({4, row + 8, variant.term << 24});
                memory.push_back({4, row + 0xc, fee_club});
                for (const auto table : {0x9c0u, 0x9ccu}) {
                    memory.push_back({4, manager + table, row});
                    memory.push_back({4, manager + table + 8, 1});
                }
            }
            cases.push_back({.entry = 0x103e8d30, .registers = {{3, manager}, {4, fixture}, {5, lineups}}, .memory = memory,
                             .label = describe("match squad", unsigned(v), k), .answers = variant.answers});
        }

    // The human-club route reads the 50-word list at frame+0x38 without filling it: write it below the entry sp
    // (the frame is 0x220 bytes, the list at +0x38), with valid ids, a null, and (low limit) ids at or above the limit.
    {
        const auto sp = std::uint32_t(scenario.s.r<4>(guest::reg::gpr(1)));
        const auto list_base = sp - 0x220 + 0x38;
        for (const bool low_limit : {false, true})
            for (unsigned v = 0; v < 4; ++v) {
                std::vector<Write> memory;
                memory.push_back({4, fixture, 0xfffffffeu});
                memory.push_back({4, fixture + 0x14, scenario.competition(1)});
                memory.push_back({4, fixture + 0x1c, scenario.club(squad_clubs.empty() ? 0 : squad_clubs[v % squad_clubs.size()])});
                memory.push_back({4, fixture + 0x20, scenario.club(squad_clubs.empty() ? 0 : squad_clubs[v % squad_clubs.size()])});
                memory.push_back({1, fixture + 0x40, 1});
                memory.push_back({1, fixture + 0x41, 1});
                for (unsigned side = 0; side < 2; ++side)
                    for (unsigned i = 0; i < 20; ++i) {
                        const auto at = lineups + side * 0x664 + i * 0x4c;
                        const auto index = (v * 211u + side * 20 + i * 97u + 9u) % people;
                        memory.push_back({4, at + 0x194, index});
                        memory.push_back({1, at + 0x1b1, i % 7 == 6 ? 0xfbu : 0u});
                        memory.push_back({1, at + 0x1c6, 0});
                        memory.push_back(flags_write(scenario, index, 0x24));
                    }
                for (unsigned i = 0; i < 50; ++i) {
                    const auto index = (v * 211u + i * 97u + 9u) % people;  // some of these are lined up
                    const auto other = (i * 389u + 17u) % people;
                    const bool lined = i % 3 == 0;
                    const auto id = i % 11 == 10 ? 0u : (lined ? index : other);
                    memory.push_back({4, list_base + 4 * i, id ? scenario.person(id) : 0u});
                    if (id) memory.push_back(flags_write(scenario, id, 0x24));
                }
                if (low_limit) memory.push_back({4, scenario.toc(-0x7be4), 100});
                cases.push_back({.entry = 0x103e8d30, .registers = {{3, manager}, {4, fixture}, {5, lineups}}, .memory = memory,
                                 .label = describe("match human stale", v, low_limit), .answers = {{0x100c7d00, 1}, {0x10082f20, contract}}});
            }
    }

    // 0x103ed520: the contract term and club tests, the 2001 name tests, the level history, sample sizes.
    const auto date_year = scenario.toc(-0x7b80) + 2;
    const auto strings = scenario.toc(-0x4b9c);
    const std::uint32_t rate_answers[] = {40, 0, 5, 0xfffffffd, 16};
    const std::vector<std::uint32_t> real_rows = table_rows(scenario, 0xa6c, 0xa70, 6);
    for (unsigned n = 0; n < 12 && !offers.empty() && !real_rows.empty(); ++n) {
        const auto person_index = (n * (people / 12 + 1) + 1) % people;
        const auto person = scenario.person(person_index);
        const auto offer = offers[n % offers.size()];
        const auto row_ptr = real_rows[n % real_rows.size()];
        const auto competition = scenario.r32(person + 0x14);
        // Terms: the contract (type 2 or other) against the offer's term and club, with chosen games.
        for (unsigned t = 0; t < 12; ++t) {
            std::vector<Write> memory = {{4, contract + 4, 3}, {1, contract + 0x38, t % 2 ? 8u : 0x14u},
                                         {4, contract + 0x30, (t % 3 == 0 ? 0x00050005u : t % 3 == 1 ? 0xffff0009u : 0x00050001u)},
                                         {2, offer + 0x32, 5}, {2, offer + 0x30, 0}, {4, offer + 4, t % 4 == 2 ? 3u : 4u},
                                         {2, row_ptr + 6, t % 5 == 0 ? 2u : 10u}, {2, row_ptr + 4, t % 5 == 0 ? 1u : 20u}};
            cases.push_back({.entry = 0x103ed520, .registers = {{3, manager}, {4, person}, {6, offer}, {7, row_ptr}, {8, t & 1}},
                             .memory = memory, .label = describe("refused terms", n, t), .answers = {{0x10082f20, contract}}});
        }
        // The year 2001 name tests.
        for (unsigned y = 0; y < 6; ++y) {
            std::vector<Write> memory = {{2, date_year, y < 4 ? 0x7d1u : 0x7d5u},
                                         {4, person + 0x28, strings + (y % 2 ? 0x1a36u : 0x1a27u)}};
            cases.push_back({.entry = 0x103ed520, .registers = {{3, manager}, {4, person}, {6, offer}, {7, 0}, {8, y & 1}},
                             .memory = memory, .label = describe("refused names", n, y)});
        }
        // Level history, sample sizes and the competition/age tests.
        for (unsigned h = 0; h < 20; ++h) {
            const auto status = status_row(person_index);
            std::vector<Write> memory = {{2, date_year, 0x7d5}, {2, status + 4, h % 3 == 0 ? 0xffffu : h % 3 == 1 ? 0x5555u : 0u},
                                         {2, status + 6, h % 4 == 0 ? 0x7fffu : h % 4 == 1 ? 0x8001u : h % 4 == 2 ? 0xfff0u : 0u}};
            if (h % 5 == 1 && competition) memory.push_back({2, competition + 0xc, 0x7fff});
            if (h % 5 == 2 && competition) memory.push_back({2, competition + 0xc, 0});
            if (h % 7 == 3) { memory.push_back({4, person + 0x14, 0}); memory.push_back({4, person + 0x1c, 0}); }
            if (h % 7 == 4) memory.push_back({4, person + 0xc, 0});
            if (h % 3 == 2) { memory.push_back({1, person + 0x5d, 0x30}); memory.push_back({1, person + 0x5e, 0x18}); }
            if (h % 3 == 1) { memory.push_back({1, person + 0x5d, 0x10}); memory.push_back({1, person + 0x5e, 0x18}); }
            if (scenario.r32(person + 0xc)) memory.push_back({2, scenario.r32(person + 0xc) + 0xac, 0x7000});
            cases.push_back({.entry = 0x103ed520, .registers = {{3, manager}, {4, person}, {6, h % 4 == 3 ? 0u : offer}, {7, row_ptr}, {8, h & 1}},
                             .memory = memory, .label = describe("refused level", n, h), .answers = {{0x10487140, rate_answers[h % 5]}}});
        }
    }
    // The 2001 name test ending in a refusal: the club's text at +0x1ee equals the compared string; also without an offer.
    for (unsigned n = 0; n < 4 && !offers.empty(); ++n) {
        const auto person = scenario.person((n * 1009u + 7u) % people);
        const auto offer = offers[n % offers.size()];
        std::vector<Write> memory = {{2, date_year, 0x7d1}, {4, person + 0x28, strings + (n % 2 ? 0x1a36u : 0x1a27u)}, {4, offer + 4, 4}};
        for (unsigned w = 0; w < 8; ++w) memory.push_back({4, scenario.club(4) + 0x1ee + 4 * w, scenario.r32(strings + 0x1a2e + 4 * w)});
        cases.push_back({.entry = 0x103ed520, .registers = {{3, manager}, {4, person}, {6, offer}, {7, 0}, {8, 0}},
                         .memory = memory, .label = describe("refused names equal", n)});
        cases.push_back({.entry = 0x103ed520, .registers = {{3, manager}, {4, person}, {6, 0}, {7, 0}, {8, 0}},
                         .memory = memory, .label = describe("refused names no offer", n)});
    }
    // The level section reached on purpose: no competition, no contract, enough games, an adult with potential.
    const auto rating_table = scenario.global(-0x7a64);
    unsigned deep = 0;
    for (unsigned n = 0; n < people && deep < 10 && !real_rows.empty(); n += people / 61 + 1) {
        const auto person_index = n;
        const auto person = scenario.person(person_index);
        const auto club_record = scenario.r32(person + 0xc);
        if (!club_record) continue;
        ++deep;
        const auto level = std::int16_t(scenario.r32(scenario.r32(rating_table + 4) + scenario.r32(club_record + 0x68) * 0x74 + 2) >> 16);
        const auto status = status_row(person_index);
        const auto row_ptr = real_rows[deep % real_rows.size()];
        for (const std::uint32_t gap : {0u, 3u, 12u, 20u})
            for (const std::uint32_t answer : {40u, 0u, 5u, 16u, 0xfffffffdu})
                for (const std::uint32_t noisy : {0u, 1u}) {
                    const std::uint32_t old_level = gap == 0 ? 0x7fffu : (std::uint32_t(level) - gap) & 0xffff;
                    std::vector<Write> memory = {{2, date_year, 0x7d5}, {4, person + 0x14, 0}, {4, person + 0x1c, 0}, {1, person + 0x5d, 0x30},
                                                 {1, person + 0x5e, 0x18}, {2, row_ptr + 6, 10}, {2, row_ptr + 4, 20},
                                                 {2, status + 4, answer == 5 || answer == 16 ? 0xffffu : 0x5555u}, {2, status + 6, old_level}};
                    cases.push_back({.entry = 0x103ed520, .registers = {{3, manager}, {4, person}, {6, 0}, {7, row_ptr}, {8, noisy}},
                                     .memory = memory, .label = describe("refused deep", deep, gap * 100 + answer % 100 + noisy),
                                     .answers = {{0x10082f20, 0}, {0x10487140, answer}}});
                }
        // The same without a club record (potential test with a null record).
        cases.push_back({.entry = 0x103ed520, .registers = {{3, manager}, {4, person}, {6, 0}, {7, row_ptr}, {8, 0}},
                         .memory = {{4, person + 0xc, 0}, {4, person + 0x14, 0}, {4, person + 0x1c, 0}, {1, person + 0x5d, 0x30}, {1, person + 0x5e, 0x18}, {2, row_ptr + 6, 10}, {2, row_ptr + 4, 20}},
                         .label = describe("refused no club", deep), .answers = {{0x10082f20, 0}}});
    }
    cases.push_back({.entry = 0x103ed520, .registers = {{3, manager}, {4, 0}, {6, 0}, {7, 0}, {8, 0}}, .label = describe("refused null", 0)});
}
}
