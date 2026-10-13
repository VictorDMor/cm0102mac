#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::team_line {
inline constexpr std::array<std::uint32_t,5> entries{0x101d56f0,0x101d7d30,0x101e4e10,0x101e4ea0,0x101e4ed0};
void tactical_flags(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void line_adjustment_enabled(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void actor_column_excess(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void column_excess(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void row_lane_clear(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
