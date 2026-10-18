#pragma once
#include "shortlist_assessment.hpp"

namespace cm::aot {
enum class ShortlistAssessmentOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ShortlistAssessmentBoundaryObserver : SemanticGuestObserver {
    virtual ~ShortlistAssessmentBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ShortlistAssessmentOutcome) = 0;
};
struct ShortlistAssessmentDiagnostics {
    bool original = false;
    std::array<bool, shortlist_assessment::entries.size()> entry_original{};
    std::array<std::uint64_t, shortlist_assessment::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ShortlistAssessmentBoundaryObserver* observer = nullptr;
};
ShortlistAssessmentDiagnostics& shortlist_assessment_diagnostics(State&);
const ShortlistAssessmentDiagnostics& shortlist_assessment_diagnostics(const State&);
bool try_modern_shortlist_assessment(State&, std::uint32_t);
}
