#pragma once
#include "shortlist_recruitment.hpp"

namespace cm::aot {
enum class ShortlistRecruitmentOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ShortlistRecruitmentBoundaryObserver : SemanticGuestObserver {
    virtual ~ShortlistRecruitmentBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ShortlistRecruitmentOutcome) = 0;
};
struct ShortlistRecruitmentDiagnostics {
    bool original = false;
    std::array<bool, shortlist_recruitment::entries.size()> entry_original{};
    std::array<std::uint64_t, shortlist_recruitment::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ShortlistRecruitmentBoundaryObserver* observer = nullptr;
};
ShortlistRecruitmentDiagnostics& shortlist_recruitment_diagnostics(State&);
const ShortlistRecruitmentDiagnostics& shortlist_recruitment_diagnostics(const State&);
bool try_modern_shortlist_recruitment(State&, std::uint32_t);
}
