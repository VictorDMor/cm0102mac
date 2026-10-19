#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::actor_effects {
inline constexpr std::array<std::uint32_t,3> entries{0x10237b50,0x10285ed0,0x10285ee0};
// Complete periodic balance and temporary counter maintenance. Keep live
// statistic/role reads, signed narrowing, float rounding and shared pool order.
void update_balance(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void add_counter(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void assign_counter(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
