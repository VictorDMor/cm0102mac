#pragma once
// TACTICS_SCREENS.CPP of the original: the team-instructions screen. The
// tactics screen keeps its state in one object (0xc6b4 bytes); the
// instructions screen works on a copy of it (0x103c0d10 handles its events,
// 0x103c0cf0 builds it): team options (mentality, passing, tackling, ...)
// as bits of the formation's word +0x588, set-piece takers and playmakers.
// OK copies the edited object back over the tactics screen's (0x103c11b0).
// Each routine leaves what the original leaves (see guest_abi.hpp).
#include "guest_abi.hpp"

namespace cm::aot::tactics_screens::instructions {
namespace field {
// The tactics screen's state object.
constexpr std::uint32_t club = 0x0;            // -> the club whose tactics are shown
constexpr std::uint32_t match = 0x4;           // -> the live match, 0 outside a match
constexpr std::uint32_t managed = 0x8;         // int8: CTACTICS's `managed` argument
constexpr std::uint32_t match_state = 0xc;     // int32: > -1 while the match lists hold the squads
constexpr std::uint32_t teams = 0x14;          // the match lists (0xdfc bytes; per side 0x664 at +0x194)
constexpr std::uint32_t match_lists = 0x1a8;   // per side: 0x664 bytes, 20 slots of 0x4c
constexpr std::uint32_t settings = 0xe10;      // 0x50 bytes
constexpr std::uint32_t opponent = 0xe64;      // -> the opposing club in a match, or 0
constexpr std::uint32_t side = 0xe68;          // int8: our side in the match
constexpr std::uint32_t opponent_side = 0xe69; // int8: the opponent's side
constexpr std::uint32_t opponent_squad = 0xe70; // staff records [20] of the opponent
constexpr std::uint32_t read_only = 0xec0;     // int8: nothing may be changed
constexpr std::uint32_t tactics = 0xec4;       // the edited tactics (a Tactics record, below)
constexpr std::uint32_t players = 0x27b0;      // = tactics + Tactics::players
constexpr std::uint32_t playmakers = 0x4b18;   // = tactics + Tactics::playmakers
constexpr std::uint32_t takers = 0x4b20;       // = tactics + Tactics::takers
constexpr std::uint32_t changed = 0x4b53;      // = tactics + Tactics::changed
constexpr std::uint32_t undo = 0x4b54;         // the tactics before the last edit
constexpr std::uint32_t saved = 0x87e4;        // a third copy of the tactics
constexpr std::uint32_t cells = 0xc474;        // uint16[2][11]
constexpr std::uint32_t spots = 0xc4a0;        // uint16[2][132]
constexpr std::uint32_t free_kick = 0xc6b0;    // int8
constexpr std::uint32_t undo_valid = 0xc6b1;   // int8: `undo` holds a copy

// A Tactics record (0x3c90 bytes): the formation record (0x18e0 bytes,
// formation.hpp) and what the screen edits beside it.
namespace record {
constexpr std::uint32_t players = 0x18ec;      // 20 player entries of 0x1c4 bytes
constexpr std::uint32_t playmakers = 0x3c54;   // staff records [2]: first choice, alternative
                                               // (the team record's "captains", tactics.hpp)
constexpr std::uint32_t takers = 0x3c5c;       // staff records [5][2]: set piece, first and second taker
constexpr std::uint32_t changed = 0x3c8f;      // int8
}
}
// CR0 as the original's last compare leaves it (cmpw/cmpwi and
// cmplw/cmplwi): LT, GT or EQ, and XER[SO]. Callers read every CR field
// back through later mfcr stores, so each return path sets it.
inline void compare(guest::Cpu& c, std::int32_t a, std::int32_t b) {
    c.set_cr(0, std::uint8_t((a < b ? 8 : a > b ? 4 : 2) | (c.s.r<1>(guest::reg::xer_so) & 1)));
}
inline void compare_unsigned(guest::Cpu& c, std::uint32_t a, std::uint32_t b) {
    c.set_cr(0, std::uint8_t((a < b ? 8 : a > b ? 4 : 2) | (c.s.r<1>(guest::reg::xer_so) & 1)));
}

constexpr std::uint32_t tactics_size = 0x3c90, formation_size = 0x18e0, player_size = 0x1c4;
constexpr std::uint32_t side_size = 0x664, slot_size = 0x4c;

// Char and short arguments are taken as the register holds them (callees
// save the whole register into the stack image).

// 0x103baca0: asks 0x100c7d00 about the screen's club.
std::uint32_t club_query(guest::Cpu& c, std::uint32_t screen);
// 0x103b3b50: destroys the state object's three tactics records and, when
// `dispose` > 0, frees it; returns it.
std::uint32_t destroy(guest::Cpu& c, std::uint32_t screen, std::uint32_t dispose);
// 0x103b3bd0: destroys a tactics record (its player entries, then the
// formation) and, when `dispose` > 0, frees it; returns it.
std::uint32_t destroy_tactics(guest::Cpu& c, std::uint32_t tactics, std::uint32_t dispose);
// 0x103c0cc0: the formation's team options word (+0x588).
std::uint32_t team_options(guest::Cpu& c, std::uint32_t screen);
// 0x103c0cf0: builds the team-instructions screen (0x103becb0).
void open(guest::Cpu& c);
// 0x103becb0: builds the team-instructions screen: the dialog, the pitch,
// the option grid and the set-piece taker and playmaker lists.
void build(guest::Cpu& c);
// 0x103c0d10: the screen's event handler.
std::uint32_t handle(guest::Cpu& c, std::uint32_t event);
// 0x103c11b0: copies a whole state object (assignment); returns `to`.
std::uint32_t copy_screen(guest::Cpu& c, std::uint32_t to, std::uint32_t from);
// 0x103c1340: copies a Tactics record; returns `to`.
std::uint32_t copy_tactics(guest::Cpu& c, std::uint32_t to, std::uint32_t from);
// 0x103c14a0: copies a player entry (0x1c4 bytes); returns `to`.
std::uint32_t copy_player(guest::Cpu& c, std::uint32_t to, std::uint32_t from);
// 0x103c1980: copies a formation record; returns `to`.
std::uint32_t copy_formation(guest::Cpu& c, std::uint32_t to, std::uint32_t from);
// 0x103c1c00: copies the 0x50-byte settings block; returns `to`.
std::uint32_t copy_settings(guest::Cpu& c, std::uint32_t to, std::uint32_t from);
// 0x103c1d40: copies 10 bytes; returns `to`.
std::uint32_t copy_ten(guest::Cpu& c, std::uint32_t to, std::uint32_t from);
// 0x103c1da0: copies the match lists block (0xdfc bytes); returns `to`.
std::uint32_t copy_teams(guest::Cpu& c, std::uint32_t to, std::uint32_t from);
// 0x103c6d80: a set piece's (0..4) first or second (0..1) taker; asserts
// and returns 0 out of range.
std::uint32_t taker(guest::Cpu& c, std::uint32_t screen, std::uint32_t kind, std::uint32_t order);
// 0x103c6e80: sets a set-piece taker (keeping an undo copy); clearing the
// first moves the second up. 1 when set.
std::uint32_t set_taker(guest::Cpu& c, std::uint32_t screen, std::uint32_t kind, std::uint32_t order,
                        std::uint32_t staff);
// 0x103c7050: the first (0) or alternative (1) playmaker; asserts and returns 0 out of range.
std::uint32_t playmaker(guest::Cpu& c, std::uint32_t screen, std::uint32_t which);
// 0x103c7120: sets a playmaker (keeping an undo copy); clearing the first
// moves the alternative up. 1 when set.
std::uint32_t set_playmaker(guest::Cpu& c, std::uint32_t screen, std::uint32_t which, std::uint32_t staff);
// 0x103c80b0: sets a team option (formation 0x10113b60, keeping an undo
// copy, restored when refused); 1 when set.
std::uint32_t set_team_option(guest::Cpu& c, std::uint32_t screen, std::uint32_t option);
// 0x103c8e70: a squad slot's label (shirt number, or the slot's position
// name) into `text`; asserts and returns 0 on bad arguments, else 1.
std::uint32_t slot_number(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t text,
                          std::uint32_t size, std::uint32_t no_position, std::uint32_t opponent);
// 0x103c9810: whether a squad slot (0..19) holds a player who may play;
// writes his state to `out` when given.
std::uint32_t slot_available(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t out,
                             std::uint32_t opponent);
// 0x103ca4e0: the staff record in a squad slot (0..19), or 0; asserts out of range.
std::uint32_t slot_player(guest::Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t opponent);
// 0x103cbb30: draws the pitch in a rectangle, upright or across (9th
// argument), and writes the inner pitch's corners to the four outputs.
std::uint32_t draw_pitch(guest::Cpu& c, double scale, std::uint32_t left, std::uint32_t top, std::uint32_t right,
                         std::uint32_t bottom, std::uint32_t out_left, std::uint32_t out_top,
                         std::uint32_t out_right, std::uint32_t out_bottom);
}
