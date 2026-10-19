#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::localization_lookup {
inline constexpr std::uint32_t initialize_record_entry = 0x10727860;
inline constexpr std::uint32_t current_language_entry = 0x10727880;
inline constexpr std::uint32_t lookup_entry = 0x10728400;
inline constexpr std::array entries{initialize_record_entry,current_language_entry,lookup_entry};
void initialize_record(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void current_language(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void lookup(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
