#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
class GuestDispatchScope;
// Host-only scope identity: normal dispatch and guest memory are unchanged.
GuestDispatchScope* set_quick_save_dispatch_boundary(GuestDispatchScope*) noexcept;
void run_quick_save_until(State&, std::uint32_t return_pc);
}
