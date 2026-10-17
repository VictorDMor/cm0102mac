#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::injury_persistence {
inline constexpr std::uint32_t load_entry=0x10184530;
inline constexpr std::uint32_t save_entry=0x10184e60;
inline constexpr std::array<std::uint32_t,2> entries{load_entry,save_entry};
void load_controller(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void save_controller(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
