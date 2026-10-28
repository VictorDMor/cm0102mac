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
std::int32_t larger_value(Cpu&, std::int32_t first, std::int32_t second);
std::uint32_t marking_trait(Cpu&, std::uint32_t player);
std::int32_t heading(Cpu&, std::uint32_t player);
std::uint32_t has_possession(Cpu&, std::uint32_t player);
std::uint32_t contains_marker(Cpu&, std::uint32_t player, std::uint32_t marker);
void remove_marker(Cpu&, std::uint32_t player, std::uint32_t marker);
std::uint32_t side(Cpu&, std::uint32_t player);
std::int32_t team_line(Cpu&, std::uint32_t player);
std::int32_t match_context(Cpu&, std::uint32_t player);
void add_pressure(Cpu&, std::uint32_t player, std::uint32_t amount);
void set_pressure(Cpu&, std::uint32_t player, std::uint32_t amount);
std::int32_t cached_role(Cpu&, std::uint32_t player);
std::uint32_t current_role(Cpu&, std::uint32_t player);
std::uint32_t previous_role(Cpu&, std::uint32_t player);
std::uint32_t secondary_role(Cpu&, std::uint32_t player);
std::uint64_t strategy_flags(Cpu&, std::uint32_t player);
std::uint64_t alternate_strategy_flags(Cpu&, std::uint32_t player);
std::uint32_t strategy_bit21(Cpu&, std::uint32_t player);
std::uint32_t strategy_bit25(Cpu&, std::uint32_t player);
std::uint32_t strategy_bit29(Cpu&, std::uint32_t player);
std::uint32_t strategy_bit27(Cpu&, std::uint32_t player);
std::uint32_t team_or_slot_status(Cpu&, std::uint32_t player);
std::uint32_t forward_option(Cpu&, std::uint32_t player);
std::uint32_t wide_option(Cpu&, std::uint32_t player);
double distance_to(Cpu&, std::uint32_t player, std::int8_t column, std::int8_t row);
double distance_to_goal(Cpu&, std::uint32_t player);
std::uint32_t near_own_goal(Cpu&, std::uint32_t player);
std::uint32_t can_face(Cpu&, std::uint32_t player, std::uint32_t column, std::uint32_t row);
void scale_intensity(Cpu&, std::uint32_t player, double factor);
std::int32_t selection_score(Cpu&, std::uint32_t player, std::uint32_t opponent);
void improve_condition(Cpu&, std::uint32_t player);
std::uint32_t sample_range(Cpu&, std::uint32_t bound);
void consume_condition(Cpu&, std::uint32_t player, std::int16_t action, double work);
void refresh_attributes(Cpu&, std::uint32_t player);
void refresh_condition(Cpu&, std::uint32_t player);
void restore_condition(Cpu&, std::uint32_t player, std::int8_t projection);
void set_heading(Cpu&, std::uint32_t player, std::uint32_t from_column, std::uint32_t from_row,
                 std::uint32_t to_column, std::uint32_t to_row);
void set_coordinates(Cpu&, std::uint32_t player, std::uint32_t column, std::uint32_t row);
std::uint32_t can_cover_goal(Cpu&, std::uint32_t player, std::uint32_t column, std::uint32_t row);
std::uint32_t turn_direction(Cpu&, std::uint32_t player, std::int16_t action, double duration, std::uint32_t penalty);
std::uint32_t marked_opponent(Cpu&, std::uint32_t player);
std::int32_t readiness_rating(Cpu&, std::uint32_t player);
void publish_incident(Cpu&, std::uint32_t player);
std::uint32_t adjust_action_bias(Cpu&, std::uint32_t player, std::uint32_t bias);
std::uint32_t adjust_morale(Cpu&, std::uint32_t player, std::uint32_t change);
std::uint32_t accepts_target(Cpu&, std::uint32_t player, std::uint32_t column, std::uint32_t row,
                            std::int8_t adaptive, std::uint32_t feedback, double time, double reach);
std::uint32_t react_to_contact_failure(Cpu&, std::uint32_t player, std::int16_t quality);
std::uint32_t accepts_response(Cpu&, std::uint32_t player, std::uint32_t kind, std::uint32_t opponent);
std::uint32_t sample_decision(Cpu&, std::uint32_t player, std::uint32_t trait);
}
