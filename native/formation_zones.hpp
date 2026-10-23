#pragma once
// FORMATION.CPP: where each player stands. The pitch is cut into 3 rows by 4
// columns of cells; for every cell the ball can be in, a formation keeps each
// player's spot (x 0..26, y 0..35) without and with the ball, derived from
// his position codes (0x10117690) or set by hand. A 9 by 12 grid of zone
// bytes (and its mirror image, for the opponent's view) summarises them.
//
// Cells and spots travel as the original's 4-byte structs passed by value:
// a cell is row << 16 | column, a spot x << 16 | y. Arguments the original
// keeps in a non-volatile register are taken as the whole register (callees
// save it into the stack image); the routine reads only their low byte or
// halfword, as the original does.
#include "formation.hpp"

namespace cm::aot::formation {
namespace field {
constexpr std::uint32_t spots = 0x166;           // uint16[11][2][3][4][2]: player, ball, row, column -> x, y
constexpr std::uint32_t zone_grid = 0x5f0;       // uint8[11][2][9][12]: y / 3 << 4 | x / 3
constexpr std::uint32_t mirrored_grid = 0xf38;   // the same grids turned round: (8 - x / 3, 11 - y / 3)
constexpr std::uint32_t players_spots = 0x60, ball_spots = 0x30;
}
constexpr std::uint16_t spot_rows = 3, spot_columns = 4, spot_x_max = 26, spot_y_max = 35;

// 0x10111310: puts a player at a position (without or with the ball); a new
// position without the ball updates the shape and which player holds it
// first; then recomputes his spots from his two positions.
std::uint32_t set_position(guest::Cpu& c, std::uint32_t formation, std::uint32_t player, std::uint32_t position,
                           std::int8_t with_ball);
// 0x10111630: whether the first player central (0x200) in the line of
// `position` exists and, when `table` is given, is named in one of its 20
// entries (0x1c4 bytes apart: a slot at +0x19, or -2 and the slot at +0x1a).
std::uint32_t central_in_line(guest::Cpu& c, std::uint32_t formation, std::int16_t position, std::uint32_t table);
// 0x10111890: swaps two players' positions, spots, instructions, roles and staff.
std::uint32_t swap_players(guest::Cpu& c, std::uint32_t formation, std::uint32_t first, std::uint32_t second);
// 0x10111ba0: the player slots in team order (position_order), into `order[11]`.
std::uint32_t order_players(guest::Cpu& c, std::uint32_t formation, std::uint32_t order, std::int32_t size);
// 0x10111d30: *displaced = 1 when another player holds the same position and
// took it earlier (smaller position_set); returns 0 when he took it later.
std::uint32_t displaced(guest::Cpu& c, std::uint32_t formation, std::int8_t player, std::uint32_t out);
// 0x10111e60: sets a player's spot for a ball state and cell by hand, and
// re-chooses his positions from the codes whose default spots fit his spots best.
std::uint32_t move_spot(guest::Cpu& c, std::uint32_t formation, std::uint32_t player, std::int8_t with_ball,
                        std::uint32_t cell, std::uint32_t spot);
// 0x101124c0: a player's spot for a ball state and cell, into out[2].
std::uint32_t spot(guest::Cpu& c, std::uint32_t formation, std::int8_t player, std::int8_t with_ball,
                   std::uint32_t cell, std::uint32_t out);
// 0x10112690 / 0x10112850: every player's spot in one cell (spots[11]).
std::uint32_t move_spots(guest::Cpu& c, std::uint32_t formation, std::uint32_t with_ball, std::uint32_t cell,
                         std::uint32_t spots);
std::uint32_t spots(guest::Cpu& c, std::uint32_t formation, std::uint32_t with_ball, std::uint32_t cell,
                    std::uint32_t out);
// 0x10112a00 / 0x10112b90: every player's spot in every cell (spots[3][4][11]).
std::uint32_t move_all_spots(guest::Cpu& c, std::uint32_t formation, std::uint32_t with_ball, std::uint32_t spots);
std::uint32_t all_spots(guest::Cpu& c, std::uint32_t formation, std::uint32_t with_ball, std::uint32_t out);
// 0x10112d20: the first player standing on `spot` in a cell, or -1.
std::int32_t player_at_spot(guest::Cpu& c, std::uint32_t formation, std::uint32_t with_ball, std::uint32_t cell,
                            std::uint32_t spot);
// 0x10112f00: swaps two players' spots in a cell.
std::uint32_t swap_spots(guest::Cpu& c, std::uint32_t formation, std::uint32_t first, std::uint32_t second,
                         std::uint32_t with_ball, std::uint32_t cell);
// 0x10113160: mirrors a cell's spots into the cell of the opposite row,
// swapping the outermost players of each line left to right.
std::uint32_t mirror_spots(guest::Cpu& c, std::uint32_t formation, std::uint32_t with_ball, std::uint32_t cell);
// 0x10113430: whether any of a player's spots differs from its default.
std::uint32_t has_custom_spot(guest::Cpu& c, std::uint32_t formation, std::uint32_t player);
// 0x10113550: whether a player's spot in a cell differs from its default.
std::uint32_t custom_spot(guest::Cpu& c, std::uint32_t formation, std::int8_t player, std::uint32_t with_ball,
                          std::uint32_t cell);
// 0x10113940: *later = 1 when a later player shares the player's spot in a
// cell; returns 0 when an earlier one does.
std::uint32_t shared_spot(guest::Cpu& c, std::uint32_t formation, std::int8_t player, std::uint32_t with_ball,
                          std::uint32_t cell, std::uint32_t later);
// 0x10116560: rebuilds the zone grids from the spots.
std::uint32_t update_zone_grid(guest::Cpu& c, std::uint32_t formation);
// 0x10117360: the position between two position codes (middle line, middle
// side). `a_line` / `b_line` are the caller's r0 / r5: the original keeps
// them as the line index of a code without a line bit (and the code itself as
// its side index).
std::int16_t between_positions(guest::Cpu& c, std::uint32_t a, std::uint32_t b, std::uint32_t a_line,
                               std::uint32_t b_line);
// 0x10117490: the position bits a player plays best, from his position and
// side ratings (+0xf..+0x19).
std::uint32_t natural_positions(guest::Cpu& c, std::uint32_t player);
// 0x10117690: the default spot of a position code for a ball state and cell, into out[2].
std::uint32_t default_spot(guest::Cpu& c, std::int16_t position, std::uint32_t with_ball, std::uint32_t cell,
                           std::uint32_t out);
}
