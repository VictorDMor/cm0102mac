#pragma once
// DATABASE.CPP's save block (tools/database_audit.py): typed functions, one per
// original routine, as database.hpp describes.
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;
// 0x100abfb0 ("Saving database"): writes (writing != 0) or reads (writing == 0) the database's record tables through
// a zip directory, one file per table, each with its directory entry, then the index file; returns 1 on success and
// 0 on the first failure, after closing what it opened and (when the loose flag byte at TOC -0x39d0 is set)
// resolving the indexes again. `archive`, `extra_a` and `extra_b` are passed to the stream opener (inferred: the
// archive handle and two names).
std::uint32_t save_database(Cpu& c, std::uint32_t archive, std::uint32_t writing, std::uint32_t extra_a,
                            std::uint32_t extra_b);
}
