#pragma once
#include "localization_format.hpp"

namespace cm::aot {
enum class LocalizationFormatOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct LocalizationFormatBoundaryObserver : SemanticGuestObserver {
    virtual ~LocalizationFormatBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, LocalizationFormatOutcome) = 0;
};
struct LocalizationFormatDiagnostics {
    bool original = false;
    std::array<bool, localization_format::entries.size()> entry_original{};
    std::array<std::uint64_t, localization_format::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    LocalizationFormatBoundaryObserver* observer = nullptr;
};
LocalizationFormatDiagnostics& localization_format_diagnostics(State&);
const LocalizationFormatDiagnostics& localization_format_diagnostics(const State&);
bool try_modern_localization_format(State&, std::uint32_t);
}
