#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::injury_club_events {
inline constexpr std::uint32_t enqueue_entry=0x1018bb50;
inline constexpr std::uint32_t process_entry=0x1018bdd0;
inline constexpr std::uint32_t compare_entry=0x101900b0;
inline constexpr std::array<std::uint32_t,3> entries{enqueue_entry,process_entry,compare_entry};
void enqueue_club_event(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void process_club_events(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void compare_event_date(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
