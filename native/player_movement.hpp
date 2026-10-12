#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::player_movement {
inline constexpr std::uint32_t entry=0x1022e7e0, noncarrier_phase=0x10234e40, end=0x10235da4;
inline constexpr std::array<std::uint32_t,1> entries{entry};
inline constexpr std::uint32_t carrier_phase=0x1022ec48;
inline constexpr std::uint32_t carrier_decision_phase=0x1022f514;
inline constexpr std::uint32_t carrier_interaction_phase=0x1022fa58;
inline constexpr std::uint32_t carrier_contact_phase=0x1022ffe0;
inline constexpr std::uint32_t carrier_contact_decision_phase=0x102303fc;
inline constexpr std::uint32_t carrier_contact_followup_phase=0x10230a20;
inline constexpr std::uint32_t carrier_restart_selection_phase=0x102311b4;
inline constexpr std::uint32_t common_return_phase=0x10235d88;
inline constexpr std::uint32_t carrier_no_contact_phase=0x10231768;
inline constexpr std::uint32_t carrier_play_decision_phase=0x10231c04;
inline constexpr std::uint32_t carrier_play_ready_phase=0x10231e60;
inline constexpr std::uint32_t carrier_repeated_contact_phase=0x102338f8;
inline constexpr std::uint32_t carrier_transfer_phase=0x10233f7c;
inline constexpr std::uint32_t carrier_distance_phase=0x102342c4;
inline constexpr std::uint32_t carrier_statistics_phase=0x102346f8;
inline constexpr std::uint32_t carrier_ratings_phase=0x10234a64;
inline constexpr std::uint32_t carrier_null_contact_phase=0x10234dec;
inline constexpr std::uint32_t carrier_kind17_phase=0x10232354;
inline constexpr std::uint32_t carrier_kind17_clear_phase=0x102327b4;
inline constexpr std::uint32_t carrier_kind18_phase=0x10232e30;
inline constexpr std::uint32_t carrier_kind15_phase=0x10233830;
inline constexpr std::uint32_t carrier_other_kind_phase=0x10233890;
inline constexpr std::uint32_t carrier_failed_contact_phase=0x102316c8;
inline constexpr std::uint32_t carrier_resolved_contact_phase=0x10231708;
enum class Preparation { returned, carrier, noncarrier };
void move(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
// Named internal phases retain the original frame and dependency ABI.
// Production dispatch enters only the complete move() coordinator.
void continue_noncarrier(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void return_movement_frame(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
Preparation prepare_movement(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_sampling(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_environment(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_interaction(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_contact(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_contact_outcome(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_contact_action(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_contact_followup(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_restart(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_possession(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_play_preparation(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_play_bias(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_kind17_contact(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_kind17_clear(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_kind18(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_simple_play(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_repeated_table(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_repeated_contact(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_transfer(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_distance(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_statistics(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_ratings(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void continue_carrier_null_contact(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
