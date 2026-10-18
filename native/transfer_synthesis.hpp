#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::transfer_synthesis {
inline constexpr std::uint32_t synthesize_offer_entry=0x104126d0;
inline constexpr std::array<std::uint32_t,1> entries{synthesize_offer_entry};
// r3 destination, r4 staff, r5 buyer club, r6 optional source terms,
// r7 amount limit, r8 mode byte, r9 optional output-contract slot.
void synthesize_offer(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
