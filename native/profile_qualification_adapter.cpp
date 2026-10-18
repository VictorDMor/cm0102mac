#include "profile_qualification_adapter.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <string_view>

namespace cm::aot {
namespace {
struct ProfileQualificationDispatch final : SemanticDependencyDispatch {
    void call(State& state, std::uint32_t, std::uint32_t continuation) override {
        run_until(state, continuation);
    }
};
struct OriginalBody {
    State& state; std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(profile_qualification_diagnostics(s).original_body) {
        profile_qualification_diagnostics(state).original_body = entry;
    }
    ~OriginalBody() { profile_qualification_diagnostics(state).original_body = previous; }
};
void leave(State& s, unsigned index, bool original, ProfileQualificationOutcome outcome) {
    auto& d = profile_qualification_diagnostics(s);
    if (!original) {
        if (outcome == ProfileQualificationOutcome::returned) ++d.returns[index];
        else if (outcome == ProfileQualificationOutcome::budget_stop) ++d.budget_stops[index];
        else if (outcome == ProfileQualificationOutcome::context_transfer) ++d.transfers[index];
        else if (outcome == ProfileQualificationOutcome::terminated) ++d.terminations[index];
        else ++d.faults[index];
    }
    if (d.observer) d.observer->leave(s, profile_qualification::entries[index], original, outcome);
}
}
bool try_modern_profile_qualification(State& s, std::uint32_t entry) {
    const auto found = std::find(profile_qualification::entries.begin(), profile_qualification::entries.end(), entry);
    if (found == profile_qualification::entries.end()) throw std::runtime_error("Unknown profile-qualification entry");
    const auto index = unsigned(found - profile_qualification::entries.begin()); s.pc = entry;
    auto& d = profile_qualification_diagnostics(s);
    if (d.original_body == entry) return false;
    bool original = d.original || d.entry_original[index];
    if (original) ++d.original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++d.profile_original_calls[index]; }
    else ++d.calls[index];
    if (d.observer) d.observer->enter(s, entry, original);
    try {
        if (original) {
            const auto caller = std::uint32_t(s.r<4>(4128)); OriginalBody selected(s, entry);
            if (!dispatch_page(s)) throw std::runtime_error("Missing original profile-qualification body");
            if (s.pc != caller) run_until(s, caller);
        } else {
            using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
            constexpr Function methods[]{profile_qualification::combine_qualification_masks,profile_qualification::classify_profile_qualifications,profile_qualification::candidate_category_reference,profile_qualification::club_category_reference};
            ProfileQualificationDispatch dependencies;
            methods[index](s, &dependencies, d.observer);
        }
    } catch (const ApplicationExit&) { leave(s, index, original, ProfileQualificationOutcome::terminated); throw; }
    catch (const GuestContextTransfer&) { leave(s, index, original, ProfileQualificationOutcome::context_transfer); throw; }
    catch (const std::runtime_error& error) {
        const std::string_view message(error.what());
        if (message.starts_with("Uncompiled original address:")) ++d.dispatch_escapes;
        leave(s, index, original, message == "Native instruction budget exhausted" ? ProfileQualificationOutcome::budget_stop : ProfileQualificationOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ProfileQualificationOutcome::fault); throw; }
    leave(s, index, original, ProfileQualificationOutcome::returned); return true;
}
}
