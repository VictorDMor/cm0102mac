#pragma once
#include "guest_abi.hpp"

// The original's sort: one entry (0x10768300) that picks a heapsort by the
// width of the rows, and the three heapsorts, which differ only in how they
// swap two rows (qsort4x.c, qsort8x.c and an unnamed byte-wise one).
namespace cm::aot::qsort4x {
// `compare` is the comparator's transition vector; it is called as
// compare(row, row) and a negative result means "first row sorts earlier".
void sort(guest::Cpu&, std::uint32_t rows, std::uint32_t count, std::uint32_t width, std::uint32_t compare);
void sort_by_bytes(guest::Cpu&, std::uint32_t rows, std::uint32_t count, std::uint32_t width, std::uint32_t compare);
void sort_by_words(guest::Cpu&, std::uint32_t rows, std::uint32_t count, std::uint32_t width, std::uint32_t compare);
void sort_by_pairs(guest::Cpu&, std::uint32_t rows, std::uint32_t count, std::uint32_t width, std::uint32_t compare);
// A call of one of the family's routines from another: direct, or through
// the dispatcher where a check run wants to see it (qsort4x_adapter.cpp).
void call(guest::Cpu&, std::uint32_t entry, std::uint32_t back);
}
