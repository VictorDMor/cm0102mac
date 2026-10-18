#pragma once
#include "contract_display.hpp"

namespace cm::aot {
enum class ContractDisplayOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ContractDisplayBoundaryObserver : SemanticGuestObserver {
    virtual ~ContractDisplayBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ContractDisplayOutcome) = 0;
};
struct ContractDisplayDiagnostics {
    bool original = false;
    std::array<bool, contract_display::entries.size()> entry_original{};
    std::array<std::uint64_t, contract_display::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ContractDisplayBoundaryObserver* observer = nullptr;
};
ContractDisplayDiagnostics& contract_display_diagnostics(State&);
const ContractDisplayDiagnostics& contract_display_diagnostics(const State&);
bool try_modern_contract_display(State&, std::uint32_t);
}
