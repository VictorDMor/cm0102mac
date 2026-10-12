#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::player_decision {
inline constexpr std::uint32_t entry = 0x102836a0;
inline constexpr std::array<std::uint32_t, 1> entries{entry};
using DependencyDispatch = SemanticDependencyDispatch;
void decide(State&, DependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
