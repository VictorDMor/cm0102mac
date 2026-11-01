#pragma once
// MATCH_ENG.CPP, team_a: the search for the best receiving player (0x101e8950), the search for the
// best player in the ball's cell (0x101eaf60) and two small helpers. The pitch is 9 rows (the
// players' +0x105 byte, 0..8) by 12 columns (+0x106, 0..11).
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101e8950: scores each player of the side that last touched the ball as the receiver of a pass
// of the given kind (the ninth argument, a byte on the caller's stack at +0x38), from the cell
// (origin_row, origin_column). Returns the best player (0 for none); its action code, "moved"
// flag, destination row and column and its score go to the five out pointers.
std::uint32_t best_pass_receiver(Cpu&, std::uint32_t match, std::uint32_t origin_row, std::uint32_t origin_column,
                                 std::uint32_t out_action, std::uint32_t out_flag, std::uint32_t out_row,
                                 std::uint32_t out_column, std::uint32_t out_score);
// 0x101eaf10: pow(base, exponent) through the C library glue.
double power(Cpu&, double base, double exponent);
}

namespace cm::aot::match_eng {
// 0x101eaf60: the best player of a side in the ball's cell for an action of the given kind. The
// ninth argument (the pointer at the caller's stack +0x38, r20) and the tenth (a byte at +0x3f) are
// read from the caller's frame.
std::uint32_t best_player_in_ball_cell(Cpu&, std::uint32_t match, std::uint32_t reference, std::uint32_t side,
                                       std::uint32_t kind, std::uint32_t arg_7, std::uint32_t arg_8,
                                       std::uint32_t out_score, std::uint32_t out_kind);
// 0x101ec7b0: a random mark character ('*', '-', '+', '.'); an assertion for any other draw.
std::uint32_t random_mark(Cpu&);
}
