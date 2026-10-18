#pragma once
#include "shortlist_links.hpp"

namespace cm::aot {
enum class ShortlistLinksOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct ShortlistLinksBoundaryObserver : SemanticGuestObserver {
    virtual ~ShortlistLinksBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ShortlistLinksOutcome) = 0;
};
struct ShortlistLinksDiagnostics {
    bool original = false;
    std::array<bool, shortlist_links::entries.size()> entry_original{};
    std::array<std::uint64_t, shortlist_links::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    ShortlistLinksBoundaryObserver* observer = nullptr;
};
ShortlistLinksDiagnostics& shortlist_links_diagnostics(State&);
const ShortlistLinksDiagnostics& shortlist_links_diagnostics(const State&);
bool try_modern_shortlist_links(State&, std::uint32_t);
}
