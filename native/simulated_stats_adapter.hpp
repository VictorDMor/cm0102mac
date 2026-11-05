#pragma once

#include <cstdint>

namespace cm::aot {
struct State;

bool try_modern_simulated_stats(State&, std::uint32_t entry);
void simulated_stats_run_original(State&, std::uint32_t entry);
void simulated_stats_run_rewrite(State&, std::uint32_t entry);
}
