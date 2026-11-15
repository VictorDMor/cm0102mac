#pragma once
// HUMAN_MANAGER.CPP block "lifecycle": the human-manager table and its records are built, set up
// for one human, and destroyed. `humans` is the table (r3 in the original), `human` one record of
// it; every routine returns its first argument as the original does, except 0x10145fd0.
#include "guest_abi.hpp"

namespace cm::aot::human_manager {
using guest::Cpu;
// The table: the 16 records are allocated and constructed, each set up for its human (0x10145fd0),
// or read from `file` (0x1014a300, when given); first waits for the game CD folder, showing the
// "insert CD" dialog until it is found. A failed allocation reports the failed check.
std::uint32_t build_table(Cpu&, std::uint32_t humans, std::uint32_t file);                    // 0x10145af0
// The table's destructor: offers and sorted entries of every record, then the array; the table
// itself is freed when `flag` is positive (its signed halfword).
std::uint32_t destroy_table(Cpu&, std::uint32_t humans, std::uint32_t flag);                   // 0x10145ea0
// One human's record from the tactics template (five tactic slots), the folders' paths and the
// defaults; the table's counters of humans on holiday / away are adjusted for the old state.
// Returns 1, or 0 after a failed check (no staff, or outside the table).
std::uint32_t start_human(Cpu&, std::uint32_t humans, std::uint32_t staff);                   // 0x10145fd0
// The record's and the tactic slot's constructors and destructors (vectors of the object arrays).
std::uint32_t construct_record(Cpu&, std::uint32_t human);                                    // 0x10145e10
std::uint32_t destroy_record(Cpu&, std::uint32_t human, std::uint32_t flag);                   // 0x10145d70
std::uint32_t construct_tactic(Cpu&, std::uint32_t tactic);                                   // 0x10145e70
std::uint32_t destroy_tactic(Cpu&, std::uint32_t tactic, std::uint32_t flag);                  // 0x10145dd0
}
