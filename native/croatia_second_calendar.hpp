#pragma once
#include <cstdint>

namespace cm::aot::guest { class Cpu; }

namespace cm::aot::croatia_second {
// The single 3.89 Second Division uses stage -1 for its entire calendar.
std::uint32_t calendar(guest::Cpu&, std::uint32_t competition, std::uint32_t stage,
                       std::uint32_t count_out, std::uint32_t code_out, std::uint32_t enabled_out);
std::uint32_t configure(guest::Cpu&, std::uint32_t competition);
}
