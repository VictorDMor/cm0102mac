#pragma once
#include <cstdint>

namespace cm::aot::guest { class Cpu; }

namespace cm::aot::croatia_first {
// The pilot's first division links to the single 18-club second division of 3.89.
// Both entries check that link before changing club divisions or disposing of a season.
void exchange_divisions(guest::Cpu&, std::uint32_t league);
void rollover(guest::Cpu&, std::uint32_t league);
}
