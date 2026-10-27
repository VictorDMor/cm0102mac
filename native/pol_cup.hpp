#pragma once
#include "guest_abi.hpp"

namespace cm::aot::pol_cup {
std::uint32_t construct(guest::Cpu&, std::uint32_t object, std::uint32_t year, std::uint32_t definition);
std::uint32_t destroy(guest::Cpu&, std::uint32_t object, std::uint32_t deleting);
std::uint32_t calendar(guest::Cpu&, std::uint32_t object, std::uint32_t stage,
                       std::uint32_t rounds, std::uint32_t extra, std::uint32_t flag);
std::uint32_t select_clubs(guest::Cpu&, std::uint32_t object);
void next_season(guest::Cpu&, std::uint32_t object);
std::uint32_t configure(guest::Cpu&, std::uint32_t object);
}
