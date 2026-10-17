#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::actor_attributes {
inline constexpr std::uint32_t refresh_entry = 0x10235ea0;
inline constexpr std::uint32_t maximum_entry = 0x102373a0;
inline constexpr std::uint32_t recovery_entry = 0x10235db0;
inline constexpr std::array<std::uint32_t, 3> entries{refresh_entry, maximum_entry, recovery_entry};
// Refresh the ten transient action attributes from the actor's live baseline
// traits and condition. Preserve the shared random stream and ordered writes.
void refresh(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void signed_maximum(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
// Mark recovery and add the original random increment to live condition.
void recover_condition(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
