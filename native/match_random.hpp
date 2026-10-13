#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::match_random {
inline constexpr std::uint32_t entry = 0x101d3c90;
inline constexpr std::array<std::uint32_t, 1> entries{entry};
// Draw from the original shared pool. Signed bounds, recursive wide-range
// correction, live constants and guest-visible side effects remain Mac ABI.
void sample(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
