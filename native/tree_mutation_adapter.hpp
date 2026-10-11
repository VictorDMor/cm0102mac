#pragma once
#include "tree_mutation.hpp"

namespace cm::aot {
enum class TreeMutationOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct TreeMutationBoundaryObserver {
    virtual ~TreeMutationBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, TreeMutationOutcome) = 0;
};
struct TreeMutationDiagnostics {
    bool original = false;
    std::array<bool, 3> entry_original{};
    std::array<std::uint64_t, 3> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    TreeMutationBoundaryObserver* observer = nullptr;
};
TreeMutationDiagnostics& tree_mutation_diagnostics(State&);
const TreeMutationDiagnostics& tree_mutation_diagnostics(const State&);
bool try_modern_tree_mutation(State&, std::uint32_t);
void reset_tree_mutation_diagnostics(State&);
std::string tree_mutation_call_summary(const State&);
}
