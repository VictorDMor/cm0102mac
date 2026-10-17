#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::injury_reporting {
inline constexpr std::uint32_t special_type_entry=0x1018f340;
inline constexpr std::uint32_t notice_text_entry=0x1018f470;
inline constexpr std::uint32_t match_context_entry=0x1018fbe0;
inline constexpr std::uint32_t clear_history_entry=0x1018fdf0;
inline constexpr std::array<std::uint32_t,4> entries{special_type_entry,notice_text_entry,match_context_entry,clear_history_entry};
void has_special_injury_type(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void compose_injury_notice(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void describe_injury_match(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void clear_injury_history(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
