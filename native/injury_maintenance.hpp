#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::injury_maintenance {
inline constexpr std::uint32_t apply_fitness_entry=0x10186a20;
inline constexpr std::uint32_t merge_pending_entry=0x10186cd0;
inline constexpr std::uint32_t find_pending_entry=0x10186e10;
inline constexpr std::uint32_t reset_staff_entry=0x10186ea0;
inline constexpr std::uint32_t apply_match_entry=0x10186fc0;
inline constexpr std::array entries{apply_fitness_entry,merge_pending_entry,find_pending_entry,reset_staff_entry,apply_match_entry};
void apply_staff_fitness(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void merge_pending_injury(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void find_pending_injury(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void reset_staff_physical_state(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
void apply_match_physical_state(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
}
