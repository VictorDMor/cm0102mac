#pragma once
// DATABASE.CPP's resolve block (tools/database_audit.py): typed functions, one per
// original routine, as database.hpp describes.
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;
// 0x100af8a0: turns the stored indexes of every loaded record table into pointers (an index becomes the table's
// first record plus index * record size), clearing invalid ones to 0 and reporting some of them. `skip_squad_check` is the full r3 passed
// in; only its low byte counts (nonzero: club squads are not range-checked).
void resolve_database_indexes(Cpu& c, std::uint32_t skip_squad_check);
}
