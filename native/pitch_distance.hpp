#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::pitch_distance {
inline constexpr std::array<std::uint32_t, 3> entries{0x101ef8d0, 0x1022e760, 0x10245160};
void point_to_point(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void actor_to_cell(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void actor_to_attacking_goal(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
