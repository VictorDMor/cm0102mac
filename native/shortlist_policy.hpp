#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::shortlist_policy {
inline constexpr std::uint32_t classify_contract_offer_entry=0x1036f170;
inline constexpr std::uint32_t check_candidate_country_policy_entry=0x1036f590;
inline constexpr std::uint32_t evaluate_row_contract_gate_entry=0x1036fbf0;
inline constexpr std::array<std::uint32_t,3> entries{classify_contract_offer_entry,check_candidate_country_policy_entry,evaluate_row_contract_gate_entry};
void classify_contract_offer(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void check_candidate_country_policy(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void evaluate_row_contract_gate(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
