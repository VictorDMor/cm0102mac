#pragma once
// COMP.CPP, block `stages` (comp_stages*.cpp): the walk over a competition's stages and the queries about
// clubs and rounds that ask it. A stage is a competition object: stage -1 is the object itself, stages 0..last
// (+0x30) are the objects of the word array at +0xc. A league stage (kind 1) keeps its clubs in 8-byte rows at
// +0xa4, a knockout stage (kind 2) in 0x44-byte rows at +0xb0 (comp_detail.hpp).
#include "comp.hpp"

namespace cm::aot::comp {
// Where a club finished (0x10450710): walks the stages from the last to -1, skips a stage of code 0x3fc, and for
// the first stage that holds the club writes the stage's code (halfword) through `code_out`, the club's league
// byte or knockout byte through `flag_out`, and for a league stage the round the club's row appears in through
// `round_out` (the latest round of the stage's played rounds whose match rows name it).
void club_finish(Cpu&, std::uint32_t competition, std::uint32_t club, std::uint32_t code_out,
                 std::uint32_t round_out, std::uint32_t flag_out);                                   // 0x10450710
// Whether this competition has no parent and it and every competition chained after it (+0x20) answer the
// virtual +0x18 (arguments 1, 1) with a nonzero byte.
bool all_done(Cpu&, std::uint32_t competition);                                                      // 0x10450900
// The stage the competition is in: the first whose virtual +0x18 (0, 1) is false, moved back to the first stage
// of its group when it is a league that has played no round; the competition itself when there is none.
std::uint32_t current_stage(Cpu&, std::uint32_t competition);                                         // 0x10450a50
// The first stage of the run of stages with the same descriptor group (0x10464010 of the stage code) that ends
// at `index`; the checked stage accessor's assertion (line 0x36e) fires for an index outside -1..last.
std::int32_t first_stage_of_group(Cpu&, std::uint32_t competition, std::int32_t index);               // 0x10450c20
// Whether round `round` of stage `index` is played: always for a knockout stage, otherwise when the round is
// before the stage's current round, or is the current and last round and that round's kind is 0x8c. Stages past
// the last answer 0.
std::uint32_t round_played(Cpu&, std::uint32_t competition, std::int32_t index, std::int32_t round);  // 0x10450db0
// Whether the club takes part in the competition (its nation/continent against the record's, then its rows in
// the stages).
bool takes_part(Cpu&, std::uint32_t competition, std::uint32_t club);                               // 0x10451570
// Every distinct club of all stages into a new[] of 800 bytes (at most 200, in first-seen order); the count is
// returned and the array's address stored through `out`. An allocation failure asserts (line 0x67f), stores 0.
std::int32_t all_clubs(Cpu&, std::uint32_t competition, std::uint32_t out);                         // 0x10451d10
// Whether a club is still in the competition. Called by the match engine.
std::uint32_t club_still_in(Cpu&, std::uint32_t competition, std::uint32_t club);                   // 0x10452ed0
// `!0x10464880(halfword at +0x30 of the record)`; `record` is r4 (the object in r3 is unused).
std::uint32_t lacks_property(Cpu&, std::uint32_t record);                                            // 0x10451f50
// Three bytes of a club's stats row (+0x76, +0x77, +0x78 of 0x10459b50's row) when the competition is its own
// (owner -1); otherwise the same call on the competition object of the record's id (virtual +0x70). r4, r5 pass
// through untouched to the callee.
void set_club_bytes(Cpu&, std::uint32_t competition, std::uint32_t first, std::uint32_t second, std::uint32_t third);  // 0x10451f80
// The bench size (+0x46) and the substitutes (+0x47) of an own competition packed into the high and low nibbles
// of the fixture's +0x4e byte; otherwise the virtual +0x84 of the owning competition object. r4 passes through.
void pack_fixture_rules(Cpu&, std::uint32_t competition, std::uint32_t fixture);                    // 0x104530f0
// The virtual +0x5c (r4 passes through) of the competition when the signed byte at +0x42 of r4 is -1, else of
// the stage it names (unchecked).
std::uint32_t stage_virtual_5c(Cpu&, std::uint32_t competition, std::uint32_t other);               // 0x10451ca0
}
