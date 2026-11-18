#pragma once
// COMP.CPP's calendar block: a competition's fixture dates, their sort order, its first date, and the
// nation descriptors' codes that fixtures ask for. Declared in namespace cm::aot::comp (comp.hpp includes this).
#include "guest_abi.hpp"

namespace cm::aot::comp {
using guest::Cpu;
// Every fixture date of a competition (comp_calendar.cpp): a new[] of 0x14-byte rows, `count_out` the number of
// rows (the allocation's 1000 until the end), or 0 when the competition has no dates or an allocation fails
// (the assertion; the success flag cleared). Row: +0 stage (-1 the competition itself), +4 round, +8 the
// competition record's id, +0xc/+0xe the date's first two halfwords, +0x10 the virtual's second out halfword,
// +0x12 a round's kind offset. Sorted with compare_fixture_dates (toc -0x61a0).
std::uint32_t fixture_dates(Cpu&, std::uint32_t competition, std::uint32_t count_out);          // 0x10450ee0
// qsort's order of two rows: halfword +0xe, then +0xc, then the competitions' reputation (record +0x18, the
// larger first), then their texts (0x104769d0 into the shared text buffer) by strcmp. The strcmp result as is.
std::int32_t compare_fixture_dates(Cpu&, std::uint32_t left, std::uint32_t right);               // 0x10451470
// The competition's first date, a 5-byte date (two halfwords and a byte) written to `out`.
void first_date(Cpu&, std::uint32_t out, std::uint32_t competition);                              // 0x10453190
// The row of toc::descriptors whose code byte equals the signed byte `code`: its index, or the assertion and 3.
std::uint32_t descriptor_index(Cpu&, std::int8_t code);                                           // 0x10451720
// A fixture's descriptor code (comp_regions.cpp); `context` may be 0 (a fixture without a match).
std::uint32_t fixture_descriptor_code(Cpu&, std::uint32_t fixture, std::uint32_t context);        // 0x10451800
// The descriptor code of a club's nation (descriptor::none or 0 from 0x100c7d00 when no row matches).
std::uint32_t nation_descriptor_code(Cpu&, std::uint32_t club);                                   // 0x10451990
}
