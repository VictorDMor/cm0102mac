#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cm::aot::guest { class Cpu; }

namespace cm::aot::holiday_status {
std::string league_position(guest::Cpu&, std::uint32_t club);
std::vector<std::string> absences(guest::Cpu&, std::uint32_t club);
std::vector<std::string> pending(guest::Cpu&, std::uint32_t manager);
}
