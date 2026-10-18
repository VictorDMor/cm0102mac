#pragma once
#include "money_value.hpp"

namespace cm::aot {
enum class MoneyValueOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct MoneyValueBoundaryObserver : SemanticGuestObserver {
    virtual ~MoneyValueBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, MoneyValueOutcome) = 0;
};
struct MoneyValueDiagnostics {
    bool original = false;
    std::array<bool, money_value::entries.size()> entry_original{};
    std::array<std::uint64_t, money_value::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    MoneyValueBoundaryObserver* observer = nullptr;
};
MoneyValueDiagnostics& money_value_diagnostics(State&);
const MoneyValueDiagnostics& money_value_diagnostics(const State&);
bool try_modern_money_value(State&, std::uint32_t);
}
