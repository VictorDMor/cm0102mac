#pragma once
// MANAGER_MANAGER.CPP standing block, written native-first: 0x101bd320 (and, with it,
// 0x101bb9f0 and 0x101bd460).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x101bd320: files a message of kind 0x1786 about the club entry *entry_index of the table
// *table (rows of 0x4c bytes) with the news desk. Its six fields: the index, two byte
// arguments, the row's halfword at +6, whether the relation to the employer is -2..-1 or
// the employer's 64-bit counter is under 1000000, and 0. Nothing happens when the index
// pointer or the employer record is null. Native arguments; the frame it needs for the
// buffer and its callees is its own.
void post_club_message(guest::Cpu& c, std::uint32_t table, std::uint32_t entry_index, std::int8_t first,
                       std::int8_t second);
// 0x101bb9f0: a club's request of `kind` (1..6, a byte) to its board, decided from the club's row
// (rows of 0x4c bytes in the table *list), the manager's and the employer's records, with the
// module's draw table for chance. It writes the row (counters, flags, the amount at +0x28) and
// files the outcome with post_club_message: code 0 granted, 2..0xb the reason for a refusal.
// Called from another module with r3 = list, r4 = club, r5 = kind.
void club_request(guest::Cpu& c, std::uint32_t list, std::uint32_t club, std::uint32_t kind);
// 0x101bd460: the text of the board's reply to a request, from the message record the request filed
// (0x101bd320: fields 0 club, 1 kind of request, 2 reason code, 3 the row's rating, 4 whether the club is
// short of money). With `flag` set the reply text fills the shared text buffer from the (kind, reason) table
// and `out` receives the text and the club's name; with the flag clear only the headline (a fixed text per
// kind) is made. r3 = the module object, r4 = the record, r5 = flag, r6 = out. Text only.
void board_reply_text(guest::Cpu& c, std::uint32_t object, std::uint32_t record, std::int8_t flag, std::uint32_t out);
}
