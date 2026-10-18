#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::contract_display {
inline constexpr std::uint32_t format_wage_text_entry=0x10372590;
inline constexpr std::uint32_t format_contract_end_entry=0x10372730;
inline constexpr std::array entries{format_wage_text_entry,format_contract_end_entry};
void format_wage_text(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void format_contract_end(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
