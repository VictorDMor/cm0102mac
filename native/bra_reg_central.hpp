#pragma once
#include "guest_abi.hpp"

namespace cm::aot::bra_reg_central {
std::uint32_t construct(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t destroy(guest::Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t calendar(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t collect_clubs(guest::Cpu&, std::uint32_t);
std::uint32_t configure(guest::Cpu&, std::uint32_t);
void next_season(guest::Cpu&, std::uint32_t);
void advance_stage(guest::Cpu&, std::uint32_t);
void build_knockout(guest::Cpu&, std::uint32_t);
std::uint32_t record_result(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
void announce(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
void champion(guest::Cpu&, std::uint32_t);
void seed_clubs(guest::Cpu&, std::uint32_t);
void fixture_times(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
void base_forward(guest::Cpu&);
}
