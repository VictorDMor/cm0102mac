#pragma once
// LEAGUE.CPP, block `storage`: the league's per-club arrays, their copies and the save writer. The names of the
// fields these routines use that league_base_detail.hpp does not have; "(inferred)" marks a meaning no reader has
// confirmed. The coordinator merges them into the shared header.
#include "league_base_detail.hpp"

namespace cm::aot::league_base::storage {
// The league's fields (the object is 0xec bytes: league_base_detail.hpp's `league::`, comp's `competition::`).
namespace field {
inline constexpr std::uint32_t
    stats = 0x08,            // the competition's statistics object (a flag in the save: nonzero -> 0x10460230 writes it)
    round_rows = 0x14,       // new[] array of 8-byte rows (+0 a club pointer), `used` of them (+0x36), `saved` written (+0x38)
    word_18 = 0x18,          // new[] array of 0x18-byte rows, `row_count_34` of them
    word_1c = 0x1c, word_20 = 0x20, word_24 = 0x24, word_28 = 0x28, word_2c = 0x2c, last_stage = 0x30,
    row_count_34 = 0x34, used_36 = 0x36, saved_38 = 0x38,
    rounds = 0x3c,           // halfword: the number of rounds (comp's `rounds`)
    clubs = 0x3e,            // halfword: the number of clubs (comp's `club_count`)
    year = 0x40,
    kind = 0x43, owner = 0x44, active = 0x45, bench = 0x46, substitutes = 0x47, byte_48 = 0x48, byte_49 = 0x49,
    byte_4b = 0x4b, byte_4c = 0x4c,
    name = 0x4d,             // 0x50-byte text: "%s_%d.tmp" of the competition's name and id (or year), '/' turned into '_'
    history_bytes = 0xa4,    // halfword: -1 until set to (clubs - 1) * rounds, the size of a standings row's +0xc block (inferred)
    schedule_count = 0xa6,   // halfword: how many 0x4c-byte rounds `league::schedule` (+0xbc) holds
    tie_breaks = 0xc7,       // four signed bytes: a 4 asks for the per-opponent block (head to head) (inferred)
    schedule = 0xbc;
inline constexpr std::uint32_t stages = 0x0c;  // word array of stage objects 0..last_stage (comp's `stages`)
}
// A standings row, 0x44 bytes (league_base_detail.hpp's `Standing`; the club is the first word).
namespace row {
inline constexpr std::uint32_t club = 0x00,
                               slot = 0x04,         // byte: the row's index (what 0x104a60e0 was given)
                               opponents = 0x08,    // new[] of (clubs - 1) 16-byte records (+0 the opponent club); 0: none (head to head)
                               history = 0x0c,      // new[] of `history_bytes` bytes, or 0
                               stats_10 = 0x10,     // the counters that are cleared at placement (words/halves/bytes up to +0x3c)
                               group = 0x40, flag = 0x41, one = 0x42, clubs = 0x43,
                               size = 0x44;
}
// A club row's byte 0x23d, which a placement turns from 3 into 2.
inline constexpr std::uint32_t club_state = 0x23d;
// A head-to-head record (16 bytes).
inline constexpr std::uint32_t opponent_size = 0x10;
inline constexpr std::uint32_t club_size = 0x248;
// A stage object (a base competition or a league): the fields the writer reads.
namespace stage {
inline constexpr std::uint32_t rounds = 0x3c, clubs = 0x3e, kind = 0x43,
                               round_table = 0xa0,        // league stage (kind 1): 0x78-byte rounds
                               schedule_count = 0xa6;     // table stage (kind 2)
inline constexpr std::uint32_t round_size = 0x78, round_half = 0x1c;
}
// Routines of other modules this block calls (generated unless noted).
namespace external {
inline constexpr std::uint32_t
    memcpy_object = 0x10739ef0,   // (to, from, bytes)
    memcpy_small = 0x10771110,    // (to, from, bytes)
    copy_date = 0x101fe9f0,       // (to, from): DATE.CPP-style 8-byte copy (MATCH_ENG leaf)
    allocate_block = 0x10775130,  // new[]
    free_block = 0x10775170,      // delete[]
    format = 0x1077b720,          // sprintf
    data_path = 0x10746280,       // (folder, name) -> path
    open_file = 0x10739de0,       // (path, mode) -> file or 0
    write_file = 0x10776360,      // fwrite (data, size, count, file)
    close_file = 0x107766f0,
    write_stats = 0x10460230,     // (stats object, file)
    write_league_stage = 0x104bd550,  // (stage, file): a kind-1 stage's own records
    write_table_stage = 0x1056f6f0;   // (stage, file): a kind-2 stage's own records
}
// The virtual the writer calls on the league after the stats: slot +0x2c (the derived leagues' extra records).
inline constexpr std::uint32_t write_extra_slot = 0x2c;
}
