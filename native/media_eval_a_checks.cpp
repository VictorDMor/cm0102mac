// The eval_a block's cases: the manager's verdict 0x106584d0 over real managers (a person with a staff record
// and a club) and real players (a person with a player record), with the traits, staff attributes, contract,
// reputations, the story's amount and the random table set by the case. The club-record questions, the
// contract finder and the staff trait routine are answered in most cases (so a case can steer each draw's range
// and every branch) and run for real in the rest; the story value runs for real, scaled by the pool's float.
// The transfer kinds (0x28-0x3b) are told by 0x1069ecf0, which the C++ calls directly: it cannot be answered, so
// the cases pick the kind.
#include "media_checks.hpp"
#include <cstdlib>
#include <cstring>
#include <limits>
#include <random>

using namespace cm::aot::media_checks;

namespace {
constexpr std::uint32_t entry = 0x106584d0;
constexpr std::uint32_t contract_at = scratch_base + 0x4000, mover_at = scratch_base + 0x4100, table_at = scratch_base + 0x9000;
Write b8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write h16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
void append(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }
std::uint32_t single_bits(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}
// The press's random table (see the eval_c cases): `length` entries before the refill.
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

MEDIA_CASES("eval_a") {
    const auto log = scenario.global(-0x7bf0);
    const auto persons = scenario.global(-0x7be0), count = scenario.global(-0x7be4);
    const auto indexes = scenario.global(-0x7b9c);
    const auto singles = scenario.toc(-0x4200);
    std::vector<std::uint32_t> managers, players;
    for (unsigned i = 0; i < count && (managers.size() < 14 || players.size() < 20); i += 7) {
        const auto row = persons + i * 0x70;
        if (scenario.r32(row + 0x1c) && scenario.r32(row + 0x24) && scenario.r32(row) < count && managers.size() < 14) managers.push_back(row);
        else if (scenario.r32(row + 0x14) && scenario.r32(row) < count && players.size() < 20) players.push_back(row);
    }
    if (managers.size() < 3 || players.size() < 4) return;
    std::mt19937 random(0xe0a1);
    const auto pick = [&](std::initializer_list<int> values) { return *(values.begin() + random() % values.size()); };

    struct Knobs {
        bool answered = true;     // answer the questions, the contract and the staff trait
        float factor = 1.0f;      // the pool's scale of the story value (0x10661a50 runs for real: the verdict calls it directly)
        std::uint32_t state = 0, contract_46 = 0, contract_44 = 0, contract_54 = 0, queries[6] = {0, 0, 0, 0, 0, 0};
        std::int32_t trait = 5;   // what 0x100d2f50 answers
        std::uint32_t mover = 0;  // 0: none; 1: the player's id; 2: another
        unsigned wrap = 700;
        bool no_player = false;
        bool steady = false;      // traits of 2 or more, so that the draws before the amount's are nonzero
        bool lucky = false;       // small traits (their draws come up zero) and a table of zero or full samples
    };
    // EVAL_A_KIND=<hex kind> runs the cases of one kind (to bisect a difference).
    const auto only_kind = std::getenv("EVAL_A_KIND") ? std::strtoul(std::getenv("EVAL_A_KIND"), nullptr, 16) : ~0ul;
    const auto make = [&](std::uint32_t kind, const Knobs& k, const char* label) {
        if (only_kind != ~0ul && kind != only_kind) return;
        const auto manager = managers[random() % managers.size()];
        const auto player = k.no_player ? 0 : players[random() % players.size()];
        const auto staff = scenario.r32(manager + 0x1c);
        const auto club = scenario.r32(manager + 0x24);
        std::vector<Write> memory;
        for (unsigned offset = 0x63; offset <= 0x69; ++offset) memory.push_back(b8(manager + offset, k.lucky ? pick({1, 1, 1, 2}) : k.steady ? pick({2, 3, 5, 8, 16, 40, 127}) : pick({-128, -1, 0, 1, 1, 2, 3, 5, 8, 15, 16, 20, 40, 127})));
        for (const unsigned offset : {0x34u, 0x39u, 0x3bu, 0x3du, 0x3fu}) memory.push_back(b8(staff + offset, k.lucky ? pick({1, 1, 1, 2}) : k.steady ? pick({2, 4, 7, 12, 30, 100, 127}) : pick({-128, -3, 0, 1, 1, 2, 4, 7, 12, 30, 100, 127})));
        memory.push_back(h16(club + 0x1ec, pick({0, 100, 2000, 5000, 0x7fff, 0x8000, 0xffff})));
        if (player) {
            const auto record = scenario.r32(player + 0x14);
            const auto id = scenario.r32(player);
            memory.push_back(b8(player + 0x66, k.lucky ? 1 : k.steady ? pick({2, 9, 40, 127}) : pick({-5, 0, 1, 2, 9, 40, 127})));
            memory.push_back(b8(player + 0x67, k.lucky ? 1 : k.steady ? pick({2, 9, 40, 127}) : pick({-5, 0, 1, 1, 2, 9, 40, 127})));
            memory.push_back(b8(record + 0x44, k.lucky ? pick({0, 1}) : pick({-20, 0, 0, 1, 12, 20})));
            memory.push_back(b8(record + 0x45, k.lucky ? 1 : pick({-3, 0, 1, 1, 9, 60})));
            memory.push_back(b8(record + 0x25, k.lucky ? 1 : k.steady ? pick({2, 4, 9, 60}) : pick({-3, 0, 1, 4, 9, 60})));
            memory.push_back(h16(record + 0xa, pick({0, 100, 2000, 5000, 5250, 5251, 5300, 0x7fff, 0x8000, 0xffff})));
            memory.push_back(b8(indexes + id * 0x64 + 0xa, k.lucky ? 1 : pick({0, 1, 1, 3, 40, 255})));
            memory.push_back(b8(indexes + id * 0x64 + 0xc, pick({0, 1, 2, 3, 0xff})));
            if (k.mover == 1) memory.push_back(w32(mover_at + 0x10, id));
            else if (k.mover == 2) memory.push_back(w32(mover_at + 0x10, id + 1));
        }
        memory.push_back(b8(contract_at + 0x60, 0xf0 | k.state));
        memory.push_back(b8(contract_at + 0x46, k.contract_46));
        memory.push_back(b8(contract_at + 0x44, k.contract_44));
        memory.push_back(b8(contract_at + 0x54, k.contract_54));
        memory.push_back(w32(singles + 8, single_bits(1.0f)));
        memory.push_back(w32(singles + 0xc, single_bits(k.factor)));
        std::vector<std::uint32_t> entries;
        for (unsigned i = 0; i < 80; ++i) entries.push_back(k.lucky ? pick({0, 0, 1, 0xffff, 0x7fff, 3, 100}) : random() % 3 ? random() % 0x10000 : pick({0, 1, 0xffff, 0x1000}));
        append(memory, table(scenario, entries, k.lucky ? pick({0, 0, 5, 0x7ffe}) : random() % 2 ? 0 : random() % 0x10000, k.wrap));
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
        if (k.answered) {
            answers = {{0x10082f90, contract_at}, {0x100d2f50, std::uint32_t(k.trait)}, {0x100d9580, k.queries[0]}, {0x100d9520, k.queries[1]},
                       {0x100d3150, k.queries[2]}, {0x100d3040, k.queries[3]}, {0x100d9b80, k.queries[5]},
                       {0x1036d240, k.mover ? mover_at : 0}};
        }
        cases.push_back({.entry = entry,
                         .registers = {{3, log}, {4, manager}, {5, player}, {6, kind}, {7, 0x81818181}, {8, 0x91919191}, {9, 0xa1a1a1a1}, {10, 0xb1b1b1b1}},
                         .memory = memory, .f1 = 1.0, .label = label, .answers = answers});
    };
    const std::uint32_t kinds[] = {0x83, 0x82, 0x1f, 0x3c, 0x3d, 0x10, 0x00, 0x11, 0x52, 0x0d, 0x0e, 0x02, 0x21, 0x22, 0x23, 0x09, 0x0c, 0x08, 0x25, 0x01, 0x1e,
                                   0x20, 0x5a, 0x5b, 0x5c, 0x5d, 0x42, 0x43, 0x44, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x3f, 0x40, 0x41, 0x03, 0x04, 0x05, 0x06, 0x07,
                                   0x45, 0xffff, 0x10083, 0x10006, 0xff, 0x27, 0x28, 0x2c, 0x3b};
    const auto random_knobs = [&](bool answered) {
        Knobs k;
        k.answered = answered;
        k.factor = float(pick({0, 1, -1, 3, 10, -10, 50, -50, 100, 300, -300, 1000, 5000, -5000, 20000, -20000, 1000000, -1000000, 2000000000})) / 100.0f;
        k.state = random() % 16;
        k.contract_46 = pick({0, 1, 2, 5, 0xff});
        k.contract_44 = pick({0, 1, 5, 40, 90, 127, 0xff});
        k.contract_54 = pick({0, 0, 1, 0x80});
        for (auto& q : k.queries) q = pick({0, 1, 0x100, 0x80, 0xff});
        k.trait = pick({-128, -1, 0, 1, 2, 3, 5, 9, 10, 11, 15, 16, 30, 127, 0x80, 0x100});
        k.mover = random() % 3;
        k.wrap = random() % 3 ? 700 : random() % 14;
        k.no_player = random() % 30 == 0;
        return k;
    };
    for (const auto kind : kinds) {
        for (unsigned n = 0; n < 90; ++n) make(kind, random_knobs(true), "answered");
        for (unsigned n = 0; n < 8; ++n) make(kind, random_knobs(false), "real callees");
    }
    // Draws that come up zero and samples at the extremes, to reach the ends of the chains; and amounts beyond 16 bits.
    for (const auto kind : kinds)
        for (unsigned n = 0; n < 70; ++n) {
            auto k = random_knobs(true);
            k.lucky = true;
            k.trait = pick({1, 1, 2, 0x101});
            k.factor = n % 3 == 0 ? float(pick({1000000, -1000000, 7000000, -300000})) : float(pick({0, 1, -1, 3, 100, -100, 1000})) / 100.0f;
            make(kind, k, n % 3 == 0 ? "wide" : "lucky");
        }
    // Amounts beyond 16 bits through the chains that draw against the amount (the float form of the draw), with
    // traits that leave the draws before them nonzero.
    for (const auto kind : {0x00u, 0x11u, 0x52u, 0x0du, 0x0eu, 0x02u, 0x21u, 0x22u, 0x23u, 0x09u, 0x0cu, 0x03u, 0x04u, 0x05u, 0x06u,
                            0x28u, 0x31u, 0x3bu})
        for (unsigned n = 0; n < 260; ++n) {
            auto k = random_knobs(true);
            k.factor = float(pick({1000000000, -1000000000, 3000000, -3000000, 50000, -50000, 1000000}));
            k.steady = true;
            k.trait = pick({2, 3, 5, 9, 16, 30, 127});
            k.state = pick({0, 1, 2, 3, 4, 7, 8, 9});
            make(kind, k, "far amounts");
        }
    // A factor that is not a number makes the amount 0x80000000 (fctiwz of a NaN), so `20 - amount` of the transfer
    // stories wraps: the C++ computes it unsigned as the original's subfic does.
    for (const auto kind : {0x28u, 0x31u, 0x3bu})
        for (unsigned n = 0; n < 260; ++n) {
            auto k = random_knobs(true);
            k.factor = std::numeric_limits<float>::quiet_NaN();
            k.steady = n % 2 == 0;
            k.lucky = n % 5 == 4;
            k.trait = pick({2, 3, 5, 9, 16, 30, 127});
            make(kind, k, "wrapping amount");
        }
    // Every pair of questions, every contract state, on the kinds that ask.
    for (const auto kind : {0x82u, 0x1fu, 0x3cu, 0x83u, 0x28u, 0x03u})
        for (unsigned state = 0; state < 16; ++state)
            for (unsigned q = 0; q < 4; ++q)
                for (unsigned n = 0; n < 3; ++n) {
                    auto k = random_knobs(true);
                    k.state = state;
                    k.queries[0] = q & 1;
                    k.queries[1] = q >> 1;
                    make(kind, k, "pairs");
                }
    // The early exits: no manager, no staff, no club, no contract.
    {
        const auto manager = managers[0], player = players[0];
        const auto basic = [&](std::uint32_t m, std::uint32_t p, std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers, const char* label) {
            cases.push_back({.entry = entry, .registers = {{3, log}, {4, m}, {5, p}, {6, 0x3c}, {7, 0x81818181}}, .memory = std::move(memory), .f1 = 1.0, .label = label, .answers = std::move(answers)});
        };
        basic(0, player, {}, {}, "no manager");
        basic(manager, player, {w32(manager + 0x1c, 0)}, {}, "no staff record");
        basic(manager, player, {w32(player + 0x14, 0)}, {}, "player without record");
        basic(manager, manager, {}, {}, "the manager himself");
        basic(manager, player, {w32(manager + 0x24, 0), w32(manager + 0x20, 0)}, {}, "no club");
        basic(manager, player, {w32(manager + 0x24, 0)}, {{0x10082f90, 0}}, "club from +0x20, no contract");
        basic(manager, 0, {}, {{0x10082f90, contract_at}, {0x100d9580, 0}, {0x100d9520, 0}}, "no player");
    }
}

// Closing cases: one manager and one player with every byte the verdict reads set by the case, and the random
// table written entry by entry, for the outcomes the random sweeps above cannot choose: the contract-review
// question saying yes (kind 0x1f), the second chain of kind 6 passing, and amounts of exactly 20 and between
// 10 and 19 in the transfer stories (the factor is picked from the story value the probe printed).
MEDIA_CASES("eval_a") {
    const auto log = scenario.global(-0x7bf0);
    const auto persons = scenario.global(-0x7be0), count = scenario.global(-0x7be4);
    const auto indexes = scenario.global(-0x7b9c);
    const auto singles = scenario.toc(-0x4200);
    std::vector<std::uint32_t> managers, players;
    for (unsigned i = 0; i < count && (managers.size() < 3 || players.size() < 40); i += 7) {
        const auto row = persons + i * 0x70;
        if (scenario.r32(row + 0x1c) && scenario.r32(row + 0x24) && scenario.r32(row) < count && managers.size() < 3) managers.push_back(row);
        else if (scenario.r32(row + 0x14) && scenario.r32(row) < count && players.size() < 40) players.push_back(row);
    }
    if (managers.size() < 3 || players.size() < 40) return;
    struct Spec {
        std::uint32_t kind = 0;
        float factor = 1.0f;
        std::vector<std::uint32_t> entries;  // the random table, from its first sample; zeros after
        unsigned who = 0, pl = 0;            // which manager and which player
        int manager_traits[7] = {5, 5, 5, 5, 5, 5, 5};  // bytes 0x63-0x69 of the manager
        int staff_skills = 5;                            // the five staff bytes
        int sportsmanship = 5, dirtiness = 5, subject_loyalty = 5;
        int trait = 1;                                   // what the effective-skill routine answers
        std::uint32_t queries[6] = {0, 0, 0, 0, 0, 0};
        std::uint32_t c44 = 0, c46 = 0, c50 = 0, c54 = 0, c5c = 0, state = 0;
        std::uint32_t contract_person = 0, contract_club = 0;
        const char* label = "closing";
    };
    const auto build = [&](const Spec& k) {
        const auto manager = managers[k.who], player = players[k.pl];
        const auto staff = scenario.r32(manager + 0x1c), club = scenario.r32(manager + 0x24);
        std::vector<Write> memory;
        for (unsigned i = 0; i < 7; ++i) memory.push_back(b8(manager + 0x63 + i, std::uint32_t(k.manager_traits[i])));
        for (const unsigned offset : {0x34u, 0x39u, 0x3bu, 0x3du, 0x3fu}) memory.push_back(b8(staff + offset, std::uint32_t(k.staff_skills)));
        memory.push_back(h16(club + 0x1ec, 5000));
        const auto record = scenario.r32(player + 0x14);
        const auto id = scenario.r32(player);
        memory.push_back(b8(player + 0x66, 9)); memory.push_back(b8(player + 0x67, std::uint32_t(k.sportsmanship)));
        memory.push_back(b8(player + 0x65, std::uint32_t(k.subject_loyalty)));
        memory.push_back(b8(record + 0x44, 1)); memory.push_back(b8(record + 0x45, 9));
        memory.push_back(b8(record + 0x25, std::uint32_t(k.dirtiness)));
        memory.push_back(h16(record + 0xa, 5000));
        memory.push_back(b8(indexes + id * 0x64 + 0xa, 1)); memory.push_back(b8(indexes + id * 0x64 + 0xc, 1));
        memory.push_back(b8(contract_at + 0x60, 0xf0 | k.state));
        memory.push_back(b8(contract_at + 0x44, k.c44)); memory.push_back(b8(contract_at + 0x46, k.c46));
        memory.push_back(w32(contract_at + 0x50, k.c50)); memory.push_back(b8(contract_at + 0x54, k.c54));
        memory.push_back(b8(contract_at + 0x5c, k.c5c));
        memory.push_back(w32(contract_at, k.contract_person ? scenario.r32(k.contract_person) : 0));
        memory.push_back(w32(contract_at + 4, k.contract_club ? scenario.r32(k.contract_club) : 0));
        memory.push_back(w32(singles + 8, single_bits(1.0f)));
        memory.push_back(w32(singles + 0xc, single_bits(k.factor)));
        std::vector<std::uint32_t> entries = k.entries;
        entries.resize(120, 0);
        append(memory, table(scenario, entries, 0, 700));
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {
            {0x10082f90, contract_at}, {0x100d2f50, std::uint32_t(k.trait)}, {0x100d9580, k.queries[0]}, {0x100d9520, k.queries[1]},
            {0x100d3150, k.queries[2]}, {0x100d3040, k.queries[3]}, {0x100d9b80, k.queries[5]}, {0x1036d240, 0}};
        cases.push_back({.entry = entry,
                         .registers = {{3, log}, {4, manager}, {5, player}, {6, k.kind}, {7, 0x81818181}, {8, 0x91919191}, {9, 0xa1a1a1a1}, {10, 0xb1b1b1b1}},
                         .memory = memory, .f1 = 1.0, .label = k.label, .answers = answers});
    };
    // Kind 0x1f: no team mate, a happiness above the draw, and the contract-review question saying yes (the
    // visited person's loyalty under 15 and the contract's bit 1 in +0x5c, with some flag set).
    for (unsigned who = 0; who < 3; ++who)
        for (const std::uint32_t flags : {0x1000u, 0x20000u, 0x40000u, 0x2u, 0x800u}) {
            Spec k;
            k.kind = 0x1f; k.who = who; k.c44 = 100; k.c50 = flags; k.c5c = 1;
            k.contract_person = players[who]; k.pl = who; k.contract_club = scenario.r32(managers[who] + 0x24);
            k.label = "contract review says yes";
            build(k);
        }
    // Kind 6, second chain: the manager's loyalty draw comes up 0 (the first chain ends), then the draws of his
    // effective management and of the staff's discipline come up 0, the player's dirtiness beats a remainder of
    // 50 and the amount's draw falls under a remainder of 5. The table is shifted by the draws that came before.
    for (unsigned shift = 0; shift < 9; ++shift)
        for (const std::uint32_t last : {3u, 1u, 4u})
            for (const bool no_amount : {false, true}) {
                Spec k;
                k.kind = 6; k.pl = 17; k.dirtiness = 127; k.staff_skills = 1; k.trait = 1;
                k.factor = no_amount ? 0.0f : 100.0f;  // an amount of 0 draws nothing, so the remainder follows at once
                k.manager_traits[2] = 1;  // loyalty
                k.entries.assign(shift, 0);
                for (const auto e : {0u, 0u, 0u, 100u, 0u}) k.entries.push_back(e);
                if (!no_amount) k.entries.push_back(0);
                k.entries.push_back(last);
                k.label = "kind 6 second chain";
                build(k);
            }
    // Transfer kinds with an amount of exactly 20 (the range of the next draw is 0) and of 10 to 19 (the draw of 0 is
    // not under 10). The story value of the player at index 17 is 0.633332968 for these kinds (the value routine
    // printed it for the case with factor 1), so the factor makes the amount: trunc(factor * value).
    constexpr double value_of_player_17 = 0.633332968;
    for (unsigned shift = 0; shift < 9; ++shift)
        for (const unsigned amount : {20u, 19u, 15u, 12u, 10u})
            for (const std::uint32_t kind : {0x28u, 0x31u, 0x3bu}) {
                Spec k;
                k.kind = kind; k.pl = 17; k.sportsmanship = 127; k.trait = 1; k.factor = float((amount + 0.5) / value_of_player_17);
                k.entries.assign(shift, 0);
                for (const auto e : {100u, 0u, 0u, 0u, 0u, 0u}) k.entries.push_back(e);
                k.label = "transfer story, exact amount";
                build(k);
            }
}
