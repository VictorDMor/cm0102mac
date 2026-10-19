#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::range_sampling {
inline constexpr std::uint32_t match_entry = 0x10226520;
inline constexpr std::uint32_t general_entry = 0x107124e0;
inline constexpr std::array<std::uint32_t, 2> entries{match_entry, general_entry};
// Both draw from the original shared pool. Each retains its own captured slots,
// inlined wide-range corrections, recursive child, ordered accesses and Mac ABI.
void sample_match(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void sample_general(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
