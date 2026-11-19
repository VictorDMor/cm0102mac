// CUP.CPP's pilot cases, from the real career (--world): the game's own competition objects (one per competition
// id, toc -0x7b5c) and their stages, of which the cups are those whose kind byte (+0x43) is 1 and whose vtable
// names a CUP.CPP body. Objects a case needs that the career has no instance of (a fresh object, rows for the
// comparators, a cup with another entrant count) are built in the scratch area from real values with one field
// changed. Labels start with "pilot:" (CM_CUP_BLOCK=pilot).
#include "cup_base_checks.hpp"
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::cup_base_checks {
namespace {
constexpr std::uint32_t construct = 0x10478f20, destroy = 0x10478f70, unsupported_44 = 0x10478fd0,
                        unsupported_94 = 0x10479060, clear_b0 = 0x104790f0, has_entrant = 0x10486050,
                        by_byte_6 = 0x10481e70, by_state_descending = 0x10485830, by_state_ascending = 0x10485870;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60, clubs_slot = -0x7be8,
                       club_count_slot = -0x7b50;
// The base class's fields and the cup's up to +0xb0; the derived cups' own fields are not copied.
constexpr std::uint32_t object_size = 0xb4;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
struct World {
    const Scenario& s;
    std::vector<std::uint32_t> cups;  // every cup object the career has: competitions and their stages
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto table = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        std::set<std::uint32_t> seen;
        const auto add = [&](std::uint32_t object) { if (object && is_cup(object) && seen.insert(object).second) cups.push_back(object); };
        for (std::uint32_t id = 0; table && id < count && id < 4096; ++id) {
            const auto object = s.r32(table + id * 4);
            if (!object) continue;
            add(object);
            const auto last = std::int32_t(s.r32(object + 0x30));
            const auto stages = s.r32(object + 0xc);
            for (std::int32_t n = 0; stages && n <= last && n < 64; ++n) add(s.r32(stages + std::uint32_t(n) * 4));
        }
    }
    // A cup's vtable has the base class's 40 slots (+0x08..+0xa4); one the derived class does not override names
    // CUP.CPP's body. The kind byte +0x43 is 1 (the cup constructor's; a league's is 2).
    bool is_cup(std::uint32_t object) const {
        if (s.r32(object) < 0x108b0000 || s.r32(object) >= 0x10a00000 || (s.r32(object + 0x40) & 0xff) != 1) return false;
        for (std::uint32_t slot = 0x08; slot <= 0xa4; slot += 4) {
            const auto descriptor = s.r32(s.r32(object) + slot);
            if (descriptor < 0x108b0000 || descriptor >= 0x108c0000) return false;
            const auto code = s.r32(descriptor);
            if (code >= module_start && code < module_end) return true;
        }
        return false;
    }
    std::uint32_t club_count() const { return s.global(club_count_slot); }
    std::uint32_t club(unsigned index) const { return s.global(clubs_slot) + index * 0x248; }
    std::int16_t entrants(std::uint32_t cup) const { return std::int16_t(s.r32(cup + 0x3c) & 0xffff); }
    std::uint32_t entrant_table(std::uint32_t cup) const { return s.r32(cup + 0xa4); }
};
std::vector<Write> object_copy(const Scenario& s, std::uint32_t at, std::uint32_t object) {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < object_size; offset += 4) writes.push_back({4, at + offset, s.r32(object + offset)});
    return writes;
}
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

CUP_CASES("pilot") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.cups.size() < 3) throw std::runtime_error("pilot: the career has fewer than three cup objects");
    std::fprintf(stderr, "pilot: %zu cup objects\n", w.cups.size());
    const auto fresh = scratch_base + 0x100;
    // The constructor: a fresh object whose untouched bytes keep a pattern, so a stray store shows; over real cups.
    std::vector<Write> pattern;
    for (std::uint32_t offset = 0; offset < 0x100; offset += 4) pattern.push_back({4, fresh + offset, 0xa5a5a5a5u + offset});
    cases.push_back({.entry = construct, .registers = {{3, fresh}}, .memory = pattern, .label = "pilot: construct fresh"});
    for (std::size_t n = 0; n < w.cups.size() && n < 6; ++n)
        cases.push_back({.entry = construct, .registers = {{3, w.cups[n]}}, .label = "pilot: construct over " + hex(w.cups[n])});
    // The destructor: null; a copy without blocks under each flag word, which frees nothing of the base; real cups
    // with their blocks, keeping and freeing the object.
    cases.push_back({.entry = destroy, .registers = {{3, 0}, {4, 1}}, .label = "pilot: destroy null"});
    auto empty = object_copy(scenario, fresh, w.cups.front());
    empty.push_back({4, fresh + 0x14, 0});
    empty.push_back({4, fresh + 0x18, 0});
    for (const auto flag : {0u, 0x10000u, 0xffffu, 0x8000u})
        cases.push_back({.entry = destroy, .registers = {{3, fresh}, {4, flag}}, .memory = empty,
                         .label = "pilot: destroy without blocks, flag " + hex(flag)});
    for (std::size_t n = 0; n < w.cups.size() && n < 10; ++n)
        for (const auto flag : {0u, 1u})
            cases.push_back({.entry = destroy, .registers = {{3, w.cups[n]}, {4, flag}},
                             .label = "pilot: destroy " + hex(w.cups[n]) + " flag " + hex(flag)});
    // The free answered on both sides: `delete` (0x107735a0) reaches the dispatcher from the generated original too.
    cases.push_back({.entry = destroy, .registers = {{3, w.cups[1]}, {4, 1}}, .answers = {{0x107735a0, 0}},
                     .label = "pilot: destroy " + hex(w.cups[1]) + " with delete answered"});
    // The virtuals a derived cup must override, and the byte clear, on real cups.
    for (std::size_t n = 0; n < w.cups.size() && n < 3; ++n) {
        for (const auto entry : {unsupported_44, unsupported_94})
            cases.push_back({.entry = entry, .registers = {{3, w.cups[n]}, {4, 1}}, .label = "pilot: unsupported " + hex(entry)});
        cases.push_back({.entry = clear_b0, .registers = {{3, w.cups[n]}, {4, 7}}, .label = "pilot: clear +b0 of " + hex(w.cups[n])});
    }
    cases.push_back({.entry = clear_b0, .registers = {{3, fresh}}, .memory = pattern, .label = "pilot: clear +b0 fresh"});
}

CUP_CASES("pilot") {
    if (scenario.match) return;
    const World w(scenario);
    // has_entrant (vt +0x88): every real cup with an entrant table, for its first, middle and last entrants, a
    // club it doesn't have and 0; the loop's end at each count. Cups without a table only with a count <= 0
    // (the original reads the table at the first pass).
    std::size_t with_table = 0;
    std::uint32_t sample = 0;
    for (const auto cup : w.cups) {
        const auto count = w.entrants(cup);
        const auto table = w.entrant_table(cup);
        if (!table) {
            if (count <= 0) cases.push_back({.entry = has_entrant, .registers = {{3, cup}, {4, w.club(0)}}, .label = "pilot: has_entrant, no table, " + hex(cup)});
            continue;
        }
        if (!sample && count > 2) sample = cup;
        if (++with_table > 24) continue;
        std::set<std::uint32_t> members;
        for (std::int32_t n = 0; n < count; ++n) members.insert(scenario.r32(table + std::uint32_t(n) * 8));
        std::uint32_t outsider = 0;
        for (unsigned n = 0; n < w.club_count() && !outsider; ++n) if (!members.count(w.club(n))) outsider = w.club(n);
        for (const std::int32_t index : {0, count / 2, count - 1})
            if (index >= 0 && index < count)
                cases.push_back({.entry = has_entrant, .registers = {{3, cup}, {4, scenario.r32(table + std::uint32_t(index) * 8)}},
                                 .label = "pilot: has_entrant " + std::to_string(index) + " of " + hex(cup)});
        cases.push_back({.entry = has_entrant, .registers = {{3, cup}, {4, outsider}}, .label = "pilot: has_entrant outsider of " + hex(cup)});
        cases.push_back({.entry = has_entrant, .registers = {{3, cup}, {4, 0}}, .label = "pilot: has_entrant 0 of " + hex(cup)});
    }
    std::fprintf(stderr, "pilot: %zu cups with an entrant table\n", with_table);
    if (!sample) throw std::runtime_error("pilot: no cup with more than two entrants");
    // The count's sign and width on a copy of a real cup: 0, 1, negative, the halfword's extremes (the table
    // stands in a scratch block of rows, the last of which names the club looked for).
    const auto copy = scratch_base + 0x300, rows = scratch_base + 0x8000;
    const auto club = w.club(5);
    for (const std::uint32_t count : {0u, 1u, 2u, 0xffffu, 0x8000u, 0x7fffu}) {
        auto memory = joined(object_copy(scenario, copy, sample), {{4, copy + 0xa4, rows}, {2, copy + 0x3e, count}});
        const auto filled = std::min<std::uint32_t>(count & 0x8000 ? 0 : count, 0x7fff);
        for (std::uint32_t n = 0; n < filled; ++n) memory.push_back({4, rows + n * 8, n + 1 == filled ? club : w.club(n % 4)});
        cases.push_back({.entry = has_entrant, .registers = {{3, copy}, {4, club}}, .memory = memory,
                         .label = "pilot: has_entrant count " + hex(count)});
        cases.push_back({.entry = has_entrant, .registers = {{3, copy}, {4, w.club(9)}}, .memory = memory,
                         .label = "pilot: has_entrant absent, count " + hex(count)});
    }
}

CUP_CASES("pilot") {
    if (scenario.match) return;
    const World w(scenario);
    const auto left = scratch_base + 0x400, right = scratch_base + 0x440;
    const std::pair<std::uint32_t, std::uint32_t> bytes[] = {
        {3, 1}, {1, 3}, {5, 5}, {0x80, 0x7f}, {0x7f, 0x80}, {0xff, 0}, {0, 0xff}, {0xff, 0xff}};
    for (const auto& [a, b] : bytes) {
        cases.push_back({.entry = by_byte_6, .registers = {{3, left}, {4, right}},
                         .memory = {{1, left + 6, a}, {1, right + 6, b}, {1, left + 5, 0x11}, {1, right + 7, 0x22}},
                         .label = "pilot: byte +6 " + hex(a) + " " + hex(b)});
    }
    // Entrant rows: the byte +4 decides unless equal; then the halfword +0x1ec of the records the rows point at.
    const auto record_a = scratch_base + 0x1000, record_b = scratch_base + 0x1400;
    const std::pair<std::uint32_t, std::uint32_t> halves[] = {
        {30, 20}, {20, 30}, {12, 12}, {0xffff, 1}, {0x8000, 0x7fff}, {0x7fff, 0x8000}, {0, 0xffff}};
    for (const auto entry : {by_state_descending, by_state_ascending}) {
        for (const auto& [a, b] : bytes)
            cases.push_back({.entry = entry, .registers = {{3, left}, {4, right}},
                             .memory = {{4, left, record_a}, {4, right, record_b}, {1, left + 4, a}, {1, right + 4, b},
                                        {2, record_a + 0x1ec, 7}, {2, record_b + 0x1ec, 9}},
                             .label = "pilot: state " + hex(entry) + " " + hex(a) + " " + hex(b)});
        for (const auto& [a, b] : halves)
            cases.push_back({.entry = entry, .registers = {{3, left}, {4, right}},
                             .memory = {{4, left, record_a}, {4, right, record_b}, {1, left + 4, 2}, {1, right + 4, 2},
                                        {2, record_a + 0x1ec, a}, {2, record_b + 0x1ec, b},
                                        {2, record_a + 0x1ea, 0x3333}, {2, record_b + 0x1ee, 0x4444}},
                             .label = "pilot: state " + hex(entry) + " equal, +1ec " + hex(a) + " " + hex(b)});
        // Real entrant rows: neighbouring rows of the cups' tables, both ways round.
        unsigned taken = 0;
        for (const auto cup : w.cups) {
            const auto table = w.entrant_table(cup);
            const auto count = w.entrants(cup);
            if (!table || count < 2) continue;
            for (std::int32_t n = 0; n + 1 < count && n < 6; ++n) {
                const auto a = table + std::uint32_t(n) * 8, b = a + 8;
                cases.push_back({.entry = entry, .registers = {{3, a}, {4, b}}, .label = "pilot: state " + hex(entry) + " rows " + hex(a)});
                cases.push_back({.entry = entry, .registers = {{3, b}, {4, a}}, .label = "pilot: state " + hex(entry) + " rows " + hex(b)});
            }
            if (++taken >= 6) break;
        }
        for (unsigned n = 0; n + 1 < w.club_count() && n < 30; n += 3)
            cases.push_back({.entry = entry, .registers = {{3, left}, {4, right}},
                             .memory = {{4, left, w.club(n)}, {4, right, w.club(n + 1)}, {1, left + 4, 1}, {1, right + 4, 1}},
                             .label = "pilot: state " + hex(entry) + " equal, clubs " + std::to_string(n)});
    }
}
}
}
