#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::target_acceptance {
inline constexpr std::array<std::uint32_t,4> entries{0x10244c60,0x101eeac0,0x101eeeb0,0x101f09d0};
// Target-facing eligibility and corridor occupant accumulation. Preserve signed
// byte coordinates, live table reads, ordered pool samples and legacy ABI.
void can_face_target(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void target_position_allowed(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void route_obstruction(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void cell_occupants(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
