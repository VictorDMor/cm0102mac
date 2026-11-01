#pragma once
// MATCH_ENG.CPP's record offsets and one-line accessors. Generated pages include this
// (through match_eng_leaves.hpp), so it stays small: routines go in match_eng.hpp.
#include "guest_values.hpp"

namespace cm::aot::match_eng {
using guest::Cpu;
using guest::compare;
using guest::compare_logical;
using guest::result;

// The match object: 40 players (20 a side) from +0x311c, each 0x1c4 bytes.
inline constexpr std::uint32_t players_at = 0x311c, player_size = 0x1c4, side_size = 0x2350;
inline constexpr std::uint32_t fixture_at = 0x3118, side_stats_at = 0x7aac, side_stats_size = 0xd8;
inline std::int32_t s8(std::uint32_t value) { return std::int8_t(value); }
inline std::uint32_t u(std::int32_t value) { return std::uint32_t(value); }

// --- Player fields (r3 is the player) ---
inline std::uint32_t player_u8(Cpu& c, std::uint32_t player, std::uint32_t at) { return result(c, c.u8(player + at)); }
inline std::uint32_t position(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x19); }
inline void set_position(Cpu& c, std::uint32_t player, std::uint32_t value) { c.put8(player + 0x19, value); }
inline std::uint32_t previous_position(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x1a); }
inline void set_previous_position(Cpu& c, std::uint32_t player, std::uint32_t value) { c.put8(player + 0x1a, value); }
inline std::uint32_t marking_count(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x104); }
inline std::uint32_t row(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x105); }
inline std::uint32_t column(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x106); }
inline void set_row(Cpu& c, std::uint32_t player, std::uint32_t value) { c.put8(player + 0x105, value); }
inline void set_column(Cpu& c, std::uint32_t player, std::uint32_t value) { c.put8(player + 0x106, value); }
inline std::uint32_t primary_rating(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x107); }
inline std::uint32_t secondary_rating(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x108); }
inline std::uint32_t adjustment_byte(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x109); }
inline std::uint32_t trait_10a(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x10a); }
inline std::uint32_t trait_10c(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x10c); }
inline std::uint32_t trait_10d(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x10d); }
inline std::uint32_t trait_10e(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x10e); }
inline std::uint32_t trait_10f(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x10f); }
inline std::uint32_t status_110(Cpu& c, std::uint32_t player) { return player_u8(c, player, 0x110); }
inline std::uint32_t action_code(Cpu& c, std::uint32_t player) { return result(c, u(c.i16(player + 0x19c))); }
inline void set_action_code(Cpu& c, std::uint32_t player, std::uint32_t value) { c.put16(player + 0x19c, value); }
inline void set_action_19e(Cpu& c, std::uint32_t player, std::uint32_t value) { c.put16(player + 0x19e, value); }
inline std::uint32_t pressure_adjustment(Cpu& c, std::uint32_t player) { return result(c, u(c.i16(player + 0x1a0))); }
inline std::uint32_t word_1ac(Cpu& c, std::uint32_t player) { return result(c, c.u32(player + 0x1ac)); }
inline std::uint32_t marking_target(Cpu& c, std::uint32_t player) { return result(c, c.u32(player + 0x1b0)); }
inline void set_marking_target(Cpu& c, std::uint32_t player, std::uint32_t value) { c.put32(player + 0x1b0, value); }
inline void clear_marking_list(Cpu& c, std::uint32_t player) { c.put8(player + 0x104, 0); }
// 0x101f1dc0: one of a player's markers.
inline std::uint32_t marking_member(Cpu& c, std::uint32_t player, std::uint32_t index) {
    const auto slot = u(s8(index)) * 4;
    c.set(5, slot);
    return result(c, c.u32(player + slot + 0x1b4));
}
inline double fitness(Cpu& c, std::uint32_t player) { const double value = c.f32(player + 0x50); c.set_f(1, value); return value; }
inline double fitness_1a8(Cpu& c, std::uint32_t player) { const double value = c.f32(player + 0x1a8); c.set_f(1, value); return value; }
inline std::uint32_t word_50(Cpu& c, std::uint32_t object) { return result(c, c.u32(object + 0x50)); }
inline std::uint32_t tactical_flags(Cpu& c, std::uint32_t team) { return result(c, c.u32(team + 0x588)); }
inline std::uint32_t byte_2f(Cpu& c, std::uint32_t object) { return player_u8(c, object, 0x2f); }
inline std::uint32_t word_20(Cpu& c, std::uint32_t object) { return result(c, c.u32(object + 0x20)); }
inline std::uint32_t word_24(Cpu& c, std::uint32_t object) { return result(c, c.u32(object + 0x24)); }
inline std::uint32_t word_6c(Cpu& c, std::uint32_t object) { return result(c, c.u32(object + 0x6c)); }
inline std::uint32_t byte_0(Cpu& c, std::uint32_t object) { return player_u8(c, object, 0); }
inline std::uint32_t byte_44(Cpu& c, std::uint32_t object) { return player_u8(c, object, 0x44); }
inline std::uint32_t signed_byte_46(Cpu& c, std::uint32_t object) { return result(c, u(c.i8(object + 0x46))); }
inline void set_byte_0(Cpu& c, std::uint32_t object, std::uint32_t value) { c.put8(object, value); }
inline void set_byte_18(Cpu& c, std::uint32_t object, std::uint32_t value) { c.put8(object + 0x18, value); }
inline std::uint32_t halfword_0(Cpu& c, std::uint32_t object) { return result(c, u(c.i16(object))); }
inline std::uint32_t low_nibble_60(Cpu& c, std::uint32_t object) { return result(c, u(std::int8_t(c.u8(object + 0x60) & 0xf))); }
inline void reset_list_cursor(Cpu& c, std::uint32_t list) { c.put32(list + 0xc, c.u32(list + 4)); }
inline void increment_byte(Cpu& c, std::uint32_t at) {
    const auto before = c.u8(at);
    c.set(4, before);
    c.put8(at, before + 1u);
}

// --- Match fields (r3 is the match) ---
// Offsets from the match object; the ball's cell is read like a player's +0x105 / +0x106.
inline constexpr std::uint32_t ball_row_at = 0x791d;       // the ball's row, 0..8
inline constexpr std::uint32_t ball_column_at = 0x791e;    // the ball's column, 0..11
inline constexpr std::uint32_t shot_state = 0x791f;        // -1: no shot in progress
inline constexpr std::uint32_t last_touch_side = 0x7921;   // the side that touched the ball last, -1: none
inline constexpr std::uint32_t possession_side = 0x7924;   // the side with the ball, -1: none (loose)
inline constexpr std::uint32_t play_state = 0x7928;        // 8: play stopped
inline constexpr std::uint32_t shootout_stage = 0x7929;    // penalty shoot-out stage
inline constexpr std::uint32_t added_time = 0x792d;
inline constexpr std::uint32_t clock_at = 0x7948;          // elapsed clock, int16
inline constexpr std::uint32_t period_end = 0x794c;        // the clock mark that ends the running period, int16
inline constexpr std::uint32_t message_target = 0x79f4;    // the object that receives the messages
inline constexpr std::uint32_t extra_time_flag = 0x1c1;    // extra time is being played
inline constexpr std::uint32_t pending_player = 0xe000;    // the player about to act
inline constexpr std::uint32_t ball_holder = 0xdff8;       // the player holding the ball
inline constexpr std::uint32_t team_tactics = 0x7c6c;      // each side's tactics record
inline constexpr std::uint32_t team_tactics_size = 0x18e0; // bytes per side
inline constexpr std::uint32_t restart_row = 0x30dd;       // the pending restart's cell: row, as the ball's
inline constexpr std::uint32_t restart_column = 0x30de;    // and column
inline std::uint32_t ball_row(Cpu& c, std::uint32_t match) { return player_u8(c, match, ball_row_at); }
inline std::uint32_t ball_column(Cpu& c, std::uint32_t match) { return player_u8(c, match, ball_column_at); }
inline void set_ball_row(Cpu& c, std::uint32_t match, std::uint32_t value) { c.put8(match + ball_row_at, value); }
inline void set_ball_column(Cpu& c, std::uint32_t match, std::uint32_t value) { c.put8(match + ball_column_at, value); }
inline std::uint32_t byte_7931(Cpu& c, std::uint32_t match) { return player_u8(c, match, 0x7931); }
inline std::uint32_t word_79f4(Cpu& c, std::uint32_t match) { return result(c, c.u32(match + message_target)); }
inline void set_possession_side(Cpu& c, std::uint32_t match, std::uint32_t value) { c.put8(match + possession_side, value); }
inline void set_byte_7928(Cpu& c, std::uint32_t match, std::uint32_t value) { c.put8(match + play_state, value); }
inline void set_word_e000(Cpu& c, std::uint32_t match, std::uint32_t value) {
    const auto high = match + 0x10000;
    c.set(5, high);
    c.put32(high - 0x2000, value);
}
inline std::uint32_t minute_clock(Cpu& c, std::uint32_t match) { return result(c, u(c.i16(match + period_end))); }
inline std::uint32_t match_clock(Cpu& c, std::uint32_t match) { return result(c, u(c.i16(match + clock_at))); }
inline void set_default_turn(Cpu& c, std::uint32_t match, std::uint32_t value) { c.put16(match + 0x7950, value); }
inline void clear_word_310c(Cpu& c, std::uint32_t match) { c.put32(match + 0x310c, 0); }
inline void clear_word_3108(Cpu& c, std::uint32_t match) { c.put32(match + 0x3108, 0); }
inline std::uint32_t fixture(Cpu& c, std::uint32_t match) { return result(c, c.u32(match + fixture_at)); }
inline std::uint32_t players(Cpu& c, std::uint32_t match) { return result(c, match + players_at); }
inline std::uint32_t side_stats(Cpu& c, std::uint32_t match, std::uint32_t side) {
    const auto offset = u(s8(side) * std::int32_t(side_stats_size));
    c.set(5, offset);
    return result(c, match + offset + side_stats_at);
}
inline std::uint32_t player_at(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t index) {
    const auto offset = u(s8(side) * std::int32_t(side_size)) + u(s8(index) * std::int32_t(player_size));
    c.set(6, offset);
    return result(c, match + offset + players_at);
}
inline void count_side_event(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t low) {
    const auto offset = u(s8(side) * 0x36) + 0x10000 - low;
    const auto before = c.u8(match + offset);
    c.set(5, before);
    c.set(6, offset);
    c.put8(match + offset, before + 1u);
}

// --- Plain floating point ---
inline double absolute(Cpu& c, double value) {
    const double magnitude = std::bit_cast<double>(std::bit_cast<std::uint64_t>(value) & ~(std::uint64_t(1) << 63));
    c.set_f(1, magnitude);
    return magnitude;
}
}
