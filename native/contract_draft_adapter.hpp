#pragma once
#include "contract_draft.hpp"

namespace cm::aot {
enum class ContractDraftOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ContractDraftBoundaryObserver : SemanticGuestObserver {
    virtual ~ContractDraftBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ContractDraftOutcome) = 0;
};
struct ContractDraftDiagnostics {
    bool original = false;
    std::array<bool, contract_draft::entries.size()> entry_original{};
    std::array<std::uint64_t, contract_draft::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ContractDraftBoundaryObserver* observer = nullptr;
};
ContractDraftDiagnostics& contract_draft_diagnostics(State&);
const ContractDraftDiagnostics& contract_draft_diagnostics(const State&);
bool try_modern_contract_draft(State&, std::uint32_t);
}
