// LEAGUE.CPP block `loader`: cases for the save reader 0x104a6850, from the real career's league objects. The file layer
// is answered on both sides: fopen and the folder path with fixed handles, every fread with 1 (the requested count for
// the reads that ask for more, by nth answers), the allocator's new[] with scratch blocks a case pre-fills (the club
// words the reader converts), delete with 0, and the other modules' record loaders (COMP_STATS, the stage kinds) with
// 0. The frame's locals the file layer would fill (the flags, the kind, the index) are poked before each read.
// A script of the reads and allocations the original makes is built from the case's parameters, so a failure can be
// put at any checked read or allocation (the 51 assertion sites) by its number.
#include "league_base_checks.hpp"
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::league_base_checks {
namespace {
constexpr std::uint32_t loader = 0x104a6850, open_file = 0x10739de0, file_path = 0x10746280, read_file = 0x10775fe0,
                        close_file = 0x107766f0, new_block = 0x10775130, new_object = 0x10773520,
                        new_stats = 0x104533a0, read_stats = 0x1045f6a0, new_list = 0x104bc750, read_list = 0x104bcb20,
                        new_group = 0x1056e120, read_group = 0x1056e430;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60;
constexpr std::uint32_t none = 0xffffffffu, handle = scratch_base + 0x80, path = scratch_base + 0xa0,
                        blocks = scratch_base + 0x10000, block_size = 0x400, base_vtable = 0x108d796c;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}

// The career's league objects whose virtual +0x28 is the base class's (the reader calls it; a derived league reads
// its own records there, which the case would have to script too).
std::vector<std::uint32_t> base_leagues(const Scenario& s) {
    std::vector<std::uint32_t> leagues;
    std::set<std::uint32_t> seen;
    const auto table = s.global(competitions_slot);
    const auto count = s.global(competition_count_slot);
    const auto base_virtual = s.r32(base_vtable + 0x28);
    const auto add = [&](std::uint32_t object) {
        if (!object || !seen.insert(object).second) return;
        const auto vtable = s.r32(object);
        if (vtable < 0x108b0000 || vtable >= 0x10a00000 || (s.r32(object + 0x40) & 0xff) != 2) return;
        if (s.r32(vtable + 0x28) != base_virtual) return;
        const auto descriptor = s.r32(vtable + 0x88);
        if (descriptor < 0x108b0000 || descriptor >= 0x108c0000) return;
        const auto code = s.r32(descriptor);
        if (code >= module_start && code < module_end) leagues.push_back(object);
    };
    for (std::uint32_t id = 0; table && id < count && id < 4096; ++id) {
        const auto object = s.r32(table + id * 4);
        if (!object) continue;
        add(object);
        const auto last = std::int32_t(s.r32(object + 0x30));
        const auto stages = s.r32(object + 0xc);
        for (std::int32_t n = 0; stages && n <= last && n < 64; ++n) add(s.r32(stages + std::uint32_t(n) * 4));
    }
    return leagues;
}

// What the case sets: the league's fields the reader reads back, the locals, and the records' contents.
struct Plan {
    std::uint32_t league = 0;
    std::uint32_t backup = 1;
    // Frame locals (the file layer would write them).
    std::uint32_t stats_flag = 0, record_index = none, row_flag = 0, table_flag = 0;
    std::uint32_t stage_kind = 0, list_first = 3, list_second = 2, group_first = 4, group_second = 5;
    // Fields of the league (the reader reads them after the file's values would have replaced them).
    std::int32_t schedule = 1, extra_count = 0, clubs = 0, rounds = 3, table_size = -1, stage_count = 0, last_stage = -1,
                 round_count = 0, round_capacity = 0;
    bool extra_pointer = false, stage_array = false, old_rounds = false;
    std::uint32_t old_block = 0;  // a real new[] block the reader frees (another league's schedule)
    std::uint32_t club_id = 2, other_id = 3, round_id = 4;  // the words the converted tables hold (none: -1)
    bool none_ids = false;
    // Failures: the nth checked event to fail (1-based among reads or allocations; 0: none), or the open.
    unsigned fail_read = 0, fail_alloc = 0;
    unsigned null_new = 0;  // the nth `new` (statistics object, then each stage's) that returns null, 0: none
    bool open_fails = false;
    bool premier = false;
};
struct Script {
    struct Read { std::uint32_t count; int site; };
    std::vector<Read> reads;
    std::vector<int> allocs;                  // the site of each new[] (-1: unchecked)
    std::vector<std::uint32_t> alloc_address; // the scratch block each returns
};
Script script(const Plan& p) {
    Script s;
    const auto read = [&](std::uint32_t count, int site) { s.reads.push_back({count, site}); };
    const auto alloc = [&](int site) { s.allocs.push_back(site); s.alloc_address.push_back(blocks + block_size * std::uint32_t(s.allocs.size())); };
    read(1, 0);
    read(1, -1);
    for (int site = 1; site <= 25; ++site) read(1, site);
    for (int site = 26; site <= 37; ++site) read(1, site);
    alloc(38);
    read(std::uint32_t(p.schedule), 39);
    read(1, -1);
    read(1, -1);
    if (p.extra_pointer && p.extra_count != 0) { alloc(-1); read(std::uint32_t(p.extra_count), -1); }
    if (p.clubs != 0) {
        alloc(40);
        read(std::uint32_t(p.clubs), 41);
        read(1, 42);
        for (int row = 0; row < p.clubs && p.row_flag; ++row) { alloc(43); read(std::uint32_t(p.clubs - 1), 44); }
        read(1, 45);
        for (int row = 0; row < p.clubs && p.table_flag; ++row) { alloc(46); read(1, 47); }
    }
    read(1, -1);
    read(1, -1);
    read(1, -1);
    if (!p.stage_array && p.stage_count > 0) alloc(48);
    if (p.stage_array || p.stage_count > 0)
        for (int stage = 0; stage <= p.last_stage; ++stage) {
            read(1, -1);
            if (p.stage_kind == 1) {
                read(1, -1);
                read(1, -1);
                alloc(49);
                for (std::uint32_t n = 0; n < p.list_second; ++n) read(1, -1);
            } else if (p.stage_kind == 2) { read(1, -1); read(1, -1); }
        }
    read(1, -1);
    read(1, -1);
    if (p.round_count != 0) { alloc(50); read(std::uint32_t(p.round_capacity), -1); }
    return s;
}

Case make(const Scenario& scenario, const Plan& p, const std::string& label) {
    Case item;
    item.entry = loader;
    item.registers = {{3, p.league}, {4, p.backup}};
    item.label = "loader: " + label;
    const auto base = scenario.sp() - 0x880;  // the reader's frame
    const auto field = [&](unsigned size, std::uint32_t offset, std::uint32_t value) { item.memory.push_back({size, p.league + offset, value}); };
    field(2, 0xa6, std::uint32_t(p.schedule));
    field(2, 0x34, std::uint32_t(p.extra_count));
    field(4, 0x18, p.extra_pointer ? 0x1234 : 0);
    field(2, 0x3e, std::uint32_t(p.clubs));
    field(2, 0x3c, std::uint32_t(p.rounds));
    field(2, 0xa4, std::uint32_t(p.table_size));
    field(4, 0xc, p.stage_array ? scratch_base + 0x3000 : 0);
    field(4, 0x2c, std::uint32_t(p.stage_count));
    field(4, 0x30, std::uint32_t(p.last_stage));
    field(4, 0x14, p.old_rounds ? p.old_block : 0);
    field(2, 0x36, std::uint32_t(p.round_count));
    field(2, 0x38, std::uint32_t(p.round_capacity));
    if (p.stage_array) item.memory.push_back({4, scratch_base + 0x3000, 0});
    const auto script_ = script(p);
    // The records' contents: club words in the standings, the other-club arrays, the table blocks and the round rows.
    // A reader that finds a pointer table in the scratch blocks converts them as the file's indices.
    const auto block = [&](std::size_t index) { return script_.alloc_address.at(index); };
    {
        std::size_t index = 1;  // allocation 0 is the schedule
        if (p.extra_pointer && p.extra_count != 0) ++index;
        if (p.clubs != 0) {
            const auto standings = block(index++);
            for (int row = 0; row < p.clubs; ++row)
                item.memory.push_back({4, standings + 0x44u * std::uint32_t(row), p.none_ids && (row & 1) ? none : p.club_id + std::uint32_t(row)});
            if (p.row_flag)
                for (int row = 0; row < p.clubs; ++row) {
                    const auto others = block(index++);
                    for (int j = 0; j < p.clubs - 1; ++j)
                        item.memory.push_back({4, others + 0x10u * std::uint32_t(j), p.none_ids && (j & 1) ? none : p.other_id + std::uint32_t(j)});
                }
            if (p.table_flag) index += std::size_t(p.clubs);
        }
        if (!p.stage_array && p.stage_count > 0) ++index;
        if (p.round_count != 0) {
            std::size_t last = script_.allocs.size() - 1;
            const auto rows = block(last);
            for (int row = 0; row < p.round_count; ++row)
                item.memory.push_back({4, rows + 8u * std::uint32_t(row), p.none_ids && (row & 1) ? none : p.round_id + std::uint32_t(row)});
        }
    }
    // Answers: the open and path, the counts of the reads that ask for more than one, the allocations, the rest.
    item.answers = {{open_file, p.open_fails ? 0u : handle}, {file_path, path}, {read_file, 1}, {close_file, 0},
                    {new_object, scratch_base + 0x5000}, {new_stats, 0}, {read_stats, 0}, {new_list, 0}, {read_list, 0},
                    {new_group, 0}, {read_group, 0}};
    unsigned counted_checked = 0;
    for (std::size_t n = 0; n < script_.reads.size(); ++n) {
        const auto& event = script_.reads[n];
        const bool checked = event.site >= 0;
        if (checked) ++counted_checked;
        const bool failing = checked && p.fail_read == counted_checked;
        if (failing) item.nth_answers.push_back({read_file, unsigned(n + 1), event.count == 0 ? 7u : 0u});
        else if (event.count != 1) item.nth_answers.push_back({read_file, unsigned(n + 1), event.count});
        if (failing) break;
    }
    if (p.null_new) item.nth_answers.push_back({new_object, p.null_new, 0});
    unsigned counted_allocs = 0;
    for (std::size_t n = 0; n < script_.allocs.size(); ++n) {
        const bool checked = script_.allocs[n] >= 0;
        if (checked) ++counted_allocs;
        const bool failing = checked && p.fail_alloc == counted_allocs;
        // The list values are freed by delete[], which the generated original runs in place (try_modern_allocator): a
        // real heap block, so the real delete[] works the same on both sides.
        if (failing || script_.allocs[n] != 49) item.nth_answers.push_back({new_block, unsigned(n + 1), failing ? 0u : script_.alloc_address[n]});
    }
    // The locals before each read.
    item.pokes = {{read_file, {{4, base + 0x4c, p.stats_flag}, {4, base + 0x50, p.record_index}, {4, base + 0x48, p.row_flag},
                               {4, base + 0x44, p.table_flag}, {1, base + 0x38, p.stage_kind}, {2, base + 0x3a, p.group_second},
                               {2, base + 0x3c, p.group_first}, {2, base + 0x3e, p.list_second}, {2, base + 0x40, p.list_first}}}};
    return item;
}
}

LEAGUE_CASES("loader") {
    if (scenario.match) return;
    const auto leagues = base_leagues(scenario);
    if (leagues.size() < 2) throw std::runtime_error("loader: the career has too few leagues with the base +0x28");
    std::fprintf(stderr, "loader: %zu base leagues\n", leagues.size());
    // The generated original runs fread's C++ owner in place (try_modern_stream_io), so the checker's dispatch answers
    // would reach the rewrite only. Entry 4 (0x10775fe0, stream_io_entries) is switched to its original body, which is
    // dispatched like any call: both sides are then answered alike. Only fread is touched (the writer's fwrite is entry 6).
    scenario.s.stream_io_entry_original[4] = true;
    const auto league = leagues[0];
    std::uint32_t real_block = 0;
    for (const auto other : leagues)
        if (other != leagues[0] && scenario.r32(other + 0xbc) != 0) { real_block = scenario.r32(other + 0xbc); break; }
    if (!real_block) throw std::runtime_error("loader: no league has a schedule block to free");
    const auto add = [&](Plan plan, const std::string& label) { plan.old_block = real_block; cases.push_back(make(scenario, plan, label)); };
    // The file does not open: each name form.
    for (const std::uint32_t backup : {0u, 1u, 0x100u, 0xffu}) {
        Plan p;
        p.league = league;
        p.backup = backup;
        p.open_fails = true;
        add(p, "open fails, backup " + hex(backup));
    }
    // Success paths: the smallest record, then each structure in turn.
    Plan base;
    base.league = league;
    add(base, "minimal record");
    for (const std::uint32_t backup : {0u, 0x100u, 0xffu}) {
        Plan p = base;
        p.backup = backup;
        add(p, "minimal record, backup " + hex(backup));
    }
    for (std::size_t n = 1; n < leagues.size() && n < 5; ++n) {
        Plan p = base;
        p.league = leagues[n];
        p.record_index = 0;
        add(p, "minimal record on " + hex(leagues[n]) + ", record 0");
    }
    for (const std::uint32_t index : {0u, 5u, none, 0x7fffffffu}) {
        Plan p = base;
        p.record_index = index;
        add(p, "record index " + hex(index));
    }
    {
        Plan p = base;
        p.stats_flag = 1;
        add(p, "statistics object");
        p.stats_flag = 0x100;
        add(p, "statistics flag 0x100");
    }
    for (const int schedule : {1, 2, 3, 0, -1}) {
        Plan p = base;
        p.schedule = schedule;
        add(p, "schedule " + std::to_string(schedule));
    }
    for (const int extra : {0, 1, 2, -1}) {
        for (const bool pointer : {false, true}) {
            Plan p = base;
            p.extra_pointer = pointer;
            p.extra_count = extra;
            add(p, std::string("extra rows ") + std::to_string(extra) + (pointer ? " with pointer" : " no pointer"));
        }
    }
    for (const int clubs : {1, 2, 3, -1}) {
        for (const std::uint32_t rows : {0u, 1u}) {
            for (const std::uint32_t tables : {0u, 1u}) {
                Plan p = base;
                p.clubs = clubs;
                p.row_flag = rows;
                p.table_flag = tables;
                p.table_size = tables ? 2 : -1;
                add(p, "clubs " + std::to_string(clubs) + " rows " + std::to_string(rows) + " tables " + std::to_string(tables));
                p.none_ids = true;
                if (clubs > 1) add(p, "clubs " + std::to_string(clubs) + " rows " + std::to_string(rows) + " tables " + std::to_string(tables) + " with none ids");
            }
        }
    }
    for (const std::int32_t size : {-1, 0, 4, 2}) {
        for (const int clubs : {1, 3}) {
            Plan p = base;
            p.clubs = clubs;
            p.table_flag = 1;
            p.table_size = size;
            add(p, "table size " + std::to_string(size) + " clubs " + std::to_string(clubs));
        }
    }
    {
        Plan p = base;
        p.clubs = 2;
        p.table_flag = 1;
        p.table_size = -1;
        p.rounds = 5;
        add(p, "table size -1 adjusted with rounds 5");
        p.rounds = -2;
        add(p, "table size -1 adjusted with rounds -2");
    }
    for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 0x101u, 0xffu}) {
        for (const bool array : {false, true}) {
            Plan p = base;
            p.stage_kind = kind;
            p.stage_array = array;
            p.stage_count = 3;
            p.last_stage = 1;
            add(p, "stages kind " + hex(kind) + (array ? " existing array" : " new array"));
        }
    }
    {
        Plan p = base;
        p.stage_kind = 1;
        p.stage_count = 3;
        p.last_stage = 0;
        for (const std::uint32_t second : {0u, 1u, 3u, 0xffffu}) {
            p.list_second = second;
            add(p, "stage list count " + hex(second));
        }
        p.stage_kind = 2;
        add(p, "stage group");
        p.stage_count = 0;
        p.stage_array = false;
        p.last_stage = 2;
        add(p, "no stage array, no count");
        p.stage_count = -3;
        add(p, "negative stage count");
        p.stage_count = 3;
        p.last_stage = -1;
        add(p, "last stage -1");
    }
    // `new` returning null (the reader skips the constructor, stores the null and reads on): the statistics object and each
    // stage kind.
    for (const unsigned nth : {1u, 2u}) {
        Plan p = base;
        p.stats_flag = 1;
        p.null_new = nth;
        add(p, "statistics new null, nth " + std::to_string(nth));
        for (const std::uint32_t kind : {1u, 2u}) {
            Plan q = base;
            q.stage_kind = kind;
            q.stage_count = 3;
            q.last_stage = 1;
            q.null_new = nth;
            add(q, "stage kind " + std::to_string(kind) + " new null, nth " + std::to_string(nth));
            q.stats_flag = 1;
            q.null_new = nth + 1;
            add(q, "statistics and stage kind " + std::to_string(kind) + " new null, nth " + std::to_string(nth + 1));
        }
    }
    for (const int count : {1, 2, 0, -1}) {
        for (const bool old : {false, true}) {
            Plan p = base;
            p.round_count = count;
            p.round_capacity = count > 0 ? count : 1;
            p.old_rounds = old;
            add(p, "round rows " + std::to_string(count) + (old ? " with old block" : ""));
            if (count > 1) {
                p.none_ids = true;
                add(p, "round rows " + std::to_string(count) + " with none ids");
            }
        }
    }
    {
        Plan p = base;
        p.round_count = 2;
        p.round_capacity = 5;
        add(p, "round capacity 5 of 2");
        p.round_capacity = -1;
        add(p, "round capacity -1");
    }
    // Everything together, and the failure of each checked read and allocation of that script.
    Plan full = base;
    full.schedule = 2;
    full.extra_pointer = true;
    full.extra_count = 2;
    full.clubs = 2;
    full.row_flag = 1;
    full.table_flag = 1;
    full.table_size = -1;
    full.stage_kind = 1;
    full.stage_count = 3;
    full.last_stage = 1;
    full.round_count = 2;
    full.round_capacity = 2;
    full.old_rounds = true;
    full.record_index = 3;
    add(full, "everything");
    full.none_ids = true;
    add(full, "everything with none ids");
    full.none_ids = false;
    for (const bool second : {false, true}) {
        Plan p = full;
        p.stage_kind = second ? 2 : 1;
        p.backup = second ? 0 : 1;
        add(p, std::string("everything, ") + (second ? "group stages, backup" : "list stages"));
    }
    unsigned checked_reads = 0, checked_allocs = 0;
    for (const auto& event : script(full).reads) checked_reads += event.site >= 0;
    for (const auto site : script(full).allocs) checked_allocs += site >= 0;
    for (unsigned n = 1; n <= checked_reads; ++n) {
        Plan p = full;
        p.fail_read = n;
        add(p, "short read " + std::to_string(n));
    }
    for (unsigned n = 1; n <= checked_allocs; ++n) {
        Plan p = full;
        p.fail_alloc = n;
        add(p, "allocation failure " + std::to_string(n));
    }
    {
        // Failures on the other shapes: the standings read with one club; the arrays with one club (count 0).
        Plan p = full;
        p.clubs = 1;
        p.fail_read = 40;  // the standings' read (count 1: answered 0)
        add(p, "short standings read, one club");
        p = full;
        p.clubs = 1;
        add(p, "everything with one club (row arrays of count 0)");
    }
    // The English named-reserves rule: a Premier League object with the rule on (the checker runs with the original
    // rules; a run without CM_ENGLISH_RULES_ORIGINAL shows the effect).
    const auto premier_vtable = scenario.toc(-0x6020);
    for (const auto object : leagues)
        if (scenario.r32(object) == premier_vtable) {
            Plan p = base;
            p.league = object;
            add(p, "premier league " + hex(object));
            p.record_index = 0;
            add(p, "premier league " + hex(object) + ", record 0");
            break;
        }
}
}
