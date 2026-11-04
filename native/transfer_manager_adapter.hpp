#pragma once

#include "aot_runtime.hpp"

namespace cm::aot {
bool try_modern_transfer_manager(State&, std::uint32_t entry);
bool transfer_manager_lists(std::uint32_t entry);
bool transfer_manager_direct_calls_enabled();
void transfer_manager_run_original(State&, std::uint32_t entry);
void transfer_manager_run_rewrite(State&, std::uint32_t entry);
}
