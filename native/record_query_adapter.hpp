#pragma once
#include "record_query.hpp"

namespace cm::aot {
enum class RecordQueryOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct RecordQueryBoundaryObserver {
    virtual ~RecordQueryBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, RecordQueryOutcome) = 0;
};
struct RecordQueryDiagnostics {
    bool original = false;
    std::array<bool, 6> entry_original{};
    std::array<std::uint64_t, 6> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    RecordQueryBoundaryObserver* observer = nullptr;
};
RecordQueryDiagnostics& record_query_diagnostics(State&);
const RecordQueryDiagnostics& record_query_diagnostics(const State&);
bool try_modern_record_query(State&, std::uint32_t);
void reset_record_query_diagnostics(State&);
std::string record_query_call_summary(const State&);
}
