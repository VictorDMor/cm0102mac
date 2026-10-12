#pragma once
#include "match_discipline.hpp"

namespace cm::aot {
enum class MatchDisciplineOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct MatchDisciplineBoundaryObserver : SemanticGuestObserver {
    virtual ~MatchDisciplineBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, MatchDisciplineOutcome) = 0;
};
struct MatchDisciplineDiagnostics {
    bool original = false;
    std::array<bool, 6> entry_original{};
    std::array<std::uint64_t, 6> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    MatchDisciplineBoundaryObserver* observer = nullptr;
};
MatchDisciplineDiagnostics& match_discipline_diagnostics(State&);
const MatchDisciplineDiagnostics& match_discipline_diagnostics(const State&);
bool try_modern_match_discipline(State&, std::uint32_t);
}
