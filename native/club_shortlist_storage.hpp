#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::club_shortlist_storage {
inline constexpr std::uint32_t construct_shortlist_storage_entry=0x10356410;
inline constexpr std::uint32_t destroy_shortlist_storage_entry=0x103567b0;
inline constexpr std::uint32_t reset_shortlist_storage_entry=0x103568e0;
inline constexpr std::uint32_t sample_shortlist_range_entry=0x10356e40;
inline constexpr std::uint32_t load_shortlist_storage_entry=0x10358020;
inline constexpr std::uint32_t initialize_auxiliary_record_entry=0x10358af0;
inline constexpr std::uint32_t save_shortlist_storage_entry=0x10358b30;
inline constexpr std::array<std::uint32_t,7> entries{construct_shortlist_storage_entry,destroy_shortlist_storage_entry,reset_shortlist_storage_entry,sample_shortlist_range_entry,load_shortlist_storage_entry,initialize_auxiliary_record_entry,save_shortlist_storage_entry};
void construct_shortlist_storage(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void destroy_shortlist_storage(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void reset_shortlist_storage(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void sample_shortlist_range(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void load_shortlist_storage(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void initialize_auxiliary_record(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void save_shortlist_storage(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
