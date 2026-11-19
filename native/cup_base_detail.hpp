#pragma once
// CUP.CPP's records, globals and shared pieces (cup_base.hpp has the routines). The cup derives from COMP.CPP's
// competition: comp_detail.hpp names the base object's fields (comp::competition::) and provides `invoke`,
// `virtual_call`, `stage_unchecked`, `sext8`/`sext16`; league_base_detail.hpp provides the game's sample stream
// `Pool`, which the cup draws use too. This header adds the cup's own fields and records. Field meanings come
// from how this module and the derived cups use them (european_cup_detail.hpp, inter_amer_cup_detail.hpp,
// pol_cup_detail.hpp, cup_stage_detail.hpp); "(inferred)" marks a name no reader has confirmed.
//
// COMP.CPP's comp_detail.hpp calls kind 1 "a league table" and kind 2 "a knockout": the constructors say the
// opposite. This module's constructor stores kind 1 and LEAGUE.CPP's kind 2, so COMP's "league" fields at
// +0xa0..+0xac (round_table, table, current_round, round_matches) are the cup's, named below.
#include "comp_detail.hpp"
#include "league_base_detail.hpp"
#include "cup_base.hpp"

namespace cm::aot::cup_base {
using guest::Frame;
using comp::invoke;
using comp::virtual_call;
using comp::sext8;
using comp::sext16;
using league_base::Pool;
namespace competition = comp::competition;

// TOC slots (`lwz rX,slot(r2)`, `c.toc(slot)`). The unit's run of data slots is -0x47cc..-0x47b8 and its one
// vtable slot is -0x6184 (tools/cup_audit.py). -0x3764 holds a word itself.
namespace toc {
inline constexpr std::int32_t
    pool = -0x47b8,              // the module's strings (text::), 0x10839e0c
    vtable = -0x6184,            // the cup's vtable, 0x108d7724
    doubles = -0x47bc,           // pool +0x4c0: the double 65535.0, then the int-to-double bias 0x4330000080000000
    by_byte_6 = -0x47c0,         // 0x10481e70's descriptor (qsort in 0x1047fd00, 0x10480620); 0x10485830's and
                                 // 0x10485870's descriptors are in other units' slots -0x60ec and -0x6128
    minus_one = -0x47c4,         // 0x10839e08, the word 0xffffffff just before the pool (0x10484420 writes it)
    buffer_47c8 = -0x47c8,       // 0x1098d360, a static buffer (0x104860a0) (inferred)
    buffer_47cc = -0x47cc,       // 0x1098d350, a static buffer (0x104860a0) (inferred)
    word_3764 = -0x3764,         // word: state of 0x10484b90's walk (written and read only there) (inferred)
    competitions = comp::toc::competitions, records = comp::toc::records, clubs = comp::toc::clubs,
    club_count = comp::toc::club_count, news = comp::toc::news, message = comp::toc::message,
    success = comp::toc::success;
}
// Offsets in the module's string pool (0x10839e0c).
namespace text {
inline constexpr std::uint32_t source_file = 0x00,       // "CUP.CPP"
                               report_format = 0x08,     // "v%s %s.%s %d"
                               error = 0x15,             // "Error"
                               report_format_text = 0x1b,// "v%s %s.%s %d (%s)"
                               map_tree_memory = 0x2d,   // "CUP::map_fixture_tree - out of memory"
                               memory_title = 0x53,      // "Memory Allocation Error"
                               memory_message = 0x6b,    // "Unable to allocate requested memory.\nDebug Info: ..."
                               map_tree_memory_2 = 0x17b,// "CUP::map_fixture_tree() - out of memory"
                               drawn_game = 0x1a3,       // "CUP::process_fixture() - drawn game in cup (Round %d of %s)"
                               third_match = 0x1df,      // "CUP::process_fixture() : third match should not have a replay ..."
                               score = 0x22e,            // "Gls:%d - %d, Aet:%d - %d, Pen:%d - %d"
                               sort_memory = 0x254,      // "CUP::sort_round_teams() : Cannot allocate memory"
                               nation_memory = 0x285,    // "CUP::check_round_teams_for_nation() : cannot malloc array"
                               randomize_memory = 0x2bf, // "CUP::randomize_round_teams() : Cannot allocate memory"
                               temporary_name = 0x2f5,   // "%s_%d.tmp"
                               temporary_folder = 0x2ff, // "CM3_TEMP"
                               read_mode = 0x308,        // "rb"
                               load_memory = 0x30b,      // "CUP::load_from_disk() - Unable to allocate memory"
                               load_id = 0x33d,          // "CUP::load_from_disk() - Unable to read 'comp_id'"
                               load_schedule = 0x36e,    // "CUP::load_from_disk() - Unable to allocate memory to schedule"
                               load_next_stages = 0x3ac, // "LEAGUE::load_from_disk() - cannot allocate next_stages"
                               load_schedule_2 = 0x3e3,  // "LEAGUE::load_from_disk() - Cannot allocate schedule memory"
                               load_teams = 0x41e,       // "CUP::load_from_disk() : Unable to create comp teams array"
                               write_mode = 0x458,       // "wb"
                               save_open = 0x45b,        // "CUP::save_cup_to_disk() - unable to open save file'%s'"
                               rank_memory = 0x492;      // "CUP::rank_teams() : cannot malloc clubs array"
}

// The cup object. The constructor 0x10478f20 sets the base competition's kind (+0x43) to 1 and clears +0xa8 and
// +0xa9; derived cups set the rest when they configure a season (european_cup_calendar.cpp, pol_cup_calendar.cpp).
namespace cup {
inline constexpr std::uint32_t
    rounds = 0xa0,           // the 0x78-byte rounds (Round below), one per round; a new[] block
    entrants = 0xa4,         // the 8-byte entrant rows (Entrant below), competition::club_count of them
    current_round = 0xa8,    // byte, 0 at construction (inferred from COMP's name for it)
    byte_a9 = 0xa9,          // byte, 0 at construction
    round_matches = 0xac,    // word per round: that round's 0x36-byte match rows (Match below)
    byte_b0 = 0xb0;          // byte; vt +0x10 (0x104790f0) clears it
inline constexpr std::uint8_t kind = 1;  // what the constructor stores at competition::kind
inline constexpr std::uint32_t round_row = 0x78, entrant_row = 8, match_row = 0x36;
}

// A row of cup::rounds (0x104854f0 and 0x10485570 write its draw and fixture dates, 0x10485600 a slot; the
// derived cups' calendars fill the format: european_cup_calendar.cpp `format_round`).
struct Round {
    static constexpr std::uint32_t label = 0x08,      // halfword (inferred)
                                   index = 0x0a,      // halfword: the round's number (inferred)
                                   flags = 0x0e,      // halfword (inferred)
                                   field_10 = 0x10,   // halfword, or-ed into a match's +0x38 by 0x10485340
                                   entrants = 0x1a,   // halfword: clubs in the round (inferred)
                                   ties = 0x1c,       // halfword: ties in the round (inferred)
                                   leg_rule = 0x23,   // byte (inferred)
                                   prizes = 0x6c;     // three words (inferred)
};
// A row of cup::entrants: a club and its state in the cup.
struct Entrant {
    static constexpr std::uint32_t club = 0x00,   // the club row (0x248 bytes)
                                   state = 0x04;  // signed byte: compared by 0x10485340, 0x10485830, 0x10485870 (inferred: round reached)
};

// Routines of other modules (and the run-time library) this module calls: generated ones go through invoke.
namespace callee {
inline constexpr std::uint32_t
    allocate_object = comp::callee::allocate_object, free_object = comp::callee::free_object,
    allocate_block = comp::callee::allocate_block, free_block = comp::callee::free_block,
    sort = comp::callee::sort, rand = 0x1077b770, exit = 0x1076f7d0,
    read = 0x10775fe0, write = 0x10776360, close = 0x107766f0, open_temporary = 0x10746280;
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
}
