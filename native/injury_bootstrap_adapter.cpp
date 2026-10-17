#include "injury_bootstrap_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct BootstrapDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(injury_bootstrap_diagnostics(s).original_body) {
        injury_bootstrap_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { injury_bootstrap_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, InjuryBootstrapOutcome outcome) {
    auto& d = injury_bootstrap_diagnostics(s);
    if (!original) {
        if (outcome == InjuryBootstrapOutcome::returned) ++d.returns[index];
        else if (outcome == InjuryBootstrapOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == InjuryBootstrapOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == InjuryBootstrapOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, injury_bootstrap::entries[index], original, outcome);
}
}
bool try_modern_injury_bootstrap(State& s, std::uint32_t entry) {
    const auto found = std::find(injury_bootstrap::entries.begin(), injury_bootstrap::entries.end(), entry);
    if (found == injury_bootstrap::entries.end()) throw std::runtime_error("Unknown injury-bootstrap entry");
    const auto index = unsigned(found - injury_bootstrap::entries.begin()); s.pc = entry;
    auto& d = injury_bootstrap_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original injury-bootstrap body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{injury_bootstrap::construct_controller,injury_bootstrap::destroy_controller,injury_bootstrap::initialize_club_thresholds,injury_bootstrap::set_staff_injury_flag,injury_bootstrap::staff_availability_gate,injury_bootstrap::populate_staff_records};
            BootstrapDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, InjuryBootstrapOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, InjuryBootstrapOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? InjuryBootstrapOutcome::budget_stop : InjuryBootstrapOutcome::fault); throw;
    } catch (...) { leave(s, index, original, InjuryBootstrapOutcome::fault); throw; }
    leave(s, index, original, InjuryBootstrapOutcome::returned); return true;
}
}
