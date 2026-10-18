#pragma once
#include "semantic_dependency_dispatch.hpp"
#include <array>

namespace cm::aot::injury_daily {
inline constexpr std::uint32_t entry=0x10185410;
inline constexpr std::array<std::uint32_t,1> entries{entry};
void process_daily_injuries(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
}
