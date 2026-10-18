#pragma once
#include "profile_qualification.hpp"

namespace cm::aot {
enum class ProfileQualificationOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ProfileQualificationBoundaryObserver : SemanticGuestObserver {
    virtual ~ProfileQualificationBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ProfileQualificationOutcome) = 0;
};
struct ProfileQualificationDiagnostics {
    bool original = false;
    std::array<bool, profile_qualification::entries.size()> entry_original{};
    std::array<std::uint64_t, profile_qualification::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ProfileQualificationBoundaryObserver* observer = nullptr;
};
ProfileQualificationDiagnostics& profile_qualification_diagnostics(State&);
const ProfileQualificationDiagnostics& profile_qualification_diagnostics(const State&);
bool try_modern_profile_qualification(State&, std::uint32_t);
}
