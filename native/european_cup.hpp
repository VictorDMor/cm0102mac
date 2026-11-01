#pragma once
#include "guest_abi.hpp"

namespace cm::aot::european_cup {
std::uint32_t construct(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t destroy(guest::Cpu&, std::uint32_t, std::uint32_t);
void award_result(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t calendar(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
void announce(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t collect_qualifiers(guest::Cpu&, std::uint32_t);
void champion(guest::Cpu&, std::uint32_t);
std::uint32_t record_result(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
void next_season(guest::Cpu&, std::uint32_t);
std::uint32_t configure(guest::Cpu&, std::uint32_t);
std::uint32_t collect_entrants(guest::Cpu&, std::uint32_t);
void advance_stage(guest::Cpu&, std::uint32_t);
void first_group_draw(guest::Cpu&, std::uint32_t);
std::uint32_t random_below(guest::Cpu&, std::uint32_t);
void second_group_draw(guest::Cpu&, std::uint32_t, std::uint32_t);
void knockout_draw(guest::Cpu&, std::uint32_t);
std::uint32_t entered_clubs(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t);
void seed_clubs(guest::Cpu&, std::uint32_t);
void fixture_times(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
void compare_seeds(guest::Cpu&);
void stage_name(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
void compare_draw_order(guest::Cpu&);
}
