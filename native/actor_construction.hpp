#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::actor_construction {
inline constexpr std::array<std::uint32_t, 2> entries{0x1020e8f0, 0x10223ac0};
using DependencyDispatch = SemanticDependencyDispatch;
void prepare_actor(State&, DependencyDispatch* = nullptr);
void initialize_actor(State&, DependencyDispatch* = nullptr);
}
