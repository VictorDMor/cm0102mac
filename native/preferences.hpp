#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::preferences {
inline constexpr std::uint32_t initialize_preferences_entry=0x1013f030;
inline constexpr std::uint32_t copy_preferences_entry=0x1013f410;
inline constexpr std::uint32_t destroy_preferences_entry=0x1013f4d0;
inline constexpr std::uint32_t set_defaults_entry=0x1013f570;
inline constexpr std::uint32_t select_option_entry=0x1013f680;
inline constexpr std::uint32_t set_language_entry=0x1013fb60;
inline constexpr std::uint32_t set_currency_entry=0x1013fc50;
inline constexpr std::uint32_t currency_for_language_entry=0x1013fd00;
inline constexpr std::uint32_t set_interval_entry=0x1013fe00;
inline constexpr std::uint32_t apply_preferences_entry=0x1013feb0;
inline constexpr std::uint32_t video_mode_entry=0x101406e0;
inline constexpr std::uint32_t set_video_mode_entry=0x101406f0;
inline constexpr std::array entries{initialize_preferences_entry,copy_preferences_entry,destroy_preferences_entry,set_defaults_entry,select_option_entry,set_language_entry,set_currency_entry,currency_for_language_entry,set_interval_entry,apply_preferences_entry,video_mode_entry,set_video_mode_entry};
void initialize_preferences(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void copy_preferences(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void destroy_preferences(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void set_defaults(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void select_option(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void set_language(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void set_currency(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void currency_for_language(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void set_interval(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void apply_preferences(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void video_mode(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void set_video_mode(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
