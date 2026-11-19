// CUP.CPP block `storage`: cases for the cup's save reader 0x104831a0 and writer 0x10484420, from the real career's
// cup objects (--world). The file layer is answered on both sides: fopen and the folder path with fixed scratch
// addresses, every fread and fwrite with its requested count (a nth answer for a failure), the allocator's new[]
// with scratch blocks a case pre-fills (the words the reader converts), `new` with a scratch object, the other
// modules' loaders and writers (COMP_STATS, the stage kinds, the derived cup's own records at vt +0x28/+0x2c) with
// 0. fread, fwrite and new[] run in place in the generated original, so a case lists them in `original_owners`
// (never the stream or allocator switches of the shared scenario state): they reach the dispatcher on both sides
// and the answers apply alike. The frame's locals the file layer would fill (the flag, the kind, the index) are
// poked before each read. A script of the reads and allocations the original makes is built from the case's
// parameters, so a failure can be put at any checked read or allocation (the 26 assertion sites) by its number.
// The sites that call `exit` end the run on both sides, and the state at the stop is compared. Labels start with
// "storage:" (CM_CUP_BLOCK=storage).
#include "cup_base_checks.hpp"
#include <algorithm>
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::cup_base_checks {
namespace {
constexpr std::uint32_t loader = 0x104831a0, writer = 0x10484420, open_file = 0x10739de0, file_path = 0x10746280,
                        read_file = 0x10775fe0, write_file = 0x10776360, close_file = 0x107766f0, new_block = 0x10775130,
                        new_object = 0x10773520, stats_destroy = 0x104536c0, stats_construct = 0x104533a0,
                        stats_read = 0x1045f6a0, stats_write = 0x10460230, list_construct = 0x104bc750,
                        list_read = 0x104bcb20, list_write = 0x104bd550, group_construct = 0x1056e120,
                        group_read = 0x1056e430, group_write = 0x1056f6f0;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60;
constexpr std::uint32_t none = 0xffffffffu, handle = scratch_base + 0x80, path = scratch_base + 0xa0,
                        object_copy_at = scratch_base + 0x200, record_copy_at = scratch_base + 0x400,
                        stage_list = scratch_base + 0x3000, rows_block = scratch_base + 0x4000,
                        blocks = scratch_base + 0x10000, block_size = 0x400, objects = scratch_base + 0x8000, object_size = 0xb4;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
Write w8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write w16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

struct World {
    const Scenario& s;
    std::vector<std::uint32_t> cups, leagues, others;
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto table = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        std::set<std::uint32_t> seen;
        const auto add = [&](std::uint32_t object) {
            if (!object || !seen.insert(object).second) return;
            const auto vtable = s.r32(object);
            if (vtable < 0x108b0000 || vtable >= 0x10a00000) return;
            const auto kind = u8(object + 0x43);
            if (kind == 1 && is_cup(object)) cups.push_back(object);
            else if (kind == 2) leagues.push_back(object);
            else if (kind != 1) others.push_back(object);
        };
        for (std::uint32_t id = 0; table && id < count && id < 4096; ++id) {
            const auto object = s.r32(table + id * 4);
            if (!object) continue;
            add(object);
            const auto last = std::int32_t(s.r32(object + 0x30));
            const auto stages = s.r32(object + 0xc);
            for (std::int32_t n = 0; stages && n <= last && n < 64; ++n) add(s.r32(stages + std::uint32_t(n) * 4));
        }
    }
    std::uint32_t u8(std::uint32_t at) const { return (s.r32(at & ~3u) >> (24 - 8 * (at & 3))) & 0xff; }
    std::int32_t i16(std::uint32_t at) const { return std::int16_t(u8(at) << 8 | u8(at + 1)); }
    // A cup: its vtable has a CUP.CPP body among the base class's slots (+0x08..+0xa4).
    bool is_cup(std::uint32_t object) const {
        for (std::uint32_t slot = 0x08; slot <= 0xa4; slot += 4) {
            const auto descriptor = s.r32(s.r32(object) + slot);
            if (descriptor < 0x108b0000 || descriptor >= 0x108c0000) return false;
            const auto code = s.r32(descriptor);
            if (code >= module_start && code < module_end) return true;
        }
        return false;
    }
    std::uint32_t club_table() const { return s.global(-0x7be8); }
    std::uint32_t club(unsigned index) const { return club_table() + index * 0x248; }
    // The code a virtual slot of the object reaches (what the generated glue dispatches to).
    std::uint32_t target(std::uint32_t object, std::uint32_t slot) const { return s.r32(s.r32(s.r32(object) + slot)); }
};

// ---------------------------------------------------------------------------------------------------------------
// The reader.

// What the case sets: the cup's fields the reader reads back (the file would have replaced them), the locals, and
// the records' contents.
struct Plan {
    std::uint32_t cup = 0;      // the real cup the scratch copy is made of (vtable, record, name)
    std::uint32_t backup = 1;
    // Frame locals (the file layer would write them).
    std::uint32_t stats_flag = 0, record_index = none, stage_kind = 0, list_first = 3, list_second = 2, group_first = 4, group_second = 5;
    // Fields of the object.
    std::int32_t half_34 = 0, rounds = 2, clubs = 2, word_2c = 0, last_stage = -1, rows = 0, capacity = 0, ties = 2;
    bool block_18 = false, stages_array = false, old_stats = false, none_ids = false, fake_record = false;
    std::uint32_t year = 2001;
    std::string record_name;     // a fake record's name (a '/' in it)
    // Failures: the nth checked event to fail (1-based among reads or allocations; 0: none), or the open.
    unsigned fail_read = 0, fail_alloc = 0, null_new = 0;
    bool open_fails = false;
    std::optional<std::uint32_t> a9_answer;     // the answer for the +0xa9 read (the original fails on 0 only)
    std::optional<std::uint32_t> field_answer;  // an answer for the nth checked read other than 0 (a count that is not 1)
};
struct Script {
    struct Read { std::uint32_t count; int site; };
    std::vector<Read> reads;
    std::vector<int> allocs;                  // the site of each new[] (-1: unchecked)
    std::vector<std::uint32_t> alloc_address;
    std::vector<std::size_t> reads_before;    // how many reads came before each new[]
    std::vector<std::size_t> objects_after;   // how many reads came before each `new` (the statistics object, the stages)
};
Script script(const Plan& p) {
    Script s;
    const auto read = [&](std::uint32_t count, int site) { s.reads.push_back({count, site}); };
    const auto alloc = [&](int site) {
        s.allocs.push_back(site);
        s.alloc_address.push_back(blocks + block_size * std::uint32_t(s.allocs.size()));
        s.reads_before.push_back(s.reads.size());
    };
    read(1, 0);
    read(1, -1);
    if (p.stats_flag != 0) s.objects_after.push_back(s.reads.size());
    // The 19 fixed fields: +0x49 is site 1, the fields after the sixth sites 2..14.
    for (unsigned i = 0; i < 19; ++i) read(1, i == 4 ? 1 : i >= 6 ? int(i) - 4 : -1);
    read(1, -1);
    read(1, -1);
    if (p.block_18 && p.half_34 != 0) { alloc(15); read(std::uint32_t(p.half_34), -1); }
    read(1, 16);
    alloc(17);
    read(std::uint32_t(p.rounds), 18);
    alloc(19);
    read(std::uint32_t(p.clubs), 20);
    alloc(-1);
    for (int r = 0; r < p.rounds; ++r) alloc(21);
    for (int r = 0; r < p.rounds; ++r) read(1, 22);
    read(1, -1);
    read(1, -1);
    read(1, -1);
    if (!p.stages_array && p.word_2c > 0) alloc(23);
    if (p.stages_array || p.word_2c > 0)
        for (int stage = 0; stage <= p.last_stage; ++stage) {
            read(1, -1);
            if (p.stage_kind == 1) {
                read(1, -1);
                read(1, -1);
                alloc(24);
                for (std::int32_t n = 0; n < std::int16_t(p.list_second); ++n) read(1, -1);
                s.objects_after.push_back(s.reads.size());
            } else if (p.stage_kind == 2) { read(1, -1); read(1, -1); s.objects_after.push_back(s.reads.size()); }
        }
    read(1, -1);
    read(1, -1);
    if (p.rows != 0) { alloc(25); read(std::uint32_t(p.capacity), -1); }
    return s;
}

Case make_load(const Scenario& scenario, const World& w, const Plan& p, const std::string& label) {
    Case item;
    item.entry = loader;
    item.registers = {{3, object_copy_at}, {4, p.backup}};
    item.label = "storage: load " + label;
    item.original_owners = {read_file, new_block};
    const auto base = scenario.sp() - 0x4a0;  // the reader's frame
    // The scratch copy of a real cup: its vtable, record, name buffer; then the fields the file would have set.
    for (std::uint32_t offset = 0; offset < object_size; offset += 4) item.memory.push_back(w32(object_copy_at + offset, scenario.r32(p.cup + offset)));
    if (p.fake_record) {
        const auto real = scenario.r32(p.cup + 4);
        for (std::uint32_t offset = 0; offset < 0x6c; offset += 4) item.memory.push_back(w32(record_copy_at + offset, scenario.r32(real + offset)));
        for (std::size_t n = 0; n <= p.record_name.size() && n < 0x30; ++n) item.memory.push_back(w8(record_copy_at + 0x1a + std::uint32_t(n), n < p.record_name.size() ? std::uint8_t(p.record_name[n]) : 0));
        item.memory.push_back(w32(object_copy_at + 4, record_copy_at));
    }
    const auto field = [&](unsigned size, std::uint32_t offset, std::uint32_t value) { item.memory.push_back({size, object_copy_at + offset, value}); };
    field(2, 0x40, p.year);
    field(2, 0x34, std::uint32_t(p.half_34));
    field(4, 0x18, p.block_18 ? 0x1234 : 0);
    field(2, 0x3c, std::uint32_t(p.rounds));
    field(2, 0x3e, std::uint32_t(p.clubs));
    field(4, 0xc, p.stages_array ? scratch_base + 0x3000 : 0);
    field(4, 0x2c, std::uint32_t(p.word_2c));
    field(4, 0x30, std::uint32_t(p.last_stage));
    field(2, 0x36, std::uint32_t(p.rows));
    field(2, 0x38, std::uint32_t(p.capacity));
    field(4, 0x8, p.old_stats ? 0x1234 : 0);
    if (p.stages_array) item.memory.push_back(w32(scratch_base + 0x3000, 0));
    const auto script_ = script(p);
    const auto block = [&](std::size_t index) { return script_.alloc_address.at(index); };
    {
        std::size_t index = 0;
        if (p.block_18 && p.half_34 != 0) ++index;
        const auto rounds_block = block(index++);
        for (int row = 0; row < p.rounds; ++row) item.memory.push_back(w16(rounds_block + 0x78u * std::uint32_t(row) + 0x1c, std::uint32_t(p.ties)));
        const auto entrants_block = block(index++);
        for (int row = 0; row < p.clubs; ++row)
            item.memory.push_back(w32(entrants_block + 8u * std::uint32_t(row), p.none_ids && (row & 1) ? none : 2 + std::uint32_t(row)));
        if (p.rows != 0) {
            const auto rows = block(script_.allocs.size() - 1);
            for (int row = 0; row < p.rows; ++row)
                item.memory.push_back(w32(rows + 8u * std::uint32_t(row), p.none_ids && (row & 1) ? none : 4 + std::uint32_t(row)));
        }
    }
    // The other modules' routines: the open and the path, the stage loaders, the derived cup's records (+0x28).
    item.answers = {{open_file, p.open_fails ? 0u : handle}, {file_path, path}, {read_file, 1}, {close_file, 0},
                    {stats_destroy, 0}, {stats_construct, 0}, {stats_read, 0},
                    {list_construct, 0}, {list_read, 0}, {group_construct, 0}, {group_read, 0},
                    {w.target(p.cup, 0x28), 0}};
    // The answers stop at the failure: what runs after it (the report's formatting allocates and frees on its own)
    // must meet the real routines. A failing read at event `stop_read`, a failing allocation at `stop_alloc`.
    std::size_t stop_read = script_.reads.size(), stop_alloc = script_.allocs.size();
    {
        unsigned counted = 0;
        for (std::size_t n = 0; n < script_.reads.size(); ++n)
            if (script_.reads[n].site >= 0 && p.fail_read == ++counted) { stop_read = n; break; }
        counted = 0;
        for (std::size_t n = 0; n < script_.allocs.size(); ++n)
            if (script_.allocs[n] >= 0 && p.fail_alloc == ++counted) { stop_alloc = n; break; }
    }
    // A list stage of no values asks the real allocator for 0 bytes (or a wrapped size): it answers null, the
    // reader reports and exits, and what follows meets the real routines too.
    bool real_failure = false;
    if (stop_alloc == script_.allocs.size() && p.stage_kind == 1 && std::int16_t(p.list_second) <= 0)
        for (std::size_t n = 0; n < script_.allocs.size(); ++n)
            if (script_.allocs[n] == 24) { stop_alloc = n; real_failure = true; break; }
    unsigned counted_checked = 0;
    for (std::size_t n = 0; n < script_.reads.size() && n <= stop_read; ++n) {
        const auto& event = script_.reads[n];
        if (stop_alloc < script_.allocs.size() && n >= script_.reads_before[stop_alloc]) break;
        const bool checked = event.site >= 0;
        if (checked) ++counted_checked;
        const bool failing = checked && p.fail_read == counted_checked;
        if (failing) item.nth_answers.push_back({read_file, unsigned(n + 1), event.count == 0 ? 7u : 0u});
        else if (event.site == 10 && p.a9_answer) item.nth_answers.push_back({read_file, unsigned(n + 1), *p.a9_answer});
        else if (event.count != 1) item.nth_answers.push_back({read_file, unsigned(n + 1), event.count});
    }
    // `new` is answered too: its body takes the same allocator path as new[], which the case numbers by its own
    // events. Each object gets a scratch block of its own; a null one for the case's `null_new`.
    for (std::size_t n = 0; n < script_.objects_after.size() && script_.objects_after[n] <= stop_read; ++n) {
        if (stop_alloc < script_.allocs.size() && script_.objects_after[n] >= script_.reads_before[stop_alloc]) break;
        item.nth_answers.push_back({new_object, unsigned(n + 1), p.null_new == n + 1 ? 0u : objects + 0x400u * std::uint32_t(n)});
    }
    for (std::size_t n = 0; n < script_.allocs.size() && n <= stop_alloc; ++n) {
        if (script_.reads_before[n] > stop_read) break;
        const bool failing = n == stop_alloc;
        // The list values (site 24) are freed by delete[]: a real heap block, so the real delete[] works alike.
        if (failing && real_failure) break;
        if (failing || script_.allocs[n] != 24) item.nth_answers.push_back({new_block, unsigned(n + 1), failing ? 0u : script_.alloc_address[n]});
    }
    // The locals before each read.
    item.pokes = {{read_file, {{4, base + 0x44, p.stats_flag}, {4, base + 0x48, p.record_index}, {1, base + 0x38, p.stage_kind},
                               {2, base + 0x3a, p.group_second}, {2, base + 0x3c, p.group_first}, {2, base + 0x3e, p.list_second},
                               {2, base + 0x40, p.list_first}}}};
    return item;
}

void load_cases(const Scenario& scenario, const World& w, std::vector<Case>& cases) {
    const auto add = [&](const Plan& plan, const std::string& label) { cases.push_back(make_load(scenario, w, plan, label)); };
    Plan base;
    base.cup = w.cups.front();
    // The file does not open: each name form.
    for (const std::uint32_t backup : {0u, 1u, 0x100u, 0xffu}) {
        Plan p = base;
        p.backup = backup;
        p.open_fails = true;
        add(p, "open fails, backup " + hex(backup));
    }
    // Success: the smallest record, the name forms, other cups' objects.
    add(base, "minimal record");
    for (const std::uint32_t backup : {0u, 0x100u, 0xffu, 0x80u}) {
        Plan p = base;
        p.backup = backup;
        add(p, "minimal record, backup " + hex(backup));
    }
    for (const std::uint32_t year : {0u, 1999u, 0x8000u, 0xffffu}) {
        Plan p = base;
        p.backup = 0;
        p.year = year;
        add(p, "year " + hex(year));
    }
    for (std::size_t n = 1; n < w.cups.size() && n < 5; ++n) {
        Plan p = base;
        p.cup = w.cups[n];
        add(p, "minimal record on " + hex(w.cups[n]));
    }
    for (const char* name : {"A/B", "/", "///x", "No slash", ""}) {
        for (const std::uint32_t backup : {0u, 1u}) {
            Plan p = base;
            p.fake_record = true;
            p.record_name = name;
            p.backup = backup;
            add(p, std::string("name \"") + name + "\" backup " + std::to_string(backup));
        }
    }
    for (const std::uint32_t index : {0u, 5u, none, 0x7fffffffu, 0x80000000u}) {
        Plan p = base;
        p.record_index = index;
        add(p, "record index " + hex(index));
    }
    // The statistics object: flag, the old object, `new` returning null.
    for (const std::uint32_t flag : {0u, 1u, 0x100u, 0xffu})
        for (const bool old : {false, true}) {
            Plan p = base;
            p.stats_flag = flag;
            p.old_stats = old;
            add(p, "statistics flag " + hex(flag) + (old ? " with old object" : " without old object"));
        }
    {
        Plan p = base;
        p.stats_flag = 1;
        p.null_new = 1;
        add(p, "statistics new null");
        p.old_stats = true;
        add(p, "statistics new null, old object");
    }
    // The +0xa9 byte fails on a count of 0 only; the other checked bytes on anything but 1.
    for (const std::uint32_t answer : {0u, 1u, 2u, 0xffffffffu}) {
        Plan p = base;
        p.a9_answer = answer;
        add(p, "byte +0xa9 answered " + hex(answer));
    }
    // The 0x18-byte rows.
    for (const bool pointer : {false, true})
        for (const int count : {0, 1, 3, -1}) {
            Plan p = base;
            p.block_18 = pointer;
            p.half_34 = count;
            add(p, std::string("0x18-byte rows ") + std::to_string(count) + (pointer ? " with pointer" : " no pointer"));
        }
    for (const int rounds : {0, 1, 2, 3, -1}) {
        Plan p = base;
        p.rounds = rounds;
        add(p, "rounds " + std::to_string(rounds));
        for (const int ties : {0, 1, 3, -2}) {
            if (rounds <= 0 && ties) continue;
            Plan q = p;
            q.ties = ties;
            add(q, "rounds " + std::to_string(rounds) + " matches per round " + std::to_string(ties));
        }
    }
    for (const int clubs : {0, 1, 2, 4, -1}) {
        for (const bool nones : {false, true}) {
            Plan p = base;
            p.clubs = clubs;
            p.none_ids = nones;
            if (nones && clubs < 2) continue;
            add(p, "entrants " + std::to_string(clubs) + (nones ? " with none ids" : ""));
        }
    }
    // Stages: kinds, the array, the lists.
    for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 0x101u, 0xffu}) {
        for (const bool array : {false, true}) {
            Plan p = base;
            p.stage_kind = kind;
            p.stages_array = array;
            p.word_2c = 3;
            p.last_stage = 1;
            add(p, "stages kind " + hex(kind) + (array ? " existing array" : " new array"));
        }
    }
    {
        Plan p = base;
        p.stage_kind = 1;
        p.word_2c = 3;
        p.last_stage = 0;
        for (const std::uint32_t second : {0u, 1u, 3u, 0xffffu}) {
            p.list_second = second;
            add(p, "stage list count " + hex(second));
        }
        p.list_second = 2;
        p.stage_kind = 2;
        add(p, "stage group");
        p.word_2c = 0;
        p.last_stage = 2;
        add(p, "no stage array, no count");
        p.word_2c = -3;
        add(p, "negative stage count");
        p.word_2c = 3;
        p.last_stage = -1;
        add(p, "last stage -1");
        p.last_stage = 0;
        p.stage_kind = 1;
        p.stages_array = true;
        p.word_2c = 0;
        add(p, "existing array, no count, kind 1");
    }
    // `new` returning null for a stage (the reader skips the constructor, stores the null and reads on).
    for (const unsigned nth : {1u, 2u}) {
        for (const std::uint32_t kind : {1u, 2u}) {
            Plan p = base;
            p.stage_kind = kind;
            p.word_2c = 3;
            p.last_stage = 1;
            p.null_new = nth;
            add(p, "stage kind " + std::to_string(kind) + " new null, nth " + std::to_string(nth));
            p.stats_flag = 1;
            p.null_new = nth + 1;
            add(p, "statistics and stage kind " + std::to_string(kind) + " new null, nth " + std::to_string(nth + 1));
        }
    }
    for (const int rows : {0, 1, 2, -1}) {
        for (const int capacity : {0, 1, 2, 5, -1}) {
            if (rows == 0 && capacity != 0) continue;
            Plan p = base;
            p.rows = rows;
            p.capacity = capacity;
            add(p, "round rows " + std::to_string(rows) + " capacity " + std::to_string(capacity));
            if (rows > 1 && capacity == 2) {
                p.none_ids = true;
                add(p, "round rows " + std::to_string(rows) + " with none ids");
            }
        }
    }
    // Everything together, and the failure of each checked read and allocation of that script.
    Plan full = base;
    full.block_18 = true;
    full.half_34 = 2;
    full.rounds = 2;
    full.clubs = 3;
    full.stage_kind = 1;
    full.word_2c = 3;
    full.last_stage = 1;
    full.rows = 2;
    full.capacity = 2;
    full.record_index = 3;
    full.stats_flag = 1;
    full.old_stats = true;
    add(full, "everything");
    full.none_ids = true;
    add(full, "everything with none ids");
    full.none_ids = false;
    {
        Plan p = full;
        p.stage_kind = 2;
        p.backup = 0;
        add(p, "everything, group stages, by year");
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
        // The reads that compare their count with the cup's: no rounds and no entrants (a count of 0 read returns 0).
        Plan p = full;
        p.rounds = 0;
        p.clubs = 0;
        add(p, "everything without rounds and entrants");
        // The reads of no rows (count 0) fail on an answer that is not 0: the rounds' read is the 17th checked read, the
        // entrants' the 18th.
        p.fail_read = 17;
        add(p, "everything without rounds, a mismatched read at the rounds' count");
        p.fail_read = 18;
        add(p, "everything without rounds and entrants, a mismatched read at the entrants' count");
    }
}

// ---------------------------------------------------------------------------------------------------------------
// The writer.

void save_cases(const Scenario& scenario, const World& w, std::vector<Case>& cases) {
    const auto base_answers = [&](std::uint32_t cup) {
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers{{file_path, path}, {open_file, handle}, {write_file, 1}, {close_file, 0},
                                                                     {stats_write, 0}, {list_write, 0}, {group_write, 0}};
        answers.emplace_back(w.target(cup, 0x2c), 0);
        return answers;
    };
    const auto add = [&](const std::string& label, std::uint32_t cup, std::uint32_t temporary, std::vector<Write> memory = {},
                         std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
        auto all = base_answers(cup);
        for (const auto& [entry, answer] : answers) {
            std::erase_if(all, [&](const auto& item) { return item.first == entry; });
            all.emplace_back(entry, answer);
        }
        cases.push_back({.entry = writer, .registers = {{3, cup}, {4, temporary}}, .memory = std::move(memory), .answers = std::move(all),
                         .label = "storage: save " + label + " " + hex(cup), .original_owners = {write_file}});
    };
    // The real cups, by year and temporary, and the byte of the flag.
    const auto picks = std::min<std::size_t>(w.cups.size(), 12);
    for (std::size_t n = 0; n < picks; ++n) {
        add("temporary", w.cups[n], 1);
        add("by year", w.cups[n], 0);
    }
    const auto cup = w.cups.front();
    add("temporary flag bytes 0x100 (year)", cup, 0x100);
    add("temporary flag 0xff", cup, 0xff);
    add("temporary flag 0x80", cup, 0x80);
    add("file does not open", cup, 1, {}, {{open_file, 0}});
    add("by year, file does not open", cup, 0, {}, {{open_file, 0}});
    add("inactive cup by year", cup, 0, {w8(cup + 0x45, 0)});
    add("inactive cup temporary", cup, 1, {w8(cup + 0x45, 0)});
    add("inactive cup, flag byte only", cup, 0x100, {w8(cup + 0x45, 0)});
    add("no rounds", cup, 1, {w32(cup + 0xa0, 0)});
    add("no entrants", cup, 1, {w32(cup + 0xa4, 0)});
    add("no match rows", cup, 1, {w32(cup + 0xac, 0)});
    add("no rounds, by year", cup, 0, {w32(cup + 0xa0, 0)});
    add("no statistics", cup, 1, {w32(cup + 8, 0)});
    add("with statistics", cup, 1, {w32(cup + 8, 0x1234)});
    add("0x18-byte rows", cup, 1, {w32(cup + 0x18, rows_block), w16(cup + 0x34, 3)});
    add("0x18-byte rows, none counted", cup, 1, {w32(cup + 0x18, rows_block), w16(cup + 0x34, 0)});
    add("0x18-byte rows, negative count", cup, 1, {w32(cup + 0x18, rows_block), w16(cup + 0x34, 0xffff)});
    add("no rounds count", cup, 1, {w16(cup + 0x3c, 0)});
    add("negative rounds count", cup, 1, {w16(cup + 0x3c, 0xffff)});
    add("no entrants count", cup, 1, {w16(cup + 0x3e, 0)});
    add("negative entrants count", cup, 1, {w16(cup + 0x3e, 0xffff)});
    add("one entrant", cup, 1, {w16(cup + 0x3e, 1)});
    // The entrant rows: a null club (-1 for the file), a club already -1 (read back as none), other words.
    {
        const auto table = scenario.r32(cup + 0xa4);
        add("entrant without club", cup, 1, {w32(table, 0)});
    }
    // A cup with no record: the first name reads a null pointer's words (--null-page maps them).
    add("no record", cup, 1, {w32(cup + 4, 0)});
    add("no record, by year", cup, 0, {w32(cup + 4, 0)});
    // Stages: the cup's own real stage objects (kind 1: cups; kind 2: leagues), the kind byte, the counts.
    const auto with_stages = [&](const std::string& what, std::vector<std::uint32_t> objects, std::int32_t last, std::vector<Write> more = {}) {
        std::vector<Write> memory{w32(cup + 0xc, stage_list), w32(cup + 0x30, std::uint32_t(last))};
        for (std::size_t i = 0; i < objects.size(); ++i) memory.push_back(w32(stage_list + std::uint32_t(i) * 4, objects[i]));
        add(what, cup, 1, joined(memory, more));
    };
    const auto league = w.leagues.empty() ? 0u : w.leagues.front();
    const auto other = w.cups.size() > 1 ? w.cups[1] : cup;
    with_stages("one stage of kind 1", {other}, 0);
    with_stages("two stages of kind 1", {other, w.cups[w.cups.size() > 2 ? 2 : 0]}, 1);
    with_stages("a stage of kind 1 without rounds", {other}, 0, {w16(other + 0x3c, 0)});
    with_stages("a stage of kind 1 with negative rounds", {other}, 0, {w16(other + 0x3c, 0xffff)});
    if (league) {
        with_stages("one stage of kind 2", {league}, 0);
        with_stages("stages of kinds 1 and 2", {other, league}, 1);
        with_stages("stages of kinds 2 and 1", {league, other}, 1);
        with_stages("kind 2 stage with kind 1 after a longer one", {other, league, other}, 2);
        with_stages("last stage -1", {league}, -1);
        with_stages("last stage -2", {league}, -2);
    }
    if (!w.others.empty()) with_stages("a stage of another kind", {w.others.front()}, 0);
    {
        // A stage whose kind byte is neither 1 nor 2.
        with_stages("a stage with kind byte 3", {other}, 0, {w8(other + 0x43, 3)});
        with_stages("a stage with kind byte 0xff", {other}, 0, {w8(other + 0x43, 0xff)});
        with_stages("a kind 1 stage after a kind 2 one (register leftovers)", {other, league ? league : other, other}, 2);
    }
    // The 8-byte rows of +0x14: their clubs become indices and back.
    const auto row_memory = [&](int count, bool without_club, int capacity) {
        std::vector<Write> memory{w32(cup + 0x14, rows_block), w16(cup + 0x36, std::uint32_t(count)), w16(cup + 0x38, std::uint32_t(capacity))};
        for (int row = 0; row < count; ++row)
            memory.push_back(w32(rows_block + 8u * std::uint32_t(row), without_club && row == 1 ? 0 : w.club(unsigned(row) + 1)));
        return memory;
    };
    add("3 round rows", cup, 1, row_memory(3, false, 3));
    add("3 round rows, one without club", cup, 1, row_memory(3, true, 3));
    add("1 round row", cup, 1, row_memory(1, false, 1));
    add("3 round rows, 2 saved", cup, 1, row_memory(3, false, 2));
    add("no round rows", cup, 1, row_memory(0, false, 0));
    {
        auto memory = row_memory(0, false, 4);
        memory[0] = w32(cup + 0x14, 0);
        add("round rows count without block", cup, 1, memory);
        memory = row_memory(0, false, 0);
        memory.push_back(w16(cup + 0x36, 0xffff));
        add("negative round row count", cup, 1, memory);
    }
    // Every real cup with its own stages and rows as the career has them.
    for (std::size_t n = picks; n < w.cups.size() && n < picks + 8; ++n) add("temporary", w.cups[n], 1);
}
}

CUP_CASES("storage") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.cups.size() < 3) throw std::runtime_error("storage: the career has fewer than three cup objects");
    std::fprintf(stderr, "storage: %zu cups, %zu leagues, %zu objects of other kinds\n", w.cups.size(), w.leagues.size(), w.others.size());
    load_cases(scenario, w, cases);
    save_cases(scenario, w, cases);
}
}
