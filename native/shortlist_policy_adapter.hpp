#pragma once
#include "shortlist_policy.hpp"

namespace cm::aot {
enum class ShortlistPolicyOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ShortlistPolicyBoundaryObserver : SemanticGuestObserver {
    virtual ~ShortlistPolicyBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ShortlistPolicyOutcome) = 0;
};
struct ShortlistPolicyDiagnostics {
    bool original = false;
    std::array<bool, shortlist_policy::entries.size()> entry_original{};
    std::array<std::uint64_t, shortlist_policy::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ShortlistPolicyBoundaryObserver* observer = nullptr;
};
ShortlistPolicyDiagnostics& shortlist_policy_diagnostics(State&);
const ShortlistPolicyDiagnostics& shortlist_policy_diagnostics(const State&);
bool try_modern_shortlist_policy(State&, std::uint32_t);
}
