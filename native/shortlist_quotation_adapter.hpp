#pragma once
#include "shortlist_quotation.hpp"

namespace cm::aot {
enum class ShortlistQuotationOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ShortlistQuotationBoundaryObserver : SemanticGuestObserver {
    virtual ~ShortlistQuotationBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ShortlistQuotationOutcome) = 0;
};
struct ShortlistQuotationDiagnostics {
    bool original = false;
    std::array<bool, shortlist_quotation::entries.size()> entry_original{};
    std::array<std::uint64_t, shortlist_quotation::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ShortlistQuotationBoundaryObserver* observer = nullptr;
};
ShortlistQuotationDiagnostics& shortlist_quotation_diagnostics(State&);
const ShortlistQuotationDiagnostics& shortlist_quotation_diagnostics(const State&);
bool try_modern_shortlist_quotation(State&, std::uint32_t);
}
