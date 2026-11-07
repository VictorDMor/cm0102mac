#pragma once
// DATABASE.CPP's tables block (tools/database_audit.py): typed functions, one per
// original routine, as database.hpp describes.
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// Which clubs of the club table stay in the database (inferred). The clubs are grouped by their nation's number
// (up to 20, or 15 for the small size classes, per group, keeping the highest-rated: halfword +0x1ec), then the
// clubs of the groups and those the staff, link and pair tables point at are marked. `out` (one word per club id)
// receives the address the club's record will have in the compacted table, or 0 for a dropped club; the kept
// clubs' new indexes are reported through 0x10159050. Returns 1; 0 (after the assert) when the first word of
// the 0x128-byte record table (TOC -0x7b98) is 0 or `out` is null. `mode` is the whole register (its low byte,
// signed, is the mode: 1 and 2 are the small size classes).
std::uint32_t choose_kept_clubs(Cpu&, std::uint32_t out, std::uint32_t mode);    // 0x100a34e0

// The club table compaction (inferred): 0x100a34e0 decides which clubs stay, the other tables' club pointers are
// rewritten to the new records, the kept clubs move to the front (their id becomes the new index), the table
// shrinks, and "Club table has %d entries" is formatted. `mode` is passed on to choose_kept_clubs.
std::uint32_t compact_club_table(Cpu&, std::uint32_t mode);    // 0x100a2da0

// Expands the loaded records (0x9d bytes each, TOC-relative variables at -0x39e0/-0x39dc, plus `extra` rows
// counted at -0x39d8) into the 0x70-byte staff table at TOC -0x7be0 and the 0x34-byte table at -0x74bc (row count
// at -0x74c0), then releases the loaded records. Inferred: staff the database file lists. Failed preconditions
// assert and call the exit routine.
void build_staff_tables(Cpu&);    // 0x100ab5d0

// Which staff of the staff table stay (inferred), the counterpart of choose_kept_clubs: first generates the extra
// staff (0x100a3db0), then (unless mode 1 or 2) groups staff by nation keeping the best by rating, and finally a pass of
// tests with inline pool draws keeps or drops the rest. `out` (one word per staff id) receives the address the
// staff's record will have in the compacted table, or 0. Returns 1; 0 after an assert. `mode` is the whole register.
std::uint32_t choose_kept_staff(Cpu&, std::uint32_t out, std::uint32_t mode);    // 0x100a1950

// Drops the staff 0x100a1950 did not keep from the staff table and from the three tables of rows its staff point at
// (0x48-, 0x44- and 0x34-byte), rewriting the staff pointers of the club table and the other tables, then adds
// generated ("virtual") staff and writes "Staff table has %d(+%d Virtual) entries" and "Contract table has %d
// entries". `mode` is the whole register (its low byte goes to 0x100a1950; 1 and 2 pick the virtual share).
void compact_staff_table(Cpu&, std::uint32_t mode);    // 0x1009efc0
}
