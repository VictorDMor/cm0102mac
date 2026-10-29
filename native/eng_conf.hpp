#pragma once
#include "guest_abi.hpp"

namespace cm::aot::eng_conf {
std::uint32_t construct(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t destroy(guest::Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t calendar(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t collect_clubs(guest::Cpu&, std::uint32_t);
std::uint32_t configure(guest::Cpu&, std::uint32_t);
void next_season(guest::Cpu&, std::uint32_t);
}
