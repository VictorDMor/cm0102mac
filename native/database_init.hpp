#pragma once
// DATABASE.CPP's init block (tools/database_audit.py): typed functions, one per
// original routine, as database.hpp describes.
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// "Player Initialisation" (0x100b8cf0), run when a new game starts: for every row of the person table (TOC -0x7be0,
// count at TOC -0x7be4; 0x70-byte rows) it fills in what the database leaves unset, drawing from the sample pool
// (the compiler's inline bounded draw, in database_init_detail.hpp). Per row, in this order: missing name records,
// the dates (a birth date no later than the table's limit, two 31-day dates in 1900-1901 range), a progress
// message every count/100 rows, the nine 1-20 ratings of bytes +0x61.. (-1 or out-of-range ones generated), a
// nation-based shift of two of them, the nation (from the club, else a random row), the player record's three
// abilities and attributes, the age, and the staff record's values; at the end the rescale of all people
// (0x100c2520). `date` is the reference date (r3: a 16-bit year-day, a 16-bit year and a byte, as the dates
// at +0x4c are). Without people the assertion fires ("Nation '%s' has no reputation set" is the message
// text of the pool's entry, kept as the original's) and the routine goes on.
void player_initialisation(Cpu&, std::uint32_t date);   // 0x100b8cf0
}
