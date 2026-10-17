#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::injury_fitness {
inline constexpr std::uint32_t condition_update_entry=0x10187450;
inline constexpr std::uint32_t condition_read_entry=0x10187650;
inline constexpr std::uint32_t recovery_phase_entry=0x10187680;
inline constexpr std::uint32_t fitness_read_entry=0x101877e0;
inline constexpr std::uint32_t effective_fitness_entry=0x101878d0;
inline constexpr std::uint32_t refresh_entry=0x10187a40;
inline constexpr std::array<std::uint32_t,6> entries{condition_update_entry,condition_read_entry,recovery_phase_entry,fitness_read_entry,effective_fitness_entry,refresh_entry};
void update_staff_condition(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void read_staff_condition(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void classify_injury_recovery(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void read_staff_fitness(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void read_effective_fitness(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void refresh_staff_fitness(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
