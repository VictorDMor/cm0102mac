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
    ~StackAudit();
    MemoryObserver* observer();
    // Sites by original PC (run start in generated code), most reads first.
    void report(std::ostream&, unsigned limit = 60) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
