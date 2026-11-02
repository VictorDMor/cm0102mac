// Cases for the story routines 0xbd0 and 0xbd9 over the real career: real players and clubs, with
// the bytes the routines test set to chosen values, the shared random table scripted, and the
// callees of other modules either run as they are or answered.
#include "media_checks.hpp"
#include <iostream>
#include <random>

using namespace cm::aot::media_checks;

namespace {
using Writes = std::vector<Write>;
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
constexpr std::uint32_t bd0 = 0x10649fb0, bd9 = 0x10655d60;
constexpr std::uint32_t table_at = scratch_base + 0x8000, standing_at = scratch_base + 0x1000, store_at = scratch_base + 0x100,
    history_at = scratch_base + 0x400, request_at = scratch_base + 0x600, other_at = scratch_base + 0xc00;

Write b8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write h16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
void append(Writes& to, const Writes& more) { to.insert(to.end(), more.begin(), more.end()); }

// The random table: the cursor starts at table_at and each draw steps it by two bytes, so the
// first draw reads entries[0]. `seed` is added to every entry.
Writes table(const Scenario& s, const std::vector<std::uint32_t>& entries, std::uint32_t seed, bool refill = false) {
    Writes writes;
    for (std::size_t i = 0; i < entries.size(); ++i) writes.push_back(h16(table_at + 2 * std::uint32_t(i + 1), entries[i]));
    const auto end = table_at + 0x200;
    writes.push_back(w32(s.toc(-0x7b84), refill ? end : table_at));
    writes.push_back(w32(s.toc(-0x7b88), end));
    writes.push_back(w32(s.toc(-0x7b8c), table_at));
    writes.push_back(w32(s.toc(-0x7b90), seed));
    if (refill) for (std::uint32_t i = 1; i <= 8; ++i) writes.push_back(h16(table_at + 2 * i, 0x1111u * i));
    return writes;
}
Registers args(std::initializer_list<std::pair<unsigned, std::uint32_t>> set) {
    Registers r{{3, 0x31313131}, {4, 0x41414141}, {5, 0x51515151}, {6, 0x61616161}, {7, 0x71717171}, {8, 0x81818181}, {9, 0x91919191}, {10, 0xa1a1a1a1}};
    for (const auto& [n, v] : set) r[n - 3] = {n, v};
    return r;
}

// A player: the 0x70-byte person with a profile (+0x14) rated at least 3500, his links and a person behind +0xc.
std::vector<std::uint32_t> players(const Scenario& s, unsigned count) {
    std::vector<std::uint32_t> found;
    const auto base = s.global(-0x7be0);
    for (unsigned i = 0; found.size() < count && i < 20000; ++i) {
        const auto p = base + i * 0x70, profile = s.r32(p + 0x14);
        if (!profile || !s.r32(p + 0xc) || !s.r32(p + 0x28) || !s.r32(p + 0x2c) || !s.r32(p + 0x30)) continue;
        if (std::int16_t(s.s.memory.read<2>(profile + 0xa)) < 0xdac) continue;
        found.push_back(p);
    }
    return found;
}
// A club with the records the routines follow (+0x2c, +0x3c and its +0x10, +0x30, +0x80 and its +0x1c, a manager at +0x90).
std::vector<std::uint32_t> clubs(const Scenario& s, unsigned count) {
    std::vector<std::uint32_t> found;
    for (unsigned i = 0; found.size() < count && i < 400; ++i) {
        const auto c = s.club(i);
        const auto manager = s.r32(c + 0x90);
        if (!s.r32(c + 0x2c) || !s.r32(c + 0x3c) || !s.r32(s.r32(c + 0x3c) + 0x10) || !s.r32(c + 0x30) || !manager) continue;
        if (!s.r32(manager + 0x28) || !s.r32(manager + 0x2c) || !s.r32(manager + 0x30)) continue;
        found.push_back(c);
    }
    return found;
}
}

MEDIA_CASES("pick_a") {
    try {
    const auto people = players(scenario, 40);
    const auto teams = clubs(scenario, 12);
    if (people.size() < 4 || teams.size() < 2) return;
    const auto persons_base = scenario.global(-0x7be0);
    const auto real_press = scenario.global(-0x7b94);
    std::mt19937 gen(0x5ca1ab1e);
    const auto pick = [&](std::initializer_list<std::uint32_t> values) { return *(values.begin() + gen() % values.size()); };
    const auto sample_table = [&](bool negative) {
        std::vector<std::uint32_t> entries;
        for (int i = 0; i < 120; ++i) {
            const auto bits = gen();
            // Mostly small draws so the moduli's low values come up, sometimes the whole range.
            entries.push_back(i % 5 == 4 ? bits & 0xffff : (bits >> 8) % 160);
        }
        const std::uint32_t seeds[] = {0, 0, 1, 0x1fff, 5000, 6999, 0x80000001u, 0xfffffff0u, 0x7fff0000u};
        return table(scenario, entries, negative ? seeds[6 + gen() % 3] : seeds[gen() % 6]);
    };

    // ---- 0x10649fb0 post_item_bd0(press, player, club, kind, other, third)
    const std::uint32_t kinds[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 30, 31, 32, 33, 34, 35, 36, 37, 38,
                                   0x1e, 0x20, 0x21, 0x22, 0x23, 0x52, 0x7fff, 0xffff, 0x10005, 0x18001u, 0xffffffffu};
    // The guards, one at a time.
    {
        const auto p = people[0], t = teams[0];
        const auto go = [&](Registers r, Writes m, const char* label) {
            cases.push_back({.entry = bd0, .registers = std::move(r), .memory = std::move(m), .f1 = {}, .label = label});
        };
        go(args({{3, real_press}, {4, 0}, {5, t}, {6, 3}, {7, 0}, {8, 0}}), {}, "no player");
        go(args({{3, real_press}, {4, p}, {5, 0}, {6, 3}, {7, 0}, {8, 0}}), {}, "no club");
        go(args({{3, real_press}, {4, p}, {5, t}, {6, 3}, {7, 0}, {8, 0}}), {w32(p + 0x14, 0)}, "no profile");
        go(args({{3, real_press}, {4, p}, {5, t}, {6, 3}, {7, 0}, {8, 0}}), {h16(scenario.r32(p + 0x14) + 0xa, 0xdab)}, "low rating");
        go(args({{3, real_press}, {4, p}, {5, t}, {6, 3}, {7, 0}, {8, 0}}), {w32(p + 0xc, 0)}, "no link");
        go(args({{3, real_press}, {4, p}, {5, t}, {6, 3}, {7, 0}, {8, 0}}), {w32(t + 0x2c, 0)}, "club without +0x2c");
        go(args({{3, real_press}, {4, p}, {5, t}, {6, 3}, {7, 0}, {8, 0}}), {w32(t + 0x3c, 0)}, "club without +0x3c");
        go(args({{3, real_press}, {4, p}, {5, t}, {6, 3}, {7, 0}, {8, 0}}), {w32(scenario.r32(t + 0x3c) + 0x10, 0)}, "+0x3c without +0x10");
        go(args({{3, real_press}, {4, p}, {5, t}, {6, 3}, {7, 0}, {8, 0}}), {w32(t + 0x90, 0)}, "club without manager");
        go(args({{3, real_press}, {4, p}, {5, t}, {6, 3}, {7, 0}, {8, 0}}), {}, "real, no answers");
        go(args({{3, real_press}, {4, p}, {5, t}, {6, 3}, {7, 0}, {8, 0}}), {w32(p + 0x24, 0)}, "no key");
        go(args({{3, real_press}, {4, p}, {5, t}, {6, 3}, {7, people[1]}, {8, people[2]}}), {w32(people[1] + 0x1c, t), w32(people[1] + 0x20, t)}, "real, with persons");
    }
    // The standing the routine works on, scripted in a scratch record when the standing lookup is answered.
    const auto squad_of = [&](std::uint32_t club) {
        std::vector<std::uint32_t> squad;
        const auto first = scenario.global(-0x7be0), last = first + scenario.global(-0x7be4) * 0x70;
        for (unsigned k = 0; k < 16; ++k) {
            const auto member = scenario.r32(club + 0x98 + 4 * k);
            if (member < first || member >= last) continue;
            const auto profile = scenario.r32(member + 0x14);
            if (!profile || !scenario.r32(member + 0xc) || !scenario.r32(member + 0x28) || !scenario.r32(member + 0x2c) || !scenario.r32(member + 0x30)) continue;
            squad.push_back(member);
        }
        return squad;
    };
    struct Style { bool scripted = false, known = false, steered = false, young = false, chain = false; };
    const auto run_case = [&](std::uint32_t kind, Style style, const std::string& label) {
        const auto [scripted, known, steered, young, chain] = style;
        auto t = teams[gen() % teams.size()];
        auto p = people[gen() % people.size()];
        // Three in five are a member of the club's own squad, whose standing the real lookup finds.
        if (const auto squad = squad_of(t); !squad.empty() && gen() % 5 < 3) p = squad[gen() % squad.size()];
        const auto profile = scenario.r32(p + 0x14);
        if (chain) p = squad_of(t).empty() ? p : squad_of(t)[gen() % squad_of(t).size()];
        Writes m = chain ? table(scenario, std::vector<std::uint32_t>(120, 0), 0) : sample_table(gen() % 6 == 0);
        // Player.
        m.push_back(b8(p + 0x67, pick({0, 0, 1, 2, 3, 5, 6, 9, 12, 14, 15, 0x80, 0xf8})));
        m.push_back(b8(p + 0x65, pick({0, 0, 4, 9, 14, 15, 16, 0x80})));
        m.push_back(b8(p + 0x69, pick({0, 1, 2, 5, 6, 9, 10, 11, 0x81})));
        m.push_back(b8(p + 0x63, pick({0, 10, 15, 16, 20})));
        m.push_back(b8(p + 0x66, pick({0, 3, 9, 10, 29, 30, 31, 40, 0x90})));
        m.push_back(h16(p + 0x5c, pick({10, 17, 18, 19, 20, 21, 22, 23, 24, 25, 31, 32, 33, 34, 35, 36, 50, 0x180})));
        m.push_back(b8(p + 0x5e, pick({0, 1, 50})));
        m.push_back(h16(profile + 0xa, pick({0xdac, 0x1000, 0x1869, 0x186a, 0x186b, 0x1b57, 0x1b58, 0x2000, 0x5000, 0x7fff})));
        m.push_back(b8(profile + 0xf, pick({0x14, 0x14, 0, 3})));
        m.push_back(b8(profile + 0x11, pick({0x14, 0, 5})));
        m.push_back(b8(profile + 0x21, pick({0, 1, 3, 5, 6, 9, 0x82})));
        m.push_back(h16(t + 0x1ec, pick({0, 3000, 6000, 9000, 0x7fff, 0xf000})));
        // A young player of modest rating with a high seed, so the draw against his rating can pass.
        if (young) append(m, {h16(p + 0x5c, 10), h16(profile + 0xa, 0xdac), w32(scenario.toc(-0x7b90), 6999), w32(scenario.toc(-0x7b84), table_at)});
        Answers answers;
        // Half the cases carry a standing of their own: with no key the standing lookup is skipped and r30,
        // which the original reads uninitialised, is a scratch record whose bytes the case chooses.
        Registers extra;
        if (steered) {
            // Or no standing of ours (r30 zero): the lookup by club runs.
            const bool own = gen() % 4 == 0;
            append(m, {w32(p + 0x24, 0), w32(standing_at + 0xc, pick({0, 0x1d4c, 0x1d4d, 0x4000})), b8(standing_at + 0x45, pick({0, 1, 2, 0x80, 0xff})),
                       b8(standing_at + 0x4c, pick({0, 1, 5, 0x30, 0x64, 0x7f, 0x80, 0x9c, 0xb0, 0xe0, 0xff})),
                       w32(standing_at + 0x50, pick({0, 8, 0x2000, 0x321, 0x30000, 0x200200, 0x200300, 0x2000a0, 0x2008, 0x180, 0x200380, 0x200321, 0x300321})),
                       b8(standing_at + 0x60, pick({0, 1, 2, 3, 4, 0x13, 0x22}))});
            extra.push_back({30, own ? 0 : standing_at});
        }
        if (known) {
            // A history row for this player and club and the kind: the scan finds it, whatever the date.
            append(m, {w32(store_at, history_at), w32(store_at + 4, 4), w32(store_at + 8, 1), w32(store_at + 0xc, 0), w32(store_at + 0x10, 0), w32(store_at + 0x14, 0),
                       w32(history_at + 8, scenario.r32(p)), w32(history_at + 0xc, scenario.r32(t)), h16(history_at + 0x12, kind), w32(history_at + 0x14, 0xbd0)});
        } else {
            append(m, {w32(store_at, history_at), w32(store_at + 4, 4), w32(store_at + 8, 0), w32(store_at + 0xc, 0), w32(store_at + 0x10, 0), w32(store_at + 0x14, 0)});
        }
        if (scripted) {
            // Answers only for routines no other module owns (an owned one runs in place on the original's side).
            answers.push_back({0x10718130, pick({0, 0x2d, 0x2e, 0x16c, 0x16d, 0x16e, 0x1000, 0xfffff000u})});
            answers.push_back({0x1014c2e0, pick({0, 1, 0x100})});
            answers.push_back({0x10658370, pick({0, 1})});
            answers.push_back({0x106487d0, pick({0xffffffffu, 0xffffffffu, 3, 20, 700})});
            answers.push_back({0x100d9b80, 0});
            answers.push_back({0x100d8260, chain ? 0x200200 : pick({0, 8, 0x200200, 0x200208, 0x200380, 0x200100, 0x2000, 0x40208})});
            answers.push_back({0x100d1cb0, chain ? 0x7f : pick({0, 5, 9, 12, 20, 0x7f, 0x80})});
            // (0x106891b0 is called directly and runs for real on both sides.)
        }
        const auto other = pick({0u, 0u, 1u});
        const auto third = pick({0u, 1u});
        if (other) append(m, {w32(people[1] + 0x1c, pick({t, teams[1]})), w32(people[1] + 0x20, pick({t, teams[1]})),
                              b8(people[1] + 0x49, pick({0xff, 0, 1, 2, 5})), b8(people[1] + 0x4a, pick({0xff, 0, 1, 2, 5}))});
        // The club's other links.
        const auto manager = scenario.r32(t + 0x90), listeners = scenario.global(-0x7b5c);
        if (gen() % 8 == 0) m.push_back(w32(t + 0x30, 0));
        else if (const auto entry = scenario.r32(listeners + scenario.r32(scenario.r32(t + 0x30)) * 4); entry && gen() % 8 == 0) m.push_back(w32(entry + 8, 0));
        if (manager && scenario.r32(manager + 0x1c)) {
            m.push_back(b8(scenario.r32(manager + 0x1c) + 0x39, pick({0, 9, 10, 0x80})));
            if (gen() % 10 == 0) m.push_back(w32(manager + 0x1c, 0));
        }
        switch (gen() % 5) {
        case 0: m.push_back(w32(t + 0x80, 0)); break;
        case 1: append(m, {w32(t + 0x80, people[3]), w32(people[3] + 0x1c, t), b8(t + 0x36, pick({0, 5, 0x7f, 0x80, 0xff}))}); break;
        case 2: append(m, {w32(t + 0x80, people[3]), w32(people[3] + 0x1c, 0)}); break;
        default: break;
        }
        // The other person the club's draw may name: its +0x24 link.
        m.push_back(w32(persons_base + pick({3u, 20u, 700u}) * 0x70 + 0x24, pick({0u, t})));
        if (gen() % 8 == 0) m.push_back(w32(scenario.toc(-0x7b40), 0));
        auto registers = args({{3, store_at}, {4, p}, {5, t}, {6, kind}, {7, other ? people[1] : 0}, {8, third ? people[2] : 0}});
        for (const auto& [n, v] : extra) registers.push_back({n, v});
        cases.push_back({.entry = bd0, .registers = std::move(registers), .memory = std::move(m), .f1 = {}, .label = label, .answers = std::move(answers)});
    };
    for (const auto kind : kinds) {
        for (unsigned n = 0; n < 80; ++n) run_case(kind, {.scripted = n % 2 == 0, .steered = n % 4 < 2}, "bd0 kinds");
        for (unsigned n = 0; n < 4; ++n) run_case(kind, {.scripted = n % 2 == 0, .known = true, .steered = n % 2 == 1}, "bd0 known");
        if (kind == 0 || kind == 2)
            for (unsigned n = 0; n < 120; ++n) run_case(kind, {.scripted = n % 2 == 0, .steered = n % 4 < 2, .young = true}, "bd0 young");
    }
    // The club's listener chain (0x10457d00, the lineup and the flags of the first tactic), reached from a squad member with
    // every draw zero; the covering draws (table all zero) make the figure draw fail and the reaction draw pass.
    for (unsigned n = 0; n < 300; ++n) run_case(pick({3u, 5u, 14u}), {.scripted = true, .steered = n % 3 == 0, .chain = true}, "bd0 chain");

    // ---- 0x10655d60 post_item_bd9(press, request, involved)
    const auto clubs_base = scenario.global(-0x7be8);
    const auto last_club = scenario.global(-0x7b50);
    const auto index_of = [&](std::uint32_t person) { return (person - persons_base) / 0x70; };
    const auto club_index = [&](std::uint32_t club) { return (club - clubs_base) / 0x248; };
    const auto request = [&](std::uint32_t player_index, std::uint32_t club_idx, std::uint32_t amount, std::uint32_t flag, std::uint32_t second, std::uint32_t kind) {
        return Writes{w32(request_at, player_index), w32(request_at + 4, club_idx), b8(request_at + 0xc, amount), b8(request_at + 0xe, flag),
                      w32(request_at + 0x10, second), h16(request_at + 0x14, kind)};
    };
    const auto call9 = [&](std::uint32_t at, std::uint32_t involved, Writes m, Answers answers, const char* label) {
        cases.push_back({.entry = bd9, .registers = args({{3, store_at}, {4, at}, {5, involved}}), .memory = std::move(m), .f1 = {}, .label = label,
                         .answers = std::move(answers)});
    };
    const auto empty_press = Writes{w32(store_at, history_at), w32(store_at + 4, 4), w32(store_at + 8, 0), w32(store_at + 0xc, 0), w32(store_at + 0x10, 0), w32(store_at + 0x14, 0)};
    {
        const auto p = people[0], t = teams[0], q = people[1];
        const auto base = [&](Writes more) { Writes m = empty_press; append(m, request(index_of(p), club_index(t), 3, 0, 0xffffffffu, 0x1e)); append(m, more); return m; };
        call9(0, 0, empty_press, {}, "bd9 no request");
        call9(request_at, 0, base({w32(p + 0x14, 0)}), {}, "bd9 no profile");
        call9(request_at, 0, base({h16(scenario.r32(p + 0x14) + 0xa, 0xdab)}), {}, "bd9 low rating");
        call9(request_at, 0, base({w32(request_at + 4, 0xffffffffu)}), {}, "bd9 negative club");
        call9(request_at, 0, base({w32(request_at + 4, last_club + 1)}), {}, "bd9 club past the table");
        call9(request_at, 0, base({w32(request_at + 4, last_club)}), {}, "bd9 last club");
        call9(request_at, 0, base({w32(t + 0x2c, 0)}), {}, "bd9 club without +0x2c");
        call9(request_at, 0, base({}), {}, "bd9 real, nothing answered");
        // The same story already told: a history row for the player, club, story and grade.
        auto known = base({w32(store_at + 8, 1), w32(history_at + 8, scenario.r32(p)), w32(history_at + 0xc, scenario.r32(t)), h16(history_at + 0x12, 0x1e), w32(history_at + 0x14, 0xbd9)});
        call9(request_at, 0, known, {}, "bd9 already told");
        // A flagged request needs a second person.
        auto flagged = [&](std::uint32_t second) { Writes m = empty_press; append(m, request(index_of(p), club_index(t), 3, 1, second, 0x1e)); return m; };
        call9(request_at, 0, flagged(0xffffffffu), {}, "bd9 flagged without second");
        call9(request_at, 0, flagged(index_of(q)), {}, "bd9 flagged");
        { auto m = flagged(index_of(q)); append(m, {w32(q + 0x1c, t), w32(q + 0x20, t), b8(q + 0x49, 3), b8(q + 0x4a, 1)}); call9(request_at, q, m, {}, "bd9 flagged with involved"); }
        for (const auto other_club : {t, teams[1]})
            for (const auto [before, after] : {std::pair<std::uint32_t, std::uint32_t>{0xff, 0}, {3, 1}, {1, 3}, {2, 2}})
                call9(request_at, q, [&] { Writes m = empty_press; append(m, request(index_of(p), club_index(t), 3, 0, 0xffffffffu, 0x1e)); append(m, {b8(q + 0x49, before), b8(q + 0x4a, after), w32(q + 0x1c, other_club), w32(q + 0x20, other_club)}); return m; }(), {}, "bd9 involved");
    }
    for (unsigned n = 0; n < 400; ++n) {
        const auto p = people[gen() % people.size()], t = teams[gen() % teams.size()], q = people[gen() % people.size()];
        const auto inv = people[3];
        const auto flag = pick({0, 0, 1});
        const auto second = pick({0xffffffffu, 0xffffffffu, index_of(q)});
        const auto kind = pick({0x1e, 0x20, 0x52, 0, 5, 0xffff8000u, 0x1ff});
        Writes m = sample_table(false);
        append(m, empty_press);
        append(m, request(index_of(p), club_index(t), pick({0, 3, 0xff, 0x80}), flag, second, kind));
        append(m, {b8(inv + 0x49, pick({0xff, 0, 1, 2, 5})), b8(inv + 0x4a, pick({0xff, 0, 1, 2, 5})), w32(inv + 0x1c, pick({t, teams[1]})),
                   w32(inv + 0x20, pick({t, teams[1]}))});
        if (n % 7 == 0) append(m, {w32(scenario.toc(-0x7b40), 0)});
        Answers answers;
        if (n % 2) {
            answers.push_back({0x1014c2e0, pick({0, 1})});
            answers.push_back({0x106584d0, pick({0, 1, 2})});
            answers.push_back({0x1069eb10, pick({0, 1, 3, 6, 0x81})});
            answers.push_back({0x106a95c0, pick({0, 5, 0xffffffffu})});
            answers.push_back({0x1069e700, 0});
        }
        call9(request_at, pick({0u, inv, inv}), m, answers, "bd9 random");
    }
    (void)other_at;
} catch (const std::exception& error) { std::cerr << "pick_a cases: " << error.what() << '\n'; throw; }
}
