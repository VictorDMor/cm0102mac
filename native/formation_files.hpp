#pragma once
// FORMATION.CPP: the .tct formation files (and the .pct presets shipped with
// the game). A file holds an identifier (the format version 0x98ec59 ..
// 0x98ec5e; presets add 123456789 to it), the lock (the name of the manager
// who locked the file, each byte but 0xff inverted on disk), the creator, then
// the record's shape, positions, display order, spots, team and player
// instructions and roles. Files written on a PC are little-endian; the C
// library's platform tag of the open file tells (0xf1fffff1 for a Mac file).
// Each routine leaves what the original leaves (see guest_abi.hpp).
#include "formation.hpp"

namespace cm::aot::formation {
namespace disk {
// Formation record fields the files hold besides formation::field's.
constexpr std::uint32_t name = 0x0;                // char[0x33]: the file's name, set from its path
constexpr std::uint32_t lock = 0x33;               // char[0x32]: the locking manager's name, "" when unlocked
constexpr std::uint32_t creator = 0x65;            // char[0xc8]
constexpr std::uint32_t display_order = 0x144;     // int8[11] (formation::field::position_set)
constexpr std::uint32_t spots = 0x166;             // uint16[0x210]
constexpr std::uint32_t team_instructions = 0x588; // uint32
constexpr std::uint32_t locked = 0x1885;           // int8: the lock is set
constexpr std::uint32_t on_disk = 0x1886;          // int8: loaded from or saved to a file
constexpr std::uint32_t lock_size = 0x32, creator_size = 0xc8;
}

// The caller's char arguments (`quiet`, `preset`) are taken as the register
// holds them: the original keeps them in non-volatile registers, whose full
// value callees save into the stack image.

// 0x10115000: loads a formation record from a file; with `quiet` set no
// error is reported; `preset` for the game's own preset files. 1 when loaded.
std::uint32_t load_from_disk(guest::Cpu& c, std::uint32_t formation, std::uint32_t path, std::uint32_t quiet,
                             std::uint32_t preset);
// 0x10115cd0: saves a formation record to a file in the current format; 1
// when the header and positions were written (later write errors are only
// reported).
std::uint32_t save_to_disk(guest::Cpu& c, std::uint32_t formation, std::uint32_t path, std::uint32_t quiet,
                           std::uint32_t preset);
// 0x10117fa0: reads a file's lock (0x32 bytes) and creator (0xc8 bytes); 1
// when both were read from a file of a readable version.
std::uint32_t read_lock(guest::Cpu& c, std::uint32_t path, std::uint32_t lock, std::uint32_t creator,
                        std::uint32_t preset);
// 0x10118180 (`preset` 0) and 0x10118210 (`preset` 1), the file dialog's
// callbacks: 2 when the file cannot be read, 1 when it is not locked, 9 when
// the current human manager locked it, 5 when somebody else did.
std::uint32_t lock_state(guest::Cpu& c, std::uint32_t entry, std::uint32_t path, std::uint32_t preset);
}
