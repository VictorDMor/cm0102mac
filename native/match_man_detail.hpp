#pragma once
// MATCH_MAN.CPP's shared record views and constants. Offsets are the
// original's; meanings come from the callers and are named as far as the
// code shows them.
#include "guest_abi.hpp"
#include "guest_fpu.hpp"
#include <bit>
#include <cstdint>

namespace cm::aot::match_man {
// A match side (0xd8 bytes at match+0x7aac + side * 0xd8).
namespace match_side {
constexpr std::uint32_t players = 0x28;  // its 20 player records
constexpr std::int32_t player_count = 20;
}
// A player record of a match (0x1c4 bytes).
namespace match_player {
constexpr std::uint32_t size = 0x1c4;
constexpr std::uint32_t person = 0x6c;  // the person record it plays
}
// TOC slots of the unit (tools/match_man_audit.py): -0x5274 is its literal pool.
namespace slots {
constexpr std::int32_t pool = -0x5274, constants = -0x5278, values = -0x527c;
}
// A person record (a player or a staff member).
namespace person {
constexpr std::uint32_t id = 0x0;       // inferred: added to the noise seed so each person rolls differently
constexpr std::uint32_t player = 0x14;  // the player data (database::person::player)
constexpr std::uint32_t staff = 0x1c;   // inferred: the staff data, with the judging attributes
constexpr std::uint32_t age = 0x5c;     // 16-bit, used as a signed byte (inferred: age in years)
}
// A person's player data.
namespace player_data {
constexpr std::uint32_t goalkeeper_rating = 0xf;  // 20: natural goalkeeper (database::player::goalkeeper)
constexpr std::uint32_t current = 0x4;            // 16-bit (inferred: current ability)
constexpr std::uint32_t potential = 0x6;          // 16-bit (inferred: potential ability)
}
// A person's staff data: signed bytes rated by 0x100d2f50 (inferred: judging
// player ability and potential).
namespace staff_data {
constexpr std::uint32_t judging_current = 0x37, judging_potential = 0x38;
}
// Floating-point idioms of the original, shared by blocks that use them.
// The compiler's int-to-double conversion: the words 0x43300000 and
// value ^ 0x80000000 stored at `at` and `at + 4` (the stack image keeps them),
// loaded as a double and reduced by the pool's 2^52 + 2^31 (`conversion`)
// with fsubs.
inline double converted_int(guest::Cpu& c, guest::Fpu& fpu, std::uint32_t at, std::int32_t value, double conversion) {
    constexpr std::uint32_t exponent_word = 0x43300000;
    const std::uint32_t flipped = std::uint32_t(value) ^ 0x80000000u;
    c.put32(at, exponent_word);
    c.put32(at + 4, flipped);
    return fpu.fsubs(std::bit_cast<double>(std::uint64_t(exponent_word) << 32 | flipped), conversion);
}
// fctiwz into f0, stored with stfd at `at` (both words reach the stack
// image), then the integer read back (lwz of the low word at `at` + 4).
inline std::uint32_t truncated_to_stack(guest::Cpu& c, guest::Fpu& fpu, std::uint32_t at, double value) {
    const auto bits = fpu.fctiwz(0, value);
    c.put64(at, bits);
    return std::uint32_t(bits);
}
}
