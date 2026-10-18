#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::preferences_screen {
inline constexpr std::uint32_t build_screen_entry=0x1033d030;
inline constexpr std::array entries{build_screen_entry};
void build_screen(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
