#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::injury_bootstrap {
inline constexpr std::uint32_t constructor_entry=0x101840e0;
inline constexpr std::uint32_t destructor_entry=0x10184310;
inline constexpr std::uint32_t club_thresholds_entry=0x101843d0;
inline constexpr std::uint32_t staff_flag_entry=0x10187a20;
inline constexpr std::uint32_t availability_entry=0x10188490;
inline constexpr std::uint32_t population_entry=0x101885f0;
inline constexpr std::array<std::uint32_t,6> entries{constructor_entry,destructor_entry,club_thresholds_entry,staff_flag_entry,availability_entry,population_entry};
void construct_controller(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void destroy_controller(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void initialize_club_thresholds(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void set_staff_injury_flag(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void staff_availability_gate(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void populate_staff_records(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
