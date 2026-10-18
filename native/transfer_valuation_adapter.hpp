#pragma once
#include "transfer_valuation.hpp"

namespace cm::aot {
enum class TransferValuationOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct TransferValuationBoundaryObserver : SemanticGuestObserver {
    virtual ~TransferValuationBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, TransferValuationOutcome) = 0;
};
struct TransferValuationDiagnostics {
    bool original = false;
    std::array<bool, transfer_valuation::entries.size()> entry_original{};
    std::array<std::uint64_t, transfer_valuation::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    TransferValuationBoundaryObserver* observer = nullptr;
};
TransferValuationDiagnostics& transfer_valuation_diagnostics(State&);
const TransferValuationDiagnostics& transfer_valuation_diagnostics(const State&);
bool try_modern_transfer_valuation(State&, std::uint32_t);
}
