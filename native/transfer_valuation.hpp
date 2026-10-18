#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::transfer_valuation {
inline constexpr std::uint32_t sample_offer_range_entry=0x10415e50;
inline constexpr std::uint32_t adjusted_offer_value_entry=0x10416840;
inline constexpr std::uint32_t positive_installment_total_entry=0x10416d30;
inline constexpr std::uint32_t staff_offer_status_entry=0x10416d60;
inline constexpr std::uint32_t equivalent_offer_terms_entry=0x10416e10;
inline constexpr std::uint32_t remove_exchange_staff_entry=0x104170f0;
inline constexpr std::uint32_t find_exchange_staff_entry=0x10417260;
inline constexpr std::uint32_t rebalance_offer_terms_entry=0x104172f0;
inline constexpr std::array<std::uint32_t,8> entries{sample_offer_range_entry,adjusted_offer_value_entry,positive_installment_total_entry,staff_offer_status_entry,equivalent_offer_terms_entry,remove_exchange_staff_entry,find_exchange_staff_entry,rebalance_offer_terms_entry};
void sample_offer_range(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void adjusted_offer_value(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void positive_installment_total(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
// The status query takes a staff ID in r4 and can update the main target's status.
void staff_offer_status(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void equivalent_offer_terms(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
// These exchange operations take a staff pointer in r4, unlike the status query.
void remove_exchange_staff(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void find_exchange_staff(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void rebalance_offer_terms(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
