#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::shortlist_assessment {
inline constexpr std::uint32_t assess_candidate_contribution_entry=0x10361490;
inline constexpr std::uint32_t evaluate_club_interest_entry=0x10369080;
inline constexpr std::array<std::uint32_t,2> entries{assess_candidate_contribution_entry,evaluate_club_interest_entry};
// Original instructions retained, with separately checked source/data invariants.
inline constexpr std::array<std::uint32_t,17> structurally_unreachable{
    0x10369dfc,0x10369e00,0x10369e04,0x10369e08,0x10369e0c,0x10369e10,0x10369e14,
    0x10369e24,0x10369e28,0x10369e2c,0x1036bdd8,0x1036bdfc,0x1036be60,0x1036c858,
    0x1036cffc,0x1036d13c,0x1036d140};
void assess_candidate_contribution(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void evaluate_club_interest(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
