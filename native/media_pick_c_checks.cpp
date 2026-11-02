// Cases for the press reactions of pick_c: real players and clubs of the career, the bytes the routines test set to
// chosen values, the random table scripted, and a standing record built in the scratch area when the standing
// lookup is answered.
#include "media_checks.hpp"
#include <iostream>
#include <random>
#include <string_view>

using namespace cm::aot::media_checks;

namespace {
using Writes = std::vector<Write>;
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
constexpr std::uint32_t pick = 0x1069f270, be6 = 0x106a1f00, settle = 0x106a2e10, be8 = 0x106a40a0, react = 0x106a52d0, be5 = 0x106a5e90, be4 = 0x106a7d70;
constexpr std::uint32_t table_at = scratch_base + 0x8000;

[[maybe_unused]] Write b8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write h16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
[[maybe_unused]] void append(Writes& to, const Writes& more) { to.insert(to.end(), more.begin(), more.end()); }

// The random table: the cursor starts at table_at and each draw steps it by two bytes. `seed` is added to every entry.
Writes table(const Scenario& s, const std::vector<std::uint32_t>& entries, std::uint32_t seed, bool refill = false) {
    Writes writes;
    for (std::size_t i = 0; i < entries.size(); ++i) writes.push_back(h16(table_at + 2 * std::uint32_t(i + 1), entries[i]));
    const auto end = table_at + 0x400;
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

// Players: the 0x70-byte person with a profile (+0x14), a link (+0xc), a club (+0x24) that has a manager and the
// records the routines follow, and three links.
std::vector<std::uint32_t> players(const Scenario& s, unsigned count) {
    std::vector<std::uint32_t> found;
    const auto base = s.global(-0x7be0);
    for (unsigned i = 0; found.size() < count && i < 40000; ++i) {
        const auto p = base + i * 0x70, profile = s.r32(p + 0x14), club = s.r32(p + 0x24);
        if (!profile || !s.r32(p + 0xc) || !club || !s.r32(club + 0x90) || !s.r32(club + 0x3c) || !s.r32(p + 0x28) || !s.r32(p + 0x2c) || !s.r32(p + 0x30)) continue;
        const auto manager = s.r32(club + 0x90);
        if (!s.r32(manager + 0x28) || !s.r32(manager + 0x2c) || !s.r32(manager + 0x30)) continue;
        found.push_back(p);
    }
    return found;
}
std::vector<std::uint32_t> clubs(const Scenario& s, unsigned count) {
    std::vector<std::uint32_t> found;
    for (unsigned i = 0; found.size() < count && i < 400; ++i) {
        const auto c = s.club(i);
        const auto manager = s.r32(c + 0x90);
        if (!manager || !s.r32(c + 0x3c) || !s.r32(c + 0x2c) || !s.r32(c + 0x30)) continue;
        if (!s.r32(manager + 0x28) || !s.r32(manager + 0x2c) || !s.r32(manager + 0x30)) continue;
        found.push_back(c);
    }
    return found;
}
}

MEDIA_CASES("pick_c") {
    try {
    const auto people = players(scenario, 400);
    const auto teams = clubs(scenario, 12);
    std::cerr << "pick_c: players " << people.size() << " clubs " << teams.size() << '\n';
    if (people.size() < 4 || teams.size() < 2) return;
    std::mt19937 gen(0x1c0ffee);
    const auto choose = [&](std::initializer_list<std::uint32_t> values) { return *(values.begin() + gen() % values.size()); };
    const auto real_press = scenario.global(-0x7b94);
    // PICKC_LABEL=text keeps only the cases whose label contains it (for narrowing a difference).
    const char* only_label = std::getenv("PICKC_LABEL");
    const auto go = [&](std::uint32_t entry, Registers r, Writes m, Answers a, const char* label) {
        if (only_label && std::string_view(label).find(only_label) == std::string_view::npos) return;
        cases.push_back({.entry = entry, .registers = std::move(r), .memory = std::move(m), .f1 = {}, .label = label, .answers = std::move(a)});
    };
    // A standing record the lookup would find: the head of the person's chain when it is for the club.
    const auto standing_for = [&](std::uint32_t player, std::uint32_t club) -> std::uint32_t {
        if (!player || !club) return 0;
        const auto id = scenario.r32(player), total = scenario.global(-0x7be4);
        if (id >= total) return 0;
        const auto head = std::int32_t(scenario.r32(scenario.global(-0x7b9c) + id * 0x64));
        if (head < 0) return 0;
        const auto record = scenario.r32(scenario.global(-0x7b7c)) + std::uint32_t(head) * 0x64;
        return scenario.r32(record + 4) == scenario.r32(club) ? record : 0;
    };
    std::vector<std::uint32_t> settled;  // players that have a standing at their own club
    for (const auto p : people) if (standing_for(p, scenario.r32(p + 0x24))) settled.push_back(p);
    std::cerr << "pick_c: with a standing " << settled.size() << '\n';
    if (settled.size() < 8) return;

    const auto ratings = {0x1000u, 0x109au, 0x109bu, 0x128eu, 0x128fu, 0x1482u, 0x1483u, 0x1c52u, 0x1c53u, 0x1e46u, 0x1e47u, 0x222eu, 0x222fu, 0x2800u, 0x7fffu, 0x8000u};
    const auto bytes = {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 9u, 10u, 11u, 14u, 15u, 16u, 19u, 20u, 21u, 0x28u, 0x7fu, 0x80u, 0xf6u, 0xe7u, 0xffu};
    const auto builds = {0x12u, 0x13u, 0x14u, 0x16u, 0x17u, 0x18u, 0x1bu, 0x1cu, 0x21u, 0x22u, 0x23u, 0x24u, 0x80u, 0x113u, 0x117u};
    const auto points = {0u, 0x9c3u, 0x9c4u, 0x1481u, 0x1482u, 0x1c51u, 0x1c52u, 0x2000u, 0xf000u, 0x7000u};
    const auto pick_from = [&](const auto& list) { return *(list.begin() + gen() % list.size()); };
    // The random table, mostly small draws, sometimes whole 16-bit values.
    const auto sample_table = [&]() {
        std::vector<std::uint32_t> entries;
        for (int i = 0; i < 160; ++i) entries.push_back(i % 5 == 4 ? gen() & 0xffff : (gen() >> 8) % 120);
        const std::uint32_t seeds[] = {0, 0, 1, 0x1fff, 0x80000001u, 0xfffffff0u};
        return table(scenario, entries, seeds[gen() % 6], gen() % 9 == 0);
    };
    // A player with random traits (the bytes the routines test) and a random contract.
    const auto traits = [&](Writes& m, std::uint32_t p) {
        const auto profile = scenario.r32(p + 0x14);
        if (!profile) return;
        for (const unsigned offset : {0x63u, 0x64u, 0x65u, 0x66u, 0x67u, 0x68u, 0x69u}) m.push_back(b8(p + offset, pick_from(bytes)));
        m.push_back(h16(p + 0x5c, pick_from(builds)));
        m.push_back(h16(profile + 0xa, pick_from(ratings)));
        m.push_back(h16(profile + 0xc, choose({0x1000, 0x186a, 0x186b, 0x2000, 0x7fff})));
        m.push_back(h16(profile + 4, choose({0x10, 0x64, 0x65, 0x500})));
        for (const unsigned offset : {0x1cu, 0x25u, 0x44u}) m.push_back(b8(profile + offset, pick_from(bytes)));
    };
    const auto standing_bytes = [&](Writes& m, std::uint32_t st) {
        m.push_back(b8(st + 0x44, pick_from(bytes)));
        m.push_back(b8(st + 0x45, pick_from(bytes)));
        m.push_back(b8(st + 0x4c, choose({0, 0x19, 0xe6, 0xe7, 0xe8, 0x50, 0x64, 0x7f, 0x80, 0x9c, 0x9b})));
        m.push_back(b8(st + 0x60, choose({0, 1, 0x12, 0x2f, 0xf3})));
        m.push_back(w32(st + 0x48, choose({0, 0x200, 0x800, 0x400, 0xa80, 0xe80, 0x80, 0x1000, 0xffff, 0x1234})));
        m.push_back(w32(st + 0x50, choose({0, 0x20000000, 0x80000000, 0xa0000000, 0x101, 0x1, 0x40000000})));
    };
    // The answers for callees that no other rewrite owns.
    const auto answers_for = [&](bool full) {
        Answers a;
        if (!full) return a;
        a.push_back({0x1014c2e0, choose({0, 1, 0x100})});
        a.push_back({0x100d3150, choose({0, 1, 0x100})});
        a.push_back({0x100c7d00, choose({0, 0, 0, 1})});
        a.push_back({0x106584d0, choose({0, 1, 2, 3})});
        a.push_back({0x106a95c0, choose({0, 5, 0xffffffffu})});
        // (The stories' rolls, 0x1069eb10 and 0x1069e990, run for real: the items call their typed C++ directly.)
        a.push_back({0x100d71a0, choose({0, 7, 0xffffffffu})});
        return a;
    };
    const auto region_answers = [&](Answers& a) {
        a.push_back({0x100db1b0, choose({0, 1, 2, 3})});
        a.push_back({0x100db120, choose({0, 1, 2, 3, 4, 5, 6, 7, 0x80})});
    };
    const auto club_of = [&](std::uint32_t p) { return scenario.r32(p + 0x24); };

    // ---- 0x106a1f00 (be6): the two people and the event object
    const auto event = scratch_base + 0x2000;
    const auto event_writes = [&](std::uint32_t n) {
        return Writes{w32(event + 0x1c, teams[0]), w32(event + 0x20, teams[1]), h16(event + 0x28, 100 + n), h16(event + 0x2a, 2001), h16(event + 0x30, 3), h16(event + 0x32, 4), w32(event + 8, teams[1])};
    };
    {
        const auto p = settled[0], q = settled[1];
        const auto call = [&](Registers r, Writes more, const char* label) {
            Writes m = event_writes(1);
            append(m, more);
            go(be6, std::move(r), std::move(m), {}, label);
        };
        const auto base = args({{3, real_press}, {4, p}, {5, q}, {6, event}});
        call(args({{3, real_press}, {4, 0}, {5, q}, {6, event}}), {}, "be6 no first");
        call(base, {w32(p + 0xc, 0)}, "be6 first without link");
        call(base, {w32(p + 0x24, 0)}, "be6 first without club");
        call(base, {w32(club_of(p) + 0x90, 0)}, "be6 first club without manager");
        call(base, {w32(p + 0x14, 0)}, "be6 first without contract");
        call(args({{3, real_press}, {4, p}, {5, 0}, {6, event}}), {}, "be6 no second");
        call(base, {w32(q + 0x24, 0)}, "be6 second without club");
        call(base, {w32(club_of(q) + 0x90, 0)}, "be6 second club without manager");
        call(base, {w32(q + 0x14, 0)}, "be6 second without contract");
    }
    for (unsigned n = 0; n < 160; ++n) {
        const auto p = settled[gen() % settled.size()], q = settled[gen() % settled.size()];
        Writes m = sample_table();
        append(m, event_writes(n));
        m.push_back(h16(scenario.r32(p + 0x14) + 0xc, pick_from(ratings)));
        if (n % 3 == 0) m.push_back(w32(club_of(p) + 0x0, gen() % 400));
        go(be6, args({{3, real_press}, {4, p}, {5, q}, {6, event}}), m, answers_for(n % 2 == 0), "be6 random");
    }

    // ---- 0x106a40a0 (be8)
    {
        const auto p = settled[0];
        const auto base = args({{3, real_press}, {4, p}, {5, 0}});
        go(be8, args({{3, real_press}, {4, 0}, {5, 0}}), {}, {}, "be8 no person");
        go(be8, base, {w32(p + 0xc, 0)}, {}, "be8 without link");
        go(be8, base, {w32(p + 0x24, 0)}, {}, "be8 without club");
        go(be8, base, {w32(club_of(p) + 0x90, 0)}, {}, "be8 without manager");
        go(be8, base, {w32(club_of(p) + 0x3c, 0)}, {}, "be8 club without +0x3c");
        go(be8, base, {w32(p + 0x14, 0)}, {}, "be8 without contract");
        go(be8, base, {w32(club_of(p) + 0x2c, 0)}, {}, "be8 club without +0x2c");
        go(be8, args({{3, real_press}, {4, p}, {5, 1}}), {}, {}, "be8 flag");
        go(be8, base, {}, {{0x10082f90, 0}}, "be8 no standing");
    }
    for (unsigned n = 0; n < 200; ++n) {
        const auto p = settled[gen() % settled.size()];
        Writes m = sample_table();
        traits(m, p);
        standing_bytes(m, standing_for(p, club_of(p)));
        if (n % 5 == 0) m.push_back(w32(club_of(p) + 0x2c, 0));
        go(be8, args({{3, real_press}, {4, p}, {5, choose({0, 0, 1, 0x100})}}), m, answers_for(n % 2 == 0), "be8 random");
    }

    // ---- 0x106a5e90 (be5)
    {
        const auto p = settled[0], t = club_of(p);
        const auto base = [&](std::uint32_t kind, std::uint32_t flag, std::uint32_t other) { return args({{3, real_press}, {4, p}, {5, t}, {6, kind}, {7, flag}, {8, other}}); };
        go(be5, args({{3, real_press}, {4, 0}, {5, t}, {6, 1}, {7, 1}, {8, 0}}), {}, {}, "be5 no person");
        go(be5, base(1, 1, 0), {w32(p + 0xc, 0)}, {}, "be5 without link");
        go(be5, base(1, 1, 0), {w32(p + 0x24, 0)}, {}, "be5 without club");
        go(be5, base(1, 1, 0), {w32(t + 0x90, 0)}, {}, "be5 without manager");
        go(be5, base(1, 1, 0), {w32(t + 0x3c, 0)}, {}, "be5 club without +0x3c");
        go(be5, base(1, 1, 0), {w32(p + 0x14, 0)}, {}, "be5 without contract");
        go(be5, base(2, 1, 0), {}, {}, "be5 kind 2 without other");
        go(be5, base(2, 1, settled[1]), {}, {}, "be5 kind 2 with other");
        go(be5, base(3, 0, 0), {}, {}, "be5 kind 3");
        go(be5, base(0, 0, 0), {}, {}, "be5 kind 0");
        go(be5, base(1, 1, 0), {w32(t + 0x2c, 0)}, {}, "be5 club without +0x2c");
    }
    for (unsigned n = 0; n < 200; ++n) {
        const auto p = settled[gen() % settled.size()], q = settled[gen() % settled.size()];
        Writes m = sample_table();
        traits(m, p);
        m.push_back(h16(teams[n % 2] + 0x1ec, pick_from(points)));
        if (n % 5 == 0) m.push_back(w32(teams[n % 2] + 0x2c, 0));
        const auto kind = choose({1, 2, 3, 0, 0x102, 0xffffff02u});
        go(be5, args({{3, real_press}, {4, p}, {5, teams[n % 2]}, {6, kind}, {7, choose({0, 1, 0x80, 0x100})}, {8, choose({0, 0, (unsigned)q})}}), m, answers_for(n % 2 == 0), "be5 random");
    }

    // ---- 0x106a7d70 (be4)
    {
        const auto p = settled[0], q = settled[1], t = club_of(p);
        const auto base = args({{3, real_press}, {4, t}, {5, club_of(q)}, {6, p}, {7, q}});
        go(be4, args({{3, real_press}, {4, 0}, {5, club_of(q)}, {6, p}, {7, q}}), {}, {}, "be4 no club");
        go(be4, base, {w32(t + 0x90, 0)}, {}, "be4 without manager");
        go(be4, base, {}, {{0x100c7d00, 1}}, "be4 national team");
        go(be4, args({{3, real_press}, {4, t}, {5, club_of(q)}, {6, p}, {7, 0}}), {}, {}, "be4 no second");
        go(be4, base, {w32(q + 0x14, 0)}, {}, "be4 second without contract");
        go(be4, args({{3, real_press}, {4, t}, {5, club_of(q)}, {6, 0}, {7, q}}), {}, {}, "be4 no first");
        go(be4, base, {w32(p + 0x14, 0)}, {}, "be4 first without contract");
        go(be4, base, {w32(t + 0x2c, 0)}, {}, "be4 club without +0x2c");
    }
    for (unsigned n = 0; n < 160; ++n) {
        const auto p = settled[gen() % settled.size()], q = settled[gen() % settled.size()];
        Writes m = sample_table();
        m.push_back(b8(q + 0x67, pick_from(bytes)));
        if (n % 5 == 0) m.push_back(w32(club_of(p) + 0x2c, 0));
        auto a = answers_for(n % 2 == 0);
        a.push_back({0x100c7d00, 0});
        go(be4, args({{3, real_press}, {4, club_of(p)}, {5, club_of(q)}, {6, p}, {7, q}}), m, a, "be4 random");
    }

    // ---- 0x106a52d0 (react)
    {
        const auto p = settled[0], t = club_of(p), st = standing_for(p, t);
        const auto base = [&](std::uint32_t kind) { return args({{3, real_press}, {4, p}, {5, t}, {6, kind}}); };
        go(react, args({{3, real_press}, {4, 0}, {5, t}, {6, 1}}), {}, {}, "react no person");
        go(react, base(1), {w32(p + 0x24, 0)}, {}, "react without club");
        go(react, base(1), {w32(p + 0x14, 0)}, {}, "react without contract");
        go(react, args({{3, real_press}, {4, p}, {5, 0}, {6, 1}}), {}, {}, "react no club argument");
        go(react, base(1), {w32(t + 0x90, 0)}, {}, "react without manager");
        go(react, base(1), {}, {{0x10082f90, 0}}, "react no standing");
        for (const auto kind : {0u, 1u, 2u, 3u, 4u}) go(react, base(kind), {}, {}, "react kinds");
        (void)st;
    }
    for (unsigned n = 0; n < 400; ++n) {
        const auto p = settled[gen() % settled.size()], t = club_of(p), st = standing_for(p, t);
        Writes m = sample_table();
        traits(m, p);
        standing_bytes(m, st);
        // The press history's first row: sometimes the item the search looks for.
        const auto press_history = scratch_base + 0x600;
        if (n % 3 == 0) {
            append(m, {w32(real_press, press_history), w32(real_press + 8, 1), w32(press_history + 0x14, choose({0xbe8, 0xbe8, 0xbe7})), h16(press_history + 0x12, choose({scenario.r32(t), scenario.r32(t), 0x77})), w32(press_history + 8, choose({0, 5}))});
        } else if (n % 7 == 0) {
            m.push_back(w32(real_press + 8, 0));
        }
        auto a = answers_for(false);
        a.push_back({0x100d3150, choose({0, 1, 0x100})});
        go(react, args({{3, real_press}, {4, p}, {5, t}, {6, choose({1, 2, 3, 1, 2, 3, 0, 4, 0x101, 0xffffff02u})}}), m, a, "react random");
    }

    // ---- 0x106a2e10 (settle)
    {
        const auto p = settled[0], t = club_of(p);
        const auto base = [&](std::uint32_t other, std::uint32_t kind) { return args({{3, real_press}, {4, p}, {5, t}, {6, other}, {7, kind}}); };
        go(settle, args({{3, real_press}, {4, 0}, {5, t}, {6, teams[1]}, {7, 1}}), {}, {}, "settle no person");
        go(settle, base(teams[1], 1), {w32(p + 0x24, 0)}, {}, "settle without club");
        go(settle, base(teams[1], 1), {w32(p + 0x14, 0)}, {}, "settle without contract");
        go(settle, args({{3, real_press}, {4, p}, {5, 0}, {6, teams[1]}, {7, 1}}), {}, {}, "settle no club argument");
        go(settle, base(teams[1], 1), {w32(t + 0x90, 0)}, {}, "settle without manager");
        go(settle, base(teams[1], 1), {}, {{0x10082f90, 0}}, "settle no standing");
        for (const auto kind : {0u, 1u, 2u, 3u, 4u}) go(settle, base(teams[1], kind), {}, {}, "settle kinds");
    }
    for (unsigned n = 0; n < 500; ++n) {
        const auto p = settled[gen() % settled.size()], t = club_of(p), st = standing_for(p, t);
        Writes m = sample_table();
        traits(m, p);
        standing_bytes(m, st);
        const auto other = choose({0, 0, (unsigned)teams[n % 12 < teams.size() ? n % 12 : 0], (unsigned)teams[1]});
        if (other && n % 4 == 0) m.push_back(w32(other + 0x90, 0));
        auto a = answers_for(false);
        for (const auto entry : {0x100d9520u, 0x100d9e10u, 0x100d9b80u, 0x100d9580u, 0x100d9e90u, 0x100d9960u}) a.push_back({entry, choose({0, 1, 0x100})});
        go(settle, args({{3, real_press}, {4, p}, {5, t}, {6, other}, {7, choose({1, 2, 3, 1, 2, 3, 0, 0x101})}}), m, a, "settle random");
    }

    // ---- 0x1069f270 (pick)
    {
        const auto p = settled[0], t = club_of(p);
        const auto base = [&](std::uint32_t other, std::uint32_t flag) { return args({{3, real_press}, {4, p}, {5, t}, {6, other}, {7, flag}}); };
        go(pick, args({{3, real_press}, {4, 0}, {5, t}, {6, teams[1]}, {7, 1}}), {}, {}, "pick no person");
        go(pick, args({{3, real_press}, {4, p}, {5, 0}, {6, teams[1]}, {7, 1}}), {}, {}, "pick no club");
        go(pick, base(teams[1], 1), {w32(p + 0x14, 0)}, {}, "pick without contract");
        go(pick, base(teams[1], 1), {h16(p + 0x5c, 0x12)}, {}, "pick young");
        go(pick, base(teams[1], 1), {h16(scenario.r32(p + 0x14) + 0xa, 0x1099)}, {}, "pick low rating");
        go(pick, base(0, 0), {}, {}, "pick real");
        go(pick, base(teams[1], 1), {}, {}, "pick real with second club");
    }
    for (unsigned n = 0; n < 900; ++n) {
        const auto p = settled[gen() % settled.size()], t = club_of(p);
        Writes m = sample_table();
        traits(m, p);
        m.push_back(h16(p + 0x5c, choose({0x12, 0x13, 0x14, 0x16, 0x17, 0x18, 0x19, 0x1b, 0x1c, 0x21, 0x22, 0x23, 0x24})));
        m.push_back(h16(scenario.r32(p + 0x14) + 0xa, choose({0x109a, 0x109b, 0x128e, 0x1482, 0x1e46, 0x1e47, 0x2000, 0x222e, 0x2800, 0x7fff})));
        m.push_back(h16(scenario.r32(p + 0x14) + 0xc, choose({0x186a, 0x186b, 0x2000})));
        m.push_back(h16(scenario.r32(p + 0x14) + 4, choose({0x64, 0x65, 0x500})));
        m.push_back(h16(t + 0x1ec, pick_from(points)));
        const auto other = choose({0, 0, (unsigned)teams[0], (unsigned)teams[1], (unsigned)teams[2]});
        if (other) m.push_back(h16(other + 0x1ec, pick_from(points)));
        // The press history's first row: sometimes the one the rival search needs (story 0xbd0, the club's id 0x25).
        const auto press_history = scratch_base + 0x600;
        if (n % 3 != 0) {
            const auto rival = settled[gen() % settled.size()];
            if (n % 4 == 1 && scenario.r32(rival + 0x14)) append(m, {b8(scenario.r32(rival + 0x14) + 0xf, 0x14), b8(scenario.r32(p + 0x14) + 0xf, 3)});
            append(m, {w32(real_press, press_history), w32(t, choose({0x25, 0x25, 0x25, 0x24})), w32(real_press + 8, 1), w32(press_history + 0x14, choose({0xbd0, 0xbd0, 0xbd0, 0xbd1})), h16(press_history + 0x12, choose({0x25, 0x25, 0x25, 0x26})),
                       b8(press_history + 0x10, choose({1, 2, 3, 0, 0x81})), w32(press_history + 8, (rival - scenario.global(-0x7be0)) / 0x70)});
            if (n % 4 == 0) m.push_back(w32(real_press + 8, 0));
        }
        // The squad: random traits on the first members; one slot may hold the person himself, another be empty.
        for (unsigned slot = 0; slot < 50; ++slot) {
            const auto member = scenario.r32(t + 0x98 + slot * 4);
            if (!member) continue;
            if (slot == 3 && n % 6 == 0) { m.push_back(w32(t + 0x98 + slot * 4, p)); continue; }
            if (slot == 5 && n % 7 == 0) { m.push_back(w32(t + 0x98 + slot * 4, 0)); continue; }
            if (slot == 4 && n % 8 == 0) { m.push_back(w32(member + 0x14, 0)); continue; }
            if (slot < 12 || gen() % 6 == 0) {
                traits(m, member);
                m.push_back(h16(member + 0x5c, choose({0x12, 0x13, 0x14, 0x16, 0x17, 0x18, 0x1b, 0x1c, 0x21, 0x22, 0x23, 0x24, 0x80})));
                if (scenario.r32(member + 0x14)) m.push_back(h16(scenario.r32(member + 0x14) + 0xa, choose({0x1000, 0x109a, 0x109b, 0x1482, 0x2000, 0x7fff})));
            }
        }
        if (other) {
            for (unsigned slot = 0; slot < 50; ++slot) {
                const auto member = scenario.r32(other + 0x98 + slot * 4);
                if (!member) continue;
                if (slot == 2 && n % 6 == 0) { m.push_back(w32(other + 0x98 + slot * 4, p)); continue; }
                if (slot == 6 && n % 7 == 0) { m.push_back(w32(other + 0x98 + slot * 4, 0)); continue; }
                if (slot == 1 && n % 8 == 0) { m.push_back(w32(member + 0x14, 0)); continue; }
                if (slot < 10 || gen() % 6 == 0) {
                    traits(m, member);
                    m.push_back(h16(member + 0x5c, choose({0x12, 0x13, 0x14, 0x16, 0x17, 0x18, 0x1b, 0x1c, 0x21, 0x22, 0x23, 0x24, 0x80})));
                    if (scenario.r32(member + 0x14)) m.push_back(h16(scenario.r32(member + 0x14) + 0xa, choose({0x1000, 0x109a, 0x109b, 0x1482, 0x2000, 0x7fff})));
                }
            }
            if (n % 5 == 0) m.push_back(w32(other + 0x30, 0));
            if (n % 11 == 0) m.push_back(w32(other + 0x2c, 0));
        }
        {
            const auto rows = scenario.r32(scenario.global(-0x79f0));
            m.push_back(h16(rows + scenario.r32(t) * 0x4c + 8, choose({0x8000, 0xfa00, 0x7000, 0x2400, 0x2000, 0x100})));
            const int top_slots[] = {-0x7598, -0x7514, -0x757c, -0x7594, -0x7574, -0x7578, -0x7650};
            if (other && n % 6 == 0 && scenario.r32(other + 0x2c)) m.push_back(w32(scenario.r32(other + 0x2c) + 0x68, scenario.global(top_slots[n % 7])));
        }
        // The shared standing for all members, or the real lookups.
        Answers a = answers_for(false);
        const auto st = standing_for(p, t);
        if (n % 2 == 0) {
            a.push_back({0x10082f90, st});
            standing_bytes(m, st);
        }
        a.push_back({0x10718130, choose({0, 0x10, 0x20, 0x30, 0x1000, 0xfffff000u})});
        a.push_back({0x100c9070, choose({0x10, 0x20, 0x27, 0x28, 0x29, 0x2d, 0x30, 0x7f, 0x80, 0xf0})});
        a.push_back({0x1007c140, choose({0, 1, 2, 3})});
        a.push_back({0x100dbc20, choose({0, 1})});
        if (n % 3 != 0) region_answers(a);
        else a.push_back({0x100db120, choose({0, 1, 2, 3, 4, 5, 6, 7})});
        go(pick, args({{3, real_press}, {4, p}, {5, t}, {6, other}, {7, choose({0, 1, 1, 0x80, 0x100})}}), m, a, "pick random");
    }
    // The rival the press history names: every mark, regions that agree and differ, ratings either side of the limits.
    for (unsigned n = 0; n < 120; ++n) {
        const auto p = settled[gen() % settled.size()], t = club_of(p), rival = settled[gen() % settled.size()];
        if (rival == p) continue;
        const auto press_history = scratch_base + 0x600;
        Writes m = sample_table();
        traits(m, p);
        m.push_back(h16(scenario.r32(p + 0x14) + 0xa, choose({0x109a, 0x128e, 0x1300, 0x2000})));
        m.push_back(h16(scenario.r32(rival + 0x14) + 0xa, choose({0x1000, 0x1300, 0x2000})));
        append(m, {w32(real_press, press_history), w32(t, 0x25), w32(real_press + 8, 1), w32(press_history + 0x14, 0xbd0), h16(press_history + 0x12, 0x25),
                   b8(press_history + 0x10, choose({1, 2, 3, 0x81, 0})), w32(press_history + 8, (rival - scenario.global(-0x7be0)) / 0x70)});
        append(m, {b8(scenario.r32(rival + 0x14) + 0xf, n % 2 ? 0x14 : 3), b8(scenario.r32(p + 0x14) + 0xf, n % 4 < 2 ? 0x14 : 3)});
        const auto st = standing_for(p, t);
        standing_bytes(m, st);
        go(pick, args({{3, real_press}, {4, p}, {5, t}, {6, 0}, {7, 0}}), m, {{0x10082f90, st}}, "pick rival");
    }
    // The react draws that come out zero (every sample 0).
    for (unsigned n = 0; n < 60; ++n) {
        const auto p = settled[gen() % settled.size()], t = club_of(p), st = standing_for(p, t);
        Writes m = table(scenario, std::vector<std::uint32_t>(40, 0), 0);
        traits(m, p);
        standing_bytes(m, st);
        m.push_back(b8(p + 0x65, choose({6, 7, 10, 20})));
        m.push_back(b8(p + 0x66, choose({2, 5, 10})));
        m.push_back(b8(st + 0x45, choose({0, 1, 2})));
        go(react, args({{3, real_press}, {4, p}, {5, t}, {6, choose({1, 2, 3})}}), m, {{0x100d3150, 0}}, "react zero draws");
    }
    // Targeted: a strong person, the second club's members ready for the stories, the standing ready for them. Mode 1
    // gives the person a low +0xc (the first story then takes its young-person branch: modify with 0x40000000), mode 2
    // empties the tier table's row of the second club's division (the pass ends before its squad).
    for (unsigned mode = 0; mode < 3; ++mode) for (unsigned n = 0; n < (mode == 0 ? 400u : 160u); ++n) {
        const auto p = settled[gen() % settled.size()], t = club_of(p);
        const auto other = teams[1 + gen() % 3];
        if (other == t) continue;
        const auto st = standing_for(p, t);
        Writes m = sample_table();
        const auto profile = scenario.r32(p + 0x14);
        const unsigned variant = n % 5;
        m.push_back(h16(p + 0x5c, variant == 1 || variant == 4 ? 0x19 : 0x14));
        m.push_back(h16(profile + 0xa, variant == 2 ? 0x2000 : variant == 1 ? 0x1100 : 0x1800));
        m.push_back(h16(profile + 0xc, mode == 1 ? 0x1000 : 0x2000));
        m.push_back(h16(profile + 4, 0x500));
        m.push_back(b8(p + 0x67, 15));
        if (mode == 2) m.push_back(w32(scenario.global(-0x7b5c) + scenario.r32(scenario.r32(other + 0x30)) * 4, 0));
        m.push_back(b8(p + 0x65, variant == 3 ? 6 : 10));
        m.push_back(h16(t + 0x1ec, variant == 1 ? 0x7000 : 0x1000));
        if (variant == 4) m.push_back(b8(profile + 0xf, 3));
        m.push_back(h16(other + 0x1ec, choose({0x1000, 0x1000, 0x1500, 0x9c3, 0x1c52})));
        // The shared standing: no 0x200/0x800/0x400 flags, a word without the top bit, a high +0x44.
        append(m, {b8(st + 0x44, 0x7f), w32(st + 0x48, choose({0, 0, 0x400, 0x200})), b8(st + 0x4c, choose({0, 0x19, 0xe0})), w32(st + 0x50, choose({1, 1, 0, 0x20000000})), b8(st + 0x45, 5)});
        for (unsigned slot = 0; slot < 50; ++slot) {
            const auto member = scenario.r32(other + 0x98 + slot * 4);
            if (!member || !scenario.r32(member + 0x14) || member == p) continue;
            const auto contract = scenario.r32(member + 0x14);
            append(m, {h16(contract + 0xa, 0x1500), b8(member + 0x63, variant == 1 || variant == 4 ? 0 : 0x7f), b8(member + 0x67, choose({5, 3, 8})), b8(member + 0x66, choose({0, 1, 1})),
                       b8(member + 0x65, choose({5, 12})), b8(member + 0x64, choose({0x7f, 3})), b8(member + 0x69, 5), h16(member + 0x5c, choose({0x18, 0x1a, 0x14})),
                       b8(contract + 0xf, variant == 1 || variant == 4 ? 0x14 : 0)});
        }
        for (unsigned slot = 0; slot < 50; ++slot) {
            const auto member = scenario.r32(t + 0x98 + slot * 4);
            if (!member || !scenario.r32(member + 0x14) || member == p || slot > 6) continue;
            append(m, {h16(scenario.r32(member + 0x14) + 0xa, 0x1500), b8(member + 0x67, choose({5, 12})), b8(member + 0x66, choose({0, 1, 6})), b8(member + 0x65, 12)});
        }
        Answers a = answers_for(false);
        a.push_back({0x10082f90, st});
        a.push_back({0x10718130, 0x1000});
        a.push_back({0x100c9070, choose({0x10, 0x20})});
        a.push_back({0x100db120, choose({0, 1})});
        a.push_back({0x1007c140, choose({0, 1, 2, 3})});
        a.push_back({0x100dbc20, 1});
        go(pick, args({{3, real_press}, {4, p}, {5, t}, {6, other}, {7, 1}}), m, a,
           mode == 0 ? "pick targeted" : mode == 1 ? "pick targeted, low +0xc" : "pick targeted, no tier row");
    }
    } catch (const std::exception& error) { std::cerr << "pick_c cases: " << error.what() << '\n'; throw; }
}
