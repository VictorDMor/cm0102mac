#pragma once
// TRANSFER_MANAGER.CPP native-first block "terms": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x103f89e0: writes into `out` (a five-byte date) the date a contract-term record `record` ends/starts
// by its kind (byte +0x38 >> 4): the default date, the club's own, or the contract's, as its kind says
// (0x103f8cc0 and 0x103fb130 for kinds 1/4 and 5); returns 0 except for kinds 1 and 4.
std::uint32_t term_date(guest::Cpu& c, std::uint32_t manager, std::uint32_t record, std::uint32_t out);
// 0x104058e0: builds the news item (kind 0xfa5) of a contract term for the offer `record`'s person and posts
// it to the person's own club (not for status 12 or a person with a pending kind) and to the buying club.
void post_term_news(guest::Cpu& c, std::uint32_t manager, std::uint32_t record);
// 0x103f9d60: whether club `club` (the buying club or the person's own) accepts offer `offer`'s terms: the date,
// the nations' rules, the money and the squad; `code_out` (or 0) receives the reason code, a nonzero byte in
// `skip_rules` skips the rule checks. Returns 0 (refused), 1 (cannot pay) or 2 (agreed).
std::uint32_t check_offer_terms(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t club,
                                std::uint32_t code_out, std::uint32_t skip_rules);
// 0x103f4fd0: whether club `club` has room for the people offer `offer` moves in or out (its squad rows against
// their limits, counting the club's other side `0x100ce320` returns); `trim_flag` (a byte) lets a full squad be
// trimmed first (0x103e6360). May sell a weak person to make room. `code_out` (or 0) receives the reason code
// (4 and 5: squad full; 0x22 and 0x23: a row full). Returns 1 when there is room, 0 when not.
std::uint32_t check_squad_room(guest::Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t offer,
                               std::uint32_t trim_flag, std::uint32_t code_out);
// 0x103f45a0: lists the offers of club `club` (those it bids with, those for its own people) as rows of 0x1c bytes
// (person, other club, fee, the date the terms start, a kind from the stage, listed extras after each offer), sorts
// them and stores the allocated array in the word at `rows_out`; returns the row count (0 and a null array when
// the allocation fails).
std::uint32_t club_offer_rows(guest::Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t rows_out);
}
