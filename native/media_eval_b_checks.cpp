// The eval_b block's cases: the reaction 0x106620b0 over real clubs of the career (their persons, contracts and
// rating rows), with the flag bits, the story kind, the factor, the third-column amount, the reporter's list
// and the random table set by the case. Every callee runs for real.
#include "media_checks.hpp"
#include <cstring>
#include <random>

using namespace cm::aot::media_checks;

namespace {
constexpr std::uint32_t entry = 0x106620b0;
constexpr std::uint32_t list_at = scratch_base + 0x2000, table_at = scratch_base + 0x9000;
Write b8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write h16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
void put_double(std::vector<Write>& to, std::uint32_t at, double value) {
    std::uint64_t bits;
    std::memcpy(&bits, &value, 8);
    to.push_back(w32(at, std::uint32_t(bits >> 32)));
    to.push_back(w32(at + 4, std::uint32_t(bits)));
}
// The press's random table (as eval_c's cases): the cursor steps through uint16 entries at `table_at`.
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

MEDIA_CASES("eval_b") {
    const auto log = scenario.global(-0x7bf0);
    const auto ratings = scenario.global(-0x79f0);
    const auto pool = scenario.toc(-0x41fc), singles = scenario.toc(-0x4200);
    // Clubs whose squads have players: the persons in the club's 50 slots (+0x98) with a player record.
    struct Squad {
        std::uint32_t club;
        std::vector<std::uint32_t> players;
    };
    std::vector<Squad> squads;
    for (unsigned i = 0; i < 600 && squads.size() < 24; ++i) {
        const auto club = scenario.club(i);
        if (!scenario.r32(club) || scenario.r32(club) >= 100 || !scenario.r32(club + 0x90)) continue;
        Squad squad{club, {}};
        for (unsigned slot = 0; slot < 50; ++slot) {
            const auto person = scenario.r32(club + 0x98 + 4 * slot);
            if (person && scenario.r32(person + 0x14)) squad.players.push_back(person);
        }
        if (squad.players.size() >= 5) squads.push_back(squad);
    }
    if (squads.size() < 3) return;
    std::mt19937 random(0xeb01);
    const auto pick = [&](std::initializer_list<int> values) { return *(values.begin() + random() % values.size()); };

    struct Knobs {
        std::uint32_t kind = 1, flags = 0, side = 1, amount = 0, third = 5000;
        double factor = 1.0;
        bool listed = false, rival = true, own_player = true;
        std::uint32_t rival_trait = 5;
        unsigned wrap = 700;
        bool answered = false;  // the club-record queries and a few predicates answered by the case
        std::uint32_t ask[7] = {0, 0, 0, 0, 0, 0, 0};
        bool all_pass = false;  // a table of entries that make every "chance" draw come out the same way
    };
    const auto make = [&](const Knobs& k, const char* label) {
        const auto& squad = squads[random() % squads.size()];
        const auto club = squad.club;
        const auto player = k.own_player ? squad.players[random() % squad.players.size()] : (random() % 4 ? squads[(random() % squads.size())].players[0] : 0);
        std::vector<Write> memory;
        const auto row = ratings ? scenario.r32(ratings) + scenario.r32(club) * 0x4c : 0;
        memory.push_back(h16(row + 0xa, k.third));
        const auto rival = k.rival ? squad.players[random() % squad.players.size()] : 0;
        memory.push_back(w32(club + 0x80, rival));
        if (rival) memory.push_back(b8(rival + 0x63, k.rival_trait));
        // The reporter's list: three rows of 0x18 bytes, one of them (when listed) the player's id with byte 2.
        memory.push_back(w32(log, list_at));
        memory.push_back(w32(log + 8, 3));
        for (unsigned i = 0; i < 3; ++i) {
            memory.push_back(w32(list_at + i * 0x18 + 8, k.listed && i == 1 && player ? scenario.r32(player) : 0x7777 + i));
            memory.push_back(b8(list_at + i * 0x18 + 0x10, k.listed ? 2 : i));
        }
        put_double(memory, pool + 0x38, 0.5);
        std::vector<std::uint32_t> entries;
        // Random entries, or a pattern that makes the draws come out alike (all low, all high, alternating, in threes).
        const auto low = std::uint32_t(pick({0, 1, 2, 3, 5, 7, 19, 29, 49, 99, 119}));
        const auto high = std::uint32_t(pick({0xffff, 0x7fff, 0xfffe, 100, 127, 200}));
        const unsigned shape = k.all_pass ? 1 + random() % 4 : 0;
        for (unsigned i = 0; i < 700; ++i)
            entries.push_back(shape == 0 ? random() % 0x10000 : shape == 1 ? (i % 2 ? high : low) : shape == 2 ? low : shape == 3 ? (i % 3 ? low : high) : (i % 3 == 1 ? high : low));
        {
            const auto t = table(scenario, entries, random() % 2 ? 0 : random() % 0x10000, k.wrap);
            memory.insert(memory.end(), t.begin(), t.end());
        }
        (void)singles;
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
        if (k.answered)
            answers = {{0x100d9480, k.ask[0]}, {0x100d94d0, k.ask[1]}, {0x100d9520, k.ask[2]}, {0x100d9580, k.ask[3]},
                       {0x10003dc0, k.ask[4]}, {0x103f43b0, k.ask[5]}, {0x100cab10, k.ask[6]}};
        cases.push_back({.entry = entry,
                         .registers = {{3, log}, {4, club}, {5, k.kind}, {6, k.flags}, {7, 0x71717171}, {8, k.side}, {9, k.amount}, {10, player}},
                         .memory = memory, .f1 = k.factor, .label = label, .answers = answers});
    };
    const std::uint32_t kinds[] = {1, 3, 6, 0xa, 0x13, 0x1e, 0x20, 0x4, 0x22, 0};
    const std::uint32_t singles_of_flags[] = {0, 1, 2, 4, 8, 0x10, 0x20, 0x40, 0x200, 0x80, 0x100, 0x400, 0x480, 0x500, 0x440, 0x540, 0x21, 0x60, 0x41};
    const auto random_knobs = [&] {
        Knobs k;
        k.kind = kinds[random() % std::size(kinds)];
        k.flags = singles_of_flags[random() % std::size(singles_of_flags)];
        if (random() % 4 == 0) k.flags |= singles_of_flags[random() % std::size(singles_of_flags)];
        k.side = pick({1, 2, 3, 0, 4});
        k.amount = std::uint32_t(pick({0, 5, -5, 300, -300, 20000, -20000}));
        k.third = std::uint32_t(pick({0, 1, 2, 9999, 10000, 0x2711, 0x7fff, 0x8000, 5000}));
        k.factor = double(pick({-30, -3, 0, 5, 10, 30, 100, 400, 2000, 700000, -700000, 90000, 3000000, -9000000})) / 10.0;
        k.listed = random() % 3 == 0;
        k.rival = random() % 8 != 0;
        k.rival_trait = std::uint32_t(pick({0, 3, 10, 12, 20, 127, 0x80, 0xff}));
        k.wrap = random() % 3 ? 700 : random() % 14;
        k.all_pass = random() % 3 == 0;
        k.answered = random() % 2 == 0;
        for (auto& ask : k.ask) ask = random() % 2;
        k.own_player = random() % 8 != 0;
        return k;
    };
    for (unsigned n = 0; n < 8000; ++n) make(random_knobs(), "sweep");
    // Each verdict flag with each kind, the player and the others.
    for (const auto kind : kinds)
        for (const auto flag : {0x80u, 0x100u, 0x400u | 0x80u, 0x400u | 0x100u, 0x400u, 0x40u})
            for (unsigned n = 0; n < 6; ++n) {
                auto k = random_knobs();
                k.kind = kind;
                k.flags = flag;
                make(k, "flag");
            }
    // Each single flag with patterned tables, big factors (the float-sized ranges of flag 0x20) and each kind.
    for (const auto flag : singles_of_flags)
        for (unsigned n = 0; n < 80; ++n) {
            auto k = random_knobs();
            k.flags = flag;
            k.all_pass = true;
            if (flag & 0x20) k.factor = double(pick({700000, -700000, 90000, 3000000, 1000, 100})) / 10.0;
            make(k, "directed");
        }
    {
        // The early exits: no club, no player, a player without a contract.
        cases.push_back({.entry = entry, .registers = {{3, log}, {4, 0}, {5, 1}, {6, 0x80}, {8, 1}, {9, 5}, {10, squads[0].players[0]}}, .memory = {}, .f1 = 1.0, .label = "no club"});
        cases.push_back({.entry = entry, .registers = {{3, log}, {4, squads[0].club}, {5, 1}, {6, 0x80}, {8, 1}, {9, 5}, {10, 0}}, .memory = {}, .f1 = 1.0, .label = "no player"});
        cases.push_back({.entry = entry, .registers = {{3, log}, {4, squads[0].club}, {5, 1}, {6, 0x80}, {8, 1}, {9, 5}, {10, squads[1].players[0]}}, .memory = {}, .f1 = 1.0, .label = "player of another club"});
    }
}
