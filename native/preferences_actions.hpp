#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::preferences_actions {
inline constexpr std::uint32_t open_with_mode_entry = 0x1033ce70;
inline constexpr std::uint32_t open_from_game_state_entry = 0x1033cf20;
inline constexpr std::uint32_t dispatch_action_entry = 0x1033f840;
inline constexpr std::array entries{open_with_mode_entry, open_from_game_state_entry, dispatch_action_entry};
void open_with_mode(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void open_from_game_state(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void dispatch_action(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
