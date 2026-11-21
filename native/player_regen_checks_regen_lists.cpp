// PLAYER_REGEN.CPP, block `regen_lists`: cases for the regen object's lists (A retirement entries at +4/+8/+0xc, B regen queue at
// +0x10/+0x14/+0x18, C non-player staff pool at +0x1c/+0x20/+0x24, D player pool at +0x3c/+0x40/+0x44) and the routines that
// append to them, remove from them and register a regenerated person: 0x102deb70, 0x102ded90, 0x102dbeb0, 0x102dea50, 0x102dec50,
// 0x102dee70, 0x102df020, 0x102df1a0, 0x102df6d0, 0x102eb670.
//
// The regen object is the career's own (a captured state: season-day; the word the TOC slot -0x7b28 holds); the cases write its
// counts, capacities and entries over the object's real blocks (each holds 250 entries, so a count up to 250 stays inside it)
// and undo them afterwards. A full list (count = capacity - 1) makes an appender reallocate its block through the real
// allocator (the heap is compared); the allocator's answer 0 reaches the original's assertion and its end of the game
// (answered by the checker's scripted dialog and exit). Real people give the records of the registration: staff only, player
// only, both, and (with the nation word written to 0) staff without a nation, whose random nation is drawn from the pooled
// generator at the cases' positions of the pool (before the end, at the last sample, the scaled form for a nation count of
// 0x10000 or more in magnitude). Lists A's cases are sorted by the comparator's own order (the retirement search is a bsearch).
// Labels start with "regen_lists:" (CM_PLAYER_REGEN_BLOCK=regen_lists).
#include "player_regen_checks.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t remove_staff = 0x102deb70, remove_player = 0x102ded90, pop_staff = 0x102dbeb0, append_staff = 0x102dea50,
                        append_player = 0x102dec50, append_retire = 0x102dee70, remove_retire = 0x102df020, register_regen = 0x102df1a0,
                        pop_queue = 0x102df6d0, remove_by_id = 0x102eb670, reallocate = 0x107751b0;
constexpr std::int32_t regen_slot = -0x7b28, nation_count_slot = -0x7b04, date_slot = -0x7b80, cursor_slot = -0x7b84, end_slot = -0x7b88,
                       salt_slot = -0x7b90, start_slot = -0x7b8c;
constexpr std::uint32_t area = scratch_base + 0x20000, key_at = area, out_at = area + 0x100, rec_at = area + 0x200;

struct ListAt { std::uint32_t entries, count, capacity, size; };
constexpr ListAt list_a{0x04, 0x08, 0x0c, 0x1c}, list_b{0x10, 0x14, 0x18, 0xc}, list_c{0x1c, 0x20, 0x24, 0x10}, list_d{0x3c, 0x40, 0x44, 0x10};

struct World {
    const Scenario& s;
    std::uint32_t regen;
    explicit World(const Scenario& scenario) : s(scenario), regen(scenario.global(regen_slot)) {
        if (regen == 0) throw std::runtime_error("regen_lists: the career has no regen object");
    }
    std::uint32_t entries(const ListAt& l) const { return s.r32(regen + l.entries); }
    std::uint32_t capacity(const ListAt& l) const { return s.r32(regen + l.capacity); }
    std::uint32_t entry(const ListAt& l, std::uint32_t index) const { return entries(l) + index * l.size; }
    // The count word and the capacity written for a case.
    std::vector<Write> counted(const ListAt& l, std::int32_t count) const { return {w32(regen + l.count, std::uint32_t(count))}; }
};

// An entry of a pool list (C, D): four words, the first the key; the others distinct (the fourth is copied by the shifts too).
std::vector<Write> pool_entry(std::uint32_t at, std::uint32_t id, std::uint32_t salt) {
    return {w32(at, id), w32(at + 4, 0x1000 + salt), w32(at + 8, 0x2000 + salt), w32(at + 12, 0x3000 + salt)};
}
// `count` entries of a pool list with the ids 100, 107, ... (id of entry n = base + 7 n).
std::vector<Write> pool_entries(const World& w, const ListAt& l, std::int32_t count, std::uint32_t base = 100) {
    std::vector<Write> memory;
    for (std::int32_t n = 0; n < count && n < 250; ++n) append(memory, pool_entry(w.entry(l, std::uint32_t(n)), base + 7 * std::uint32_t(n), std::uint32_t(n) * 16));
    return memory;
}
// An entry of the queue: id, two halfwords, a byte and filler (the byte +8 is copied by a shift, +9..+0xb are not).
std::vector<Write> queue_entry(std::uint32_t at, std::uint32_t id, std::uint32_t salt) {
    return {w32(at, id), w16(at + 4, 0x8000 + salt), w16(at + 6, 0x100 + salt), w8(at + 8, 0x40 + salt), w8(at + 9, 0xa1), w8(at + 10, 0xa2), w8(at + 11, 0xa3)};
}
// An entry of list A: every field written, the unused bytes too (the original copies and clears only some of them).
struct Retire { std::uint32_t id, club; std::int32_t day, year, day2, year2; std::uint32_t b0c, b14, kind, flag; };
std::vector<Write> retire_entry(std::uint32_t at, const Retire& r) {
    return {w32(at, r.id), w32(at + 4, r.club), w16(at + 8, std::uint32_t(r.day)), w16(at + 0xa, std::uint32_t(r.year)), w8(at + 0xc, r.b0c),
            w8(at + 0xd, 0xb1), w8(at + 0xe, 0xb2), w8(at + 0xf, 0xb3), w16(at + 0x10, std::uint32_t(r.day2)), w16(at + 0x12, std::uint32_t(r.year2)),
            w8(at + 0x14, r.b14), w8(at + 0x15, 0xb4), w8(at + 0x16, 0xb5), w8(at + 0x17, 0xb6), w8(at + 0x18, r.kind), w8(at + 0x19, r.flag),
            w8(at + 0x1a, 0xb7), w8(at + 0x1b, 0xb8)};
}

// The generator's cells and the writes that put it at a position with a salt and the samples of the next draws.
struct Pool {
    std::uint32_t cursor_cell, end_cell, salt_cell, start, cursor, end;
    explicit Pool(const Scenario& s)
        : cursor_cell(s.toc(cursor_slot)), end_cell(s.toc(end_slot)), salt_cell(s.toc(salt_slot)), start(s.toc(start_slot)),
          cursor(s.r32(cursor_cell)), end(s.r32(end_cell)) {}
    std::vector<Write> at(std::uint32_t position, std::uint32_t salt, const std::vector<std::uint32_t>& samples = {}) const {
        std::vector<Write> memory{w32(cursor_cell, position), w32(salt_cell, salt)};
        std::uint32_t next = position;
        for (const auto sample : samples) {
            next += 2;
            memory.push_back(w16(next > end ? start : next, sample));
        }
        return memory;
    }
};

struct Kinds { std::vector<std::uint32_t> staff_only, player_only, both; };
Kinds people_kinds(const Scenario& s) {
    Kinds k;
    for (std::uint32_t n = 0; n + 16 < s.people(); ++n) {
        const auto p = s.person(n);
        const bool staff = s.r32(p + 0x1c) != 0, player = s.r32(p + 0x14) != 0, nation = s.r32(p + 0xc) != 0;
        if (!nation) continue;
        (staff && player ? k.both : staff ? k.staff_only : k.player_only).push_back(n);
    }
    return k;
}
}

PLAYER_REGEN_CASES("regen_lists") {
    // First (the call trace pairs only an entry's first calls and every 64th): the registration whose random nation refills the pool,
    // for the first draw's and for the scaled form's second draw's refill, and a growth of each list (a traced reallocation).
    const World w(scenario);
    const Pool pool(scenario);
    const auto kinds = people_kinds(scenario);
    if (kinds.staff_only.empty() || kinds.player_only.empty()) throw std::runtime_error("regen_lists: the career has no staff-only or player-only person");
    const auto staff = kinds.staff_only[kinds.staff_only.size() / 2];
    const auto nation_cell = scenario.toc(nation_count_slot);
    const auto record = [&](std::uint32_t n) { return std::vector<Write>{w32(rec_at, n), w16(rec_at + 4, 0x1234), w16(rec_at + 6, 0x5678)}; };
    for (const std::uint32_t before : {0u, 1u, 2u})
        for (const std::int32_t bound : {7, 0x10000}) {
            auto memory = pool.at(pool.end - 2 * before, 5, {1000, 20});
            append(memory, record(staff));
            append(memory, {w32(scenario.person(staff) + 0xc, 0), w32(nation_cell, std::uint32_t(bound))});
            cases.push_back({.entry = register_regen, .registers = {{3, w.regen}, {4, rec_at}}, .memory = memory,
                             .label = "regen_lists: traced refill, nation count " + std::to_string(bound) + " samples before the end " + std::to_string(before)});
        }
    // A traced growth of lists B, C and D through the registration (a person with both records).
    if (!kinds.both.empty()) {
        const auto both = kinds.both[kinds.both.size() / 2];
        auto memory = record(both);
        append(memory, w.counted(list_b, std::int32_t(w.capacity(list_b)) - 1));
        append(memory, w.counted(list_c, std::int32_t(w.capacity(list_c)) - 1));
        append(memory, w.counted(list_d, std::int32_t(w.capacity(list_d)) - 1));
        cases.push_back({.entry = register_regen, .registers = {{3, w.regen}, {4, rec_at}}, .memory = memory, .label = "regen_lists: traced growth of B, C and D"});
    }
    // Every assertion of the registration (the id test, the growth of B, C and D failing, the D test reading the count), each once, so
    // that the call trace pairs their calls (the dialog's and the end of the game's included).
    {
        const auto cap_b = std::int32_t(w.capacity(list_b)), cap_c = std::int32_t(w.capacity(list_c)), cap_d = std::int32_t(w.capacity(list_d));
        const auto n = !kinds.both.empty() ? kinds.both[0] : staff;
        cases.push_back({.entry = register_regen, .registers = {{3, w.regen}, {4, rec_at}}, .memory = record(0xffffffffu), .label = "regen_lists: traced register id -1 (assertion 0xa9d)"});
        struct Failing { const char* name; std::int32_t b, c, d; };
        for (const auto& count : {Failing{"B", cap_b - 1, 3, 3}, Failing{"C", 3, cap_c - 1, 3}, Failing{"D", 3, 3, cap_d - 1}}) {
            auto memory = record(n);
            append(memory, w.counted(list_b, count.b));
            append(memory, w.counted(list_c, count.c));
            append(memory, w.counted(list_d, count.d));
            cases.push_back({.entry = register_regen, .registers = {{3, w.regen}, {4, rec_at}}, .memory = memory, .answers = {{reallocate, 0}},
                             .label = std::string("regen_lists: traced register, growth of ") + count.name + " fails", .original_owners = {reallocate}});
        }
        // The test of the pool D growth reads its COUNT: an empty list of capacity 1 reports (and ends the game); a list that is not
        // empty goes on storing through the missing block (both sides stop alike on the unmapped store).
        for (const std::int32_t count_d : {0, 3}) {
            auto memory = record(n);
            append(memory, {w32(w.regen + list_d.capacity, std::uint32_t(count_d) + 1), w32(w.regen + list_d.count, std::uint32_t(count_d))});
            cases.push_back({.entry = register_regen, .registers = {{3, w.regen}, {4, rec_at}}, .memory = memory, .answers = {{reallocate, 0}},
                             .label = "regen_lists: traced register, D growth fails with count " + std::to_string(count_d), .original_owners = {reallocate}});
        }
    }
}

PLAYER_REGEN_CASES("regen_lists") {
    // 0x102df1a0: the registration of a real person of each kind, over the counts of the three lists.
    const World w(scenario);
    const auto kinds = people_kinds(scenario);
    const auto record = [&](std::uint32_t n) { return std::vector<Write>{w32(rec_at, n), w16(rec_at + 4, 0x8765), w16(rec_at + 6, 0xfedc)}; };
    std::fprintf(stderr, "regen_lists: %zu staff-only, %zu player-only and %zu both-record people with a nation; lists A..D: counts %u %u %u %u, capacities %u %u %u %u\n",
                 kinds.staff_only.size(), kinds.player_only.size(), kinds.both.size(), scenario.r32(w.regen + 8), scenario.r32(w.regen + 0x14),
                 scenario.r32(w.regen + 0x20), scenario.r32(w.regen + 0x40), w.capacity(list_a), w.capacity(list_b), w.capacity(list_c), w.capacity(list_d));
    const auto cap_b = std::int32_t(w.capacity(list_b)), cap_c = std::int32_t(w.capacity(list_c)), cap_d = std::int32_t(w.capacity(list_d));
    struct Counts { const char* name; std::int32_t b, c, d; };
    const std::array<Counts, 9> counts{{{"as it is", -1, -1, -1}, {"empty", 0, 0, 0}, {"B full", cap_b - 1, 3, 3}, {"C full", 3, cap_c - 1, 3}, {"D full", 3, 3, cap_d - 1},
                                        {"all full", cap_b - 1, cap_c - 1, cap_d - 1}, {"B at capacity", cap_b, 3, 3}, {"C at capacity", 3, cap_c, 3}, {"D at capacity", 3, 3, cap_d}}};
    struct Person { const char* name; std::vector<std::uint32_t> list; };
    for (const auto& kind : {Person{"staff only", kinds.staff_only}, Person{"player only", kinds.player_only}, Person{"both", kinds.both}})
        for (std::size_t pick = 0; pick < std::min<std::size_t>(kind.list.size(), 3); ++pick)
            for (const auto& count : counts) {
                const auto n = kind.list[pick * kind.list.size() / std::min<std::size_t>(kind.list.size(), 3)];
                auto memory = record(n);
                if (count.b >= 0) append(memory, w.counted(list_b, count.b));
                if (count.c >= 0) append(memory, w.counted(list_c, count.c));
                if (count.d >= 0) append(memory, w.counted(list_d, count.d));
                cases.push_back({.entry = register_regen, .registers = {{3, w.regen}, {4, rec_at}}, .memory = memory,
                                 .label = std::string("regen_lists: register ") + kind.name + " " + hex(n) + " lists " + count.name});
            }
    // A person with neither record joins only the queue B (the staff and player words zeroed for the case).
    if (!kinds.player_only.empty()) {
        const auto n = kinds.player_only[0];
        auto memory = record(n);
        append(memory, {w32(scenario.person(n) + 0x1c, 0), w32(scenario.person(n) + 0x14, 0)});
        cases.push_back({.entry = register_regen, .registers = {{3, w.regen}, {4, rec_at}}, .memory = memory, .label = "regen_lists: register a person with neither record"});
    }
    // Ids at the edges of the test (negative, 0, the count itself accepted, count + 1 reports): the people's count word.
    const auto count = scenario.people();
    for (const std::uint32_t id : {0xffffffffu, 0x80000000u, 0u, 1u, count - 1, count, count + 1, 0x7fffffffu})
        cases.push_back({.entry = register_regen, .registers = {{3, w.regen}, {4, rec_at}}, .memory = record(id),
                         .label = "regen_lists: register id " + hex(id)});
    // Staff without a nation: the random nation of the entry, over the nation count (the bound), the pool's positions and samples.
    if (!kinds.staff_only.empty()) {
        const Pool pool(scenario);
        const auto n = kinds.staff_only[0];
        const auto nation_cell = scenario.toc(nation_count_slot);
        const auto real_bound = std::int32_t(scenario.global(nation_count_slot));
        const std::array<std::int32_t, 20> bounds{real_bound, 0, 1, -1, 2, -7, 100, 0xffff, -0xffff, 0x10000, -0x10000, 0x10001, 0x20000, 0x33333334,
                                                  0x7fffffff, std::int32_t(0x80000000), -0x20000, 65535 * 3, 70000, 8191750};
        const auto middle = pool.cursor + 0x1000 < pool.end ? pool.cursor : pool.start + 0x400;
        for (const auto bound : bounds)
            for (const std::uint32_t position : {middle, pool.end, pool.end - 2, pool.end - 4})
                for (const std::array<std::uint32_t, 3> sample : {std::array<std::uint32_t, 3>{0, 0, 0}, {0xffff, 0xffff, 0xffff}, {12345, 54321, 7}, {0xfffe, 1, 99}}) {
                    auto memory = pool.at(position, sample[2], {sample[0], sample[1]});
                    append(memory, record(n));
                    append(memory, {w32(scenario.person(n) + 0xc, 0), w32(nation_cell, std::uint32_t(bound))});
                    cases.push_back({.entry = register_regen, .registers = {{3, w.regen}, {4, rec_at}}, .memory = memory,
                                     .label = "regen_lists: register staff without nation, count " + std::to_string(bound) + " pool " + hex(position - pool.start) + " sample " + hex(sample[0])});
                }
        // Both records without a nation: the staff entry draws, the player entry reads the word at 0x68 of address 0.
        const auto both = kinds.both.empty() ? n : kinds.both[0];
        auto memory = record(both);
        append(memory, {w32(scenario.person(both) + 0xc, 0)});
        cases.push_back({.entry = register_regen, .registers = {{3, w.regen}, {4, rec_at}}, .memory = memory, .label = "regen_lists: register both records, nation word 0"});
    }
}

PLAYER_REGEN_CASES("regen_lists") {
    // 0x102deb70 and 0x102ded90: the removal from pool lists C and D, then 0x102dbeb0 (pop C).
    const World w(scenario);
    struct Where { const char* name; std::int32_t index; };
    for (const auto& list : {std::pair{&list_c, remove_staff}, std::pair{&list_d, remove_player}})
        for (const std::int32_t count : {0, 1, 2, 3, 6, 250, -1})
            for (const Where where : {Where{"first", 0}, Where{"second", 1}, Where{"middle", 2}, Where{"last", 5}, Where{"last of a full list", 249}, Where{"absent", -1}}) {
                if (where.index >= count && where.index >= 0) continue;
                auto memory = pool_entries(w, *list.first, count);
                append(memory, w.counted(*list.first, count));
                const std::uint32_t key = where.index >= 0 ? 100 + 7 * std::uint32_t(where.index) : 99;
                cases.push_back({.entry = list.second, .registers = {{3, w.regen}, {4, key}}, .memory = memory,
                                 .label = std::string("regen_lists: remove from ") + (list.second == remove_staff ? "C" : "D") + " count " + std::to_string(count) + " key " + where.name});
            }
    // A duplicated key: the first entry goes; a negative key; the id -1 of a vacated entry.
    for (const auto& list : {std::pair{&list_c, remove_staff}, std::pair{&list_d, remove_player}}) {
        auto memory = pool_entries(w, *list.first, 5);
        append(memory, pool_entry(w.entry(*list.first, 3), 100 + 7, 77));
        append(memory, w.counted(*list.first, 5));
        cases.push_back({.entry = list.second, .registers = {{3, w.regen}, {4, 107}}, .memory = memory, .label = "regen_lists: remove a duplicated id"});
        auto minus = pool_entries(w, *list.first, 4);
        append(minus, pool_entry(w.entry(*list.first, 2), 0xffffffff, 5));
        append(minus, w.counted(*list.first, 4));
        cases.push_back({.entry = list.second, .registers = {{3, w.regen}, {4, 0xffffffffu}}, .memory = minus, .label = "regen_lists: remove id -1"});
    }
    // 0x102dbeb0: pops the first entry of C: the three words to the out pointers, then removes the id the first out pointer holds.
    for (const std::int32_t count : {0, 1, 2, 5, 250}) {
        auto memory = pool_entries(w, list_c, count);
        append(memory, w.counted(list_c, count));
        cases.push_back({.entry = pop_staff, .registers = {{3, w.regen}, {4, out_at}, {5, out_at + 4}, {6, out_at + 8}}, .memory = memory,
                         .label = "regen_lists: pop C count " + std::to_string(count)});
    }
    // An out pointer inside the entry itself (the stores change what the later reads see): the second into the first word,
    // the first into the entry's second word, all three the same word.
    {
        auto memory = pool_entries(w, list_c, 4);
        append(memory, w.counted(list_c, 4));
        const auto first = w.entry(list_c, 0);
        cases.push_back({.entry = pop_staff, .registers = {{3, w.regen}, {4, out_at}, {5, first}, {6, out_at + 8}}, .memory = memory, .label = "regen_lists: pop C, nation out pointer in the entry's id"});
        cases.push_back({.entry = pop_staff, .registers = {{3, w.regen}, {4, first + 4}, {5, out_at}, {6, out_at + 8}}, .memory = memory, .label = "regen_lists: pop C, id out pointer in the entry's second word"});
        cases.push_back({.entry = pop_staff, .registers = {{3, w.regen}, {4, out_at}, {5, out_at}, {6, out_at}}, .memory = memory, .label = "regen_lists: pop C, one out word"});
        cases.push_back({.entry = pop_staff, .registers = {{3, w.regen}, {4, first}, {5, first + 4}, {6, first + 8}}, .memory = memory, .label = "regen_lists: pop C, out pointers over the entry"});
    }
}

PLAYER_REGEN_CASES("regen_lists") {
    // 0x102dea50 (append to C, the growth tested on D) and 0x102dec50 (append to D).
    const World w(scenario);
    const auto kinds = people_kinds(scenario);
    const auto cap_c = std::int32_t(w.capacity(list_c)), cap_d = std::int32_t(w.capacity(list_d));
    struct Counts { const char* name; std::int32_t c, d; };
    const std::array<Counts, 8> counts{{{"as is", -1, -1}, {"empty", 0, 0}, {"C three D three", 3, 3}, {"D full", 3, cap_d - 1}, {"C full only", cap_c - 1, 3},
                                        {"both full", cap_c - 1, cap_d - 1}, {"C at capacity, D three", cap_c, 3}, {"D at capacity", 3, cap_d}}};
    for (const auto& count : counts) {
        std::vector<Write> memory;
        if (count.c >= 0) append(memory, w.counted(list_c, count.c));
        if (count.d >= 0) append(memory, w.counted(list_d, count.d));
        cases.push_back({.entry = append_staff, .registers = {{3, w.regen}, {4, 0xdead0001}, {5, 0xdead0002}, {6, 0xdead0003}}, .memory = memory,
                         .label = std::string("regen_lists: append to C, ") + count.name});
        if (!kinds.both.empty() || !kinds.player_only.empty()) {
            const auto n = !kinds.player_only.empty() ? kinds.player_only[kinds.player_only.size() / 3] : kinds.both[0];
            cases.push_back({.entry = append_player, .registers = {{3, w.regen}, {4, n}, {5, 0xbeef0001}, {6, 0xbeef0002}}, .memory = memory,
                             .label = std::string("regen_lists: append to D, ") + count.name});
        }
    }
    // Several real people, and the register values the original leaves (the entry's second and third words).
    for (std::size_t n = 0; n < std::min<std::size_t>(kinds.player_only.size(), 12); ++n)
        cases.push_back({.entry = append_player, .registers = {{3, w.regen}, {4, kinds.player_only[n * kinds.player_only.size() / 12]}, {5, std::uint32_t(n)}, {6, 0xffffffff}},
                         .label = "regen_lists: append to D, person " + std::to_string(n)});
    // The allocator fails: C grown by the appender of C (the test on D), D grown by its own.
    cases.push_back({.entry = append_staff, .registers = {{3, w.regen}, {4, 1}, {5, 2}, {6, 3}}, .memory = w.counted(list_d, cap_d - 1), .answers = {{reallocate, 0}},
                     .label = "regen_lists: append to C, growth fails", .original_owners = {reallocate}});
    if (!kinds.player_only.empty())
        cases.push_back({.entry = append_player, .registers = {{3, w.regen}, {4, kinds.player_only[0]}, {5, 2}, {6, 3}}, .memory = w.counted(list_d, cap_d - 1), .answers = {{reallocate, 0}},
                         .label = "regen_lists: append to D, growth fails", .original_owners = {reallocate}});
}

PLAYER_REGEN_CASES("regen_lists") {
    // 0x102dee70: append to A a copy of an entry (only some fields are copied), 0x102df020: remove by bsearch,
    // 0x102eb670: remove by the id.
    const World w(scenario);
    const auto date = scenario.toc(date_slot);
    const auto today_day = std::int32_t(std::int16_t(scenario.u16(date))), today_year = std::int32_t(scenario.u16(date + 2));
    const auto cap_a = std::int32_t(w.capacity(list_a));
    const Retire source{0x01020304, 0x05060708, 11, 2003, 22, 2004, 0x91, 0x92, 8, 0x93};
    for (const std::int32_t count : {-1, 0, 1, 5, cap_a - 1, cap_a}) {
        auto memory = retire_entry(key_at, source);
        // The destination row holds other bytes (the unused fields keep them).
        if (count >= 0) append(memory, w.counted(list_a, count));
        const auto row = w.entry(list_a, std::uint32_t(std::max(count, 0)));
        append(memory, {w32(row, 0x11111111), w32(row + 4, 0x22222222), w8(row + 0xc, 0x33), w8(row + 0x14, 0x44), w8(row + 0x19, 0x55), w16(row + 8, 0x6666)});
        cases.push_back({.entry = append_retire, .registers = {{3, w.regen}, {4, key_at}}, .memory = memory, .label = "regen_lists: append to A, count " + std::to_string(count)});
    }
    cases.push_back({.entry = append_retire, .registers = {{3, w.regen}, {4, key_at}}, .memory = [&] { auto m = retire_entry(key_at, source); append(m, w.counted(list_a, cap_a - 1)); return m; }(),
                     .answers = {{reallocate, 0}}, .label = "regen_lists: append to A, growth fails", .original_owners = {reallocate}});
    // A source in the list itself (the realloc moves the block under it).
    {
        auto memory = retire_entry(w.entry(list_a, 2), source);
        append(memory, w.counted(list_a, cap_a - 1));
        cases.push_back({.entry = append_retire, .registers = {{3, w.regen}, {4, w.entry(list_a, 2)}}, .memory = memory, .label = "regen_lists: append to A, the source inside the list that grows"});
    }
    // A sorted list A (by the comparator's due date and id), the key entries found, absent, and an unsorted list.
    struct Dated { Retire r; };
    const auto due = [&](const Retire& r) {
        // The entry's due date: its first unless that is before today, then its second (the comparator 0x102eb550).
        const bool passed = r.year < today_year || (r.year == today_year && r.day < today_day);
        return passed ? std::pair{r.year2, r.day2} : std::pair{r.year, r.day};
    };
    const auto build = [&](std::uint32_t seed, std::size_t count, bool mix) {
        std::vector<Retire> rows;
        for (std::size_t n = 0; n < count; ++n) {
            const auto k = std::int32_t(n);
            Retire r{std::uint32_t(5000 + 3 * n + seed), 0x100u + std::uint32_t(n), 10 + 3 * k, today_year + 1 + (mix && n % 3 == 0 ? 1 : 0), 20 + 5 * k, today_year + 2,
                     0x20 + std::uint32_t(n), 0x30 + std::uint32_t(n), 8, 1 + std::uint32_t(n) % 2};
            if (mix && n % 4 == 1) { r.year = today_year - 1; r.day = 100 - k; }                     // passed: due at the second date
            if (mix && n % 4 == 2) { r.year = today_year; r.day = today_day - 1 - k % 5; }            // passed by days
            if (mix && n % 4 == 3) { r.year = today_year; r.day = today_day + k % 5; }                // today or later
            rows.push_back(r);
        }
        std::ranges::sort(rows, [&](const Retire& a, const Retire& b) {
            const auto da = due(a), db = due(b);
            return da != db ? da < db : std::int32_t(a.id - b.id) < 0;
        });
        return rows;
    };
    for (const bool mix : {false, true})
        for (const std::size_t count : {std::size_t(0), std::size_t(1), std::size_t(2), std::size_t(7), std::size_t(60)}) {
            const auto rows = build(0, count, mix);
            std::vector<Write> base;
            for (std::size_t n = 0; n < rows.size(); ++n) append(base, retire_entry(w.entry(list_a, std::uint32_t(n)), rows[n]));
            append(base, w.counted(list_a, std::int32_t(count)));
            struct Pick { const char* name; std::int32_t index; };
            for (const Pick pick : {Pick{"first", 0}, Pick{"second", 1}, Pick{"middle", std::int32_t(count / 2)}, Pick{"last", std::int32_t(count) - 1}, Pick{"absent", -1}}) {
                if (pick.index >= std::int32_t(count) && pick.index >= 0) continue;
                // The key: a copy of the row in the scratch area, or (absent) a row that no entry has.
                Retire key = pick.index >= 0 ? rows[std::size_t(pick.index)] : Retire{7777, 1, today_year + 1, today_year + 1, 1, 1, 0, 0, 8, 1};
                if (pick.index < 0) { key.day = 9000; key.year = today_year + 5; }
                auto memory = base;
                append(memory, retire_entry(key_at, key));
                const auto label = std::string(mix ? "mixed dates" : "later dates") + " count " + std::to_string(count) + " key " + pick.name;
                cases.push_back({.entry = remove_retire, .registers = {{3, w.regen}, {4, key_at}}, .memory = memory, .label = "regen_lists: remove from A, " + label});
                // By the id (the linear scan of 0x102eb670: key = a record whose first word is the id).
                auto by_id = base;
                append(by_id, {w32(key_at, pick.index >= 0 ? rows[std::size_t(pick.index)].id : 7777u)});
                cases.push_back({.entry = remove_by_id, .registers = {{3, w.regen}, {4, key_at}}, .memory = by_id, .label = "regen_lists: remove from A by id, " + label});
            }
        }
    // A list the search can not find an entry in (not sorted by the comparator): the scan finds the id and returns 1 all the same.
    {
        auto rows = build(0, 8, false);
        std::ranges::reverse(rows);
        std::vector<Write> memory;
        for (std::size_t n = 0; n < rows.size(); ++n) append(memory, retire_entry(w.entry(list_a, std::uint32_t(n)), rows[n]));
        append(memory, w.counted(list_a, 8));
        append(memory, {w32(key_at, rows[6].id)});
        cases.push_back({.entry = remove_by_id, .registers = {{3, w.regen}, {4, key_at}}, .memory = memory, .label = "regen_lists: remove from A by id, unsorted list"});
        auto direct = memory;
        append(direct, retire_entry(key_at, rows[6]));
        cases.push_back({.entry = remove_retire, .registers = {{3, w.regen}, {4, key_at}}, .memory = direct, .label = "regen_lists: remove from A, unsorted list"});
    }
    // The key is a pointer into the list itself, a null key and a negative count.
    {
        const auto rows = build(0, 5, false);
        std::vector<Write> memory;
        for (std::size_t n = 0; n < rows.size(); ++n) append(memory, retire_entry(w.entry(list_a, std::uint32_t(n)), rows[n]));
        append(memory, w.counted(list_a, 5));
        cases.push_back({.entry = remove_retire, .registers = {{3, w.regen}, {4, w.entry(list_a, 3)}}, .memory = memory, .label = "regen_lists: remove from A, key inside the list"});
        cases.push_back({.entry = remove_by_id, .registers = {{3, w.regen}, {4, 0}}, .memory = memory, .label = "regen_lists: remove from A by id, null key"});
        auto negative = memory;
        append(negative, w.counted(list_a, -3));
        append(negative, {w32(key_at, rows[2].id)});
        cases.push_back({.entry = remove_by_id, .registers = {{3, w.regen}, {4, key_at}}, .memory = negative, .label = "regen_lists: remove from A by id, negative count"});
    }
}

PLAYER_REGEN_CASES("regen_lists") {
    // 0x102df6d0: pops the first entry of the queue B (every field written, the unused bytes too).
    const World w(scenario);
    for (const std::int32_t count : {0, 1, 2, 3, 8, 250}) {
        std::vector<Write> memory;
        for (std::int32_t n = 0; n < count && n < 250; ++n) append(memory, queue_entry(w.entry(list_b, std::uint32_t(n)), 700 + std::uint32_t(n) * 5, std::uint32_t(n)));
        append(memory, w.counted(list_b, count));
        cases.push_back({.entry = pop_queue, .registers = {{3, w.regen}}, .memory = memory, .label = "regen_lists: pop B count " + std::to_string(count)});
    }
}
}
