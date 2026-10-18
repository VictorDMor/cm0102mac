#pragma once
#include "transfer_offer_queries.hpp"

namespace cm::aot {
enum class TransferOfferOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct TransferOfferBoundaryObserver : SemanticGuestObserver {
    virtual ~TransferOfferBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, TransferOfferOutcome) = 0;
};
struct TransferOfferDiagnostics {
    bool original = false;
    std::array<bool, transfer_offer_queries::entries.size()> entry_original{};
    std::array<std::uint64_t, transfer_offer_queries::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    TransferOfferBoundaryObserver* observer = nullptr;
};
TransferOfferDiagnostics& transfer_offer_diagnostics(State&);
const TransferOfferDiagnostics& transfer_offer_diagnostics(const State&);
bool try_modern_transfer_offer(State&, std::uint32_t);
}
