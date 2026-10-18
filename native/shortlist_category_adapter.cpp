#include "shortlist_category_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ShortlistCategoryDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(shortlist_category_diagnostics(s).original_body) {
        shortlist_category_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { shortlist_category_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ShortlistCategoryOutcome outcome) {
    auto& d = shortlist_category_diagnostics(s);
    if (!original) {
        if (outcome == ShortlistCategoryOutcome::returned) ++d.returns[index];
        else if (outcome == ShortlistCategoryOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ShortlistCategoryOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ShortlistCategoryOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, shortlist_category::entries[index], original, outcome);
}
}
bool try_modern_shortlist_category(State& s, std::uint32_t entry) {
    const auto found = std::find(shortlist_category::entries.begin(), shortlist_category::entries.end(), entry);
    if (found == shortlist_category::entries.end()) throw std::runtime_error("Unknown shortlist-category entry");
    const auto index = unsigned(found - shortlist_category::entries.begin()); s.pc = entry;
    auto& d = shortlist_category_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original shortlist-category body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{shortlist_category::eligible_for_category,shortlist_category::count_and_prune_club_rows,shortlist_category::refresh_unassigned_rows,shortlist_category::compare_pairs_by_second,shortlist_category::compare_pairs_by_first};
            ShortlistCategoryDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ShortlistCategoryOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ShortlistCategoryOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ShortlistCategoryOutcome::budget_stop : ShortlistCategoryOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ShortlistCategoryOutcome::fault); throw; }
    leave(s, index, original, ShortlistCategoryOutcome::returned); return true;
}
}
