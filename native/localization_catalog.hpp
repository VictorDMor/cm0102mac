#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::localization_catalog {
inline constexpr std::uint32_t release_catalog_entry = 0x10726cc0;
inline constexpr std::uint32_t destroy_record_entry = 0x10726d10;
inline constexpr std::uint32_t language_availability_entry = 0x10726da0;
inline constexpr std::uint32_t initialize_catalog_entry = 0x10726e90;
inline constexpr std::array entries{release_catalog_entry,destroy_record_entry,language_availability_entry,initialize_catalog_entry};
void release_catalog(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void destroy_record(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void language_availability(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void initialize_catalog(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
