#pragma once
// COMP.CPP's records, globals and shared pieces (comp.hpp has the routines). Field meanings come from
// how this module and its callers use them; "(inferred)" marks a name no reader has confirmed.
#include "comp.hpp"
#include "guest_values.hpp"

namespace cm::aot::comp {
using guest::Frame;

// TOC slots (`lwz rX,slot(r2)`, `c.toc(slot)`): the word there is the address of the object or table.
namespace toc {
inline constexpr std::int32_t
    pool = -0x49c0,            // the module's strings (text::)
    vtable = -0x619c,          // the base class's vtable, 0x108cf608
    sort_column = -0x61a4,     // a signed byte: the halfword column 0x10453300 compares (0..3; else all equal)
    stop_sort = -0x61a0,       // 0x10451470's vector, for the sort in 0x10450ee0
    competitions = -0x7b5c,    // cell: the competition objects, one pointer per competition id (no bounds test)
    competition_count = -0x7b60,  // cell: how many ids the table above has
    records = -0x7bbc,         // cell: the 0x6c-byte competition records, by id
    clubs = -0x7be8,           // cell: the 0x248-byte club rows
    club_count = -0x7b50,      // cell: the number of clubs
    descriptors = -0x7b44,     // the 34 rows of 0x4c bytes, one per playable nation (the TOC word is the array)
    fixture_rules = -0x7b54,   // cell: the object 0x10494d20 asks about a fixture's competition
    news = -0x7bf0,            // cell: the news manager (0x102cdbf0 asks it whether news is on)
    text = -0x7bb8,            // the shared text buffer the news texts format into
    message = -0x7ba8,         // the shared message buffer (assertion texts)
    success = -0x7bac;         // byte: the shared success flag an assertion clears
}
// Offsets in the module's string pool (0x1082de80). The pool's "%s.tmp", "%s_%d.tmp", "CM3_TEMP" and "r"
// (+0x3ee..+0x408) are read by no instruction: the routine that used them is not in the executable.
namespace text {
inline constexpr std::uint32_t source_file = 0x380,  // "COMP.CPP"
                               report_format = 0x389,  // "v%s %s.%s %d"
                               error = 0x396;          // "Error"
}

// The base competition object (the class the vtable at toc::vtable describes). Every league, cup and stage
// object starts with these fields; a derived class's own begin at +0x50 (the constructor writes up to +0x4c).
namespace competition {
inline constexpr std::uint32_t
    vtable = 0x00,
    record = 0x04,        // the 0x6c-byte competition record (record:: below); its +0 is the competition id
    stats = 0x08,         // the competition's statistics object (COMP_STATS.CPP; 0x10459b50 looks a club up in it)
    stages = 0x0c,        // word array: the stage objects 0..last_stage (stage -1 is the object itself)
    holder = 0x10,        // an object whose +0x18 is a nation (0x10451a30) (inferred)
    block_14 = 0x14,      // a new[] block the destructor frees and leaves pointing at the freed block
    block_18 = 0x18,      // a new[] block the destructor frees and clears
    parent = 0x1c,        // word, -1: none (0x10450900 tests it for < 0) (inferred)
    next = 0x20,          // word, -1: none: the id of the next competition in a chain (0x10450900 walks it)
    word_24 = 0x24,       // word, -1 at construction
    word_28 = 0x28,       // word, 0 at construction
    word_2c = 0x2c,       // word, 0 at construction; 0x10450ee0 walks -1 .. this - 1
    last_stage = 0x30,    // word, -1: no stage
    half_34 = 0x34, half_36 = 0x36, half_38 = 0x38,
    code = 0x3a,          // halfword: the competition's code (0x3fc is skipped by 0x10450710)
    rounds = 0x3c,        // halfword: the number of rounds (0x10450db0) (inferred)
    club_count = 0x3e,    // halfword: the number of clubs (rows of entries)
    year = 0x40,          // halfword: the season's year
    type = 0x42,          // byte, 1 at construction; 0x10451ac0 tests 1 and 2
    kind = 0x43,          // byte: 1 a league table (entries at +0xa4), 2 a knockout (entries at +0xb0)
    owner = 0x44,         // signed byte: -1 for a competition that is its own (0x10452ed0, 0x104530f0) (inferred)
    byte_45 = 0x45,       // 1 at construction
    bench = 0x46,         // byte, 5 at construction: the bench size (inferred; the rules plan's +0x46)
    substitutes = 0x47,   // byte, 3 at construction: substitutions allowed (inferred)
    byte_48 = 0x48, byte_49 = 0x49,
    ruling_body = 0x4a,   // signed byte, -1: none (discipline_table_detail.hpp)
    byte_4b = 0x4b,       // 20 at construction
    byte_4c = 0x4c;
// A derived league's or cup's fields the module reads (the derived classes own them).
inline constexpr std::uint32_t
    round_table = 0xa0,   // league: 0x78-byte rows, one per round
    table = 0xa4,         // league (kind 1): 8-byte rows, +0 the club, +5 a byte
    current_round = 0xa8, // league: signed byte
    round_matches = 0xac, // league: word per round, 0x36-byte rows
    entries = 0xb0;       // knockout (kind 2): 0x44-byte rows, +0 the club
inline constexpr std::uint32_t league_row = 8, round_row = 0x78, match_row = 0x36, entry_row = 0x44;
inline constexpr std::uint8_t league = 1, knockout = 2;
}
// The 0x6c-byte competition record (the database's).
namespace record {
inline constexpr std::uint32_t id = 0x00,
                               continent = 0x04,   // compared with a club's nation's +0x74 by 0x10451570 (inferred)
                               nation = 0x08,      // compared with a club's +0x2c (its nation) by 0x10451570
                               reputation = 0x18,  // halfword: 0x10451470 orders by it (inferred)
                               name = 0x1a,        // the long name ("First Division")
                               short_name = 0x4d,
                               short_name_gender = 0x67, name_gender = 0x6a,  // signed bytes the texts pass twice
                               size = 0x6c;
}
// The 0x248-byte club row's fields the texts read.
namespace club {
inline constexpr std::uint32_t name = 0x221, name_gender = 0x23b, size = 0x248;
}
// A row of toc::descriptors: one playable nation and its season settings (player_stats_detail.hpp's Calendar).
namespace descriptor {
inline constexpr std::uint32_t nation = 0x00, code = 0x14 /* byte (inferred: the calendar's kind) */, size = 0x4c;
inline constexpr unsigned count = 34;
inline constexpr std::uint32_t nation_loaded = 0x120;  // the nation record's byte: nonzero when its leagues are loaded
inline constexpr std::uint8_t none = 3;  // what the lookups answer when no row matches
}

// Routines of other modules (and the run-time library) this module calls; generated ones go through c.call.
namespace callee {
inline constexpr std::uint32_t
    split_path = 0x10739890, version_text = 0x10768810, sprintf = 0x1077b720, error_dialog = 0x10769b60,  // the assert
    format = 0x10727890,          // (buffer, localized format, ...): the localized sprintf
    strcpy = 0x1077dbf0, strcmp = 0x1077dc90,
    allocate_object = 0x10773520, free_object = 0x107735a0,     // new, delete
    allocate_block = 0x10775130, free_block = 0x10775170,       // new[], delete[]
    resize_block = 0x107751b0,    // (block, bytes): realloc
    sort = 0x10768300,            // (base, count, size, vector): qsort
    pointer_glue = 0x10781320;    // a virtual call: r12 the vtable slot's descriptor (virtual_call below)
}

// `bl entry` with the arguments in r3 onwards; the callee's r3.
inline std::uint32_t invoke(Cpu& c, std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    c.call(entry, back);
    return c.r(3);
}
// A virtual call as CodeWarrior emits it: `lwz r12,0(object); lwz r12,slot(r12); bl 0x10781320; lwz r2,0x14(r1)`.
// The glue saves r2 at 0x14(r1), the calling routine's own frame: the caller must have built its Frame.
// `arguments` start with the object (r3).
inline std::uint32_t virtual_call(Cpu& c, std::uint32_t slot, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    c.set(12, c.u32(c.u32(c.r(3)) + slot));
    c.call(callee::pointer_glue, back);
    c.set(2, c.u32(c.sp() + 0x14));
    return c.r(3);
}

// The object of competition `id` (toc::competitions; no bounds test, as the original).
inline std::uint32_t competition_object(Cpu& c, std::uint32_t id) { return c.u32(c.u32(c.toc(toc::competitions)) + id * 4); }
// A stage of a competition as the module's routines inline it: -1 the object itself, else the stages array.
inline std::uint32_t stage_unchecked(Cpu& c, std::uint32_t competition, std::int32_t index) {
    return index == -1 ? competition : c.u32(c.u32(competition + competition::stages) + std::uint32_t(index) * 4);
}

// `extsb` / `extsh` of a register's low byte / halfword, as the 32-bit register value.
inline std::uint32_t sext8(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }
inline std::uint32_t sext16(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int16_t(value))); }

// The assertion report of a failed check: `at` is the address of the report's first call (0x10739890),
// `name` and `extension` the frame offsets of its two buffers, `line` the source line. The caller sets the
// non-volatile registers the original holds there first (the error dialog reaches imports, where a replay
// compares r13-r31).
struct CheckSite { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
inline void report(Cpu& c, const Frame& frame, const CheckSite& site) {
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {site.at, pool + text::source_file, pool + text::report_format, pool + text::error,
                                frame.at(site.name), frame.at(site.extension), site.line});
}
// The stage walk the module inlines with the assertion (line 0x36e): the stage, or 0 after the report. The caller
// has set the original's non-volatile registers.
inline std::uint32_t checked_stage(Cpu& c, const Frame& frame, std::uint32_t object, std::int32_t index, const CheckSite& site) {
    if (index < -1 || index > c.i32(object + competition::last_stage)) {
        report(c, frame, site);
        return 0;
    }
    return stage_unchecked(c, object, index);
}
}
