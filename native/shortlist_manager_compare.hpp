#pragma once
// SHORTLIST_MANAGER.CPP: the sort and search comparators the module passes to
// qsort and bsearch (through their transition vectors, slot::compare_*_tv).
// Each returns <0, 0 or >0 as qsort expects; the original's quirks are kept.
#include "guest_abi.hpp"

namespace cm::aot::shortlist_manager {
using guest::Cpu;
// 0x10358f20 and 0x10358f50 (the same code twice): by the int32 at +8, then
// at +4.
std::uint32_t compare_by_value_then_key(Cpu& c, std::uint32_t a, std::uint32_t b);
// 0x10358f80: entries with an index at +0x10, then at +0x18 (not -1), first;
// then by the int32 at +0xc, descending; equal ones compare as 1.
std::uint32_t compare_linked_first(Cpu& c, std::uint32_t a, std::uint32_t b);
// 0x10358ff0: by the int8 at +0x24; equal ones compare as 1.
std::uint32_t compare_by_rank(Cpu& c, std::uint32_t a, std::uint32_t b);
// 0x103663b0 and 0x10366410: (first, second) int32 pairs, a null pointer
// first, by one word then the other; a -1 in the other word matches anything.
std::uint32_t compare_pairs_by_second(Cpu& c, std::uint32_t a, std::uint32_t b);
std::uint32_t compare_pairs_by_first(Cpu& c, std::uint32_t a, std::uint32_t b);
}
