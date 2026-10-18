#include "money_text_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct MoneyTextDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(money_text_diagnostics(s).original_body) {
        money_text_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { money_text_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, MoneyTextOutcome outcome) {
    auto& d = money_text_diagnostics(s);
    if (!original) {
        if (outcome == MoneyTextOutcome::returned) ++d.returns[index];
        else if (outcome == MoneyTextOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == MoneyTextOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == MoneyTextOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, money_text::entries[index], original, outcome);
}
}
bool try_modern_money_text(State& s, std::uint32_t entry) {
    const auto found = std::find(money_text::entries.begin(), money_text::entries.end(), entry);
    if (found == money_text::entries.end()) throw std::runtime_error("Unknown money-text entry");
    const auto index = unsigned(found - money_text::entries.begin()); s.pc = entry;
    auto& d = money_text_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original money-text body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{money_text::selected_symbol_length,money_text::display_setting,money_text::currency_label,money_text::format_amount};
            MoneyTextDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, MoneyTextOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, MoneyTextOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? MoneyTextOutcome::budget_stop : MoneyTextOutcome::fault); throw;
    } catch (...) { leave(s, index, original, MoneyTextOutcome::fault); throw; }
    leave(s, index, original, MoneyTextOutcome::returned); return true;
}
}
