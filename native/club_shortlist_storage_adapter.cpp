#include "club_shortlist_storage_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ClubShortlistStorageDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(club_shortlist_storage_diagnostics(s).original_body) {
        club_shortlist_storage_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { club_shortlist_storage_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ClubShortlistStorageOutcome outcome) {
    auto& d = club_shortlist_storage_diagnostics(s);
    if (!original) {
        if (outcome == ClubShortlistStorageOutcome::returned) ++d.returns[index];
        else if (outcome == ClubShortlistStorageOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ClubShortlistStorageOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ClubShortlistStorageOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, club_shortlist_storage::entries[index], original, outcome);
}
}
bool try_modern_club_shortlist_storage(State& s, std::uint32_t entry) {
    const auto found = std::find(club_shortlist_storage::entries.begin(), club_shortlist_storage::entries.end(), entry);
    if (found == club_shortlist_storage::entries.end()) throw std::runtime_error("Unknown club-shortlist-storage entry");
    const auto index = unsigned(found - club_shortlist_storage::entries.begin()); s.pc = entry;
    auto& d = club_shortlist_storage_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original club-shortlist-storage body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{club_shortlist_storage::construct_shortlist_storage,club_shortlist_storage::destroy_shortlist_storage,club_shortlist_storage::reset_shortlist_storage,club_shortlist_storage::sample_shortlist_range,club_shortlist_storage::load_shortlist_storage,club_shortlist_storage::initialize_auxiliary_record,club_shortlist_storage::save_shortlist_storage};
            ClubShortlistStorageDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ClubShortlistStorageOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ClubShortlistStorageOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ClubShortlistStorageOutcome::budget_stop : ClubShortlistStorageOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ClubShortlistStorageOutcome::fault); throw; }
    leave(s, index, original, ClubShortlistStorageOutcome::returned); return true;
}
}
