#pragma once
#include <cstdint>
#include <iosfwd>
#include <memory>

namespace cm::aot {
struct State;
struct MemoryObserver;

// BlockMoveData reads its source with one bulk copy; while set, the bridge
// reports that source (address, length) here first (the audit sets it).
extern void (*block_move_reads)(State&, std::uint32_t address, std::uint32_t length);

// Finds reads of guest stack bytes that the original never wrote since they
// were last below the stack pointer: values left behind by earlier calls.
// Rewritten code that changes what it leaves on the stack changes what such
// reads see, so their sites must keep (or reproduce) the original's leftovers.
// Install as the State's date observer (it sees every scalar read and write,
// and makes C++ owners take their observed paths, which run the original's
// accesses). Stack: 0x70000000-0x70100000.
class StackAudit {
public:
    explicit StackAudit(State&);
    // Also replaces each stale byte with a pseudo-random one as the game reads
    // it, so a run shows whether the game's results depend on what it leaves on
    // the stack. CM_STACK_SCRAMBLE_ZERO=1 uses zero instead (a canonical stack),
    // CM_STACK_SCRAMBLE_PCS=lo-hi limits it to reads whose PC is in [lo, hi), and
    // CM_STACK_SCRAMBLE_SKIP=pc,pc,... leaves the reads at those PCs alone.
    StackAudit(State&, std::uint64_t scramble_seed);
    ~StackAudit();
    MemoryObserver* observer();
    // Sites by original PC (run start in generated code), most reads first,
    // with the PCs that left the bytes they read (0: before the run).
    // Complete lists every site, offset and writer: a rewrite's leftover
    // contract (CLAUDE.md) needs all of them, not the most frequent. Each site
    // also shows how many of its reads saw a stale byte that was not zero and,
    // in the complete list, the first 12 distinct values its scalar reads saw.
    void report(std::ostream&, bool complete = false) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
