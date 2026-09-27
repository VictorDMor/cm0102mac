#pragma once
#include <chrono>
#include <cstdint>
#include <string_view>

namespace cm::aot {
struct State;
// Sparse boundary observations; no per-instruction counters or clock changes.
class PerformanceTrace {
public:
    PerformanceTrace();
    void mark(std::string_view event, const State& state, std::uint32_t detail = 0) const;
private:
    bool enabled_;
    std::chrono::steady_clock::time_point start_;
};
}
