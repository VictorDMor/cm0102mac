#pragma once
#include <cstdint>

namespace cm::aot {
namespace guest { class Cpu; }
namespace holiday_match_report {
// Read a completed fixture and its optional match report during the game-thread result broadcast.
void filed(guest::Cpu&, std::uint32_t fixture, std::uint32_t report);
}
}
