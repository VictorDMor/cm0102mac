#pragma once
#include <iosfwd>
#include <memory>

namespace cm::aot {
struct State;
struct MemoryObserver;

// Diagnostic for cm-world-replay --club-sharing: what the daily shortlist work of two clubs shares. A span is
// one call the world loop makes for a club (refresh_club_shortlist 0x10357cb0, approach_targets 0x1036d6e0);
// it reports the words one club's spans write and another club's spans of the same game day read or write,
// outside the stack, the blocks the club's own spans allocated, the RNG and the allocator, by the code that
// did it. It answers what running those calls on several threads would have to separate (TASKS.md, "The
// shortlist manager's 37%"). It observes only; results are unchanged.
class ClubSharing {
public:
    explicit ClubSharing(State&);
    ~ClubSharing();
    MemoryObserver* observer();
    // Called by the dispatcher probe before the page at s.pc runs.
    void probe(State&);
    void report(std::ostream&) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
