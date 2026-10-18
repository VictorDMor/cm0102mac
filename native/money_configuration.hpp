#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::money_configuration {
inline constexpr std::uint32_t configure_currency_entry=0x1000a440;
inline constexpr std::array entries{configure_currency_entry};
void configure_currency(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
