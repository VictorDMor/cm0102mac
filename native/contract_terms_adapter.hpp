#pragma once
#include "contract_terms.hpp"

namespace cm::aot {
enum class ContractTermsOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ContractTermsBoundaryObserver : SemanticGuestObserver {
    virtual ~ContractTermsBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ContractTermsOutcome) = 0;
};
struct ContractTermsDiagnostics {
    bool original = false;
    std::array<bool, contract_terms::entries.size()> entry_original{};
    std::array<std::uint64_t, contract_terms::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ContractTermsBoundaryObserver* observer = nullptr;
};
ContractTermsDiagnostics& contract_terms_diagnostics(State&);
const ContractTermsDiagnostics& contract_terms_diagnostics(const State&);
bool try_modern_contract_terms(State&, std::uint32_t);
}
