#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
namespace guest { class Cpu; }
namespace croatia_first {
// Returns true when the 3.89 rules handled this original instruction or method.
bool dispatch(State&, std::uint32_t address);
// Handles the historical upper-group qualification wording at the common news entry.
bool format_group_news(guest::Cpu&, std::uint32_t league, std::uint32_t kind);
}
}
