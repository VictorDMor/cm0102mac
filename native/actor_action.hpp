#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::actor_action {
inline constexpr std::array<std::uint32_t,5> entries{0x102775e0,0x10278220,0x10281780,0x102818f0,0x10281af0};
// Complete actor action routing, selection and destination/turn support family.
// Preserve live classifications, source ordering, pool state and legacy ABI.
void route_actor_action(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void select_primary_action(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void can_reach_destination(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void choose_turn_side(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void select_secondary_action(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
