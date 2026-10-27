#pragma once
#include "guest_abi.hpp"
namespace cm::aot::club_history {
using guest::Cpu;
std::uint32_t construct(Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t destroy(Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t record_competition(Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t league_history(Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t cup_history(Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t league_competitions(Cpu&, std::uint32_t);
std::uint32_t cup_competitions(Cpu&, std::uint32_t);
std::uint32_t load(Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t save(Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t current_league(Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t compare_ids(Cpu&, std::uint32_t, std::uint32_t);
}
