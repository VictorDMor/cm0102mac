#include "guest_profiler.hpp"
#include "aot_runtime.hpp"
#include <array>
#include <csignal>
#include <fstream>
#include <pthread.h>
#include <stdexcept>
#include <sys/time.h>
#include <vector>

namespace cm::aot::guest_profiler {
namespace {
// The handler never allocates and reads guest memory only inside ranges
// mapped when sampling started (mappings only grow), so a read cannot fault;
// with a read trace or date observer attached it records PC and LR only, as
// those reads would call out. ITIMER_PROF counts process CPU time and may
// signal any thread: samples landing on other threads (AppKit, audio) are
// dropped, so the sample count undercounts game-thread time.
constexpr unsigned depth = 64;
struct Sample { std::uint32_t count; std::array<std::uint32_t, depth> chain; };
std::vector<Sample> buffer;
volatile std::sig_atomic_t used = 0;
const State* sampled = nullptr;
pthread_t owner;
std::vector<std::pair<std::uint32_t, std::uint64_t>> ranges;
bool readable(std::uint32_t address) {
    for (const auto& [start, size] : ranges)
        if (address >= start && std::uint64_t(address) + 4 <= start + size) return true;
    return false;
}
void sample(int) {
    if (!sampled || !pthread_equal(pthread_self(), owner) || std::size_t(used) >= buffer.size()) return;
    auto& entry = buffer[std::size_t(used)];
    unsigned count = 0;
    entry.chain[count++] = sampled->pc;
    entry.chain[count++] = std::uint32_t(sampled->r<4>(4128));
    if (sampled->memory.read_trace || sampled->memory.date_observer) { entry.count = count; used = used + 1; return; }
    // Each frame's back chain leads to its caller's frame, whose word at +8
    // holds the return address the callee saved there.
    for (auto frame = std::uint32_t(sampled->r<4>(4)); count < depth && readable(frame);) {
        const auto caller = std::uint32_t(sampled->memory.read<4>(frame));
        if (caller <= frame || !readable(caller + 8)) break;
        entry.chain[count++] = std::uint32_t(sampled->memory.read<4>(caller + 8));
        frame = caller;
    }
    entry.count = count;
    used = used + 1;
}
}
void reset(std::size_t capacity) {
    stop();
    buffer.assign(capacity, {});
    used = 0;
}
void release() { stop(); buffer = {}; used = 0; }
void start(const State& state) {
    stop();
    ranges = state.memory.mapped_ranges();
    owner = pthread_self();
    sampled = &state;
    std::signal(SIGPROF, sample);
    const itimerval every_millisecond{{0, 1000}, {0, 1000}};
    setitimer(ITIMER_PROF, &every_millisecond, nullptr);
}
void stop() {
    const itimerval off{};
    setitimer(ITIMER_PROF, &off, nullptr);
    sampled = nullptr;
}
bool running() { return sampled != nullptr; }
std::size_t samples() { return std::size_t(used); }
std::size_t write(const std::filesystem::path& path) {
    std::ofstream out(path, std::ios::binary);
    for (std::size_t i = 0; i < std::size_t(used); ++i)
        out.write(reinterpret_cast<const char*>(&buffer[i]), sizeof(std::uint32_t) * (1 + buffer[i].count));
    if (!out) throw std::runtime_error("Cannot write guest profile " + path.string());
    return std::size_t(used);
}
}
