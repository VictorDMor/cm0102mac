#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::injury_progression {
inline constexpr std::uint32_t daily_entry=0x1018b2b0;
inline constexpr std::uint32_t notice_entry=0x1018dcb0;
inline constexpr std::uint32_t end_compare_entry=0x1018ff10;
inline constexpr std::uint32_t start_compare_entry=0x1018ffe0;
inline constexpr std::array<std::uint32_t,4> entries{daily_entry,notice_entry,end_compare_entry,start_compare_entry};
void process_daily_injuries(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void prepare_progress_notice(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void compare_queued_end(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void compare_queued_start(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
