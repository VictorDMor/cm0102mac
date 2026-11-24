#pragma once
#include <cstdint>

namespace cm::aot {
struct State;

// A private heap for a guest worker (guest_worker.hpp), so matches on several threads do not meet in the game's
// allocator. It is a range of guest memory mapped whole in advance, which the worker's State hands out through
// its own host records (never growing, never mapping), and inside it one pool context of the original allocator:
// on the worker every allocation of the game goes to that pool. The range starts zeroed. A block stays in the pool it came from:
//  - the worker may release or resize only its own blocks (another's is a logic error, thrown);
//  - any other State releases a scratch block into its pool, which is safe only while that worker is idle
//    (the blocks a match leaves behind are released by the game thread after the play phase).
// In the reference week no block from outside is released inside the play phase (TASKS.md).
struct GuestScratch {
    static constexpr std::uint32_t size = 0x1000000, limit = 12;
    static constexpr std::uint32_t start(unsigned index) { return 0x74000000 + index * size; }
    // Maps the range in main's memory when it is not there, and gives `worker` its records. On the game thread,
    // before the worker runs; the registration lasts until the matching `leave`.
    static void enter(State& main, State& worker, unsigned index);
    static void leave(State& worker);
    // Bytes of the range the worker's records hold now (the pool's arenas; no peak is kept).
    static std::uint32_t in_use(const State& worker);
};

// For the allocator's core routines: the pool context this call uses in place of the one it was given, or 0.
// `block` is the block being released or resized, 0 for a new one.
std::uint32_t scratch_pool(const State&, std::uint32_t block);
// For the host's block records: the State whose records cover `address` (a scratch range's worker, else `s`).
State& heap_records(State& s, std::uint32_t address);
// Whether this State's records are a scratch range's (they must not grow).
bool is_scratch(const State&);
}
