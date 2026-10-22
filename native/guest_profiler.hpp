#pragma once
#include <cstddef>
#include <filesystem>

namespace cm::aot {
struct State;
// Samples the guest PC, LR and the return addresses on the guest stack every
// millisecond of process CPU time (SIGPROF), on the thread that started it.
// tools/guest_profile.py charges the samples to original routines.
namespace guest_profiler {
// Allocates room for `capacity` samples (about 260 bytes each) and clears.
void reset(std::size_t capacity);
// Samples `state` until stop, appending to the samples taken since reset.
void start(const State& state);
void stop();
void release();
bool running();
std::size_t samples();
// Writes the samples taken since start; returns how many.
std::size_t write(const std::filesystem::path& path);
}
}
