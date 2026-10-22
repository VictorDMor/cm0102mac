#pragma once
#include "aot_runtime.hpp"
#include "guest_context_transfer.hpp"

namespace cm::aot {
// Generated code that called an owner in place continues at the return
// address without the dispatcher, unless the innermost dispatcher stops there:
// a caller running original code up to that address must still get control.
inline bool continues_after_owner(const State& s, std::uint32_t returned) noexcept {
    return s.pc == returned && !(s.dispatch_scope && s.dispatch_scope->return_pc() == returned);
}
}
