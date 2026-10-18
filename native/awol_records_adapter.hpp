#pragma once
#include "awol_records.hpp"

namespace cm::aot {
enum class AwolRecordsOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct AwolRecordsBoundaryObserver : SemanticGuestObserver {
    virtual ~AwolRecordsBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, AwolRecordsOutcome) = 0;
};
struct AwolRecordsDiagnostics {
    bool original = false;
    std::array<bool, awol::record_entries.size()> entry_original{};
    std::array<std::uint64_t, awol::record_entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    AwolRecordsBoundaryObserver* observer = nullptr;
};
AwolRecordsDiagnostics& awol_records_diagnostics(State&);
const AwolRecordsDiagnostics& awol_records_diagnostics(const State&);
bool try_modern_awol_records(State&, std::uint32_t);
}
