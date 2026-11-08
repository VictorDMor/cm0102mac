#pragma once
// MATCH_MAN.CPP callbacks block: record offsets and entries the block names.
// Meanings are inferred from how the routines use the fields.
#include <cstdint>

namespace cm::aot::match_man {
// More fields of a match side (0xd8 bytes at match+0x7aac + side * 0xd8; match_detail.hpp
// has its player table).
namespace callback_side {
constexpr std::uint32_t club = 0x14;         // handed to the actor initializer
constexpr std::uint32_t previous_slot = 0x1a;  // a byte, handed to the actor initializer
constexpr std::uint32_t squad_kind = 0x1b;   // a byte: with `squad`, selects the squad's members
constexpr std::uint32_t squad = 0x34;
constexpr std::uint32_t squad_text = 0x44;   // an embedded record the squad release is given
constexpr std::uint32_t extras = 0x3c;       // points at a record with the bench size and a word
constexpr std::int32_t no_extras = -2;       // that word's "none" value
}
// The record at side+0x3c.
namespace side_extras {
constexpr std::uint32_t kind = 0x0;        // word; -2: none
constexpr std::uint32_t suspension = 0x14;  // word handed to the suspension query
constexpr std::uint32_t bench_byte = 0x4e;  // high nibble + 11 = the squad members considered
}
// A player record of a match (0x1c4 bytes; match_man_detail.hpp has its size and person).
namespace callback_player {
constexpr std::uint32_t position = 0x19;  // a byte; -2: not on the pitch
constexpr std::uint32_t rating = 0x36;    // a halfword
constexpr std::uint32_t form = 0x50;      // a float
constexpr std::uint32_t staff = 0x70;     // the person's staff record, 0 if none
}
namespace staff_record {
constexpr std::uint32_t goalkeeping = 0xf;  // a byte, 20 for a natural goalkeeper
}
// Entries of other modules these routines call.
namespace entries {
constexpr std::uint32_t player_category = 0x10289560;  // MATCH_PL: the player's role bits (a halfword)
constexpr std::uint32_t prepare_player = 0x10223ac0;   // MATCH_PL: fills an actor record
}
// Offsets into the pool of doubles at TOC -0x5278 (match_man::slots::constants).
namespace constants {
constexpr std::uint32_t conversion = 0x90;  // 2^52 + 2^31, the int-to-double bias
// The factors of the score comparators, by the role bits of the first player.
constexpr std::uint32_t factor_4_or_2 = 0x30, factor_16 = 0x40;
constexpr std::uint32_t factor_8_first = 0x38, factor_8_second = 0x180;
}
}
