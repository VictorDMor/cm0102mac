#pragma once
#include "money_text.hpp"

namespace cm::aot {
enum class MoneyTextOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct MoneyTextBoundaryObserver : SemanticGuestObserver {
    virtual ~MoneyTextBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, MoneyTextOutcome) = 0;
};
struct MoneyTextDiagnostics {
    bool original = false;
    std::array<bool, money_text::entries.size()> entry_original{};
    std::array<std::uint64_t, money_text::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    MoneyTextBoundaryObserver* observer = nullptr;
};
MoneyTextDiagnostics& money_text_diagnostics(State&);
const MoneyTextDiagnostics& money_text_diagnostics(const State&);
bool try_modern_money_text(State&, std::uint32_t);
}
