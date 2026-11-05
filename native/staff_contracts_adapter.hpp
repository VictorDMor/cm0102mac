#pragma once

#include <cstdint>

namespace cm::aot {
struct State;

bool staff_contracts_calls_directly();
bool try_modern_staff_contracts(State&, std::uint32_t entry);
void staff_contracts_run_original(State&, std::uint32_t entry);
void staff_contracts_run_rewrite(State&, std::uint32_t entry);
void staff_contracts_run_checked_rewrite(State&, std::uint32_t entry);
}
