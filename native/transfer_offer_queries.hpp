#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::transfer_offer_queries {
inline constexpr std::uint32_t append_staff_entry_entry=0x103e6290;
inline constexpr std::uint32_t count_club_staff_entries_entry=0x103e6360;
inline constexpr std::uint32_t contains_staff_entry_entry=0x103e6470;
inline constexpr std::uint32_t set_staff_contract_flag_entry=0x103ea250;
inline constexpr std::uint32_t clear_staff_contract_flag_entry=0x103ea270;
inline constexpr std::uint32_t query_staff_contract_flag_entry=0x103ea290;
inline constexpr std::uint32_t rejection_reason_from_squad_status_entry=0x103ea350;
inline constexpr std::uint32_t rejection_reason_allowed_entry=0x103ea440;
inline constexpr std::uint32_t describe_rejection_reason_entry=0x103ea600;
inline constexpr std::uint32_t squad_status_for_rejection_reason_entry=0x103ea760;
inline constexpr std::uint32_t collect_staff_offer_indices_entry=0x103eaea0;
inline constexpr std::uint32_t collect_club_staff_offer_indices_entry=0x103eb020;
inline constexpr std::uint32_t collect_secondary_staff_offer_indices_entry=0x103eb1f0;
inline constexpr std::uint32_t collect_club_offer_indices_entry=0x103eb360;
inline constexpr std::uint32_t primary_offer_slot_entry=0x103eb440;
inline constexpr std::uint32_t secondary_offer_slot_entry=0x103eb470;
inline constexpr std::uint32_t best_qualifying_offer_entry=0x103f4240;
inline constexpr std::uint32_t query_special_staff_offer_entry=0x103f43b0;
inline constexpr std::uint32_t offer_is_filtered_entry=0x103fb390;
inline constexpr std::array<std::uint32_t,19> entries{append_staff_entry_entry,count_club_staff_entries_entry,contains_staff_entry_entry,set_staff_contract_flag_entry,clear_staff_contract_flag_entry,query_staff_contract_flag_entry,rejection_reason_from_squad_status_entry,rejection_reason_allowed_entry,describe_rejection_reason_entry,squad_status_for_rejection_reason_entry,collect_staff_offer_indices_entry,collect_club_staff_offer_indices_entry,collect_secondary_staff_offer_indices_entry,collect_club_offer_indices_entry,primary_offer_slot_entry,secondary_offer_slot_entry,best_qualifying_offer_entry,query_special_staff_offer_entry,offer_is_filtered_entry};
void append_staff_entry(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void count_club_staff_entries(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void contains_staff_entry(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void set_staff_contract_flag(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void clear_staff_contract_flag(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void query_staff_contract_flag(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void rejection_reason_from_squad_status(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void rejection_reason_allowed(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void describe_rejection_reason(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void squad_status_for_rejection_reason(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void collect_staff_offer_indices(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void collect_club_staff_offer_indices(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void collect_secondary_staff_offer_indices(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void collect_club_offer_indices(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void primary_offer_slot(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void secondary_offer_slot(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void best_qualifying_offer(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void query_special_staff_offer(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void offer_is_filtered(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
