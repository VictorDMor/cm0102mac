#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::money_text {
inline constexpr std::uint32_t selected_symbol_length_entry=0x10009ef0;
inline constexpr std::uint32_t display_setting_entry=0x1000b150;
inline constexpr std::uint32_t currency_label_entry=0x1000b160;
inline constexpr std::uint32_t format_amount_entry=0x1000b410;
inline constexpr std::array entries{selected_symbol_length_entry,display_setting_entry,currency_label_entry,format_amount_entry};
void selected_symbol_length(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void display_setting(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void currency_label(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void format_amount(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
