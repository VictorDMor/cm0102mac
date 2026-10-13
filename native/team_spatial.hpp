#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::team_spatial {
inline constexpr std::array<std::uint32_t, 4> entries{
    0x101e32e0, 0x101e3f10, 0x101e4020, 0x101e4130};
void nearest_actor(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void nearest_distance(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void nearby_weight(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void refresh_column_limits(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
