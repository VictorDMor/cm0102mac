#pragma once
#include "semantic_dependency_dispatch.hpp"
#include <array>

namespace cm::aot::ball_contact {
inline constexpr std::array<std::uint32_t,4> facade_entries{0x101f5050,0x101f50c0,0x101f5110,0x101f5180};
inline constexpr std::uint32_t description_entry=0x101f5350, failure_entry=0x10270320;
// These facades are not productive replacements until the contact block has
// complete original-reference and shared-engine acceptance.
void screen(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void attempt(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void apply(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void resolve(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void react_to_failure(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
