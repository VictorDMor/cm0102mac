#pragma once
#include "file_controller_internal.hpp"
#include "runtime_lifecycle_adapter.hpp"

namespace cm::aot::runtime_lifecycle {
using Abi = file_controller::Abi;
inline constexpr auto entries = runtime_lifecycle_entries;
void register_destructor(Abi&);
void drain_destructors(Abi&);
void abort_runtime(Abi&);
void exit_runtime(Abi&);
void terminate_runtime(Abi&);
void raise_signal(Abi&);
}
