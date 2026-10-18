#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::contract_review {
inline constexpr std::uint32_t reset_review_state_entry=0x1037f050;
inline constexpr std::uint32_t has_review_trigger_entry=0x1037f110;
inline constexpr std::uint32_t refresh_review_conditions_entry=0x1037f590;
inline constexpr std::uint32_t clear_review_condition_entry=0x1037f7d0;
inline constexpr std::uint32_t clear_review_conditions_entry=0x1037f830;
inline constexpr std::uint32_t add_status_bits_entry=0x1037f8a0;
inline constexpr std::uint32_t clear_status_bits_entry=0x1037fa50;
inline constexpr std::uint32_t reset_status_bits_entry=0x1037fa60;
inline constexpr std::uint32_t status_bits_entry=0x1037fa70;
inline constexpr std::uint32_t apply_review_conditions_entry=0x1037fa80;
inline constexpr std::uint32_t describe_squad_status_entry=0x10380c10;
inline constexpr std::uint32_t describe_release_summary_entry=0x10380d50;
inline constexpr std::uint32_t query_start_date_window_entry=0x10380f20;
inline constexpr std::array<std::uint32_t,13> entries{reset_review_state_entry,has_review_trigger_entry,refresh_review_conditions_entry,clear_review_condition_entry,clear_review_conditions_entry,add_status_bits_entry,clear_status_bits_entry,reset_status_bits_entry,status_bits_entry,apply_review_conditions_entry,describe_squad_status_entry,describe_release_summary_entry,query_start_date_window_entry};
void reset_review_state(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void has_review_trigger(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void refresh_review_conditions(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void clear_review_condition(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void clear_review_conditions(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void add_status_bits(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void clear_status_bits(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void reset_status_bits(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void status_bits(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void apply_review_conditions(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void describe_squad_status(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void describe_release_summary(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void query_start_date_window(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
