#pragma once
#include "guest_abi.hpp"

namespace cm::aot::eng_prm {
std::uint32_t construct(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t destroy(guest::Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t calendar(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t collect_clubs(guest::Cpu&, std::uint32_t);
std::uint32_t configure(guest::Cpu&, std::uint32_t);
void next_season(guest::Cpu&, std::uint32_t);
void promote_without_conference(guest::Cpu&, std::uint32_t);
void promote_with_conference(guest::Cpu&, std::uint32_t);
void link_divisions(guest::Cpu&, std::uint32_t, std::uint32_t);
}
