#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
bool try_modern_bra_nat_second(State&, std::uint32_t);
void bra_nat_second_run_original(State&, std::uint32_t);
void bra_nat_second_run_rewrite(State&, std::uint32_t);
}
