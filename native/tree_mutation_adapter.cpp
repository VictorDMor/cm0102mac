#include "tree_mutation_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(tree_mutation_diagnostics(s).original_body) {
        tree_mutation_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { tree_mutation_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, TreeMutationOutcome outcome) {
    auto& d = tree_mutation_diagnostics(s);
    if (!original) {
        if (outcome == TreeMutationOutcome::returned) ++d.returns[index];
        else if (outcome == TreeMutationOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == TreeMutationOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == TreeMutationOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, tree_mutation::entries[index], original, outcome);
}
}
bool try_modern_tree_mutation(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto index = tree_mutation::entry_index(entry);
    auto& d = tree_mutation_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original tree-mutation body");
            if (s.pc != caller) run_until(s, caller);
        } else tree_mutation::invoke(s, index);
    } catch (const ApplicationExit&) {
        leave(s, index, original, TreeMutationOutcome::terminated); throw;
    } catch (const GuestContextTransfer&) {
        leave(s, index, original, TreeMutationOutcome::context_transfer); throw;
    } catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted"
            ? TreeMutationOutcome::budget_stop : TreeMutationOutcome::fault); throw;
    } catch (...) { leave(s, index, original, TreeMutationOutcome::fault); throw; }
    leave(s, index, original, TreeMutationOutcome::returned); return true;
}
void reset_tree_mutation_diagnostics(State& s) { tree_mutation_diagnostics(s) = TreeMutationDiagnostics{}; }
std::string tree_mutation_call_summary(const State& s) {
    const auto& d = tree_mutation_diagnostics(s); std::ostringstream out;
    for (unsigned n = 0; n < tree_mutation::entries.size(); ++n)
        out << "tree_mutation.entry=" << hex_address(tree_mutation::entries[n]) << " modern=" << d.calls[n]
            << " original=" << d.original_calls[n] << " returns=" << d.returns[n] << " faults=" << d.faults[n]
            << " budget_stops=" << d.budget_stops[n] << " transfers=" << d.transfers[n] << " terminations=" << d.terminations[n]
            << " profile_original=" << d.profile_original_calls[n] << '\n';
    out << "tree_mutation.dispatch_escapes=" << d.dispatch_escapes << '\n'; return out.str();
}
}
