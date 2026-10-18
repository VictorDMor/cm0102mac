#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::contract_storage {
inline constexpr std::uint32_t bind_primary_entry=0x10078e70;
inline constexpr std::uint32_t reserve_entry=0x10078f00;
inline constexpr std::uint32_t initializer_entry=0x103727c0;
// Complete storage creation block; production integration is a separate milestone.
inline constexpr std::array<std::uint32_t,3> prepared_entries{bind_primary_entry,reserve_entry,initializer_entry};
void bind_primary(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void reserve_record(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void initialize_contract(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
