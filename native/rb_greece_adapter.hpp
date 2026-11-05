#pragma once

#include <cstdint>

namespace cm::aot {
struct State;
bool try_modern_rb_greece(State&, std::uint32_t);
void rb_greece_run_original(State&, std::uint32_t);
void rb_greece_run_rewrite(State&, std::uint32_t);
}
