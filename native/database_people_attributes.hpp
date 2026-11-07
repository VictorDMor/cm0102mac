#pragma once
// DATABASE.CPP's people block, part "attributes": typed functions (see database_people.hpp).
// Names are inferred from what the routines do.
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// Scales a person's slot values for the generator (inferred): for every row of the people table (TOC -0x7be0,
// count at TOC -0x7be4) with a player record (+0x14) the 54 attribute slots are multiplied by 10, lowered by
// the record's 16-bit +0x4, clamped to +-200 and halved, written back, and then settled by the two routines
// below; a row with a second record (+0x1c) gets the shorter attribute list scaled the same way (with its +0x24),
// written back and balanced (balance_attributes).
void rescale_people_attributes(Cpu&);                               // 0x100c2520
// Settles the slots of a person's player record (+0x14) around a shifting base (inferred): the slots within a
// limit (widened by 5 until 12 fit) give an average that shifts the others (clamped to +-0x7d); positive slots
// above a count are pulled down; the player's bytes +0x2a/+0x3a move towards each other when its byte +0xf is at
// least 15. The hook word at TOC -0x7a7c (when set) is told before (flag 0) and after (flag 1) via 0x103d3f70.
void settle_player_attributes(Cpu&, std::uint32_t person);          // 0x100c2800
// The same with a weighted start (inferred): the base is derived from six weighted attribute bytes and the
// player's rating, slots 0x18 and 0x1e are left alone, and byte +0xf equal to 20 also evens out bytes
// +0x1e/+0x24/+0x39.
void settle_player_attributes_weighted(Cpu&, std::uint32_t person);  // 0x100c2cb0
}
