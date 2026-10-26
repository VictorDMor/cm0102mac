#pragma once
// SHORTLIST_MANAGER.CPP: the shortlist manager object: construction, destruction, the category lists' reset, the random range draw, the category sort, "shortlist manager" load and save.
//
// The manager (224 bytes) and the records it owns are records::Manager,
// ClubEntry, Row and CategoryEntry. Other groups call these routines
// directly: `enter(c, back, {arguments})` with the original `bl`'s return
// address, then the C++ call (guest_values.hpp).
#include "guest_values.hpp"
#include "shortlist_manager_records.hpp"

namespace cm::aot::shortlist_manager {
using guest::Cpu;
// The records it works on (shortlist_manager_records.hpp).
namespace storage {
using namespace records;
}

// 0x10356410: a new manager at `manager`, read from `archive` (the save's
// "shortlist manager") when that is not null, else fresh: every club's forty
// records and the category lists rebuilt. Returns the manager.
std::uint32_t construct_storage(Cpu& c, std::uint32_t manager, std::uint32_t archive);
// 0x103567b0: frees what the manager owns; deletes it when the low halfword
// of `flags` is positive. Returns the manager.
std::uint32_t destroy_storage(Cpu& c, std::uint32_t manager, std::uint32_t flags);
// 0x103568e0: rebuilds both banks of category lists from every person (the
// original's second argument is unused).
void reset_storage(Cpu& c, std::uint32_t manager);
// 0x10356e40: a draw from the game's random table in [0, range) for a
// positive range, (range, 0] for a negative one within 16 bits, 0 for 0.
// Ranges beyond 16 bits scale a draw by range / 65535 and add a finer draw
// at a tenth of the step (quirk: a negative wide range always gives
// range - 1). Callers: `enter(c, back, {range})`, then this.
std::int32_t sample_range(Cpu& c, std::int32_t range);
// 0x10357d30: rates every entry of both banks (CategoryEntry::rating) and
// sorts each list by rating, then person.
void rate_categories(Cpu& c, std::uint32_t manager);
// 0x10358020: reads the manager from the save archive (any save version).
void load_storage(Cpu& c, std::uint32_t manager, std::uint32_t archive);
// 0x10358af0: the legacy record array's element constructor (the record's
// date to its default). Returns the record.
std::uint32_t initialize_record(Cpu& c, std::uint32_t record);
// 0x10358b30: writes the manager to the save archive.
void save_storage(Cpu& c, std::uint32_t manager, std::uint32_t archive);
}
