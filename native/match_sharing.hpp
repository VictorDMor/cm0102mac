#pragma once
#include <iosfwd>
#include <memory>

namespace cm::aot {
struct State;
struct MemoryObserver;

// Diagnostic for cm-world-replay --match-sharing: which guest memory each
// background match reads and writes, and how much of it another match (or the
// code between matches) wrote before or reads after. It answers what running
// matches on several threads would have to separate. Spans come from the
// dispatcher's probe at the match routines' entries and end when the stack
// pointer is back at its entry value; every access inside a span belongs to
// that match. Stack accesses are counted and otherwise left out (a thread
// would have its own stack). It observes only; results are unchanged.
class MatchSharing {
public:
    explicit MatchSharing(State&);
    ~MatchSharing();
    MemoryObserver* observer();
    // Called by the dispatcher probe before the page at s.pc runs.
    void probe(State&);
    void report(std::ostream&) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
