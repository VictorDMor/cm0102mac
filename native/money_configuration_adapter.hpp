#pragma once
#include "money_configuration.hpp"

namespace cm::aot {
enum class MoneyConfigurationOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct MoneyConfigurationBoundaryObserver : SemanticGuestObserver {
    virtual ~MoneyConfigurationBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, MoneyConfigurationOutcome) = 0;
};
struct MoneyConfigurationDiagnostics {
    bool original = false;
    std::array<bool, money_configuration::entries.size()> entry_original{};
    std::array<std::uint64_t, money_configuration::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    MoneyConfigurationBoundaryObserver* observer = nullptr;
};
MoneyConfigurationDiagnostics& money_configuration_diagnostics(State&);
const MoneyConfigurationDiagnostics& money_configuration_diagnostics(const State&);
bool try_modern_money_configuration(State&, std::uint32_t);
}
