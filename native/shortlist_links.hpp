#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::shortlist_links {
inline constexpr std::uint32_t contains_staff_club_link_entry=0x10362d70;
inline constexpr std::uint32_t collect_club_links_entry=0x10362e00;
inline constexpr std::uint32_t collect_staff_links_entry=0x10362ef0;
inline constexpr std::uint32_t mark_primary_contract_entry=0x10363130;
inline constexpr std::uint32_t add_staff_club_link_entry=0x103632f0;
inline constexpr std::uint32_t reset_linked_club_rows_entry=0x10363b80;
inline constexpr std::uint32_t transition_staff_links_entry=0x10365590;
inline constexpr std::uint32_t clear_primary_contract_flag_one_entry=0x10365c30;
inline constexpr std::uint32_t set_primary_contract_flag_two_entry=0x10365c80;
inline constexpr std::uint32_t clear_primary_contract_flag_two_entry=0x10365cc0;
inline constexpr std::array<std::uint32_t,10> entries{contains_staff_club_link_entry,collect_club_links_entry,collect_staff_links_entry,mark_primary_contract_entry,add_staff_club_link_entry,reset_linked_club_rows_entry,transition_staff_links_entry,clear_primary_contract_flag_one_entry,set_primary_contract_flag_two_entry,clear_primary_contract_flag_two_entry};
// After a {staff, club} pair is appended to a link index of `count` pairs,
// moves it to its sorted place when that equals the original heapsort's
// result; returns false, leaving the index untouched, when the sort must run.
bool insert_last_pair(State&, std::uint32_t base, std::uint32_t count, bool club_first);
void contains_staff_club_link(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void collect_club_links(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void collect_staff_links(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void mark_primary_contract(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void add_staff_club_link(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void reset_linked_club_rows(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void transition_staff_links(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void clear_primary_contract_flag_one(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void set_primary_contract_flag_two(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void clear_primary_contract_flag_two(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
