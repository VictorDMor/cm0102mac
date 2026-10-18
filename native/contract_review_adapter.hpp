#pragma once
#include "contract_review.hpp"

namespace cm::aot {
enum class ContractReviewOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ContractReviewBoundaryObserver : SemanticGuestObserver {
    virtual ~ContractReviewBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ContractReviewOutcome) = 0;
};
struct ContractReviewDiagnostics {
    bool original = false;
    std::array<bool, contract_review::entries.size()> entry_original{};
    std::array<std::uint64_t, contract_review::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ContractReviewBoundaryObserver* observer = nullptr;
};
ContractReviewDiagnostics& contract_review_diagnostics(State&);
const ContractReviewDiagnostics& contract_review_diagnostics(const State&);
bool try_modern_contract_review(State&, std::uint32_t);
}
