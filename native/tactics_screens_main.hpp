#pragma once
// TACTICS_SCREENS.CPP of the original: the tactics screen's event handler
// (0x103baee0) and the edits it makes to the screen's state object: the
// formation being edited, its undo copy and clipboard, the captain and the
// substitutions made during a match, and the file dialogs' results (load,
// save, delete, presets). Also 0x103b3830, which opens the screen. Each
// routine leaves what the original leaves (see guest_abi.hpp).
//
// Char and short arguments are taken as the whole register: the original
// keeps them in non-volatile registers, whose full value callees save into
// the stack image; the routines read only their low byte or halfword.
#include "guest_abi.hpp"

namespace cm::aot::tactics_screens::main {
namespace field {
// The screen's state object (0xc6b4 bytes, constructed by 0x103c4b10).
constexpr std::uint32_t club = 0x0;             // -> the club whose tactics are shown
constexpr std::uint32_t match = 0x4;            // -> the match being played, 0 outside a match
constexpr std::uint32_t opened_from = 0x8;      // int8
constexpr std::uint32_t in_match = 0x9;         // int8
constexpr std::uint32_t preset = 0xc;           // int32, -1 none
constexpr std::uint32_t national = 0x10;        // int8
constexpr std::uint32_t side = 0xe68;           // int8: the match side shown
constexpr std::uint32_t own_side = 0xe69;       // int8
constexpr std::uint32_t team_kind = 0xe6a;      // int8
constexpr std::uint32_t read_only = 0xec0;      // int8: the formation cannot be edited
constexpr std::uint32_t formation = 0xec4;      // the formation record being edited (formation.hpp)
constexpr std::uint32_t view = 0x27a4;          // int8: 1 positions, 2 with the ball, 3 without
constexpr std::uint32_t previous_view = 0x27a5; // int8
constexpr std::uint32_t cell = 0x27a6;          // uint16 row, column: the pitch cell shown
constexpr std::uint32_t previous_cell = 0x27aa;
constexpr std::uint32_t players = 0x27b0;       // 20 match player entries of 0x1c4 bytes
constexpr std::uint32_t brought_on = 0x4b00;    // int8[9]: entries brought on as substitutes
constexpr std::uint32_t taken_off = 0x4b09;     // int8[9]: the entries they replaced
constexpr std::uint32_t substitutions = 0x4b12; // int8
constexpr std::uint32_t captain = 0x4b14;       // staff id
constexpr std::uint32_t takers = 0x4b20;        // -> staff[5][2]: the set-piece takers
constexpr std::uint32_t lineup = 0x4b48;        // int8[11]: the entry playing each formation place
constexpr std::uint32_t changed = 0x4b53;       // int8: edited since the last save
constexpr std::uint32_t undo = 0x4b54;          // the formation record before the last edit
constexpr std::uint32_t cell_clipboard = 0xc474;   // spots[11] copied from one cell
constexpr std::uint32_t all_clipboard = 0xc4a0;    // spots[3][4][11] copied from every cell
constexpr std::uint32_t clipboard = 0xc6b0;     // int8: 0 empty, 1 one cell, 2 every cell
constexpr std::uint32_t can_undo = 0xc6b1;      // int8
constexpr std::uint32_t size = 0xc6b4;
// A match player entry (0x1c4 bytes).
constexpr std::uint32_t entry_size = 0x1c4;
constexpr std::uint32_t entry_staff = 0x6c;     // -> the staff record
constexpr std::uint32_t entry_off = 0x1e;       // int8: substituted
constexpr std::uint32_t entry_on_minute = 0x1f; // int8: the minute he came on
constexpr std::uint32_t entry_off_minute = 0x20; // int8: the minute he went off
// The game root (TOC -0x7bb0): the current human at +0x2c58, and per human
// (0x2c0 bytes) the UI page shown at +0x24, whose controls (0x17c bytes)
// start at +0xba864.
constexpr std::uint32_t root_human = 0x2c58, root_page = 0x24, root_human_size = 0x2c0;
constexpr std::uint32_t page_controls = 0xba864, control_size = 0x17c;
// A formation record's name (+0) and its file's description (+0x33).
constexpr std::uint32_t formation_description = 0x33;
constexpr std::uint32_t formation_changed = 0x1885;  // int8
}

// 0x103b3830: opens the tactics screen for a club: allocates and constructs
// its state object (fields opened_from .. national), pushes the screen and
// passes it the arguments as page parameters. Asserts when `club` is 0 or
// when both `in_match` and `national` are set.
std::uint32_t open(guest::Cpu& c, std::uint32_t club, std::uint32_t opened_from, std::uint32_t match,
                   std::uint32_t in_match, std::uint32_t preset, std::uint32_t national);
// 0x103baee0: the screen's handler: the dialogs' answers, then one branch per
// control action and event. Returns the handler's code (-4 handled, -11
// redraw, -9 closed, ...).
std::uint32_t handle(guest::Cpu& c, std::uint32_t event);

// Control fields of the UI page the root's current human shows (by control
// index, -1 for none: 0xffffffff).
// 0x103be490 / 0x103be4e0: word +0x1c.
std::uint32_t control_1c(guest::Cpu& c, std::uint32_t root, std::uint32_t index);
std::uint32_t page_control_1c(guest::Cpu& c, std::uint32_t page, std::uint32_t index);
// 0x103be560 / 0x103be5b0: word +0x18.
std::uint32_t control_18(guest::Cpu& c, std::uint32_t root, std::uint32_t index);
std::uint32_t page_control_18(guest::Cpu& c, std::uint32_t page, std::uint32_t index);
// 0x103be630 / 0x103be680: word +0x14.
std::uint32_t control_14(guest::Cpu& c, std::uint32_t root, std::uint32_t index);
std::uint32_t page_control_14(guest::Cpu& c, std::uint32_t page, std::uint32_t index);
// 0x103be700 / 0x103be750: word +0x10.
std::uint32_t control_10(guest::Cpu& c, std::uint32_t root, std::uint32_t index);
std::uint32_t page_control_10(guest::Cpu& c, std::uint32_t page, std::uint32_t index);
// 0x103be7d0 / 0x103be820 / 0x103be880: copies 8 bytes from `source` to +0x60.
std::uint32_t set_control_60(guest::Cpu& c, std::uint32_t root, std::uint32_t index, std::uint32_t source);
std::uint32_t set_page_control_60(guest::Cpu& c, std::uint32_t page, std::uint32_t index, std::uint32_t source);
std::uint32_t copy_control_60(guest::Cpu& c, std::uint32_t control, std::uint32_t source);
// 0x103be8c0 / 0x103be910: int16 +0x76.
std::uint32_t control_76(guest::Cpu& c, std::uint32_t root, std::uint32_t index);
std::uint32_t page_control_76(guest::Cpu& c, std::uint32_t page, std::uint32_t index);

// The state object.
// 0x103be420: the player standing on `spot` in the shown cell (formation
// player_at_spot, with the ball in view 2), or -1.
std::uint32_t player_at_spot(guest::Cpu& c, std::uint32_t screen, std::uint32_t spot);
// 0x103be9c0: the formation's changed byte (+0x1885).
std::uint32_t formation_changed(guest::Cpu& c, std::uint32_t screen);
// 0x103bea00: whether the captain is one of the selected match entries (0x103c9810).
std::uint32_t captain_selected(guest::Cpu& c, std::uint32_t screen);
// 0x103bea50: saves the formation as a preset file (formation 0x10115cd0).
std::uint32_t save_preset(guest::Cpu& c, std::uint32_t screen, std::uint32_t path);
// 0x103bea90: passes an event to the club's handler 0x100c7a50; returns its r3:r4.
std::uint64_t club_event(guest::Cpu& c, std::uint32_t screen, std::uint32_t a, std::uint32_t b);
// 0x103bebb0: opens the team instructions screen (handler 0x103c0d10) on a
// copy of the state object.
void open_team_instructions(guest::Cpu& c, std::uint32_t screen);
// 0x103c1f30: opens the screen of handler 0x103c4870 (built by 0x103c2010)
// on a copy of the state object, with a flag.
void open_player_instructions(guest::Cpu& c, std::uint32_t screen, std::uint32_t flag);
// 0x103c5a90: copy-constructs a state object; asserts when `source` is 0.
std::uint32_t copy_construct(guest::Cpu& c, std::uint32_t screen, std::uint32_t source);
// 0x103c60a0: sets the view (1..3), keeping the previous view and cell.
std::uint32_t set_view(guest::Cpu& c, std::uint32_t screen, std::uint32_t view);
// 0x103c6210: makes a staff record the captain.
std::uint32_t set_captain(guest::Cpu& c, std::uint32_t screen, std::uint32_t staff);
// 0x103c6310: makes the best player on the pitch the captain.
std::uint32_t choose_captain(guest::Cpu& c, std::uint32_t screen);
// 0x103c64c0: the confirmation shown on leaving the screen during a match:
// substitutions, a captain chosen, set-piece takers off the pitch. 1 when shown.
std::uint32_t match_changes(guest::Cpu& c, std::uint32_t screen, std::uint32_t text);
// 0x103c6cb0: shows another pitch cell (row 0..2, column 0..3).
std::uint32_t set_cell(guest::Cpu& c, std::uint32_t screen, std::uint32_t cell);
// 0x103c7260: undoes the last edit.
std::uint32_t undo(guest::Cpu& c, std::uint32_t screen);
// 0x103c7380: puts match entry `entry` (0..19) in formation place `place` (0..10).
std::uint32_t set_lineup(guest::Cpu& c, std::uint32_t screen, std::uint32_t place, std::uint32_t entry);
// 0x103c7550: swaps the entries of two formation places.
std::uint32_t swap_lineup(guest::Cpu& c, std::uint32_t screen, std::uint32_t first, std::uint32_t second);
// 0x103c7ad0: loads a formation file (or a preset); 1 when loaded.
std::uint32_t load(guest::Cpu& c, std::uint32_t screen, std::uint32_t path, std::uint32_t preset);
// 0x103c7c10: saves the formation to a file, under the manager's name when `named`.
std::uint32_t save(guest::Cpu& c, std::uint32_t screen, std::uint32_t path, std::uint32_t named);
// 0x103c7df0: replaces the formation by a default one (index), keeping its name.
std::uint32_t set_default(guest::Cpu& c, std::uint32_t screen, std::uint32_t index);
// 0x103c8160 / 0x103c81e0: unlocks / locks the formation (its description is
// the manager's name when locked).
std::uint32_t unlock(guest::Cpu& c, std::uint32_t screen);
std::uint32_t lock(guest::Cpu& c, std::uint32_t screen);
// 0x103c8360 / 0x103c83f0: copies the shown cell's spots / every cell's spots.
std::uint32_t copy_cell(guest::Cpu& c, std::uint32_t screen);
std::uint32_t copy_all_cells(guest::Cpu& c, std::uint32_t screen);
// 0x103c8480: pastes the clipboard.
std::uint32_t paste(guest::Cpu& c, std::uint32_t screen);
// 0x103c85b0: mirrors the shown cell into the opposite row's cell, and shows it.
std::uint32_t mirror_cell(guest::Cpu& c, std::uint32_t screen);
// 0x103c8680: swaps two players (formation swap_players); returns 0.
std::uint32_t swap_players(guest::Cpu& c, std::uint32_t screen, std::uint32_t first, std::uint32_t second);
// 0x103c87b0: sets a player's position (formation set_position).
std::uint32_t set_position(guest::Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t position,
                           std::uint32_t with_ball);
// 0x103c88c0: swaps two players' spots in the shown cell; returns 0.
std::uint32_t swap_spots(guest::Cpu& c, std::uint32_t screen, std::uint32_t first, std::uint32_t second);
// 0x103c8a10: moves a player's spot in the shown cell.
std::uint32_t move_spot(guest::Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t spot);
// 0x103ca8d0: swaps two match entries (0..19), from the match's teams when
// `from_match`: a substitution when one is on the bench, else a swap of
// places (outside a match: two formation places).
std::uint32_t swap_entries(guest::Cpu& c, std::uint32_t screen, std::uint32_t first, std::uint32_t second,
                           std::uint32_t from_match);
// 0x103cb3c0: brings match entry `on` (on the bench) on for `off`; bringing
// back a player this screen took off undoes that substitution.
std::uint32_t substitute(guest::Cpu& c, std::uint32_t screen, std::uint32_t on, std::uint32_t off);
// 0x103cba70: the entry of the best rated player on the pitch, or -1.
std::uint32_t best_player(guest::Cpu& c, std::uint32_t screen);
// 0x103cf110: the 64-bit mask of instruction bit `index` (0..47), r3 high word.
std::uint64_t instruction_bit(std::uint32_t index);
// The same with the registers the frameless original leaves: r3:r4, r0 the
// sign-extended index, cr0 its last test (index 0..47 equal, else against 47).
void instruction_bit_registers(guest::Cpu& c);
}
