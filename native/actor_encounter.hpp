#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::actor_encounter {
inline constexpr std::array<std::uint32_t,4> entries{0x101d6860,0x102865a0,0x10286650,0x102876c0};
// Complete original score scaling, five response policies and peer resolution.
// Keep live pool reads, score limits, contact effects and recursive dependencies.
void scale_team_scores(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void scale_actor_score(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void accept_response(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void resolve_peer(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
