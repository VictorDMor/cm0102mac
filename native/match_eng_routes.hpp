#pragma once
// MATCH_ENG.CPP, routes: target routes, cell distances and the players standing in a corridor.
// Original entries of the routes block.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x10775540: |value| in r3 and the sign mask in r4; the leaf is run in line.
inline std::int32_t absolute_value(Cpu& c, std::int32_t value) {
    const auto mask = u(value >> 31);
    c.set(4, mask);
    const auto magnitude = (u(value) ^ mask) - mask;
    c.set(3, magnitude);
    return std::int32_t(magnitude);
}
// 0x101eeeb0: walks the line between two cells (row, column each) and counts the players on
// its cells; the pitch's edge ends the walk.
std::uint32_t count_line_occupants(Cpu&, std::uint32_t match, std::uint32_t row_1, std::uint32_t column_1,
                                   std::uint32_t row_2, std::uint32_t column_2);
// 0x101ef3c0: the same walk, asking each cell whether a player blocks the line; stops at the first.
std::uint32_t find_line_blocker(Cpu&, std::uint32_t match, std::uint32_t row_1, std::uint32_t column_1,
                                std::uint32_t row_2, std::uint32_t column_2);
// 0x101ef8d0: the table distance between two cells (rows and columns apart), or the pool's default.
double cell_distance(Cpu&, std::uint32_t row_a, std::uint32_t column_a, std::uint32_t row_b, std::uint32_t column_b);
// 0x101f09d0: counts the cell's players of a side that are on the pitch (r10 == 2), or returns 1
// as soon as one passes the test at 0x10239f10.
std::uint32_t cell_occupants(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t cell, std::uint32_t r7,
                             std::uint32_t r8, std::uint32_t r9, std::uint32_t r10);
// 0x101f0ad0: the same over 0x10272240's verdict (2: blocks, 1: slows); remembers the blocker.
std::uint32_t cell_obstruction(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t cell, std::uint32_t r7,
                               std::uint32_t r8, std::uint32_t r9, std::uint32_t r10);
// 0x101ef9a0: the route from a cell to the far goal mouth (column 11 when `attacking_down` is 1, else
// column 0, row 4); returns the first cell routine answer that is not 0, or 0.
std::uint32_t first_blocker_to_goal(Cpu&, std::uint32_t match, std::uint32_t row, std::uint32_t column, std::uint32_t r6,
                                    std::uint32_t attacking_down, std::uint32_t r8, std::uint32_t r9);
// 0x101f0090: the route between two cells; 1 as soon as a cell holds a player the test at
// 0x10239f10 accepts, else 0.
std::uint32_t first_taker_to_cell(Cpu&, std::uint32_t match, std::uint32_t row, std::uint32_t column, std::uint32_t target_row,
                                  std::uint32_t target_column, std::uint32_t flag, std::uint32_t r9, std::uint32_t r10);
// 0x101f0090 etc. above; the play routines:
// 0x101f0c00: when the ball has left the pitch, stops play and tells the target object which
// restart follows (a throw-in or kick for `side_flag`'s opponent); 1 if play was stopped (or
// already was), else 0.
std::uint32_t stop_for_ball_out(Cpu&, std::uint32_t match, std::uint32_t side_flag);
// 0x101f0dd0: whether a player's column is within the rear (flag 0) or front line's extreme.
std::uint32_t column_within_line(Cpu&, std::uint32_t match, std::uint32_t player);
// 0x101f0e80: the column of the front-most player found by find_line_extremes, or -1.
std::uint32_t highest_column_of_line(Cpu&, std::uint32_t match);
// 0x101f0ed0: the column of the rear-most player found by find_line_extremes, or 12.
std::uint32_t lowest_column_of_line(Cpu&, std::uint32_t match);
// 0x101f0f20: records, per side, the players with the lowest and highest columns among those on the pitch.
void find_line_extremes(Cpu&, std::uint32_t match);
// 0x101f1100: sends the ball to a random cell around (row, column) and lets the players react.
void scatter_ball(Cpu&, std::uint32_t match, std::uint32_t row, std::uint32_t column, std::uint32_t spread, std::uint32_t side,
                  std::uint32_t confined, std::uint32_t everyone, std::uint32_t keeper);
}
