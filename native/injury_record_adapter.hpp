#pragma once
#include "injury_record.hpp"

namespace cm::aot {
enum class InjuryRecordOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct InjuryRecordBoundaryObserver : SemanticGuestObserver {
    virtual ~InjuryRecordBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, InjuryRecordOutcome) = 0;
};
struct InjuryRecordDiagnostics {
    bool original = false;
    std::array<bool, injury_record::entries.size()> entry_original{};
    std::array<std::uint64_t, injury_record::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    InjuryRecordBoundaryObserver* observer = nullptr;
};
InjuryRecordDiagnostics& injury_record_diagnostics(State&);
const InjuryRecordDiagnostics& injury_record_diagnostics(const State&);
bool try_modern_injury_record(State&, std::uint32_t);
}
