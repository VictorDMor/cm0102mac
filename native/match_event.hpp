#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::match_event {
inline constexpr std::array<std::uint32_t,9> entries{0x101e3180,0x101fea10,0x101ff1d0,0x101ff5f0,0x101ffb70,0x10201080,0x102010c0,0x10201180,0x10201e30};
// Complete original match-event pipeline, preserving queue/history order,
// context rewrites, recursive dependencies, live catalog and random-pool data.
void actor_address(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void ball_cell_side(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void qualify_event(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void sample_event_jitter(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void select_transition(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void same_actor_pair(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void compatible_actor_context(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void emit_event(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void classify_event(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
