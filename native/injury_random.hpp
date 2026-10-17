#pragma once
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::injury_random {
inline constexpr std::uint32_t entry=0x101864a0;
void draw_signed_bounded(State&,SemanticDependencyDispatch* =nullptr,SemanticGuestObserver* =nullptr);
}
