#pragma once

#include <cstdint>

namespace cm::aot {
struct State;
bool try_modern_por_second(State&, std::uint32_t);
void por_second_run_original(State&, std::uint32_t);
void por_second_run_rewrite(State&, std::uint32_t);
}
