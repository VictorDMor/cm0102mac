#pragma once
#include "contract_storage.hpp"

namespace cm::aot {
enum class ContractStorageOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ContractStorageBoundaryObserver : SemanticGuestObserver {
    virtual ~ContractStorageBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ContractStorageOutcome) = 0;
};
struct ContractStorageDiagnostics {
    bool original = false;
    std::array<bool, contract_storage::prepared_entries.size()> entry_original{};
    std::array<std::uint64_t, contract_storage::prepared_entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ContractStorageBoundaryObserver* observer = nullptr;
};
ContractStorageDiagnostics& contract_storage_diagnostics(State&);
const ContractStorageDiagnostics& contract_storage_diagnostics(const State&);
bool try_modern_contract_storage(State&, std::uint32_t);
}
