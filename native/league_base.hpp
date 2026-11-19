#pragma once
// LEAGUE.CPP (tools/league_audit.py): the base league class. It derives from COMP.CPP's competition, and
// every league table of the game (the domestic divisions, the regional championships, the qualifying and
// group stages run as tables) derives from it: the object's lifetime, the standings and their order, the
// season's setup and rollover, fixtures, playoffs, the save file's league records and the news and texts the
// leagues share. league_base_detail.hpp names the object's fields. Each block of the module declares its
// routines in its own header, included at the end.
#include "guest_abi.hpp"

namespace cm::aot::league_base {
using guest::Cpu;
// The object (league_base_object.cpp). The constructor and destructor return the object, as CodeWarrior's do.
std::uint32_t construct(Cpu&, std::uint32_t league);                                          // 0x1049e4f0
// `deleting` is the r4 word: its low halfword, signed, above 0 frees the object.
std::uint32_t destroy(Cpu&, std::uint32_t league, std::uint32_t deleting);                    // 0x1049e600
// The virtuals a derived league must override: an assertion (source lines 0xb8, 0xcf, 0xe1), then 0 (slot +0x44),
// -1 (slot +0x94), or nothing (slot +0x10: the original leaves the success flag's address in r3).
std::uint32_t unsupported_44(Cpu&);                                                            // 0x1049e970
std::uint32_t unsupported_94(Cpu&);                                                            // 0x1049ea10
std::uint32_t unsupported_10(Cpu&);                                                            // 0x1049eaa0
// The address of the standings row `index` (no bounds test; 32-bit wrapping as `mulli`).
std::uint32_t standing(Cpu&, std::uint32_t league, std::uint32_t index);                     // 0x1049ea00
// Whether the league promotes automatically with bit 1 of its flags (+0xc0 nonzero and +0xdc & 2) (inferred).
bool promotes(Cpu&, std::uint32_t league);                                                    // 0x104a14f0
// Whether the league relegates (+0xc3 nonzero).
bool relegates(Cpu&, std::uint32_t league);                                                   // 0x104a1520
// Sort orders (league_base_order.cpp), as qsort comparators of two row addresses. `by_word`: the rows' words at +0,
// ascending; `by_reputation`: the halfword +0x1ec of the records the rows point at, larger first;
// `by_half_a`: the rows' halfword +0xa, larger first. Differences wrap as the original's `subf`.
std::int32_t compare_by_word(Cpu&, std::uint32_t left, std::uint32_t right);                  // 0x104aaaf0
std::int32_t compare_by_reputation(Cpu&, std::uint32_t left, std::uint32_t right);            // 0x104aab00
std::int32_t compare_by_half_a(Cpu&, std::uint32_t left, std::uint32_t right);                // 0x104aab20
}
// Blocks append their header here.
#include "league_base_loader.hpp"
#include "league_base_rollover.hpp"
#include "league_base_fixtures.hpp"
#include "league_base_table.hpp"
#include "league_base_storage.hpp"
#include "league_base_season.hpp"
#include "league_base_news.hpp"
