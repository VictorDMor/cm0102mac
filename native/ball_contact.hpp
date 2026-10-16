#pragma once
#include "semantic_dependency_dispatch.hpp"
#include <array>

namespace cm::aot::ball_contact {
inline constexpr std::array<std::uint32_t,4> facade_entries{0x101f5050,0x101f50c0,0x101f5110,0x101f5180};
inline constexpr std::uint32_t description_entry=0x101f5350, failure_entry=0x10270320;
inline constexpr std::uint32_t description_exit_phase=0x101f5800;
inline constexpr std::uint32_t description_players_phase=0x101f5a0c, description_weight_phase=0x101f5a2c;
inline constexpr std::uint32_t description_assessment_phase=0x101f5ea4;
inline constexpr std::uint32_t description_reaction_phase=0x101f5f58;
inline constexpr std::uint32_t description_next_phase=0x101f6914, description_test_phase=0x101f6918, description_return_phase=0x101f6924;
// These candidates enter production only after complete contact-block
// original-reference and shared-engine acceptance.
void screen(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void attempt(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void apply(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void resolve(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void react_to_failure(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void describe(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void prepare_description(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void select_description_exit(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void prepare_description_players(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void next_description_player(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void adjust_description_weight(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void assess_description_weight(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void select_description_reaction(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void return_description_frame(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
