#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::contract_terms {
inline constexpr std::uint32_t sample_contract_range_entry=0x10374570;
inline constexpr std::uint32_t enable_option_entry=0x10374af0;
inline constexpr std::uint32_t set_bonus_entry=0x10374b40;
inline constexpr std::uint32_t clear_option_entry=0x10374b60;
inline constexpr std::uint32_t clear_bonus_entry=0x10374b80;
inline constexpr std::uint32_t reset_terms_entry=0x10374ba0;
inline constexpr std::uint32_t describe_bonus_entry=0x10374be0;
inline constexpr std::uint32_t describe_option_entry=0x10374cb0;
inline constexpr std::uint32_t is_expired_entry=0x10374ce0;
inline constexpr std::uint32_t end_date_delta_entry=0x10374d10;
inline constexpr std::uint32_t is_rolling_entry=0x10374d50;
inline constexpr std::uint32_t describe_end_entry=0x10374d80;
inline constexpr std::uint32_t bonus_name_entry=0x10380270;
inline constexpr std::uint32_t option_name_entry=0x10380300;
inline constexpr std::uint32_t supports_bonus_entry=0x103803b0;
inline constexpr std::uint32_t supports_option_entry=0x10380400;
inline constexpr std::uint32_t has_release_terms_entry=0x10380520;
inline constexpr std::uint32_t adjust_value_primary_entry=0x10380560;
inline constexpr std::uint32_t adjust_value_alternate_entry=0x10380630;
inline constexpr std::uint32_t set_release_amount_entry=0x10380700;
inline constexpr std::uint32_t expiry_band_entry=0x10380710;
inline constexpr std::array<std::uint32_t,21> entries{sample_contract_range_entry,enable_option_entry,set_bonus_entry,clear_option_entry,clear_bonus_entry,reset_terms_entry,describe_bonus_entry,describe_option_entry,is_expired_entry,end_date_delta_entry,is_rolling_entry,describe_end_entry,bonus_name_entry,option_name_entry,supports_bonus_entry,supports_option_entry,has_release_terms_entry,adjust_value_primary_entry,adjust_value_alternate_entry,set_release_amount_entry,expiry_band_entry};
void sample_contract_range(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void enable_option(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void set_bonus(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void clear_option(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void clear_bonus(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void reset_terms(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void describe_bonus(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void describe_option(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void is_expired(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void end_date_delta(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void is_rolling(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void describe_end(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void bonus_name(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void option_name(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void supports_bonus(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void supports_option(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void has_release_terms(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void adjust_value_primary(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void adjust_value_alternate(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void set_release_amount(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void expiry_band(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
