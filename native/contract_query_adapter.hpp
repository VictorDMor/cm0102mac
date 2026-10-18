#pragma once
#include "contract_queries.hpp"

namespace cm::aot {
enum class ContractQueryOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ContractQueryBoundaryObserver : SemanticGuestObserver {
    virtual ~ContractQueryBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ContractQueryOutcome) = 0;
};
struct ContractQueryDiagnostics {
    bool original = false;
    std::array<bool, contract_queries::entries.size()> entry_original{};
    std::array<std::uint64_t, contract_queries::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ContractQueryBoundaryObserver* observer = nullptr;
};
ContractQueryDiagnostics& contract_query_diagnostics(State&);
const ContractQueryDiagnostics& contract_query_diagnostics(const State&);
bool try_modern_contract_query(State&, std::uint32_t);
}
