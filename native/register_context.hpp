#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::register_context {
// Suffix entries start at f18..f31; each operation covers that complete tail.
// Raw components publish the original architectural return/transfer destination.
// Production selection, accounting and host continuation routing belong to the
// dispatcher adapter, rather than these guest storage operations.
void save_floating_suffix(State&, unsigned first);
void restore_floating_suffix(State&, unsigned first);
void capture_nonlocal_context(State&);
void restore_nonlocal_context(State&);
}
