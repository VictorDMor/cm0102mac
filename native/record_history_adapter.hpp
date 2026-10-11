#pragma once
#include "record_history.hpp"

namespace cm::aot {
enum class RecordHistoryOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct RecordHistoryBoundaryObserver {
    virtual ~RecordHistoryBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, RecordHistoryOutcome) = 0;
};
struct RecordHistoryDiagnostics {
    bool original = false;
    std::array<bool, 2> entry_original{};
    std::array<std::uint64_t, 2> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    RecordHistoryBoundaryObserver* observer = nullptr;
};
RecordHistoryDiagnostics& record_history_diagnostics(State&);
const RecordHistoryDiagnostics& record_history_diagnostics(const State&);
bool try_modern_record_history(State&, std::uint32_t);
void reset_record_history_diagnostics(State&);
std::string record_history_call_summary(const State&);
}
