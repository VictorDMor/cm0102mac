#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::injury_readouts {
inline constexpr std::uint32_t history_entry=0x1018c1a0;
inline constexpr std::uint32_t risk_multiplier_entry=0x1018c270;
inline constexpr std::uint32_t risk_type_entry=0x1018c2f0;
inline constexpr std::uint32_t current_text_entry=0x1018c450;
inline constexpr std::uint32_t type_text_entry=0x1018c560;
inline constexpr std::uint32_t type_form_entry=0x1018d140;
inline constexpr std::uint32_t remaining_text_entry=0x1018d950;
inline constexpr std::uint32_t category_text_entry=0x1018dab0;
inline constexpr std::array<std::uint32_t,8> entries{history_entry,risk_multiplier_entry,risk_type_entry,current_text_entry,type_text_entry,type_form_entry,remaining_text_entry,category_text_entry};
void find_history_record(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void risk_multiplier(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void risk_type(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void describe_current_injury(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void describe_injury_type(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void injury_type_form(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void describe_remaining_time(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void describe_injury_category(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
