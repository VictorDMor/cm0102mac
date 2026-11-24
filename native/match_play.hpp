#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace cm::aot {
struct State;
namespace guest { class Cpu; }

// The play phase of one background match on a guest worker (TASKS.md, "Parallel play phase"): what makes the
// worker's context private, and the shared writes the phase holds back for the game thread.
// Private, set up by begin_play: the worker's copy of the game's TOC (r2 points into it), its RNG stream (the
// pool cursor, the salt and the C runtime's state, all seeded from `seed`), and its copies of the scratch
// statics that plays of one day were measured to share (DATE's buffers, one word of MATCH_ENG).
// Held back, replayed by replay_deferred in fixture order after the phase:
//  - TACTICS 0x103af130, the formation results of a finished fixture;
//  - INJURY 0x10186cd0, the contact notes: merged here by the original's rule so the play gets the bytes it
//    would get from the real list (were the person's entry new there, as it is unless a stale one survived);
//  - OFFICIALS_MANAGER 0x102d4590, the referee's grade.
struct PlayContext {
    struct Result { std::uint32_t tactics, fixture; };
    struct Note { std::uint32_t key; std::uint8_t kind, strength; };
    struct Grade { std::uint32_t officials; std::array<std::uint8_t, 8> record; };
    std::vector<Result> results;
    std::vector<Note> notes;
    std::vector<Grade> grades;
    // Offsets from r2 of the words of the TOC copy the play left changed, other than its RNG state: small
    // data the play wrote, which stays with the copy. The game thread reports each offset once.
    std::vector<std::int32_t> private_writes;
    // INJURY 0x10186cd0 on a worker.
    void note(guest::Cpu&, std::uint32_t key_at, std::uint32_t kind_at, std::uint32_t strength_at);
};
// On the worker's thread, around one match: the State must own a scratch range (guest_scratch.hpp).
void begin_play(State& worker, PlayContext&, std::uint64_t seed);
void end_play(State& worker, PlayContext&);
// On the game thread, after a stretch: names on stderr each small-data word a play wrote that was not seen before.
void report_private_writes(const PlayContext&);
// On the game thread: the held-back calls of one match. `cells` is the address of 16 free bytes of guest memory.
void replay_deferred(guest::Cpu&, const PlayContext&, std::uint32_t cells);
// For the league's routines a play enters (the order of the standings and the title check work in buffers
// the league owns): one play at a time. Does nothing on the game thread.
class LeagueTurn {
public:
    explicit LeagueTurn(const State&);
    ~LeagueTurn();
    LeagueTurn(const LeagueTurn&) = delete;
    LeagueTurn& operator=(const LeagueTurn&) = delete;
private:
    bool held;
};
}
