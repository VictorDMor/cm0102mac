#pragma once
#include "localization_catalog.hpp"

namespace cm::aot {
enum class LocalizationCatalogOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct LocalizationCatalogBoundaryObserver : SemanticGuestObserver {
    virtual ~LocalizationCatalogBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, LocalizationCatalogOutcome) = 0;
};
struct LocalizationCatalogDiagnostics {
    bool original = false;
    std::array<bool, localization_catalog::entries.size()> entry_original{};
    std::array<std::uint64_t, localization_catalog::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    LocalizationCatalogBoundaryObserver* observer = nullptr;
};
LocalizationCatalogDiagnostics& localization_catalog_diagnostics(State&);
const LocalizationCatalogDiagnostics& localization_catalog_diagnostics(const State&);
bool try_modern_localization_catalog(State&, std::uint32_t);
}
