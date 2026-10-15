#pragma once
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::ball_action {
inline constexpr std::uint32_t entry = 0x1023cd20;
inline constexpr std::uint32_t orientation_phase = 0x1023d184;
inline constexpr std::uint32_t action_bias_phase = 0x1023d344;
inline constexpr std::uint32_t area_selection_phase = 0x1023d5e4;
inline constexpr std::uint32_t ball_state_selection_phase = 0x1023d8f8;
inline constexpr std::uint32_t low_ball_selection_phase = 0x1023de18;
inline constexpr std::uint32_t selected_action_phase = 0x1023e404;
inline constexpr std::uint32_t contact_selection_phase = 0x1023ea44;
inline constexpr std::uint32_t contact_resolution_phase = 0x1023ecf4;
inline constexpr std::uint32_t contact_outcome_phase = 0x1023f1e8;
inline constexpr std::uint32_t action_execution_phase = 0x1023fa5c;
inline constexpr std::uint32_t execution_message_phase = 0x1023ff48;
inline constexpr std::uint32_t receiver_resolution_phase = 0x10240178;
inline constexpr std::uint32_t receiver_outcome_phase = 0x1024060c;
inline constexpr std::uint32_t execution_result_phase = 0x10240ff8;
inline constexpr std::uint32_t result_message_phase = 0x10241454;
inline constexpr std::uint32_t result_resolution_phase = 0x10241680;
inline constexpr std::uint32_t result_outcome_phase = 0x10241b24;
inline constexpr std::uint32_t unopposed_result_phase = 0x102424ac;
inline constexpr std::uint32_t no_receiver_phase = 0x102425f4;
inline constexpr std::uint32_t final_effects_phase = 0x102426c8;
inline constexpr std::uint32_t regular_effects_phase = 0x10242cc0;
inline constexpr std::uint32_t success_reaction_phase = 0x10242f3c;
inline constexpr std::uint32_t unsuccessful_action_phase = 0x102433d4;
inline constexpr std::uint32_t return_phase = 0x10244c38;
inline constexpr std::uint32_t end = 0x10244c58;

// Components remain outside production dispatch until the complete coordinator
// has independent original-reference and shared-engine acceptance evidence.
void prepare(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void orient(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void face_action(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void select_area_action(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void select_elevated_action(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void select_low_ball_action(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void sample_selected_action(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void prepare_contact(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void resolve_contact(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void complete_contact(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void prepare_execution(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void describe_execution(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void resolve_receiver(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void complete_receiver_contact(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void prepare_execution_result(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void describe_execution_result(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void resolve_execution_result(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void complete_execution_result(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void complete_unopposed_execution(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void prepare_final_action(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void complete_special_action(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void complete_regular_action(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void react_to_completed_action(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
