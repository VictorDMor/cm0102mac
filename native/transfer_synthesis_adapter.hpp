#pragma once
#include "transfer_synthesis.hpp"

namespace cm::aot {
enum class TransferSynthesisOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct TransferSynthesisBoundaryObserver : SemanticGuestObserver {
    virtual ~TransferSynthesisBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, TransferSynthesisOutcome) = 0;
};
struct TransferSynthesisDiagnostics {
    bool original = false;
    std::array<bool, transfer_synthesis::entries.size()> entry_original{};
    std::array<std::uint64_t, transfer_synthesis::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    TransferSynthesisBoundaryObserver* observer = nullptr;
};
TransferSynthesisDiagnostics& transfer_synthesis_diagnostics(State&);
const TransferSynthesisDiagnostics& transfer_synthesis_diagnostics(const State&);
bool try_modern_transfer_synthesis(State&, std::uint32_t);
}
