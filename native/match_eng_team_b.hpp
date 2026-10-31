// Original entries of the team_b block.
#pragma once
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101ec890: the ball arrives on a cell (row 0..8, column 0..11): the two players
// the cell's search finds contest it. Returns the winner (a player pointer) or 0.
std::uint32_t resolve_contest(Cpu&, std::uint32_t match, std::uint32_t row, std::uint32_t column);
// 0x101ee530: whether an action kind is one of 0x25, 0x2a..0x32, 0x60, 0x61.
std::uint32_t protected_action(Cpu&, std::uint32_t match, std::uint32_t kind);
// 0x101ee680: how many of a side's players stand beyond a column (side 1: before it,
// side 0: past it), leaving out the pending player when asked.
std::uint32_t count_players_ahead(Cpu&, std::uint32_t match, std::uint32_t column, std::uint32_t side,
                                  std::uint32_t skip_pending);
// 0x101ee770: 0 when the player's cell is taken or an opposing player is as near
// as his reach, 2 when one of his own side beats it, else 1.
std::uint32_t lane_status(Cpu&, std::uint32_t match, std::uint32_t player);
// 0x101eeac0: whether a player may run to a cell (row, column).
std::uint32_t run_allowed(Cpu&, std::uint32_t match, std::uint32_t player, std::uint32_t target_row,
                          std::uint32_t target_column);
// 0x101eecb0: whether the cell beside a player (one column towards his goal line) has no marker.
std::uint32_t adjacent_cell_free(Cpu&, std::uint32_t match, std::uint32_t player);
// 0x101eee20: how many of a side's players are on the pitch; stored at match+side+4.
std::uint32_t count_side_players(Cpu&, std::uint32_t match, std::uint32_t side);
}
