#include "shortlist_links_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ShortlistLinksDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(shortlist_links_diagnostics(s).original_body) {
        shortlist_links_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { shortlist_links_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ShortlistLinksOutcome outcome) {
    auto& d = shortlist_links_diagnostics(s);
    if (!original) {
        if (outcome == ShortlistLinksOutcome::returned) ++d.returns[index];
        else if (outcome == ShortlistLinksOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ShortlistLinksOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ShortlistLinksOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, shortlist_links::entries[index], original, outcome);
}
}
bool try_modern_shortlist_links(State& s, std::uint32_t entry) {
    const auto found = std::find(shortlist_links::entries.begin(), shortlist_links::entries.end(), entry);
    if (found == shortlist_links::entries.end()) throw std::runtime_error("Unknown shortlist-links entry");
    const auto index = unsigned(found - shortlist_links::entries.begin()); s.pc = entry;
    auto& d = shortlist_links_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original shortlist-links body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{shortlist_links::contains_staff_club_link,shortlist_links::collect_club_links,shortlist_links::collect_staff_links,shortlist_links::mark_primary_contract,shortlist_links::add_staff_club_link,shortlist_links::reset_linked_club_rows,shortlist_links::transition_staff_links,shortlist_links::clear_primary_contract_flag_one,shortlist_links::set_primary_contract_flag_two,shortlist_links::clear_primary_contract_flag_two};
            ShortlistLinksDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ShortlistLinksOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ShortlistLinksOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ShortlistLinksOutcome::budget_stop : ShortlistLinksOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ShortlistLinksOutcome::fault); throw; }
    leave(s, index, original, ShortlistLinksOutcome::returned); return true;
}
}
