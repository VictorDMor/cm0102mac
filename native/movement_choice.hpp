#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::movement_choice {
inline constexpr std::array<std::uint32_t,4> entries{0x101d8260,0x101d8d60,0x101ee9c0,0x101eea30};
void choose_cell(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void adjustment_byte(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void outside_central_rows(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void inside_end_area(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
