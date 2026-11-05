#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
bool try_modern_african_nations(State&, std::uint32_t);
void african_nations_run_original(State&, std::uint32_t);
void african_nations_run_rewrite(State&, std::uint32_t);
}
