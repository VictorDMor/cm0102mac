#pragma once
// TACTICS_SCREENS.CPP of the original, the tactics screen's helpers and
// callbacks: per-slot accessors and edits (each edit first keeps an undo copy
// of the formation), player names for the squad lists, applying the screen's
// edits to the game (CTACTICS, live-match substitutions), the screen
// history's callbacks, and the player-instructions dialog (its builder and
// its event handler). Each routine leaves what the original leaves (see
// guest_abi.hpp).
#include "guest_abi.hpp"

namespace cm::aot::tactics_screens::misc {
// The tactics screen object (0xc6b4 bytes, built by 0x103c5a90).
namespace field {
constexpr std::uint32_t club = 0x0;                // -> the club whose tactics are shown
constexpr std::uint32_t match = 0x4;               // -> the live match, 0 outside a match
constexpr std::uint32_t managed = 0x8;             // int8: CTACTICS's `managed` argument
constexpr std::uint32_t match_state = 0xc;         // int32: > -1 while the match lists hold the squads
constexpr std::uint32_t match_lists = 0x1a8;       // per side: 0x664 bytes, 20 slots of 0x4c
constexpr std::uint32_t substitutes = 0xe5e;       // uint8: high nibble = substitutes allowed
constexpr std::uint32_t limit_squad = 0xe60;       // int8: only 11 + substitutes slots in use
constexpr std::uint32_t opponent = 0xe64;          // -> the opposing club in a match, or 0
constexpr std::uint32_t side = 0xe68;              // uint8: our side in the match
constexpr std::uint32_t opponent_side = 0xe69;     // uint8: the opponent's side
constexpr std::uint32_t linked = 0xe6c;            // -> an object updated from the formation, or 0
constexpr std::uint32_t read_only = 0xec0;         // int8: nothing may be changed
constexpr std::uint32_t formation = 0xec4;         // the edited formation (0x3c90 bytes, record first)
constexpr std::uint32_t team_settings = 0x27a4;    // uint8 x2, uint16[2] x2, uint8: written back by apply()
constexpr std::uint32_t substitution_from = 0x4b00;  // uint8[9]: slot leaving the pitch
constexpr std::uint32_t substitution_to = 0x4b09;    // uint8[9]: slot coming on
constexpr std::uint32_t substitution_count = 0x4b12; // int8
constexpr std::uint32_t pick = 0x4b14;             // int32: staff id of the pick, -1 none
constexpr std::uint32_t captains = 0x4b18;         // staff records [2]
constexpr std::uint32_t takers = 0x4b20;           // staff records [5][2]
constexpr std::uint32_t lineup = 0x4b48;           // uint8[11]
constexpr std::uint32_t changed = 0x4b53;          // uint8: an edit was made
constexpr std::uint32_t undo = 0x4b54;             // copy of `formation` before the last edit
constexpr std::uint32_t undo_valid = 0xc6b1;       // uint8: `undo` holds a copy
// A match-list slot (0x4c bytes).
constexpr std::uint32_t slot_player = 0x0, slot_name = 0x8, slot_state = 0x1d;
}
constexpr std::uint32_t side_size = 0x664, slot_size = 0x4c;
constexpr std::int8_t squad_slots = 20;

// 0x103b3b20: whether the screen belongs to a live match.
std::uint32_t in_match(guest::Cpu& c, std::uint32_t screen);
// 0x103bac20: a formation slot's role bits.
std::uint32_t slot_role(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot);
// 0x103bac60: a formation slot's instruction fields (r3 high word, r4 low word).
std::uint64_t slot_instructions(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot);
// 0x103c4830: the staff record of a formation slot's player, or 0.
std::uint32_t slot_staff(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot);
// 0x103beae0: history callback of a read-only tactics screen when it is left:
// destroys the screen object and forgets it.
void leave_read_only(guest::Cpu& c);
// 0x103beb40: history callback of an editable tactics screen when the history
// is dropped: applies the edits, destroys the screen object and forgets it.
void leave_editable(guest::Cpu& c);
// 0x103c82a0: sets a slot's instruction fields (keeping an undo copy); 1 when set.
std::uint32_t set_slot_instructions(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t high,
                                    std::uint32_t low);
// 0x103c8c70: sets a formation slot's role bits (keeping an undo copy); 1 when set.
std::uint32_t set_slot_role(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t role);
// 0x103c8d80: puts a staff record in a formation slot; 1 when set.
std::uint32_t set_slot_staff(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t staff);
// 0x103c9720: whether a squad slot is in use (the first 11 plus the
// substitutes allowed, when the squad is limited).
std::uint32_t slot_in_use(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot);
// 0x103cac10: a squad slot's player in the live match (the opponent's with
// `opponent`), -1 for none.
std::uint32_t match_player(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t opponent);
// 0x103c9260: a squad slot's player name into `text` (format `format`; 5
// switches to format 2 when another player of the club shares the name), or
// with `no_position` 0 the slot's position name when it has no player.
std::uint32_t slot_name(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t text,
                        std::uint32_t size, std::uint32_t no_position, std::uint32_t format, std::uint32_t opponent);
// 0x103c95b0: a squad slot's position and player name, "%s %s".
std::uint32_t slot_label(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t text,
                         std::uint32_t size, std::uint32_t format, std::uint32_t opponent);
// 0x103c5b70: applies the screen's edits to the game: zones, team settings,
// pick, set-piece takers, line-up, captains and, in a match, the
// substitutions; 1 when applied.
std::uint32_t apply(guest::Cpu& c, std::uint32_t screen);
// 0x103cb760: makes a substitution in the live match; 1 when made.
std::uint32_t substitute(guest::Cpu& c, std::uint32_t screen, std::uint32_t from, std::uint32_t to);
// 0x103c4870: the player-instructions dialog's event handler.
std::uint32_t instructions_event(guest::Cpu& c, std::uint32_t control);
// 0x103c2010: builds the player-instructions dialog.
std::uint32_t instructions_build(guest::Cpu& c);
}
