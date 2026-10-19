#pragma once
#include "match_selection.hpp"

namespace cm::aot {
enum class MatchSelectionOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct MatchSelectionBoundaryObserver : SemanticGuestObserver {
    virtual ~MatchSelectionBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, MatchSelectionOutcome) = 0;
};
struct MatchSelectionDiagnostics {
    bool original = false;
    std::array<bool, match_selection::entries.size()> entry_original{};
    std::array<std::uint64_t, match_selection::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    MatchSelectionBoundaryObserver* observer = nullptr;
};
MatchSelectionDiagnostics& match_selection_diagnostics(State&);
const MatchSelectionDiagnostics& match_selection_diagnostics(const State&);
bool try_modern_match_selection(State&, std::uint32_t);
}
