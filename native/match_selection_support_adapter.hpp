#pragma once
#include "match_selection_support.hpp"

namespace cm::aot {
enum class MatchSelectionSupportOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct MatchSelectionSupportBoundaryObserver : SemanticGuestObserver {
    virtual ~MatchSelectionSupportBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, MatchSelectionSupportOutcome) = 0;
};
struct MatchSelectionSupportDiagnostics {
    bool original = false;
    std::array<bool, match_selection_support::entries.size()> entry_original{};
    std::array<std::uint64_t, match_selection_support::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    MatchSelectionSupportBoundaryObserver* observer = nullptr;
};
MatchSelectionSupportDiagnostics& match_selection_support_diagnostics(State&);
const MatchSelectionSupportDiagnostics& match_selection_support_diagnostics(const State&);
bool try_modern_match_selection_support(State&, std::uint32_t);
}
