#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::match_selection_support {
inline constexpr std::array<std::uint32_t,3> entries{0x101e1640,0x101e1a50,0x102875f0};
// Preserve context/mode bytes, live weights, ordered random draws and the
// score leaf's marker-sensitive formulas, scratch stores and floating status.
void select_context_weighted_actor(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void select_score_weighted_actor(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void actor_selection_score(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
