#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::actor_order {
inline constexpr std::array<std::uint32_t, 5> entries{0x101d5670,0x101d56b0,0x101d9080,0x101d91b0,0x101d93a0};
void secondary_rating(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void primary_rating(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void build_queue(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void compare_actors(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void current_fitness(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
