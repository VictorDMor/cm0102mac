#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
bool try_modern_contract_screens(State&, std::uint32_t entry);
bool contract_screens_lists(std::uint32_t entry);
void contract_screens_run_original(State&, std::uint32_t entry);
void contract_screens_run_rewrite(State&, std::uint32_t entry);
}
