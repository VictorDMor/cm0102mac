#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::match_selection {
inline constexpr std::array<std::uint32_t,4> entries{0x101e1bd0,0x101e2100,0x101e22e0,0x101e25a0};
// Preserve live role/profile reads, random draw order, floating score ties and
// ordered packed-cell stores, including output/source aliases, at the Mac ABI.
void select_attribute_weighted_actor(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void select_role_weighted_actor(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void select_profile_weighted_actor(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void preset_target_cell(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
