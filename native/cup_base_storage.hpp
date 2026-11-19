#pragma once
// CUP.CPP, block `storage`: the cup's save reader and writer (vt +0x20 and +0x24). The record is
// "CM3_TEMP/<competition name>_<n>.tmp" (n: the competition's id) or, for a reader called with `backup` zero, the
// season's year. Dependencies (other modules' routines or the run-time library, all called through the
// dispatcher): the file layer (fopen 0x10739de0, the folder path 0x10746280, fread 0x10775fe0, fwrite 0x10776360,
// fclose 0x107766f0), the allocator (0x10773520 new, 0x10775130 new[], 0x10775170 delete[]), the assertion report
// and the localized format (0x10739890, 0x10768810, 0x1077b720, 0x10727890, 0x10769b60), exit 0x1076f7d0,
// COMP_STATS' 0x104533a0/0x104536c0/0x1045f6a0 (reader) and 0x10460230 (writer), the stage loaders
// 0x104bc750/0x104bcb20 (CUP_STAGE.CPP) and 0x1056e120/0x1056e430 (LEAGUE_STAGE.CPP) and the stage writers
// 0x104bd550 and 0x1056f6f0, and the virtuals +0x28 (the reader) and +0x2c (the writer): each derived cup keeps
// its own records there.
#include "guest_abi.hpp"

namespace cm::aot::cup_base {
// CUP::load_from_disk (vt +0x20, 0x104831a0): returns 1 when the whole record was read and the file closed; 0 when
// the file does not open (nothing else touched) or after a failed read or allocation, which reports an assertion
// and clears the success flag. The failures of 20 of the 26 sites then call `exit`, which never returns in the
// game; the file stays open on every failure, as the original.
std::uint32_t load_from_disk(Cpu&, std::uint32_t cup, std::uint32_t backup);                  // 0x104831a0
// CUP::save_cup_to_disk (vt +0x24, 0x10484420): writes the same record (`temporary` nonzero: the id names the
// file; zero: the year names it, and an inactive cup (+0x45 zero) writes nothing). Void: the club pointers of the
// entrant rows and of the 8-byte rows at +0x14 become the clubs' first words (-1: none) for the write and back.
void save_to_disk(Cpu&, std::uint32_t cup, std::uint32_t temporary);                          // 0x10484420 (vt +0x24 slot of CUP)
}
