#include "guest_worker_pool.hpp"
#include "aot_runtime.hpp"
#include "guest_scratch.hpp"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <string_view>
#include <sys/sysctl.h>

namespace cm::aot {
namespace {
unsigned worker_count() {
    unsigned count = 0;
    if (const char* value = std::getenv("CM_MATCH_WORKERS")) {
        const std::string_view text(value);
        std::from_chars(text.data(), text.data() + text.size(), count);
    }
    if (!count) {
        int cores = 0;
        std::size_t size = sizeof cores;
        if (sysctlbyname("hw.perflevel0.physicalcpu", &cores, &size, nullptr, 0) != 0 || cores < 1) cores = 4;
        count = std::min(unsigned(cores), 8u);
    }
    return std::clamp(count, 1u, unsigned(GuestScratch::limit));
}
State* owner = nullptr;
std::vector<std::unique_ptr<GuestWorker>> list;
}
std::vector<std::unique_ptr<GuestWorker>>& guest_workers(State& main) {
    if (owner != &main || list.empty()) {
        list.clear();
        owner = nullptr;
        std::vector<std::unique_ptr<GuestWorker>> made;
        for (unsigned n = 0, count = worker_count(); n < count; ++n) made.push_back(std::make_unique<GuestWorker>(main, n));
        list = std::move(made);
        owner = &main;
    }
    return list;
}
}
