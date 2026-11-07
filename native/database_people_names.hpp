#pragma once
// DATABASE.CPP's people block, part "names": the passes that chain the generated people.
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// Fills the name fields of every row of the table at TOC -0x7be0 (0x70-byte rows, count at -0x7be4)
// that has a nation (+0xc): the first-name record (+0x28) and the second-name record (+0x2c) when the
// pickers answer a record whose first byte is not 0, and always a third record (+0x30).
void generate_names(Cpu&);                                                    // 0x100b3650
// The four generation passes in order (0x100b2b80, names, 0x100b3710, 0x100b3c70).
void generate_people(Cpu&);                                                   // 0x100b2b50
// Replaces the pointer fields of twelve record tables with the ids of the records they point at (the first
// word of the pointee, or its word at +0x8, +0x10, +0x20 or +0x68 for some fields; -1 for a null pointer).
// Inferred: the conversion before the world is saved. Leaves r0 with the last count, r3-r7 and r8-r12 as the
// tables' registers hold them, and cr0 from the last loop test.
void replace_pointers_with_ids(Cpu&);                                         // 0x100b44c0
}
