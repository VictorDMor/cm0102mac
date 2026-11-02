// The eval_d block's cases: the press story evaluation 0x10677d60 over real players and clubs of the
// career (a player with a profile and a standing record, a club with a manager), with the player's
// traits, profile figures and standing flags, the club's person slots and the random table set by the
// case. The club-record questions (0x100d9480...) and the routines of unfinished blocks are answered; the
// contract-review and dispute routines (hooked at the call site, or leaves) run for real.
#include "media_checks.hpp"
#include <random>
#include <string_view>

using namespace cm::aot::media_checks;

namespace {
constexpr std::uint32_t entry = 0x10677d60;
constexpr std::uint32_t chain_at = scratch_base + 0x2800, record_at = scratch_base + 0x2810, table_at = scratch_base + 0x9000;
Write b8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write h16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
void append(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }

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

// The callees whose answers both sides can take (not hooked at the call site, not leaves).
struct Answers {
    std::uint32_t leader = 0, follower = 0, rival_leader = 0, rival_follower = 0, club_flag = 1;
};
std::vector<std::pair<std::uint32_t, std::uint32_t>> answers(const Answers& a) {
    // (The rating routines and the press item 0x10649900 run for real: the story calls their typed C++ directly.)
    return {{0x100d9480, a.leader}, {0x100d94d0, a.follower}, {0x100d9520, a.rival_leader}, {0x100d9580, a.rival_follower}, {0x100c7d00, a.club_flag},
            {0x10457d00, 0}, {0x100d3570, 0}, {0x100d9e90, 0}, {0x100d9b80, 0}, {0x106620b0, 0}};
}

// What the story reads of the player, his profile and standing.
struct Player {
    int t63 = 15, t64 = 8, t65 = 5, t66 = 12, t67 = 5, t69 = 5;
    int age = 25, rating = 0x2000, form = 0x1000, ability = 0x80, profile44 = 5;
    int standing44 = 0x64, standing4c = -30, flags = 0;
    std::uint32_t standing50 = 1;
    int standing5c = -1;
};
}

MEDIA_CASES("eval_d") {
    const auto persons = scenario.global(-0x7be0), count = scenario.global(-0x7be4);
    const auto standings = scenario.r32(scenario.global(-0x7b7c)), indexes = scenario.global(-0x7b9c);
    // The standing record the game finds for a person (0x10082f20), or 0.
    const auto standing_of = [&](std::uint32_t person) -> std::uint32_t {
        const auto id = scenario.r32(person);
        if (id >= count) return 0;
        const auto at = std::int32_t(scenario.r32(indexes + id * 0x64));
        return at < 0 ? 0 : standings + std::uint32_t(at) * 0x64;
    };
    // Real persons with a profile and a standing; real clubs with a manager (who has +0x1c).
    std::vector<std::uint32_t> good;
    for (unsigned i = 0; i < count && good.size() < 40; i += 97) {
        const auto row = persons + i * 0x70;
        if (scenario.r32(row + 0x14) && standing_of(row)) good.push_back(row);
    }
    std::vector<std::uint32_t> clubs;
    for (unsigned i = 0; i < 400 && clubs.size() < 5; ++i) {
        const auto row = scenario.club(i);
        const auto manager = scenario.r32(row + 0x90);
        if (scenario.r32(row) && scenario.r32(row) < 100 && manager && scenario.r32(manager + 0x1c)) clubs.push_back(row);
    }
    if (good.size() < 8 || clubs.empty()) return;
    std::mt19937 random(0x5eed);
    const auto pick = [&](std::initializer_list<int> values) { return *(values.begin() + random() % values.size()); };
    const std::initializer_list<int> edges{-128, -3, 0, 1, 2, 3, 5, 6, 7, 9, 10, 11, 14, 15, 19, 20, 21, 33, 34, 100};
    const auto random_player = [&] {
        Player p;
        p.t63 = pick(edges); p.t64 = pick(edges); p.t65 = pick(edges); p.t66 = pick(edges); p.t67 = pick(edges); p.t69 = pick(edges);
        // Mostly the ages and figures that pass the reactions' gates.
        p.age = random() % 4 ? pick({20, 25, 33, 34, 19}) : pick(edges);
        p.rating = random() % 4 ? pick({0x1a5f, 0x2000, 0x1a5e}) : int(random() % 0x4000);
        p.form = random() % 4 ? pick({0xea7, 0xea6, 0x1000}) : int(random() % 0x2000);
        p.ability = random() % 4 ? pick({0x79, 0x78, 0x90}) : int(random() % 0x100);
        p.profile44 = pick(edges);
        p.standing44 = random() % 2 ? 0x64 : int(random() % 256);
        p.standing4c = pick({-0x1a, -0x19, -0x18, -30, 0, 50, -128});
        // Flag words: mostly sparse random bits (the questions about a standing test many masks), sometimes 0 or 0x800.
        const auto sparse = [&] { std::uint32_t v = 0; for (unsigned bit = 0; bit < 32; ++bit) if (random() % 5 == 0) v |= 1u << bit; return v; };
        p.standing50 = random() % 4 == 0 ? random() % 2 : sparse();
        p.flags = std::int32_t(random() % 4 == 0 ? pick({0, 0x800, 1, 0x801}) : int(sparse()));
        return p;
    };
    const auto random_answers = [&] {
        Answers a;
        a.leader = random() % 2; a.follower = random() % 2; a.rival_leader = random() % 2; a.rival_follower = random() % 2;
        return a;
    };
    // A player who passes the reactions' gates (kinds 3.. and 0..): age, figures and traits in range, a
    // standing with the odd +0x5c byte that makes the contract-review question say yes.
    const auto gate_player = [&](bool known) {
        Player p;
        p.age = pick({20, 25, 33}); p.rating = pick({0x1a5f, 0x2000}); p.form = pick({0xea7, 0x1000}); p.ability = pick({0x79, 0x90});
        p.t63 = pick({11, 15, 19}); p.t65 = pick({0, 1, 2, 3, 4, 5}); p.t66 = pick({11, 15, 19}); p.t67 = known ? pick({0, 5, 9}) : pick({0, 3, 5});
        p.t69 = known ? pick({0, 5, 9}) : pick({0, 5, 6, 9}); p.t64 = pick(edges); p.profile44 = pick(edges);
        p.standing4c = pick({-0x1a, -30, -128}); p.standing44 = 0x64; p.standing50 = random() % 4 ? 1 + random() % 255 : 0;
        p.flags = random() % 2 ? 0x800 : 0; p.standing5c = random() % 4 ? 1 + 2 * (random() % 100) : 2 * (random() % 100);
        if (random() % 4 == 0) { p.t65 = pick(edges); p.standing4c = pick({-0x19, 0}); }
        return p;
    };
    // One call: registers (reporter, player, club, kind, side), the world and the random table.
    const auto make = [&](std::uint32_t kind, std::uint32_t side, const Player& p, const Answers& a, bool chain, bool rival, unsigned wrap,
                          const char* label, bool real = false) {
        const auto player = good[random() % good.size()];
        const auto club = clubs[random() % clubs.size()];
        const auto manager = scenario.r32(club + 0x90);
        std::vector<Write> memory;
        if (std::string_view(label) == "reaction without profile") memory.push_back(w32(player + 0x14, 0));
        memory.push_back(h16(player + 0x5c, 0x1100 | std::uint32_t(p.age & 0xff)));
        memory.push_back(b8(player + 0x63, p.t63)); memory.push_back(b8(player + 0x64, p.t64)); memory.push_back(b8(player + 0x65, p.t65));
        memory.push_back(b8(player + 0x66, p.t66)); memory.push_back(b8(player + 0x67, p.t67)); memory.push_back(b8(player + 0x69, p.t69));
        const auto profile = scenario.r32(player + 0x14);
        memory.push_back(h16(profile + 0xa, p.rating)); memory.push_back(h16(profile + 0xc, p.form)); memory.push_back(h16(profile + 4, p.ability));
        memory.push_back(b8(profile + 0x44, p.profile44));
        const auto standing = standing_of(player);
        memory.push_back(b8(standing + 0x44, p.standing44)); memory.push_back(b8(standing + 0x4c, p.standing4c));
        memory.push_back(w32(standing + 0x50, p.standing50)); memory.push_back(w32(standing + 0x48, p.flags));
        memory.push_back(b8(standing + 0x5c, p.standing5c >= 0 ? std::uint32_t(p.standing5c) : random() % 256));
        // The club: its group chain (for the standings it keeps), a rival (a person), and 50 person slots holding
        // the player, nothing, and bystanders whose traits, profile and id the case sets.
        // (With real callees the club keeps its real group chain.)
        if (!real) {
            memory.push_back(w32(club + 0x30, chain ? chain_at : 0));
            memory.push_back(w32(chain_at, 7));
            memory.push_back(w32(scenario.global(-0x7b5c) + 7 * 4, random() % 8 == 0 ? 0 : record_at));
            memory.push_back(w32(record_at + 8, random() % 8 == 0 ? 0 : 0x1234));
        }
        memory.push_back(w32(club + 0x80, rival ? good[random() % good.size()] : 0));
        std::vector<std::uint32_t> list;
        for (unsigned n = 0, total = 1 + random() % 6; n < total; ++n) {
            switch (random() % 6) {
            case 0: list.push_back(0); break;
            case 1: case 2: list.push_back(player); break;
            default: {
                const auto other = good[random() % good.size()];
                if (other == player) { list.push_back(0); break; }
                list.push_back(other);
                memory.push_back(b8(other + 0x63, pick(edges))); memory.push_back(b8(other + 0x67, pick(edges)));
                if (random() % 5 == 0) memory.push_back(w32(other + 0x14, 0));
                if (random() % 5 == 0) memory.push_back(w32(other, count + 5));
            }
            }
        }
        for (std::uint32_t i = 0; i < 50; ++i) memory.push_back(w32(club + 0x98 + 4 * i, i < list.size() ? list[i] : 0));
        if (const auto rival_row = scenario.r32(club + 0x80); rival_row) memory.push_back(b8(rival_row + 0x63, pick(edges)));
        (void)manager;
        std::vector<std::uint32_t> entries;
        for (unsigned i = 0; i < 700; ++i) entries.push_back(random() % 0x10000);
        append(memory, table(scenario, entries, random() % 2 ? 0 : random() % 0x10000, wrap));
        cases.push_back({.entry = entry,
                         .registers = {{3, scenario.global(-0x7bf0)}, {4, player}, {5, club}, {6, kind}, {7, side}, {8, 0x81818181}, {9, 0x91919191}, {10, 0xa1a1a1a1}},
                         .memory = memory, .f1 = 1.5, .label = label, .answers = real ? decltype(answers(a)){} : answers(a)});
    };
    const std::uint32_t kinds[] = {0x25, 0x1f, 0x20, 3, 4, 12, 16, 35, 0, 1, 2, 5, 6, 9, 11, 13, 14, 15, 17, 18, 19, 7, 8, 10, 0x24, 0x30, 0xffffffffu, 0xfffe};
    const std::uint32_t sides[] = {1, 2, 3, 0, 4, 0xff, 0x7f};
    // A first sweep of every kind and side, so the sampled call traces see them all.
    for (const auto kind : kinds)
        for (const auto side : sides)
            make(kind, side, random_player(), random_answers(), random() % 2, random() % 2, 700, "sweep");
    // Then the families with many variants: each draw's wrap, the answers and the traits.
    for (const std::uint32_t kind : {0x25u, 0x1fu, 0x20u, 3u, 0u, 7u}) {
        for (const std::uint32_t side : {1u, 2u, 3u, 0u}) {
            for (unsigned n = 0; n < 150; ++n)
                make(kind, side, random_player(), random_answers(), random() % 4 != 0, random() % 4 != 0, random() % 3 ? 700 : random() % 14, "family");
        }
    }
    // The same with every callee real (the club-record questions, the morale moves, the club-rating and posting routines of the other
    // blocks): their arguments and the stack they leave are then compared too.
    for (const std::uint32_t kind : {0x25u, 0x1fu, 0x20u, 3u, 0u, 7u, 0x24u})
        for (const std::uint32_t side : {1u, 2u, 3u, 0u}) {
            for (unsigned n = 0; n < 60; ++n)
                make(kind, side, random_player(), random_answers(), true, random() % 4 != 0, random() % 3 ? 700 : random() % 14, "real callees", true);
        }
    for (const std::uint32_t kind : {3u, 4u, 12u, 16u, 35u, 0u, 1u, 2u, 5u, 6u, 9u, 11u, 13u, 14u, 15u, 17u, 18u, 19u})
        for (unsigned n = 0; n < 12; ++n)
            make(kind, 2, gate_player(!(kind == 3 || kind == 4 || kind == 12 || kind == 16 || kind == 35)), random_answers(), true, true, 700, "real callees", true);
    // The reactions of the other kinds, with players that pass their gates; some with no profile.
    for (const std::uint32_t kind : {3u, 4u, 12u, 16u, 35u, 0u, 1u, 2u, 5u, 6u, 9u, 11u, 13u, 14u, 15u, 17u, 18u, 19u})
        for (unsigned n = 0; n < 40; ++n) {
            const bool known = !(kind == 3 || kind == 4 || kind == 12 || kind == 16 || kind == 35);
            make(kind, 2, gate_player(known), random_answers(), true, true, random() % 3 ? 700 : random() % 14, n == 0 ? "reaction without profile" : "reaction");
        }
    // The club group chain answered the other way.
    for (const std::uint32_t kind : {0x25u, 0x1fu, 3u})
        for (unsigned n = 0; n < 40; ++n) {
            auto a = random_answers();
            a.club_flag = random() % 2;
            make(kind, 1 + random() % 3, random_player(), a, n % 3 != 0, random() % 2, 700, "chain");
        }
    {
        // The early exits: no club, no player.
        cases.push_back({.entry = entry, .registers = {{3, clubs[0]}, {4, good[0]}, {5, 0}, {6, 0x25}, {7, 1}}, .memory = {}, .f1 = {}, .label = "no club"});
        cases.push_back({.entry = entry, .registers = {{3, clubs[0]}, {4, 0}, {5, clubs[0]}, {6, 0x25}, {7, 1}}, .memory = {}, .f1 = {}, .label = "no player"});
        cases.push_back({.entry = entry, .registers = {{3, clubs[0]}, {4, good[0]}, {5, clubs[0]}, {6, 0x25}, {7, 1}},
                         .memory = {w32(good[0], count + 3)}, .f1 = {}, .label = "no standing", .answers = answers({})});
        cases.push_back({.entry = entry, .registers = {{3, clubs[0]}, {4, good[0]}, {5, clubs[0]}, {6, 0x25}, {7, 1}},
                         .memory = {w32(scenario.r32(clubs[0] + 0x90) + 0x1c, 0)}, .f1 = {}, .label = "manager without +0x1c", .answers = answers({})});
        cases.push_back({.entry = entry, .registers = {{3, clubs[0]}, {4, good[0]}, {5, clubs[0]}, {6, 0x25}, {7, 1}},
                         .memory = {w32(clubs[0] + 0x90, 0)}, .f1 = {}, .label = "no manager", .answers = answers({})});
    }
}
