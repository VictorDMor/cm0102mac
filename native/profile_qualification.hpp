#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::profile_qualification {
inline constexpr std::uint32_t combine_qualification_masks_entry=0x10117360;
inline constexpr std::uint32_t classify_profile_qualifications_entry=0x10117490;
inline constexpr std::uint32_t candidate_category_reference_entry=0x1036f670;
inline constexpr std::uint32_t club_category_reference_entry=0x1036f6f0;
inline constexpr std::array<std::uint32_t,4> entries{combine_qualification_masks_entry,classify_profile_qualifications_entry,candidate_category_reference_entry,club_category_reference_entry};
void combine_qualification_masks(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void classify_profile_qualifications(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void candidate_category_reference(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void club_category_reference(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
