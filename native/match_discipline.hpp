#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::match_discipline {
inline constexpr std::array<std::uint32_t, 6> entries{
    0x101f5110, 0x101f5180, 0x101f51e0, 0x1021bec0, 0x10221e00, 0x10223100};
void match_contact_penalty(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void match_single_actor_penalty(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void match_referee_response(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void single_actor_penalty(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void contact_penalty(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void referee_response(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
