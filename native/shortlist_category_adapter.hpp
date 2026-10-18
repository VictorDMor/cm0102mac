#pragma once
#include "shortlist_category.hpp"

namespace cm::aot {
enum class ShortlistCategoryOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ShortlistCategoryBoundaryObserver : SemanticGuestObserver {
    virtual ~ShortlistCategoryBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ShortlistCategoryOutcome) = 0;
};
struct ShortlistCategoryDiagnostics {
    bool original = false;
    std::array<bool, shortlist_category::entries.size()> entry_original{};
    std::array<std::uint64_t, shortlist_category::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ShortlistCategoryBoundaryObserver* observer = nullptr;
};
ShortlistCategoryDiagnostics& shortlist_category_diagnostics(State&);
const ShortlistCategoryDiagnostics& shortlist_category_diagnostics(const State&);
bool try_modern_shortlist_category(State&, std::uint32_t);
}
