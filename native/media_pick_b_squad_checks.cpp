// Cases for the squad story (0x1069ed10): a person and a club built in the scratch area (so every
// condition can be set), the callees answered, and the real world's persons for the region test.
#include "media_checks.hpp"

using namespace cm::aot::media_checks;

namespace {
using Writes = std::vector<Write>;
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
constexpr std::uint32_t entry_squad = 0x1069ed10;
constexpr std::uint32_t table_at = scratch_base + 0x8000, person_at = scratch_base + 0x1000, contract_at = scratch_base + 0x1100,
    link_at = scratch_base + 0x1200, standing_at = scratch_base + 0x1300, members_at = scratch_base + 0x2000;
constexpr std::uint32_t region_tier = 0x100db120, contract_with = 0x10082f90, link_kind = 0x10187680,
    staff_test = 0x100d3150;

Write b8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write h16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
void append(Writes& out, const Writes& more) { out.insert(out.end(), more.begin(), more.end()); }
// The random table as the game keeps it: the cursor steps through uint16 entries at `table_at` (the
// first draw reads entries[0]), the seed is added to each. `refill` puts the cursor at the end so the
// next step wraps to the start and draws a new seed with rand().
Writes random_table(const Scenario& s, const std::vector<std::uint32_t>& entries, std::uint32_t seed, bool refill = false) {
    Writes writes;
    for (std::size_t i = 0; i < entries.size(); ++i) writes.push_back({2, table_at + 2 * std::uint32_t(i + 1), entries[i]});
    const auto end = table_at + 0x100;
    writes.push_back({4, s.toc(-0x7b84), refill ? end : table_at});
    writes.push_back({4, s.toc(-0x7b88), end});
    writes.push_back({4, s.toc(-0x7b8c), table_at});
    writes.push_back({4, s.toc(-0x7b90), seed});
    if (refill) for (std::uint32_t i = 1; i <= 4; ++i) writes.push_back({2, table_at + 2 * i, 0x1111u * i});
    return writes;
}
Registers arguments(std::uint32_t person, std::uint32_t club) {
    return {{3, 0x31313131}, {4, person}, {5, club}, {6, 0x61616161}, {7, 0x71717171}, {8, 0x81818181}, {9, 0x91919191}, {10, 0xa1a1a1a1}};
}

// One squad member in the scratch area: a record whose contract rating, +0x5c halfword and ability bytes are given.
struct Member {
    std::uint32_t contract = 1, rating = 0x2000, half = 0x30, low = 5, high = 5, region = 0;
};
Writes member_rows(std::uint32_t club, const std::vector<std::pair<unsigned, Member>>& members) {
    Writes out;
    for (const auto& [slot, m] : members) {
        const auto at = members_at + slot * 0x200;
        append(out, {w32(club + 0x98 + slot * 4, at), w32(at + 0x14, m.contract ? at + 0x100 : 0), h16(at + 0x100 + 0xa, m.rating),
                     h16(at + 0x5c, m.half), b8(at + 0x66, m.low), b8(at + 0x67, m.high), b8(at + 0x100 + 0xf, m.region)});
    }
    return out;
}
}

MEDIA_CASES("pick_b_squad") {
    const auto club = scenario.club(0);
    const auto table = scenario.global(-0x7b5c);
    // The club's slots all empty, a link into the tier table, and the person with a contract above the cutoff.
    Writes base = {w32(link_at, 1), w32(table + 4, 1), w32(club + 0x30, link_at), w32(person_at + 0x14, contract_at), h16(contract_at + 0xa, 0x2000)};
    for (unsigned slot = 0; slot < 50; ++slot) base.push_back(w32(club + 0x98 + slot * 4, 0));
    const auto call = [&](std::uint32_t person, std::uint32_t at, Writes memory, Answers answers, const char* label) {
        cases.push_back({.entry = entry_squad, .registers = arguments(person, at), .memory = std::move(memory), .f1 = {}, .label = label,
                         .answers = std::move(answers)});
    };
    const auto standing = [&](std::uint32_t state, std::uint32_t flags, std::uint32_t mode) {
        return Writes{b8(standing_at + 0x44, state), w32(standing_at + 0x50, flags), b8(standing_at + 0x60, mode)};
    };
    const auto joined = [&](std::initializer_list<Writes> parts) {
        Writes all = base;
        for (const auto& part : parts) append(all, part);
        return all;
    };
    // The callees' answers: region_of the same for everyone.
    // (The contract flag getter and setter belong to another module's rewrite, which the original reaches before
    // the dispatcher: they cannot be answered, and run for real on the standing record's word at +0x48.)
    const auto answers = [&](std::uint32_t tier, std::uint32_t standing_ptr, std::uint32_t kind, std::uint32_t staff, std::uint32_t) {
        return Answers{{region_tier, tier}, {contract_with, standing_ptr}, {link_kind, kind}, {staff_test, staff}};
    };
    const std::vector<std::pair<unsigned, Member>> one{{0, {}}};

    // The guards before the loop.
    call(0, club, base, answers(5, standing_at, 0, 0, 0), "no person");
    call(person_at, club, joined({{w32(person_at + 0x14, 0)}}), answers(5, standing_at, 0, 0, 0), "person without contract");
    call(person_at, 0, base, answers(5, standing_at, 0, 0, 0), "no club");
    for (const std::uint32_t tier : {0u, 1u, 2u, 3u, 0x80u, 0xffu, 0x1ff7fu, 0x105u})
        call(person_at, club, joined({member_rows(club, one), standing(1, 0, 1)}), answers(tier, standing_at, 0, 0, 0), "tier");
    call(person_at, club, joined({{w32(club + 0x30, 0)}}), answers(5, standing_at, 0, 0, 0), "no link");
    call(person_at, club, joined({{w32(table + 4, 0)}}), answers(5, standing_at, 0, 0, 0), "empty table entry");
    for (const std::uint32_t rating : {0x128eu, 0x128fu, 0x8000u, 0xffffu, 0x1000u})
        call(person_at, club, joined({{h16(contract_at + 0xa, rating)}, member_rows(club, one), standing(1, 0, 1)}), answers(5, standing_at, 0, 0, 0), "rating");

    // A member for every skip of the loop, then one that passes everything. The callees' answers are the
    // same for all members, so those skips take a case of their own.
    {
        std::vector<std::pair<unsigned, Member>> members;
        members.push_back({1, {.contract = 0}});                       // no contract
        members.push_back({2, {.rating = 0}});
        members.push_back({3, {.half = 0x14}});                         // +0x5c: not above 20
        members.push_back({4, {.half = 0x1014}});                       // low byte decides
        members.push_back({5, {.half = 0x1015}});
        members.push_back({6, {.low = 10}});                            // low byte < tier + 5
        members.push_back({7, {.low = 9}});
        members.push_back({8, {.high = 15}});
        members.push_back({9, {.high = 14}});
        members.push_back({10, {.low = 0}});                            // zero draw bound
        members.push_back({11, {.low = 0xf0}});                         // negative bound
        members.push_back({12, {.high = 0xf0}});
        members.push_back({13, {.low = 1, .high = 1}});
        members.push_back({14, {.low = 7, .high = 8}});
        members.push_back({15, {.low = 8, .high = 12}});
        members.push_back({16, {.rating = 0xea6}});
        members.push_back({17, {.rating = 0xea7}});
        members.push_back({18, {.rating = 0x8000}});
        members.push_back({19, {.low = 0x7f, .high = 0x7f}});
        const std::vector<std::vector<std::uint32_t>> samples{{0, 0, 0, 0}, {1, 2, 3, 4}, {65534, 65535, 1, 2}, {7, 9, 11, 13}, {40000, 50000, 60000, 5}};
        for (const std::uint32_t tier : {3u, 5u, 0x40u, 0x7fu})
            for (const auto& entries : samples)
                for (const std::uint32_t seed : {0u, 5u, 0x1fffu, 0x80000001u})
                    call(person_at, club, joined({member_rows(club, members), standing(1, 0, 1), random_table(scenario, entries, seed)}),
                         answers(tier, standing_at, 0, 0, 0), "members");
        call(person_at, club, joined({member_rows(club, members), standing(1, 0, 2), random_table(scenario, {7, 9}, 0x1fff, true)}),
             answers(5, standing_at, 0, 0, 0), "table wraps");
        call(person_at, club, joined({member_rows(club, members), standing(1, 0, 3), random_table(scenario, {3, 2, 1, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22}, 0, true)}),
             answers(9, standing_at, 0, 0, 0), "table wraps twice");
    }
    // The regions: the person's contract byte at +0xf decides, against each member's.
    for (const std::uint32_t person_region : {0u, 0x14u})
        for (const std::uint32_t second : {0u, 0x14u, 0x13u}) {
            std::vector<std::pair<unsigned, Member>> members{{0, {.region = 0x14}}, {1, {.region = second}}, {2, {.region = 0}}, {3, {.region = 0x14, .rating = 0xea6}}};
            call(person_at, club, joined({member_rows(club, members), standing(1, 0, 1), random_table(scenario, {1, 2, 1, 2, 1, 2, 1, 2}, 0), {b8(contract_at + 0xf, person_region)}}),
                 answers(0x40, standing_at, 0, 0, 0), "regions");
        }
    // The shared answers: the standing, the link kind, the flags, the staff test and the mode byte.
    {
        std::vector<std::pair<unsigned, Member>> members{{0, {.low = 1, .high = 1}}, {1, {.low = 0xf8, .high = 2}}, {2, {.low = 3, .high = 3}}, {3, {.low = 4, .high = 4}}};
        // A slot holding the person itself, and a member that is not there.
        const auto self = Writes{w32(club + 0x98 + 4 * 4, person_at)};
        for (const std::uint32_t state : {0u, 1u, 0x7fu, 0x80u, 0xffu})
            for (const std::uint32_t flags : {0u, 1u, 0x100u, 0x101u, 0x2000u, 0x800u})
                for (const std::uint32_t mode : {0u, 1u, 2u, 3u, 4u, 0x11u, 0xfu})
                    call(person_at, club, joined({member_rows(club, members), self, standing(state, flags, mode), random_table(scenario, {3, 5, 1, 2, 4, 6}, 0)}),
                         answers(6, standing_at, 0, 0, 0), "standing");
        for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 0x102u, 0xffu})
            for (const std::uint32_t staff : {0u, 1u, 0x100u, 0xffu, 0x80u})
                for (const std::uint32_t flags : {0u, 0x800u, 0x7ffu, 0x1800u})
                    call(person_at, club, joined({member_rows(club, members), standing(1, 0, 1), random_table(scenario, {3, 5, 1, 2, 4, 6}, 0), {w32(standing_at + 0x48, flags)}}),
                         answers(6, standing_at, kind, staff, flags), "answers");
        call(person_at, club, joined({member_rows(club, members), standing(1, 0, 1)}), answers(6, 0, 0, 0, 0), "no standing");
    }

    // The real world: persons and clubs as they are, with the callees running (the region test).
    {
        std::vector<std::uint32_t> people;
        const auto first = scenario.global(-0x7be0);
        for (unsigned i = 0; people.size() < 8 && i < 6000; ++i)
            if (scenario.r32(first + i * 0x70 + 0x14)) people.push_back(first + i * 0x70);
        for (unsigned c = 0; c < 6; ++c)
            for (const auto who : people)
                cases.push_back({.entry = entry_squad, .registers = arguments(who, scenario.club(c)), .memory = {}, .f1 = {}, .label = "real"});
        // Real members and the real region test, with the other callees answered.
        for (unsigned c = 0; c < 4; ++c) {
            Writes memory = {w32(scenario.club(c) + 0x30, link_at), w32(link_at, 1), w32(table + 4, 1)};
            const auto at = scenario.club(c);
            for (unsigned slot = 0; slot < 8; ++slot) memory.push_back(w32(at + 0x98 + slot * 4, people[slot]));
            append(memory, standing(1, 0, 1));
            for (const auto who : people)
                cases.push_back({.entry = entry_squad, .registers = arguments(who, at), .memory = memory, .f1 = {}, .label = "real region",
                                 .answers = {{region_tier, 5}, {contract_with, standing_at}, {link_kind, 0}, {staff_test, 0}}});
        }
    }
}
