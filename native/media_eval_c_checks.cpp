// The eval_c block's cases: the club story evaluation 0x10672800 over real clubs of the career (a
// club with a manager and a ratings row), with the row's flag halfword and rating columns, the pool's
// factors, the rival and the random table set by the case. In the answered families the club-record
// queries and the posting routine are answered and the group chain is set by the case; in the real
// families every callee runs (the rating routines of the values block always do).
#include "media_checks.hpp"
#include <cstring>
#include <map>
#include <random>

using namespace cm::aot::media_checks;

namespace {
constexpr std::uint32_t entry = 0x10672800;
constexpr std::uint32_t chain_at = scratch_base + 0x2800, record_at = scratch_base + 0x2810, table_at = scratch_base + 0x9000;
Write b8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write h16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
void append(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }
void put_double(std::vector<Write>& to, std::uint32_t at, double value) {
    std::uint64_t bits;
    std::memcpy(&bits, &value, 8);
    to.push_back(w32(at, std::uint32_t(bits >> 32)));
    to.push_back(w32(at + 4, std::uint32_t(bits)));
}
std::uint32_t single_bits(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}

// The press's random table: the cursor steps through uint16 entries at `table_at` (the first draw reads the
// first entry), the seed is added to each; the end lies `length` entries on, so a draw past it refills.
std::vector<Write> table(const Scenario& s, const std::vector<std::uint32_t>& entries, std::uint32_t seed, unsigned length) {
    std::vector<Write> writes;
    const auto end = table_at + 2 * length;
    for (std::size_t i = 0; i < entries.size() && i < length + 2; ++i) writes.push_back(h16(table_at + 2 * std::uint32_t(i + 1), entries[i]));
    writes.push_back(w32(s.toc(-0x7b84), table_at));
    writes.push_back(w32(s.toc(-0x7b88), end));
    writes.push_back(w32(s.toc(-0x7b8c), table_at));
    writes.push_back(w32(s.toc(-0x7b90), seed));
    return writes;
}
}

MEDIA_CASES("eval_c") {
    const auto log = scenario.global(-0x7bf0);
    const auto ratings = scenario.global(-0x79f0);
    const auto pool = scenario.toc(-0x41fc), singles = scenario.toc(-0x4200);
    std::vector<std::uint32_t> clubs;
    for (unsigned i = 0; i < 400 && clubs.size() < 6; ++i) {
        const auto row = scenario.club(i);
        const auto manager = scenario.r32(row + 0x90);
        if (scenario.r32(row) && scenario.r32(row) < 100 && manager && scenario.r32(manager + 0x1c)) clubs.push_back(row);
    }
    // The objects the clubs' groups hold at +8 (r18 of the story), for the chained families.
    std::vector<std::uint32_t> groups;
    std::map<std::uint32_t, std::uint32_t> own_group;
    for (const auto club : clubs) {
        const auto group = scenario.r32(club + 0x30);
        if (!group) continue;
        const auto record = scenario.r32(scenario.global(-0x7b5c) + (scenario.r32(group) << 2));
        if (record && scenario.r32(record + 8)) {
            groups.push_back(scenario.r32(record + 8));
            own_group[club] = scenario.r32(record + 8);
        }
    }
    if (groups.empty()) return;
    std::vector<std::uint32_t> grouped_clubs;
    for (const auto& [club, group] : own_group) grouped_clubs.push_back(club);
    // 0x10459b50 searches the group object's array (base +0xc, count +0x14, rows of 0x7c bytes keyed by the
    // club's id) and the story reads the byte at +4 of the row it finds.
    const auto group_row = [&](std::uint32_t group, std::uint32_t club) -> std::uint32_t {
        const auto base = scenario.r32(group + 0xc), count = scenario.r32(group + 0x14);
        for (std::uint32_t i = 0; i < count && i < 4000; ++i)
            if (scenario.r32(base + i * 0x7c) == scenario.r32(club)) return base + i * 0x7c;
        return 0;
    };
    // The persons the clubs name as their rivals, for the rival of a case.
    std::vector<std::uint32_t> people;
    for (const auto club : clubs) {
        if (scenario.r32(club + 0x80)) people.push_back(scenario.r32(club + 0x80));
        people.push_back(scenario.r32(club + 0x90));
    }
    // The story's other person: real players with a profile (rows of the 0x70-byte table).
    const auto persons = scenario.global(-0x7be0), count = scenario.global(-0x7be4);
    std::vector<std::uint32_t> players;
    for (unsigned i = 0; i < count && players.size() < 12; i += 97) {
        const auto row = persons + i * 0x70;
        if (scenario.r32(row + 0x14)) players.push_back(row);
    }
    if (clubs.size() < 3 || people.empty() || players.size() < 4) return;
    std::mt19937 random(0xec01);
    const auto pick = [&](std::initializer_list<int> values) { return *(values.begin() + random() % values.size()); };

    enum class Mode { answered_chain, answered_plain, real };
    struct Knobs {
        std::uint32_t flags = 0, third = 5000;
        float factor = 1.0f, ratio = 100.0f, below = 0.5f;
        double scale = 0.5, pull = 0.25;
        double f1 = 1.0;
        bool rival = true;
        bool other = true;
        unsigned wrap = 700;
        std::uint32_t query = 0x10;
        std::uint32_t ask[4] = {0, 0, 0, 0};
        bool grouped = false;  // a club with a record in its own group, whose queries find it
        std::int32_t group_flag = 0;
        std::uint32_t group_byte = 1;
        Mode mode = Mode::real;
    };
    const auto make = [&](std::uint32_t kind, std::uint32_t side, const Knobs& k, const char* label) {
        auto club = clubs[random() % clubs.size()];
        if (k.grouped) club = grouped_clubs[random() % grouped_clubs.size()];
        const auto other = k.other ? players[random() % players.size()] : 0;
        const auto row = ratings ? scenario.r32(ratings) + scenario.r32(club) * 0x4c : 0;
        std::vector<Write> memory;
        memory.push_back(h16(row + 0xe, k.flags));
        memory.push_back(h16(row + 0xa, k.third));
        const auto rival = k.rival ? people[random() % people.size()] : 0;
        memory.push_back(w32(club + 0x80, rival));
        if (k.rival) memory.push_back(b8(rival + 0x63, pick({-128, -3, 0, 5, 10, 11, 15, 30, 127})));
        // The pool: the factor when the group has no record, the strength's ratio, and the limits of side 3.
        memory.push_back(w32(singles + 8, single_bits(k.factor)));
        memory.push_back(w32(singles + 0x54, single_bits(k.ratio)));
        memory.push_back(w32(singles + 0xc, single_bits(k.below)));
        put_double(memory, pool + 0x38, k.scale);
        put_double(memory, pool + 0x40, k.pull);
        if (k.mode == Mode::answered_chain) {
            memory.push_back(w32(club + 0x30, chain_at));
            memory.push_back(w32(chain_at, 7));
            memory.push_back(w32(scenario.global(-0x7b5c) + 7 * 4, record_at));
            const auto own = own_group.find(club);
            const auto group = own != own_group.end() && (k.grouped || random() % 4) ? own->second : groups[random() % groups.size()];
            memory.push_back(w32(record_at + 8, group));
            memory.push_back(w32(record_at + 0x1c, std::uint32_t(k.group_flag)));
            memory.push_back(b8(record_at + 0xc1, k.group_byte));
            if (const auto row = group_row(group, club)) memory.push_back(b8(row + 4, k.query));
        }
        std::vector<std::uint32_t> entries;
        for (unsigned i = 0; i < 700; ++i) entries.push_back(random() % 0x10000);
        append(memory, table(scenario, entries, random() % 2 ? 0 : random() % 0x10000, k.wrap));
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
        if (k.mode != Mode::real) {
            // (the base factor is answered too: it then stays the case's f1; the club queries run for real)
            answers = {{0x100c7d00, k.mode == Mode::answered_plain ? 1u : 0u}, {0x10457d00, 0}, {0x106620b0, 0},
                       {0x100d9480, k.ask[0]}, {0x100d94d0, k.ask[1]}, {0x100d9520, k.ask[2]}, {0x100d9580, k.ask[3]}};
        }
        cases.push_back({.entry = entry,
                         .registers = {{3, log}, {4, club}, {5, kind}, {6, side}, {7, other}, {8, 0x81818181}, {9, 0x91919191}, {10, 0xa1a1a1a1}},
                         .memory = memory, .f1 = k.f1, .label = label, .answers = answers});
    };
    const std::uint32_t kinds[] = {0x3c, 0x3d, 0x63, 0x5b, 0x61, 0x62, 0x5d, 0x5f, 0x5c, 0x3f, 0x60, 0x43, 0x40, 0x41, 0x42, 0x52, 0x44, 0x5a, 0x00, 0x45, 0xffff, 0x10063};
    const std::uint32_t sides[] = {1, 2, 3, 0, 4, 0xff, 0x7f}, family_sides[] = {1, 2, 3};
    const auto random_knobs = [&](Mode mode) {
        Knobs k;
        k.mode = mode;
        // Flag bits: sparse random ones, since each story tests different masks.
        k.flags = 0;
        if (random() % 2) {
            for (unsigned bit = 0; bit < 16; ++bit) if (random() % 4 == 0) k.flags |= 1u << bit;
        } else k.flags = std::uint32_t(pick({0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x400, 0x48, 0x41, 0x60, 0x9, 0x6, 0x30, 0x42, 0x44, 0x8002, 0x8000}));
        k.third = pick({0, 1, 2, 9999, 10000, 0x2710, 0x2711, 0x7fff, 0x8000, 0xffff, 5000, 1234});
        k.factor = float(pick({-3, 0, 1, 2, 3, 5, 50, 100, 150})) / 10.0f;
        k.ratio = float(pick({-30000, -2000, -70, -3, 0, 1, 7, 70, 400, 2000, 30000, 100000, 3000000}));
        k.below = float(pick({-50, 0, 5, 40, 90, 100, 150})) / 10.0f;
        k.f1 = double(pick({-30, 0, 1, 5, 10, 25, 60, 99, 100, 101, 150, 400, 1200, 2000, 5000, 9000, 20000, 5000, 20000})) / 10.0;
        k.scale = pick({0, 1, 2, 3, 4}) == 0 ? pick({-3, 0, 6, 40, 90}) : double(pick({1, 2, 5, 10, 20, 30})) / 10.0;
        k.pull = pick({0, 1, 2, 3}) == 0 ? pick({-4, 0, 7, 30}) : double(pick({1, 2, 5, 10, 30})) / 10.0;
        k.rival = random() % 40 != 0;
        k.other = random() % 5 != 0;
        k.wrap = random() % 3 ? 700 : random() % 14;
        k.query = pick({0, 5, 0x18, 0x19, 0x20, 0xff});
        for (auto& ask : k.ask) ask = random() % 2;
        k.group_flag = pick({-1, 0, 5, -1});
        k.group_byte = pick({0, 1, 0xff});
        return k;
    };
    // A sweep of every kind and side, then each family with many variants.
    for (const auto kind : kinds)
        for (const auto side : sides) make(kind, side, random_knobs(Mode::answered_chain), "sweep");
    // Side 1 of every kind with a draw that always passes and a club the queries find.
    for (const auto kind : kinds)
        for (unsigned n = 0; n < 80; ++n) {
            auto k = random_knobs(Mode::answered_chain);
            k.grouped = true;
            k.f1 = 200.0;
            k.query = pick({0, 5, 0x18, 0x18, 0x19});
            make(kind, 1, k, "queried");
        }
    for (const auto kind : kinds)
        for (const auto side : family_sides)
            for (unsigned n = 0; n < 60; ++n) {
                const auto mode = pick({0, 1, 2}) == 0 ? Mode::answered_plain : Mode::answered_chain;
                make(kind, side, random_knobs(mode), "family");
            }
    for (const auto kind : kinds)
        for (const auto side : family_sides)
            for (unsigned n = 0; n < 12; ++n) make(kind, side, random_knobs(Mode::real), "real callees");
    {
        // The early exits: no club, no manager, no standing.
        cases.push_back({.entry = entry, .registers = {{3, log}, {4, 0}, {5, 0x3c}, {6, 1}, {7, players[0]}}, .memory = {}, .f1 = {}, .label = "no club"});
        cases.push_back({.entry = entry, .registers = {{3, log}, {4, clubs[0]}, {5, 0x3c}, {6, 1}, {7, players[0]}}, .memory = {w32(clubs[0] + 0x90, 0)}, .f1 = {}, .label = "no manager"});
        cases.push_back({.entry = entry, .registers = {{3, log}, {4, clubs[0]}, {5, 0x3c}, {6, 1}, {7, players[0]}},
                         .memory = {w32(scenario.r32(clubs[0] + 0x90) + 0x1c, 0)}, .f1 = {}, .label = "manager without +0x1c"});
        cases.push_back({.entry = entry, .registers = {{3, log}, {4, clubs[0]}, {5, 0x3c}, {6, 1}, {7, players[0]}}, .memory = {w32(clubs[0] + 0x30, 0)},
                         .f1 = {}, .label = "no group", .answers = {{0x100c7d00, 0}}});
        cases.push_back({.entry = entry, .registers = {{3, log}, {4, clubs[0]}, {5, 0x3c}, {6, 1}, {7, players[0]}},
                         .memory = {w32(clubs[0] + 0x30, chain_at), w32(chain_at, 7), w32(scenario.global(-0x7b5c) + 28, 0)}, .f1 = {}, .label = "group without entry",
                         .answers = {{0x100c7d00, 0}}});
        cases.push_back({.entry = entry, .registers = {{3, log}, {4, clubs[0]}, {5, 0x3c}, {6, 1}, {7, players[0]}},
                         .memory = {w32(clubs[0] + 0x30, chain_at), w32(chain_at, 7), w32(scenario.global(-0x7b5c) + 28, record_at), w32(record_at + 8, 0)}, .f1 = {},
                         .label = "group without second record", .answers = {{0x100c7d00, 0}}});
    }
}
