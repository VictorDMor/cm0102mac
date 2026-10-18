#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::contract_queries {
inline constexpr std::uint32_t primary_entry = 0x10082f20;
inline constexpr std::uint32_t club_entry = 0x10082f90;
inline constexpr std::uint32_t secondary_entry = 0x10083110;
inline constexpr std::array<std::uint32_t, 3> entries{primary_entry, club_entry, secondary_entry};
void find_primary(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void find_for_club(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void find_secondary(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
