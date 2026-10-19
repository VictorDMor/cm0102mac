#pragma once
#include "range_sampling.hpp"

namespace cm::aot {
enum class RangeSamplingOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct RangeSamplingBoundaryObserver : SemanticGuestObserver {
    virtual ~RangeSamplingBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, RangeSamplingOutcome) = 0;
};
struct RangeSamplingDiagnostics {
    bool original = false;
    std::array<bool, range_sampling::entries.size()> entry_original{};
    std::array<std::uint64_t, range_sampling::entries.size()> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    RangeSamplingBoundaryObserver* observer = nullptr;
};
RangeSamplingDiagnostics& range_sampling_diagnostics(State&);
const RangeSamplingDiagnostics& range_sampling_diagnostics(const State&);
bool try_modern_range_sampling(State&, std::uint32_t);
}
