#pragma once
#include "transfer_records.hpp"

namespace cm::aot {
enum class TransferRecordOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct TransferRecordBoundaryObserver : SemanticGuestObserver {
    virtual ~TransferRecordBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, TransferRecordOutcome) = 0;
};
struct TransferRecordDiagnostics {
    bool original = false;
    std::array<bool, transfer_records::entries.size()> entry_original{};
    std::array<std::uint64_t, transfer_records::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    TransferRecordBoundaryObserver* observer = nullptr;
};
TransferRecordDiagnostics& transfer_record_diagnostics(State&);
const TransferRecordDiagnostics& transfer_record_diagnostics(const State&);
bool try_modern_transfer_record(State&, std::uint32_t);
}
