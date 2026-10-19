#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::actor_policy {
inline constexpr std::array<std::uint32_t,5> entries{0x101f5280,0x10285750,0x10285790,0x102857c0,0x10285ef0};
// Complete actor context queries, trait bias and linked-opponent policy.
// Preserve live classifications, source ordering, pool state and legacy ABI.
void context_control(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void column_excess(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void phase_marker(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void adjust_action_bias(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void find_linked_opponent(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
