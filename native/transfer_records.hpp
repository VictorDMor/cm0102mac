#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::transfer_records {
inline constexpr std::uint32_t construct_empty_offer_entry=0x10411e60;
inline constexpr std::uint32_t construct_generated_offer_entry=0x10411ea0;
inline constexpr std::uint32_t destroy_offer_entry=0x10411f10;
inline constexpr std::uint32_t read_offer_entry=0x10411fa0;
inline constexpr std::uint32_t write_offer_entry=0x104123e0;
inline constexpr std::uint32_t initialize_zero_fee_offer_entry=0x104163d0;
inline constexpr std::uint32_t initialize_priced_offer_entry=0x104164f0;
inline constexpr std::uint32_t offer_value_entry=0x10416710;
inline constexpr std::array<std::uint32_t,8> entries{construct_empty_offer_entry,construct_generated_offer_entry,destroy_offer_entry,read_offer_entry,write_offer_entry,initialize_zero_fee_offer_entry,initialize_priced_offer_entry,offer_value_entry};
void construct_empty_offer(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void construct_generated_offer(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void destroy_offer(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void read_offer(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void write_offer(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void initialize_zero_fee_offer(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void initialize_priced_offer(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void offer_value(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
