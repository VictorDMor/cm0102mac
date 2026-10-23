#pragma once
// FORMATION.CPP of the original: a team's formation record (0x18e0 bytes),
// the players' positions without and with the ball, the zones derived from
// them, the shape code and the per-player instruction bits; and the .tct
// formation files. Each routine leaves what the original leaves (see
// guest_abi.hpp).
#include "guest_abi.hpp"

namespace cm::aot::formation {
// Position codes are a line bit (0x1 goalkeeper .. 0x40) or'ed with a side
// bit (0x80 .. 0x800).
namespace field {
constexpr std::uint32_t position = 0x12e;            // int16[11]: each player's position without the ball
constexpr std::uint32_t position_set = 0x144;        // int8[11]: first player holding that position
constexpr std::uint32_t position_with_ball = 0x150;  // int16[11]: where the player runs with the ball
constexpr std::uint32_t role = 0x5b8;                // uint8[11]: role bits of each player slot
constexpr std::uint32_t player = 0x5c4;              // int32[11]: staff index of each slot's player, -1 none
constexpr std::uint32_t shape = 0x1880;              // uint32: or of the positions' shape bits, normalised
constexpr std::uint32_t saved = 0x1884;              // uint8: 1 until something changes the record
constexpr std::uint32_t instructions = 0x1888;       // uint64[11]: each player's instruction fields
}
constexpr std::int8_t players = 11;
constexpr std::uint32_t staff_size = 0x70;

// Typed access to one formation record in guest memory.
class Record {
public:
    Record(guest::Cpu& cpu, std::uint32_t at) : c(cpu), address(at) {}
    std::int16_t position(std::int8_t player, bool with_ball = false) const {
        return c.i16(address + (with_ball ? field::position_with_ball : field::position) + std::uint32_t(player) * 2);
    }
    std::int32_t staff(std::int8_t slot) const { return c.i32(address + field::player + std::uint32_t(slot) * 4); }
    std::uint32_t at(std::uint32_t offset) const { return address + offset; }
    guest::Cpu& c;
    std::uint32_t address;
};

// 0x10111240: a player's position code, with or without the ball.
std::int32_t position(guest::Cpu& c, std::uint32_t formation, std::int8_t player, std::int8_t with_ball);
// 0x10111840: the first player whose position (without the ball) is `position`, or -1.
std::int32_t player_at(guest::Cpu& c, std::uint32_t formation, std::int16_t position);
// 0x10113e10: a player's instruction fields (r3 high word, r4 low word).
std::uint64_t instructions(guest::Cpu& c, std::uint32_t formation, std::int8_t player);
// 0x101149f0: a player slot's role bits.
std::uint8_t role(guest::Cpu& c, std::uint32_t formation, std::int8_t slot);
// 0x10114b10: the staff record of the player in a slot, or 0.
std::uint32_t staff_record(guest::Cpu& c, std::uint32_t formation, std::int8_t slot);
// 0x10114b50: puts a staff record's player (or none) in a slot.
std::uint32_t set_staff(guest::Cpu& c, std::uint32_t formation, std::int8_t slot, std::uint32_t staff);
// 0x10114ba0: empties every slot; slots with both role bits 0x4 and 0x8 keep only 0x2.
std::uint32_t clear_staff(guest::Cpu& c, std::uint32_t formation);
// 0x10114c00: recomputes the shape code from the positions.
void update_shape(guest::Cpu& c, std::uint32_t formation);
// 0x10114d40: the shape bit of a position code (0 for none).
std::uint32_t shape_bit(guest::Cpu& c, std::int16_t position);
// 0x10116780: splits a position code into its line bit and side bit (a
// goalkeeper is central); asserts and returns 0 when either is missing.
std::uint32_t split_position(guest::Cpu& c, std::int16_t position, std::uint32_t line, std::uint32_t side);
// 0x101171b0: a position's place in the team order: 10 per line from the
// goalkeeper, then the side (right 0, left 1, right-centre 2, centre 3,
// left-centre 4); asserts and returns 0 when a part is missing.
std::int32_t position_order(guest::Cpu& c, std::int16_t position);
}
