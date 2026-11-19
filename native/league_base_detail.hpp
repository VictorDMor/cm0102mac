#pragma once
// LEAGUE.CPP's records, globals and shared pieces (league_base.hpp has the routines). The league derives
// from COMP.CPP's competition: comp_detail.hpp names the base object's fields (comp::competition::) and
// provides `invoke`, `virtual_call`, `stage_unchecked`, `sext8`/`sext16`; this header adds the league's own
// fields and records. Field meanings come from how this module and its derived leagues use them;
// "(inferred)" marks a name no reader has confirmed.
#include "comp_detail.hpp"
#include "league_base.hpp"
#include <array>

namespace cm::aot::league_base {
using guest::Frame;
using comp::invoke;
using comp::virtual_call;
using comp::sext8;
using comp::sext16;
namespace competition = comp::competition;

// TOC slots (`lwz rX,slot(r2)`, `c.toc(slot)`). The unit's run of data slots is -0x472c..-0x470c and of
// vtable slots -0x6160..-0x6158 (tools/league_audit.py). -0x3750 and -0x3754 hold words themselves.
namespace toc {
inline constexpr std::int32_t
    pool = -0x470c,              // the module's strings (text::)
    vtable = -0x6158,            // the league's vtable, 0x108d796c
    sort_league = -0x3750,       // word: the league a sort's comparators read (0x1049eb30 sets it)
    sort_compare = -0x3754,      // word: the descriptor those comparators call back (0x1049eb30 sets it)
    standings_order = -0x6160,   // 0x1049f9a0's descriptor (the standings comparator 0x1049eb30 sorts with)
    standings_order_2 = -0x615c, // 0x1049fc80's descriptor (also loaded by 0x1054aaf0)
    order_4710 = -0x4710,        // 0x1049ff90's descriptor
    order_4714 = -0x4714,        // 0x104a38d0's descriptor
    by_word = -0x4718,           // 0x104aaaf0's descriptor
    by_reputation = -0x4728,     // 0x104aab00's descriptor
    by_half_a = -0x472c,         // 0x104aab20's descriptor
    constant_4720 = -0x4720,     // a constant just before the pool (0x1083a800; read by 0x104a8530)
    constant_4724 = -0x4724,     // pool +0x68 (read by 0x104aa620)
    constant_471c = -0x471c,     // pool +0x6c (read by 0x104a28d0, 0x104aa620)
    competitions = comp::toc::competitions, records = comp::toc::records, clubs = comp::toc::clubs,
    club_count = comp::toc::club_count, news = comp::toc::news, message = comp::toc::message,
    success = comp::toc::success;
}
// Offsets in the module's string pool (0x1083a804).
namespace text {
inline constexpr std::uint32_t source_file = 0x00,     // "LEAGUE.CPP"
                               report_format = 0x0b,   // "v%s %s.%s %d"
                               error = 0x18,           // "Error"
                               fixture_clubs = 0x1e,   // "home != -1 && away != -1"
                               temporary_name = 0x37,  // "%s_%d.tmp" (0x104a6850, 0x104a8530)
                               backup_name = 0x41,     // "%s_%d.bak" (0x104a6850)
                               temporary_folder = 0x4b,// "CM3_TEMP"
                               read_mode = 0x54,       // "rb"
                               write_mode = 0x57,      // "wb"
                               team_points = 0x5a;     // "team_points" (0x104aa620)
}

// The league object (0xec bytes: eng_third_detail.hpp). The constructor 0x1049e4f0 sets the base competition's
// kind (+0x43) to 2 and writes the fields below; derived leagues set the rest when they configure a season.
namespace league {
inline constexpr std::uint32_t
    word_a0 = 0xa0,          // word, 0 at construction
    half_a4 = 0xa4,          // halfword, -1 at construction
    half_a8 = 0xa8,          // halfword, 0 at construction (COMP.CPP's current_round byte there is another class's view)
    word_ac = 0xac,          // word, 0 at construction
    standings = 0xb0,        // the 0x44-byte rows, one per club (Standing below); 0 at construction
    byte_b4 = 0xb4,          // byte, 0 at construction
    half_b6 = 0xb6,          // halfword, -1 at construction
    half_b8 = 0xb8,          // halfword, 0 at construction
    schedule = 0xbc,         // the 0x4c-byte rounds (Round below); set by the derived league
    automatic_promotion = 0xc0,  // signed byte: automatic promotion places (season-competition-rules-plan.md)
    playoff_clubs = 0xc1,        // byte: promotion playoff clubs
    byte_c2 = 0xc2,
    relegation = 0xc3,           // signed byte: relegation places
    win_points = 0xc4, draw_points = 0xc5,  // bytes (inferred)
    byte_c6 = 0xc6,              // byte, 2 at construction
    tie_breaks = 0xc7,           // four bytes, cleared at construction (memset); the tie-break order (inferred)
    date_cc = 0xcc,              // a DATE.CPP date (8 bytes), set to the default at construction
    date_d4 = 0xd4,              // a second date, likewise
    flags = 0xdc,                // halfword, 3 at construction; bit 1 is read by 0x104a14f0
    half_de = 0xde,              // halfword, 0 at construction
    byte_e0 = 0xe0,              // byte, 0 at construction
    byte_e1 = 0xe1,              // byte, 2 at construction
    half_e2 = 0xe2, half_e4 = 0xe4,  // halfwords, -1 at construction
    byte_e6 = 0xe6,              // byte, 0 at construction
    preset_fixtures = 0xe8,      // a new[] array of 16-byte rows (PresetFixture) with a 16-byte header; 0: none
    size = 0xec;
inline constexpr std::uint8_t kind = 2;  // what the constructor stores at competition::kind
inline constexpr std::uint32_t standing_row = 0x44, round_row = 0x4c, preset_fixture_row = 0x10, array_header = 0x10;
}

// A row of league::standings: one club's line of the table. Only the club is named so far.
struct Standing {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t club() const { return c.u32(address); }
};
// A row of league::schedule (0x104a9810 writes the date part, 0x104a9890 a match slot): a match day.
struct Round {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t day = 0x00,   // halfword: the date's day of the year
                                   year = 0x02,  // halfword
                                   kind = 0x04,  // byte (inferred)
                                   slots = 0x08, // eight 8-byte slots: +0 a word, +4..+6 three signed bytes
                                   slot_size = 8,
                                   tail = 0x48;  // word
};
// A row of league::preset_fixtures (0x104aab30 builds one from two club names): a date and the two clubs'
// standings rows' clubs, -1 when a name was not found.
struct PresetFixture {
    static constexpr std::uint32_t date = 0x00, home = 0x08, away = 0x0c;
};

// Routines of other modules (and the run-time library) this module calls: generated ones go through invoke.
namespace callee {
inline constexpr std::uint32_t
    release_array = 0x10773610,  // delete[] of a block with a header (cpp_object_storage)
    free_object = comp::callee::free_object,
    memset = 0x10778300,         // (to, value, size) (memory_primitives)
    sort = comp::callee::sort;
}

// The assertion report of a failed check, with this module's pool (comp::report uses COMP.CPP's): `at` is the
// address of the report's first call (0x10739890), `name` and `extension` the frame offsets of its two buffers,
// `line` the source line. The caller sets the non-volatile registers the original holds there first.
struct CheckSite { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
inline void report(Cpu& c, const Frame& frame, const CheckSite& site) {
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {site.at, pool + text::source_file, pool + text::report_format, pool + text::error,
                                frame.at(site.name), frame.at(site.extension), site.line});
}

// The game's sample stream (the cells at TOC -0x7b90 salt, -0x7b8c start, -0x7b88 end, -0x7b84 cursor): the cursor
// steps through a table of halfwords two bytes at a time; past the end it restarts at the start and a rand() gives
// the salt (its low 16 bits). A draw is (halfword at the cursor + salt) reduced to the bound.
struct Pool {
    Cpu& c;
    std::uint32_t salt, start, end, cursor;
    static Pool at(Cpu& cpu) { return {cpu, cpu.toc(-0x7b90), cpu.toc(-0x7b8c), cpu.toc(-0x7b88), cpu.toc(-0x7b84)}; }
    // `back` is the return address of the rand call in the routine that inlines the step.
    void advance(std::uint32_t back) const {
        const auto next = c.u32(cursor) + 2;
        const auto last = c.u32(end);
        c.put32(cursor, next);
        if (next > last) {
            c.put32(cursor, start);
            c.call(0x1077b770, back);  // rand
            c.put32(salt, c.r(3) & 0xffff);
        }
    }
    std::uint32_t sample() const { return c.u32(salt) + c.u16(c.u32(cursor)); }
};

// The row fields the original copies one by one: the 0x44-byte standings row's bytes 5-7, 0x1e-0x1f, 0x2e-0x2f and
// 0x3e-0x3f are never copied.
struct RowField { std::uint8_t offset, size; };
inline constexpr std::array<RowField, 32> row_fields = {{
    {0x00, 4}, {0x04, 1}, {0x08, 4}, {0x0c, 4}, {0x10, 4}, {0x14, 1}, {0x15, 1}, {0x16, 1}, {0x17, 1},
    {0x18, 2}, {0x1a, 2}, {0x1c, 2}, {0x20, 4}, {0x24, 1}, {0x25, 1}, {0x26, 1}, {0x27, 1}, {0x28, 2},
    {0x2a, 2}, {0x2c, 2}, {0x30, 4}, {0x34, 1}, {0x35, 1}, {0x36, 1}, {0x37, 1}, {0x38, 2}, {0x3a, 2},
    {0x3c, 2}, {0x40, 1}, {0x41, 1}, {0x42, 1}, {0x43, 1}}};
using RowImage = std::array<std::uint32_t, 32>;
inline RowImage read_row(Cpu& c, std::uint32_t row) {
    RowImage image{};
    for (std::size_t n = 0; n < row_fields.size(); ++n) {
        const auto at = row + row_fields[n].offset;
        image[n] = row_fields[n].size == 4 ? c.u32(at) : row_fields[n].size == 2 ? c.u16(at) : c.u8(at);
    }
    return image;
}
inline void write_row(Cpu& c, std::uint32_t row, const RowImage& image) {
    for (std::size_t n = 0; n < row_fields.size(); ++n) {
        const auto at = row + row_fields[n].offset;
        if (row_fields[n].size == 4) c.put32(at, image[n]);
        else if (row_fields[n].size == 2) c.put16(at, image[n]);
        else c.put8(at, image[n]);
    }
}
inline void copy_row(Cpu& c, std::uint32_t from, std::uint32_t to) { write_row(c, to, read_row(c, from)); }
}
