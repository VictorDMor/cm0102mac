#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::contract_draft {
inline constexpr std::uint32_t initialize_draft_entry=0x103721f0;
inline constexpr std::uint32_t prepare_new_contract_entry=0x10372260;
inline constexpr std::uint32_t prepare_from_primary_contract_entry=0x103723b0;
inline constexpr std::array<std::uint32_t,3> entries{initialize_draft_entry,prepare_new_contract_entry,prepare_from_primary_contract_entry};
void initialize_draft(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void prepare_new_contract(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void prepare_from_primary_contract(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
