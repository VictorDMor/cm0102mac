#pragma once
#include "record_group.hpp"

namespace cm::aot {
enum class RecordGroupOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct RecordGroupBoundaryObserver {
    virtual ~RecordGroupBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, RecordGroupOutcome) = 0;
};
struct RecordGroupDiagnostics {
    bool original = false;
    std::array<bool, 11> entry_original{};
    std::array<std::uint64_t, 11> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    RecordGroupBoundaryObserver* observer = nullptr;
};
RecordGroupDiagnostics& record_group_diagnostics(State&);
const RecordGroupDiagnostics& record_group_diagnostics(const State&);
bool try_modern_record_group(State&, std::uint32_t);
void reset_record_group_diagnostics(State&);
std::string record_group_call_summary(const State&);
}
