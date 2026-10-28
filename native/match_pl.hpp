#pragma once
#include "guest_values.hpp"

namespace cm::aot::match_pl {
using guest::Cpu;
std::uint32_t initialize(Cpu&, std::uint32_t player);
std::uint32_t destroy(Cpu&, std::uint32_t player, std::int16_t release);
void clear_match_target(Cpu&, std::uint32_t player);
void acquire_ball(Cpu&, std::uint32_t player);
std::uint32_t at_attacking_goal(Cpu&, std::uint32_t player);
std::int32_t team_instruction(Cpu&, std::uint32_t player);
std::uint32_t in_goal_area(Cpu&, std::uint32_t player, std::int8_t side);
std::uint32_t coordinates_in_goal_area(Cpu&, std::int8_t side, std::int8_t column, std::int8_t row);
void advance_row(Cpu&, std::uint32_t player, std::int8_t immediate);
std::uint32_t on_goal_line(Cpu&, std::uint32_t player);
std::uint32_t in_defensive_third(Cpu&, std::uint32_t player);
std::uint32_t wide_column(Cpu&, std::int8_t column);
std::uint32_t in_wide_column(Cpu&, std::uint32_t player);
std::uint32_t in_attacking_third(Cpu&, std::uint32_t player);
std::uint32_t in_attacking_band(Cpu&, std::uint32_t player);
std::uint32_t in_own_half(Cpu&, std::uint32_t player);
std::uint32_t row_in_own_half(Cpu&, std::uint32_t player, std::int8_t row);
std::uint32_t team_crossed_half(Cpu&, std::uint32_t player);
std::uint32_t both_roles_masked(Cpu&, std::uint32_t player);
std::int32_t role_distance(Cpu&, std::uint32_t player);
std::int32_t role_rank(Cpu&, std::uint32_t flags);
}
