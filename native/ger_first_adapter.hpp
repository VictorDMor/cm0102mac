#pragma once

#include <cstdint>

namespace cm::aot {
struct State;
bool try_modern_ger_first(State&, std::uint32_t);
void ger_first_run_original(State&, std::uint32_t);
void ger_first_run_rewrite(State&, std::uint32_t);
}
