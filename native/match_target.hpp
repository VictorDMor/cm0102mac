#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::match_target {
inline constexpr std::array<std::uint32_t,8> entries{0x101d5760,0x101e28b0,0x101e3060,0x101e3150,0x101e31a0,0x102860c0,0x102861b0,0x10113e10};
// Keep the original side normalization, live packed target reads, output
// aliasing, ready-actor filtering and goalkeeper tie rule at the Mac ABI.
void target_advance_enabled(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void resolve_target_cell(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void side_target_control(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void normalized_actor_side(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void is_nearest_candidate(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void controlled_target_override(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void uncontrolled_target_override(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void strategy_flags(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
