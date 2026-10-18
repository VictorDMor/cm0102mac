#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::shortlist_recruitment {
inline constexpr std::uint32_t distribute_staff_interest_entry=0x10363ca0;
inline constexpr std::uint32_t register_club_interest_entry=0x10363f60;
inline constexpr std::uint32_t consider_requested_club_interest_entry=0x10364d00;
inline constexpr std::array<std::uint32_t,3> entries{distribute_staff_interest_entry,register_club_interest_entry,consider_requested_club_interest_entry};
void distribute_staff_interest(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void register_club_interest(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void consider_requested_club_interest(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
