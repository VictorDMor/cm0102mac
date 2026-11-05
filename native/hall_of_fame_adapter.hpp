#pragma once

#include <cstdint>

namespace cm::aot {
struct State;
bool try_modern_hall_of_fame(State&, std::uint32_t);
void hall_of_fame_run_original(State&, std::uint32_t);
void hall_of_fame_run_rewrite(State&, std::uint32_t);
}
