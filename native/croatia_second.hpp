#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
namespace croatia_second {
// Handles the Mac second-division A entries for the national 3.89 competition.
bool dispatch(State&, std::uint32_t address);
}
}
