#pragma once
#include "localization_lookup.hpp"

namespace cm::aot {
enum class LocalizationLookupOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct LocalizationLookupBoundaryObserver : SemanticGuestObserver {
    virtual ~LocalizationLookupBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, LocalizationLookupOutcome) = 0;
};
struct LocalizationLookupDiagnostics {
    bool original = false;
    std::array<bool, localization_lookup::entries.size()> entry_original{};
    std::array<std::uint64_t, localization_lookup::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    LocalizationLookupBoundaryObserver* observer = nullptr;
};
LocalizationLookupDiagnostics& localization_lookup_diagnostics(State&);
const LocalizationLookupDiagnostics& localization_lookup_diagnostics(const State&);
bool try_modern_localization_lookup(State&, std::uint32_t);
}
