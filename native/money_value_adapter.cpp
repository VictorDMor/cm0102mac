#include "money_value_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct MoneyValueDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(money_value_diagnostics(s).original_body) {
        money_value_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { money_value_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, MoneyValueOutcome outcome) {
    auto& d = money_value_diagnostics(s);
    if (!original) {
        if (outcome == MoneyValueOutcome::returned) ++d.returns[index];
        else if (outcome == MoneyValueOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == MoneyValueOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == MoneyValueOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, money_value::entries[index], original, outcome);
}
}
bool try_modern_money_value(State& s, std::uint32_t entry) {
    const auto found = std::find(money_value::entries.begin(), money_value::entries.end(), entry);
    if (found == money_value::entries.end()) throw std::runtime_error("Unknown money-value entry");
    const auto index = unsigned(found - money_value::entries.begin()); s.pc = entry;
    auto& d = money_value_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original money-value body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{money_value::to_display_amount,money_value::from_display_amount,money_value::increase_amount,money_value::decrease_amount,money_value::select_standard_amount,money_value::select_table_amount,money_value::amount_quantum};
            MoneyValueDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, MoneyValueOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, MoneyValueOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? MoneyValueOutcome::budget_stop : MoneyValueOutcome::fault); throw;
    } catch (...) { leave(s, index, original, MoneyValueOutcome::fault); throw; }
    leave(s, index, original, MoneyValueOutcome::returned); return true;
}
}
