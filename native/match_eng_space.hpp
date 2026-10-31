#pragma once
// MATCH_ENG.CPP, space: target cells, the nearest players to a cell and the team's
// depth limits. Original entries of the space block. The pitch is 9 rows (0..8,
// across) by 12 columns (0..11, along), as the players' +0x105 / +0x106 bytes.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101e28b0: the cell a player should aim for, written as bytes to *row_at and
// *column_at: the player's own cell, or the formation table's cell for the
// ball's cell, moved forward or back by the team's tactics.
void home_cell(Cpu&, std::uint32_t match, std::uint32_t player, std::uint32_t row_at, std::uint32_t column_at,
               std::uint32_t flag);
// 0x101e3060: whether a side has the ball (last touch, holder or a free cell).
std::uint32_t side_has_ball(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101e31a0: whether no other eligible player of the player's side is nearer the cell.
std::uint32_t is_nearest_to_cell(Cpu&, std::uint32_t match, std::uint32_t player, std::uint32_t target_row,
                                 std::uint32_t target_column);
// 0x101e32e0: the side's nearest eligible player to the cell (a player on the cell wins at once), or 0.
std::uint32_t nearest_player_to_cell(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t target_row,
                                     std::uint32_t target_column, std::uint32_t any_condition);
// 0x101e3d50, 0x101e3de0, 0x101e3e70: a halfword from a per-side table indexed by
// the player's position (or previous position), 0 outside 0..10.
std::uint32_t position_value_a(Cpu&, std::uint32_t match, std::uint32_t player);
std::uint32_t position_value_b(Cpu&, std::uint32_t match, std::uint32_t player);
std::uint32_t previous_position_value_a(Cpu&, std::uint32_t match, std::uint32_t player);
// 0x101e3f10: the least distance from the side's eligible players to the cell.
double nearest_distance(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t target_row,
                        std::uint32_t target_column, std::uint32_t any_condition);
// 0x101e4020: a weighted count of the side's players near the cell (3 on it, 1 within range).
std::uint32_t count_nearby_players(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t target_row,
                                   std::uint32_t target_column, std::uint32_t any_condition);
// 0x101e4130: stores the rearmost columns of the two sides (+0x7925, +0x7926).
std::uint32_t update_line_limits(Cpu&, std::uint32_t match);
// 0x101e4e10: how far a player's column lies beyond the side's limit; 0 for the pending player.
std::uint32_t distance_behind_line(Cpu&, std::uint32_t match, std::uint32_t player);
// 0x101e4ed0: whether no opposing player blocks the row ahead of the column.
std::uint32_t lane_is_open(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t target_row,
                           std::uint32_t target_column);
}
