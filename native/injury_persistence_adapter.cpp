#include "injury_persistence_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct PersistenceDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(injury_persistence_diagnostics(s).original_body) {
        injury_persistence_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { injury_persistence_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, InjuryPersistenceOutcome outcome) {
    auto& d = injury_persistence_diagnostics(s);
    if (!original) {
        if (outcome == InjuryPersistenceOutcome::returned) ++d.returns[index];
        else if (outcome == InjuryPersistenceOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == InjuryPersistenceOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == InjuryPersistenceOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, injury_persistence::entries[index], original, outcome);
}
}
bool try_modern_injury_persistence(State& s, std::uint32_t entry) {
    const auto found = std::find(injury_persistence::entries.begin(), injury_persistence::entries.end(), entry);
    if (found == injury_persistence::entries.end()) throw std::runtime_error("Unknown injury-persistence entry");
    const auto index = unsigned(found - injury_persistence::entries.begin()); s.pc = entry;
    auto& d = injury_persistence_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original injury-persistence body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{injury_persistence::load_controller,injury_persistence::save_controller};
            PersistenceDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, InjuryPersistenceOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, InjuryPersistenceOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? InjuryPersistenceOutcome::budget_stop : InjuryPersistenceOutcome::fault); throw;
    } catch (...) { leave(s, index, original, InjuryPersistenceOutcome::fault); throw; }
    leave(s, index, original, InjuryPersistenceOutcome::returned); return true;
}
}
