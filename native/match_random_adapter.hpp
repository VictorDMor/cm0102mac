#pragma once
#include "match_random.hpp"

namespace cm::aot {
enum class MatchRandomOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct MatchRandomBoundaryObserver : SemanticGuestObserver {
    virtual ~MatchRandomBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, MatchRandomOutcome) = 0;
};
struct MatchRandomDiagnostics {
    bool original = false;
    std::array<bool, 1> entry_original{};
    std::array<std::uint64_t, 1> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    MatchRandomBoundaryObserver* observer = nullptr;
};
MatchRandomDiagnostics& match_random_diagnostics(State&);
const MatchRandomDiagnostics& match_random_diagnostics(const State&);
bool try_modern_match_random(State&, std::uint32_t);
}
