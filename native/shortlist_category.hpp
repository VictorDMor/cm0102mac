#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::shortlist_category {
inline constexpr std::uint32_t eligible_for_category_entry=0x10365d10;
inline constexpr std::uint32_t count_and_prune_club_rows_entry=0x103660b0;
inline constexpr std::uint32_t refresh_unassigned_rows_entry=0x10366250;
inline constexpr std::uint32_t compare_pairs_by_second_entry=0x103663b0;
inline constexpr std::uint32_t compare_pairs_by_first_entry=0x10366410;
inline constexpr std::array<std::uint32_t,5> entries{eligible_for_category_entry,count_and_prune_club_rows_entry,refresh_unassigned_rows_entry,compare_pairs_by_second_entry,compare_pairs_by_first_entry};
void eligible_for_category(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void count_and_prune_club_rows(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void refresh_unassigned_rows(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void compare_pairs_by_second(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void compare_pairs_by_first(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
