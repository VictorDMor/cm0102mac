#pragma once
// CUP.CPP (tools/cup_audit.py): the base cup class. It derives from COMP.CPP's competition, and every knockout
// competition of the game (the national cups, the super cups, the continental cups and the knockout stages of
// CUP_STAGE.CPP) derives from it: the object's lifetime, the round calendar, the draws (rand), fixtures and
// their results, the daily update, the save file's cup records and the news the cups share.
// cup_base_detail.hpp names the object's fields. Each block of the module declares its routines in its own
// header (cup_base_<block>.hpp), included at the end.
#include "guest_abi.hpp"

namespace cm::aot::cup_base {
using guest::Cpu;
// The object (cup_base_object.cpp). The constructor and destructor return the object, as CodeWarrior's do.
std::uint32_t construct(Cpu&, std::uint32_t cup);                                             // 0x10478f20
// `deleting` is the r4 word: its low halfword, signed, above 0 frees the object.
std::uint32_t destroy(Cpu&, std::uint32_t cup, std::uint32_t deleting);                       // 0x10478f70 (vt +0x08)
// The virtuals a derived cup must override: an assertion (source lines 0x54, 0x60), then 0 (slot +0x44) or
// -1 (slot +0x94).
std::uint32_t unsupported_44(Cpu&);                                                            // 0x10478fd0
std::uint32_t unsupported_94(Cpu&);                                                            // 0x10479060
// Clears the byte +0xb0 (slot +0x10).
void clear_b0(Cpu&, std::uint32_t cup);                                                        // 0x104790f0
// Whether `club` is one of the entrants (+0xa4 rows, +0x3e of them; slot +0x88).
bool has_entrant(Cpu&, std::uint32_t cup, std::uint32_t club);                                // 0x10486050
// Sort orders (cup_base_order.cpp), as qsort comparators of two row addresses; differences wrap as the
// original's `subf`. `by_byte_6`: the rows' signed byte +6, larger first. The entrant orders (other units pass
// them to qsort through their TOC slots -0x60ec and -0x6128): the signed byte +4, larger first
// (`by_state_descending`) or smaller first (`by_state_ascending`), then the halfword +0x1ec of the clubs the rows
// point at, smaller first.
std::int32_t compare_by_byte_6(Cpu&, std::uint32_t left, std::uint32_t right);                // 0x10481e70
std::int32_t compare_by_state_descending(Cpu&, std::uint32_t left, std::uint32_t right);      // 0x10485830
std::int32_t compare_by_state_ascending(Cpu&, std::uint32_t left, std::uint32_t right);       // 0x10485870
}
// Blocks append their header here.
#include "cup_base_draws.hpp"
#include "cup_base_storage.hpp"

#include "cup_base_result.hpp"
#include "cup_base_tree.hpp"
#include "cup_base_daily.hpp"
#include "cup_base_queries.hpp"
#include "cup_base_rounds.hpp"
