#pragma once
#include <cstdint>

namespace cm::aot::guest { class Cpu; }

namespace cm::aot::croatia_first {
// CRO_A1.CPP in 3.89. Stage -1 returns the complete season calendar, and 0 / 1
// return a newly allocated calendar for the championship / relegation group.
std::uint32_t calendar(guest::Cpu&, std::uint32_t competition, std::uint32_t stage,
                       std::uint32_t count_out, std::uint32_t code_out, std::uint32_t enabled_out);
std::uint32_t configure(guest::Cpu&, std::uint32_t competition);
}
