#pragma once
// MATCH_MAN.CPP squad_b block: record offsets and constants of 0x1020c5d0.
// Meanings are inferred from how the routine uses the fields.
#include <cstdint>

namespace cm::aot::match_man {
// The match side record (r3; 0xd8 bytes at match + 0x7aac + side * 0xd8).
namespace tactic_side {
constexpr std::uint32_t match = 0x14;
constexpr std::uint32_t number = 0x1a;    // byte: 0 or 1, which of the match's two sides
constexpr std::uint32_t judge = 0x20;     // the person whose eyes rate the players (0x10219e40)
constexpr std::uint32_t manager = 0x24;   // staff data with a signed byte at +0x2e
constexpr std::uint32_t club = 0x34;
constexpr std::uint32_t rival_club = 0x38;
constexpr std::uint32_t fixture = 0x3c;   // word 0 indexes the table at TOC -0x7b5c, +0x14 is a record
}
// The match object.
namespace tactic_match {
constexpr std::uint32_t losses = 0x216, draws = 0x220, wins = 0x22a;  // 16-bit tallies (inferred)
constexpr std::uint32_t flag_a = 0x1c4;       // byte per side
constexpr std::uint32_t flag_b = 0x1c6;       // byte per side (signed, tested against zero)
constexpr std::uint32_t strength = 0x30f0;    // single per side: the value this routine stores
constexpr std::uint32_t players = 0x311c;     // 20 player records per side
constexpr std::uint32_t player_stride = 0x2350, player_size = 0x1c4;
constexpr std::uint32_t live = 0x791b;        // signed byte: players are judged live
constexpr std::uint32_t clock = 0x7948;       // 16-bit: the match clock (MATCH_ENG's clock_at)
constexpr std::uint32_t period_end = 0x794c;  // 16-bit: the clock mark that ends the running period (0x3de, 0x1ef tested)
}
namespace tactic_player {
constexpr std::uint32_t slot = 0x19;          // signed byte: negative is no player
constexpr std::uint32_t person = 0x6c;
}
namespace tactic_club {
constexpr std::uint32_t rating = 0x1ec;       // 16-bit
constexpr std::uint32_t region = 0x2c;
constexpr std::uint32_t link = 0x30;
constexpr std::uint32_t persons[3] = {0x74, 0x78, 0x7c};
}
namespace tactic_fixture {
constexpr std::uint32_t index = 0x0;
constexpr std::uint32_t record = 0x14;        // 16-bit at +0x18 of it
constexpr std::uint32_t record_value = 0x18;
}
namespace tactic_table_entry {
constexpr std::uint32_t source = 0x8;         // handed to 0x10459b50
constexpr std::uint32_t weight = 0x3e;        // 16-bit
}
// The record 0x10459b50 returns (r26 for the club, r27 for the rival).
namespace tactic_stats {
constexpr std::uint32_t a1c = 0x1c, a30 = 0x30, a44 = 0x44, a50 = 0x50, a54 = 0x54, a58 = 0x58, a64 = 0x64,
                        a68 = 0x68, rank = 0x75;
}
// Stack: five signed bytes 0x101f21d0 fills (pointers r5..r9 = sp+0x3c..0x38).
namespace tactic_local {
constexpr std::uint32_t own = 0x3c, rival = 0x3b, byte_3a = 0x3a, byte_39 = 0x39, byte_38 = 0x38;
}
}
