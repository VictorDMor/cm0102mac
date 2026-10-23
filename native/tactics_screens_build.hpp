#pragma once
// TACTICS_SCREENS.CPP of the original: the tactics screen's construction.
// 0x103baec0 builds the screen (0x103b3c50: menus, panels, pitch and player
// lists); 0x103c4b10 constructs its state object; the routines here are the
// accessors and list callbacks the construction reaches. Each routine leaves
// what the original leaves (see guest_abi.hpp).
#include "guest_abi.hpp"

namespace cm::aot::tactics_screens::build {
namespace field {
// The tactics screen's state object (0xc6b4 bytes, constructed by 0x103c4b10).
constexpr std::uint32_t club = 0x0;            // -> the club whose tactics are shown
constexpr std::uint32_t match = 0x4;           // -> the match being played, 0 outside a match
constexpr std::uint32_t opened_from = 0x8;     // int8 argument of the constructor
constexpr std::uint32_t in_match = 0x9;        // int8 argument of the constructor
constexpr std::uint32_t preset = 0xc;          // int32 argument of the constructor, -1 none
constexpr std::uint32_t national = 0x10;       // int8 argument of the constructor
constexpr std::uint32_t limit_squad = 0xe60;   // int8: only 11 + substitutes entries in use
constexpr std::uint32_t side = 0xe68;          // int8: which side of the match is shown
constexpr std::uint32_t team_kind = 0xe6a;     // int8
constexpr std::uint32_t formation = 0xec4;     // the formation record being edited (formation.hpp)
constexpr std::uint32_t view = 0x27a4;         // int8: 1 positions, 2 with the ball, else without
constexpr std::uint32_t list_view = 0x27a5;    // int8
constexpr std::uint32_t cell = 0x27a6;         // uint16[2]: the pitch cell shown (row, column)
constexpr std::uint32_t ball_cell = 0x27aa;    // uint16[2]: the cell the ball is in
constexpr std::uint32_t players = 0x27b0;      // 20 player entries of 0x1c4 bytes (in-match copies)
constexpr std::uint32_t lineup = 0x4b48;       // int8[11]
constexpr std::uint32_t free_kick = 0xc6b0;    // int8
constexpr std::uint32_t player_entry_size = 0x1c4;
// Formation record bytes the screen reads.
constexpr std::uint32_t formation_locked = 0x33;   // int8
constexpr std::uint32_t formation_flag = 0x1886;   // int8
constexpr std::uint32_t formation_saved = 0x1884;  // int8
// CTACTICS (tactics.hpp): formation counts.
constexpr std::uint32_t extra_formation_count = 0x5b8, formation_count = 0x5c0;
// A match (0x103c5990 / 0x103c59c0): its two clubs.
constexpr std::uint32_t match_home = 0x1c, match_away = 0x20;
}
namespace detail {
// TACTICS_SCREENS.CPP's string pool (a TOC slot): the file name at +0, the
// assert format at +0x14, "Error" at +0x21.
constexpr std::int32_t pool_slot = -0x4c90;
// The game's assert macro at `at` (the first call), with the name and
// extension buffers at sp+name and sp+extension.
inline void fail(guest::Cpu& c, const guest::Frame& frame, std::uint32_t at, std::int32_t name,
                 std::int32_t extension, std::uint32_t line) {
    const auto pool = c.toc(pool_slot);
    guest::report_assertion(c, {at, pool, pool + 0x14, pool + 0x21, frame.at(name), frame.at(extension), line});
}
// Stores of a frameless routine below the stack pointer (`stw rN,-offset(r1)`).
inline void red_zone(guest::Cpu& c, std::initializer_list<std::pair<unsigned, std::int32_t>> saves) {
    for (const auto& [n, offset] : saves) c.put32(c.sp() + std::uint32_t(offset), c.r(n));
}
}

// Small accessors of the screen state (the arguments the original keeps as
// char or short are passed as the whole register).

// 0x103baa00: whether another player shares this player's spot in the shown cell (formation_zones shared_spot).
std::uint32_t shared_spot(guest::Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t later);
// 0x103baa70: copies the shown cell (or, with `with_ball`, the ball's cell) to `out`; returns `out`.
std::uint32_t copy_cell(guest::Cpu& c, std::uint32_t screen, std::uint32_t out, std::uint32_t with_ball);
// 0x103bab00: the first player at a position (formation player_at).
std::uint32_t player_at(guest::Cpu& c, std::uint32_t screen, std::uint32_t position);
// 0x103bab40: formation central_in_line, with the in-match player entries when in a match.
std::uint32_t central_in_line(guest::Cpu& c, std::uint32_t screen, std::uint32_t position);
// 0x103baba0: a player's position code (formation position).
std::uint32_t position(guest::Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t with_ball);
// 0x103babe0: formation displaced.
std::uint32_t displaced(guest::Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t out);
// 0x103bad10 / 0x103bada0: 1 unless the view is 1 (positions).
std::uint32_t not_positions_view(guest::Cpu& c, std::uint32_t screen);
// 0x103bad40: 1 when the view is not 1 and the free-kick byte is set.
std::uint32_t free_kick_view(guest::Cpu& c, std::uint32_t screen);
// 0x103bade0: 1 when the formation's byte +0x33 is set.
std::uint32_t formation_locked(guest::Cpu& c, std::uint32_t screen);
// 0x103bae10 / 0x103bae50: the formation's bytes +0x1886 and +0x1884 (saved).
std::uint32_t formation_flag(guest::Cpu& c, std::uint32_t screen);
std::uint32_t formation_saved(guest::Cpu& c, std::uint32_t screen);
// 0x103bae90: the number of formations (the first ones only with `first_only`).
std::uint32_t formation_total(guest::Cpu& c, std::uint32_t tactics, std::uint32_t first_only);
// 0x103baec0: builds the tactics screen (0x103b3c50).
void open_screen(guest::Cpu& c);
// 0x103c5960: a match byte per side (+0x77c5, +0x77c6).
std::uint32_t match_side_byte(guest::Cpu& c, std::uint32_t match, std::uint32_t side);
// 0x103c5990 / 0x103c59c0: the away (+0x20) and home (+0x1c) club of a match's fixture, or 0.
std::uint32_t match_away_club(guest::Cpu& c, std::uint32_t match);
std::uint32_t match_home_club(guest::Cpu& c, std::uint32_t match);
// 0x103c59f0: constructs a formation record with its 20 player entries; returns it.
std::uint32_t construct_formation(guest::Cpu& c, std::uint32_t formation);
// 0x103c5a40: constructs the two 0x664-byte team blocks at +0x120; returns the object.
std::uint32_t construct_teams(guest::Cpu& c, std::uint32_t object);
// 0x103c5e60: selects the screen's club (or the match side) in the UI; returns 1.
std::uint32_t select_club(guest::Cpu& c, std::uint32_t screen);
// 0x103c5f00: the screen's title into `text` (at least 100 bytes) and the
// club's two kit colours (0x100c7a50) into the halfwords at `first` and
// `second`; 0 and an assert on bad arguments.
std::uint32_t title(guest::Cpu& c, std::uint32_t screen, std::uint32_t text, std::uint32_t size, std::uint32_t first,
                    std::uint32_t second);
// 0x103c6180: the squad entry (0..19) holding the screen's selected player, or -1.
std::uint32_t selected_entry(guest::Cpu& c, std::uint32_t screen);
// 0x103c6450: the number of substitutes left in the match, or 0.
std::uint32_t substitutes_left(guest::Cpu& c, std::uint32_t screen);
// 0x103c6bd0: 1 when the team's competition kind (by team_kind) is 5.
std::uint32_t kind_is_five(guest::Cpu& c, std::uint32_t screen);
// 0x103c72c0: a lineup byte (index 0..10); asserts and returns -1 out of range.
std::uint32_t lineup_byte(guest::Cpu& c, std::uint32_t screen, std::uint32_t index);

// 0x103cea00: the index n (0..47) of a 64-bit value equal to 1 << n (r3 high
// word, r4 low word), or -1. Leaves r0, r5 and r6 as the original does.
std::uint32_t bit_index(guest::Cpu& c, std::uint32_t high, std::uint32_t low);

// --- lists: list and pitch callbacks (tactics_screens_build_lists.cpp) ---
// Squad entries (index 0..19) are the match's live ones (0x101f9440) with
// `live`, else the screen's copies at +0x27b0; most readers assert on a bad
// index and give 0 for an empty entry (status -5) or outside a match.
// 0x103c78a0: a player's spot (out[2]) in the ball's cell, for the list's view; 1, or 0 and an assert.
std::uint32_t list_spot(guest::Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t out);
// 0x103c79d0: a player's spot (out[2]) in the shown cell; 1, or 0 and an assert.
std::uint32_t shown_spot(guest::Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t out);
// 0x103c8b60: whether a player's spots differ from the defaults (all cells in view 1, else the shown cell).
std::uint32_t custom_spot(guest::Cpu& c, std::uint32_t screen, std::uint32_t player);
// 0x103c9970: whether a squad entry's status is -1.
std::uint32_t entry_unavailable(guest::Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live);
// 0x103c9aa0 / 0x103c9bd0 / 0x103ca120 / 0x103ca260: a squad entry's byte +0x24, +0x23,
// +0x1b (0 also for status -1) and signed +0xc.
std::uint32_t entry_byte_24(guest::Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live);
std::uint32_t entry_byte_23(guest::Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live);
std::uint32_t entry_byte_1b(guest::Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live);
std::uint32_t entry_byte_0c(guest::Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live);
// 0x103c9d00: a squad entry's marker text (the shared text buffer, its first
// numbered marker +0x22..+0x1f) and colour; clears *flag first.
std::uint32_t entry_label(guest::Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t flag,
                          std::uint32_t size, std::uint32_t colour, std::uint32_t live);
// 0x103c9f40: in a match, a squad entry's float +0x50 scaled and truncated;
// outside one (no preset), the squad player's value from 0x101878d0, -1 when
// 0x10187680 says 2.
std::uint32_t entry_rating(guest::Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live);
// 0x103ca390: whether a squad entry's club (0x101e5070) is the match's club +0x3100.
std::uint32_t entry_plays_for_match_club(guest::Cpu& c, std::uint32_t screen, std::uint32_t index,
                                         std::uint32_t live);
// 0x103ca730: the squad entry whose status is `value` (or, with status -2,
// whose 0x101e3f00 is, setting *found), -1 when none; `value` itself outside a match.
std::uint32_t entry_with_status(guest::Cpu& c, std::uint32_t screen, std::uint32_t value, std::uint32_t found,
                                std::uint32_t live);
// 0x103cad40: the staff record playing a squad entry's formation slot, or the
// player of its paired entry (0x103cac10) on the other side; 0 when none.
std::uint32_t entry_staff(guest::Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live);
// 0x103caf20: the other side's player paired with a squad entry, or 0.
std::uint32_t paired_entry(guest::Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live);
// 0x103ce330: a position code's point in the pitch rectangle (left, top,
// right, bottom), mirrored or not; 1, or 0 and an assert for a code without a line or side.
std::uint32_t position_point(guest::Cpu& c, std::uint32_t code, std::uint32_t left, std::uint32_t top,
                             std::uint32_t right, std::uint32_t bottom, std::uint32_t out_x, std::uint32_t out_y,
                             std::uint32_t mirrored);
// 0x103ce790: the two back corners of an arrow head from (x0, y0) to (x1, y1),
// `spread` radians either side and `length` long: (*out_x1, *out_y1) and the
// three more outputs passed on the stack (sp+0x38..0x40 of the caller); 1.
std::uint32_t arrow_head(guest::Cpu& c, std::uint32_t x0, std::uint32_t y0, std::uint32_t x1, std::uint32_t y1,
                         double spread, std::uint32_t length, std::uint32_t out_x1);
// --- end lists ---

// --- setup: the state object's constructor and its helpers (tactics_screens_build_setup.cpp) ---
// 0x103c4b10: constructs the screen's state object for a club (r4); `match`,
// `opened_from`, `in_match`, `preset` (-1 none) and `national` are stored at
// +0x4, +0x8, +0x9, +0xc, +0x10. Loads the formation, cells, pick, set-piece
// takers, line-up and captains to edit from a preset, a match in progress, or
// the club's tactics (a human's screen as he left it); returns the object.
// Asserts and returns early when `club` is 0 or the preset cannot be read.
std::uint32_t construct(guest::Cpu& c, std::uint32_t screen, std::uint32_t club, std::uint32_t match,
                        std::uint32_t opened_from, std::uint32_t in_match, std::uint32_t preset,
                        std::uint32_t national);
// 0x103cb0b0: lists the opponent's fit players with the positions of its
// formation into the screen (+0xe70, through 0x10210c50); 0 when there is no
// opponent or none of its players qualifies, else 1.
std::uint32_t list_opponents(guest::Cpu& c, std::uint32_t screen);
// 0x103c7690: picks the line-up: each of the 11 places gets the unused
// available squad entry with the best rank (0x1014d300); returns 1.
std::uint32_t pick_lineup(guest::Cpu& c, std::uint32_t screen);
// --- end setup ---

// --- screen: the builder 0x103b3c50 (tactics_screens_build_screen*.cpp) ---
// 0x103b3c50: builds the tactics screen (menus, tabs, squad list, pitch, buttons).
void build_screen(guest::Cpu& c);
// --- end screen ---
}
