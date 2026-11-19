#pragma once
// LEAGUE.CPP, block `loader`: the league's save reader. Dependencies (still generated or other modules' routines,
// called through the dispatcher): the file layer (fopen 0x10739de0, the folder path 0x10746280, fread 0x10775fe0,
// fclose 0x107766f0), the allocator, the assertion report, COMP_STATS' 0x104533a0/0x1045f6a0, the stage loaders
// 0x104bc750/0x104bcb20 and 0x1056e120/0x1056e430, and the virtual +0x28 (each derived league reads its own
// records there).
#include "guest_abi.hpp"

namespace cm::aot::league_base {
using guest::Cpu;
// The league's record from the save (vtable +0x20, 0x104a6850). The file is "CM3_TEMP/<competition name>_<n>.tmp"
// (`backup` nonzero: n is the competition's id) or ".bak" (`backup` zero: n is the season's year). Returns 1 when the
// whole record was read; 0 when the file does not open (nothing else touched) or after the failed read or allocation
// that reported an assertion (the success flag cleared). The file stays open on every failure, as the original.
std::uint32_t load_save(Cpu&, std::uint32_t league, std::uint32_t backup);                    // 0x104a6850
}
