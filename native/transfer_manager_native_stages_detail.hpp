#pragma once
// Shared by the "stages" block of TRANSFER_MANAGER.CPP: the offer record's fields and the small queries
// those routines repeat. Offers (at manager+0 an array of pointers, the count at +4) hold: +0 a list of the
// other offers bidding for the same person, +4 the list of the offer's extra listed people (entries with the
// person index at +0 and a stage byte at +4), +8 the buying club, +0xc the person, +0x10 the offer kind,
// +0x20 the fee, +0x34 the stage, +0x36 the refusal count.
#include "transfer_manager_native_detail.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native::stages {
using guest::Cpu;
using guest::Frame;

inline constexpr std::uint32_t offer_bidders = 0x0, offer_listed = 0x4, offer_club = 0x8, offer_person = 0xc,
                               offer_kind = 0x10, offer_fee = 0x20, offer_stage = 0x34, offer_refusals = 0x36;
inline constexpr std::uint32_t list_head = 0x4, list_cursor = 0xc;
inline constexpr std::uint32_t status_rows = 0x9a0, status_row_size = 10, status_row_listed = 2;
inline constexpr std::uint32_t contract_kind_byte = 0x38, no_club_contract_kind = 5;
inline constexpr std::uint32_t find_contract = 0x10082f20, next_listed = 0x10738e00, drop_listed = 0x10738e60,
                               list_construct = 0x10738c00, list_destroy = 0x10738c20, club_test = 0x1014c2e0;

// The contract's kind: the top six bits of its byte, signed.
inline std::int32_t contract_kind(Cpu& c, std::uint32_t contract) {
    return std::int32_t(c.i8(contract + contract_kind_byte)) >> 2;
}
// The club of a found contract, or 0 when there is none or it is of the kind that has no club.
inline std::uint32_t contract_club(Cpu& c, std::uint32_t contract) {
    if (!contract || contract_kind(c, contract) == no_club_contract_kind) return 0;
    return c.u32(c.toc(toc::clubs)) + c.u32(contract + 4) * club_size;
}
// Moves a list's cursor back to its first entry.
inline void rewind_list(Cpu& c, std::uint32_t list) { c.put32(list + list_cursor, c.u32(list + list_head)); }
inline std::uint32_t person_record(Cpu& c, std::uint32_t index) { return c.u32(c.toc(toc::people)) + index * person_size; }
inline std::uint32_t club_record(Cpu& c, std::uint32_t index) { return c.u32(c.toc(toc::clubs)) + index * club_size; }

// The report the game makes when an offer index is out of range or an offer's list is missing: builds the
// message (0x10739890, 0x10768810), prints it (0x1077b720, 0x10769b60) and clears the flag byte at `flag`.
// `first` and `second` are the frame buffers the original hands to the first calls; `back` the return
// addresses of its four calls.
struct ReportBack { std::uint32_t build, result, print, finish; };
inline void report_bad_offer(Cpu& c, std::uint32_t strings, std::uint32_t log, std::uint32_t flag, std::uint32_t first,
                             std::uint32_t second, std::uint32_t line, const ReportBack& back) {
    invoke(c, 0x10739890, back.build, {strings + 0x37, 0, 0, first, second});
    const auto text = invoke(c, 0x10768810, back.result);
    invoke(c, 0x1077b720, back.print, {log, strings + 0x6ff, text, first, second, line});
    invoke(c, 0x10769b60, back.finish, {strings + 0x70c, log, 0});
    c.put8(flag, 0);
}
}
