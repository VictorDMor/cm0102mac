#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::money_value {
inline constexpr std::uint32_t to_display_amount_entry=0x10009f20;
inline constexpr std::uint32_t from_display_amount_entry=0x1000a010;
inline constexpr std::uint32_t increase_amount_entry=0x1000a060;
inline constexpr std::uint32_t decrease_amount_entry=0x1000a170;
inline constexpr std::uint32_t select_standard_amount_entry=0x1000a270;
inline constexpr std::uint32_t select_table_amount_entry=0x1000a330;
inline constexpr std::uint32_t amount_quantum_entry=0x1000c770;
inline constexpr std::array entries{to_display_amount_entry,from_display_amount_entry,increase_amount_entry,decrease_amount_entry,select_standard_amount_entry,select_table_amount_entry,amount_quantum_entry};
void to_display_amount(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void from_display_amount(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void increase_amount(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void decrease_amount(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void select_standard_amount(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void select_table_amount(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void amount_quantum(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
