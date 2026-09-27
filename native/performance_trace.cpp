#include "performance_trace.hpp"
#include "aot_runtime.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/resource.h>

namespace cm::aot {
PerformanceTrace::PerformanceTrace() : enabled_([] {
    const auto value = std::getenv("CM_AOT_PERFORMANCE_TRACE");
    return value && std::strcmp(value, "1") == 0;
}()), start_(std::chrono::steady_clock::now()) {}
void PerformanceTrace::mark(std::string_view event, const State& s, std::uint32_t detail) const {
    if (!enabled_) return;
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage)) return;
    const auto seconds = [](timeval value) { return double(value.tv_sec) + value.tv_usec / 1e6; };
    const auto wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
    std::fprintf(stderr, "PERF %.*s wall=%.6f user=%.6f system=%.6f instructions=%llu pc=%08x lr=%08x detail=%u\n",
        int(event.size()), event.data(), wall, seconds(usage.ru_utime), seconds(usage.ru_stime),
        static_cast<unsigned long long>(s.executed), s.pc, unsigned(s.r<4>(4128)), detail);
}
}
