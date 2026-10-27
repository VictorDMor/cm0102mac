#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Process-local evidence for the global initializer transferred from the
// older engine-initialization family. A return is recorded only after its
// C++ body and original frame epilogue complete.
struct ContractManagerDiagnostics {
    std::uint64_t date_initialization_calls = 0;
    std::uint64_t date_initialization_returns = 0;
};
ContractManagerDiagnostics& contract_manager_diagnostics(State&);
const ContractManagerDiagnostics& contract_manager_diagnostics(const State&);
// Hook at the entry of each routine in contract_manager_routines.inc (the
// original's CONTRACT_MANAGER.CPP): runs it as C++ and continues at the return
// address. Drafts run only in check tools; verified entries also run in the app.
// Declines while a checker runs the original body
// (routine_check.hpp), for routines a check run leaves out (--only), or with
// CM_CONTRACT_MANAGER_ORIGINAL=1.
bool try_modern_contract_manager(State& s, std::uint32_t entry);
// For check tools (routine_check.hpp dispatch_probe): runs a listed routine
// reached through the dispatcher, hooked by the generator or not yet.
bool contract_manager_dispatch_probe(State& s);
bool contract_manager_lists(std::uint32_t entry);
unsigned contract_manager_results(std::uint32_t entry);
// For synthetic calls (routine_check.hpp): run one routine's original body or
// its C++ from its entry state to its return.
void contract_manager_run_original(State& s, std::uint32_t entry);
void contract_manager_run_rewrite(State& s, std::uint32_t entry);
// Check fixtures may answer dependency calls before the dispatcher's leaf
// shortcut. Null during normal play and capture-backed verification.
struct ContractManagerCallHook {
    virtual bool call(State&) = 0;
protected:
    ~ContractManagerCallHook() = default;
};
extern ContractManagerCallHook* contract_manager_call_hook;
}
