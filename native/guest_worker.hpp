#pragma once
#include <cstdint>
#include <memory>

namespace cm::aot {
struct State;

// A second execution context over the game's memory: its own registers and host thread, the same guest bytes.
// The first step towards playing background matches on several threads (TASKS.md, "Parallel play phase").
// For now one call at a time runs on it while the caller waits, on the caller's guest stack, so the result
// must equal running the call in line; `cm-world-replay --advance-on-worker` checks that.
// Limits of this step: the worker's State has default diagnostics (no `*_original` selection, no boundary
// observers of main), and it refuses to run under hardware bounds, the game's mode.
class GuestWorker {
public:
    explicit GuestWorker(State& main);
    ~GuestWorker();
    GuestWorker(const GuestWorker&) = delete;
    GuestWorker& operator=(const GuestWorker&) = delete;
    // Runs the routine `main` is about to enter (its pc and registers) to `return_pc` on the worker, and leaves
    // main as the routine's return leaves it. Throws what the routine throws, and when the call changed the
    // host's allocation records (a heap that grows belongs to the game thread).
    void call(std::uint32_t return_pc);
    std::uint64_t calls() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
