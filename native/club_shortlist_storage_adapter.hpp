#pragma once
#include "club_shortlist_storage.hpp"

namespace cm::aot {
enum class ClubShortlistStorageOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ClubShortlistStorageBoundaryObserver : SemanticGuestObserver {
    virtual ~ClubShortlistStorageBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ClubShortlistStorageOutcome) = 0;
};
struct ClubShortlistStorageDiagnostics {
    bool original = false;
    std::array<bool, club_shortlist_storage::entries.size()> entry_original{};
    std::array<std::uint64_t, club_shortlist_storage::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ClubShortlistStorageBoundaryObserver* observer = nullptr;
};
ClubShortlistStorageDiagnostics& club_shortlist_storage_diagnostics(State&);
const ClubShortlistStorageDiagnostics& club_shortlist_storage_diagnostics(const State&);
bool try_modern_club_shortlist_storage(State&, std::uint32_t);
}
