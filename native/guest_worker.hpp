#pragma once
#include <cstdint>
#include <memory>
#include <optional>

namespace cm::aot {
struct State;

// A second execution context over the game's memory: its own registers and host thread, the same guest bytes.
// The first step towards playing background matches on several threads (TASKS.md, "Parallel play phase").
// For now one call at a time runs on it while the caller waits. On the caller's guest stack the result must
// equal running the call in line; on its own stack the bytes a routine leaves below the stack pointer land
// there, so code that reads leftovers may differ (`cm-world-replay --advance-on-worker [--worker-stack]`).
// With its own stack it also has its own heap, and block addresses differ from a run in line.
// The worker chooses routines as main does at each call (copy_configuration) and shares its observers.
// Under hardware bounds a stray access on its thread reports the worker's context.
class GuestWorker {
public:
    // Guest stacks of workers, the size of the game's own at 0x70000000, one per index.
    static constexpr std::uint32_t stack_size = 0x100000;
    static constexpr std::uint32_t stack_start(unsigned index) { return 0x71000000 + index * stack_size; }
    // With `own` (an index below GuestScratch::limit), the worker runs on that guest stack and allocates from
    // that scratch heap (guest_scratch.hpp), both mapped in main's memory when not there yet: make it on the
    // game thread. Without, calls run on the caller's stack and use the game's heap.
    // `scratch` false keeps the game's heap on the own stack (a diagnostic: block addresses then stay comparable).
    explicit GuestWorker(State& main, std::optional<unsigned> own = std::nullopt, bool scratch = true);
    ~GuestWorker();
    GuestWorker(const GuestWorker&) = delete;
    GuestWorker& operator=(const GuestWorker&) = delete;
    // Runs the routine `main` is about to enter (its pc and registers) to `return_pc` on the worker, and leaves
    // main as the routine's return leaves it. Throws what the routine throws, and when the call changed the
    // host's allocation records (a heap that grows belongs to the game thread).
    void call(std::uint32_t return_pc);
    std::uint64_t calls() const;
    // Bytes of the scratch heap its pool holds from the host records now (0 without one).
    std::uint32_t scratch_in_use() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
