#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::actor_attributes {
inline constexpr std::uint32_t refresh_entry = 0x10235ea0;
inline constexpr std::uint32_t maximum_entry = 0x102373a0;
inline constexpr std::uint32_t recovery_entry = 0x10235db0;
inline constexpr std::uint32_t periodic_entry = 0x102393c0;
inline constexpr std::uint32_t consumption_entry = 0x10239590;
inline constexpr std::uint32_t restoration_entry = 0x10239790;
inline constexpr std::uint32_t record_scale_entry = 0x10187650;
inline constexpr std::uint32_t availability_entry = 0x10187680;
inline constexpr std::uint32_t effective_condition_entry = 0x101878d0;
inline constexpr std::uint32_t record_refresh_entry = 0x10187a40;
inline constexpr std::uint32_t capacity_entry = 0x101877e0;
inline constexpr std::uint32_t capacity_assignment_entry = 0x10186a20;
inline constexpr std::uint32_t retirement_entry = 0x1018b9f0;
inline constexpr std::uint32_t reset_entry = 0x10186ea0;
inline constexpr std::uint32_t sampling_entry = 0x101864a0;
inline constexpr std::array<std::uint32_t, 15> entries{refresh_entry, maximum_entry, recovery_entry, periodic_entry, consumption_entry,
    restoration_entry, record_scale_entry, availability_entry, effective_condition_entry, record_refresh_entry, capacity_entry, capacity_assignment_entry, retirement_entry, reset_entry, sampling_entry};
// Refresh the ten transient action attributes from the actor's live baseline
// traits and condition. Preserve the shared random stream and ordered writes.
void refresh(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void signed_maximum(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
// Mark recovery and add the original random increment to live condition.
void recover_condition(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void refresh_condition(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void consume_condition(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void restore_condition(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void record_condition_scale(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void record_availability(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void effective_condition(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void refresh_condition_record(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void sample_condition_range(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void record_capacity(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void assign_capacity(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void retire_injury(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void reset_condition_record(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
