#pragma once
// LEAGUE.CPP, block `fixtures` (league_base_fixtures.cpp): the order of a league's standings rows for the fixture
// list. Dependencies still generated: the league's virtual slots +0xb0 (the row a club is preset to, answered
// -1 by the base class's 0x104a60d0) and +0xb4 (told a club's final row, 0x104a60c0), the allocator 0x107735f0
// (new[] with a header), its release 0x10773610, the RNG 0x1077b770 (rand) and 0x1077b790 (srand), and the
// assertion's calls.
#include "league_base.hpp"

namespace cm::aot::league_base {
// 0x104a4970. Arranges the standings (the league's 0x44-byte rows at +0xb0) in a scratch copy and writes the
// copy back, so that row k and row k + pairs/2 are the two clubs of one match slot (inferred). Draws the sample
// stream of the game's random cursor (rand through the buffered stream) and reseeds srand twice: first with the
// season (the word at TOC -0x7450 plus the league's year), then back to a seed derived from the stream. A
// league with bit 0x100 of its flags (+0xdc) takes the rows the derived league presets (virtual +0xb0); the
// others pair the two clubs of the nation's derby ids first (when the competition is the one at TOC -0x67c0),
// then clubs of the same region (+0x3c of the club), then clubs one of which is the other's parent region
// (+0x14), then fill. `pairs` is the caller's second argument (0x104a00a0's r29: the number of match slots per
// round, inferred). A void routine (the generated caller reads r9 and r10 afterwards: the last release's).
void draw_fixture_order(Cpu&, std::uint32_t league, std::int32_t pairs);
}
