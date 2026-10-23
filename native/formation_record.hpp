#pragma once
// FORMATION.CPP: building a formation record (the default 4-4-2, or one side
// of a two-club tactics record), the team options word, each player's
// instruction fields and role bits, the wide-player checks, and the names of
// positions ("Defender Left", "DL"). Each routine leaves what the original
// leaves (see guest_abi.hpp).
#include "formation.hpp"

namespace cm::aot::formation {
namespace field {
constexpr std::uint32_t name = 0x0;             // char[0x33]
constexpr std::uint32_t password = 0x33;        // char[0x32], stored scrambled in .tct files
constexpr std::uint32_t creator = 0x65;         // char[200]
constexpr std::uint32_t team_options = 0x588;   // uint32: the team instruction fields
constexpr std::uint32_t changed = 0x1885;       // uint8
constexpr std::uint32_t flag_1886 = 0x1886;     // uint8, cleared with the record
}
constexpr std::uint32_t record_size = 0x18e0;

// 0x10110c80: a fresh record named "Uninitialised": a 4-4-2, default team
// options and instructions, role bits 0x12, no players, zone grids built,
// marked saved. Returns the record.
std::uint32_t initialise(guest::Cpu& c, std::uint32_t formation);
// 0x10110f40: a record from side `side` (its low byte, 0 or not) of a
// two-club tactics record (club indices at +0 and +4, then 0x664 bytes per
// side): named "Formation vs <the other club>", that side's positions without
// and with the ball and its team options, and the defaults of initialise.
// Asserts when the source is null. Returns the record.
std::uint32_t initialise_from_tactics(guest::Cpu& c, std::uint32_t formation, std::uint32_t source, std::uint32_t side);
// 0x10113b60: sets one field of the team options word to `option` (one of
// its bits); asserts and returns 0 for a value of no field.
std::uint32_t set_team_option(guest::Cpu& c, std::uint32_t formation, std::int32_t option);
// 0x10113e30: sets one of a player's instruction fields to `option` (one bit
// of the 64-bit word); asserts and returns 0 for a bad player or a value of
// no field.
std::uint32_t set_instruction(guest::Cpu& c, std::uint32_t formation, std::int8_t player, std::uint64_t option);
// 0x10114a00: sets a slot's role bits: 1, 2 or 4 replace the low three bits,
// 8 or 0x10 the next two; asserts and returns 0 otherwise.
std::uint32_t set_role(guest::Cpu& c, std::uint32_t formation, std::int8_t slot, std::uint32_t role);
// 0x10116970: a position's full name ("Defender Left") into a buffer of at
// least 100 bytes. With `strict` (low byte) a missing line or side asserts;
// without it that part keeps the buffer's initial (empty) text.
std::uint32_t position_name(guest::Cpu& c, std::int16_t position, std::uint32_t buffer, std::int32_t size,
                            std::uint32_t strict);
// 0x10116c90: a position's abbreviation ("DL") into a buffer of at least 10
// bytes; with `line_alone` (low byte) a two-letter line stands alone ("DM").
std::uint32_t position_abbreviation(guest::Cpu& c, std::int16_t position, std::uint32_t buffer, std::int32_t size,
                                    std::uint32_t line_alone);
// 0x10116f80: the name of one line or side bit ("Sweeper", "Centre Left",
// 0x1000 "Free Role", anything else empty) into a buffer of at least 100 bytes.
std::uint32_t position_part_name(guest::Cpu& c, std::int16_t part, std::uint32_t buffer, std::int32_t size);
// 0x101182a0: 1 when no player is wide (left 0x80 or right 0x800) in
// midfield or attack, with or without the ball; 0 otherwise.
std::uint32_t no_wide_midfield_or_attack(guest::Cpu& c, std::uint32_t formation);
// 0x10118a40: 1 when no player is wide in defence or defensive midfield,
// with or without the ball; 0 otherwise.
std::uint32_t no_wide_defence(guest::Cpu& c, std::uint32_t formation);
}
