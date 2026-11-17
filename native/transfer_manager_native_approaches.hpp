#pragma once
// TRANSFER_MANAGER.CPP native-first block "approaches": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x103e25d0: files the offer `source` as a new record in the manager's offer table (replacing the same
// club's offer for the person, growing the table when full), links it to its earlier approach, sets its
// first status through 0x103f3bf0, posts the news and bumps the person's status row.
void add_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t source, std::uint32_t news_extra);
// 0x103e2db0: stores a copy of `copy` in the manager's approach table at `offer`'s slot (finding a free one
// when it has none), then moves `offer` on by the club's reaction; `mark` also flags both records (+0x35) and
// tells the shortlist.
void add_approach(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t copy, std::int8_t mark);
// 0x103e3240: builds a local offer from `offer` (no list), asks the staff-contract routines for its terms,
// logs them in the manager's report list and stores it through 0x103e2db0; returns the stored slot word.
std::uint32_t add_approach_from_terms(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t term_a,
                                      std::uint32_t term_b, std::uint32_t term_c, std::uint32_t term_d);
// 0x103e3170: builds an offer from the person and club and four term words, files it through 0x103e25d0,
// announces it to the contract manager and returns the offer's slot word.
std::uint32_t build_and_add_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club,
                                  std::uint32_t term_a, std::uint32_t term_b, std::uint32_t term_c, std::uint32_t term_d);
// 0x103f3bf0: from the club levels of both sides and the person's contract, picks the first status of a new
// offer: writes the two bytes at `first` and `second` (a draw decides the second for the lower levels).
void choose_first_status(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t first, std::uint32_t second);
// 0x1040d5c0: posts news 0xfb5 about the offer to the clubs 0x103fb840 lists.
void post_offer_news(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t terms);
// 0x103e3410: builds and files an offer for `person` on behalf of `club` (the regen path), with its contract
// dates from `date`.
void add_regen_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club, std::uint32_t date,
                     std::uint32_t amount, std::int32_t cap, std::uint32_t years);
// 0x103e3de0: when the club's level and the dates allow, signs `person` for `club` (or the club the person is
// loaned from) and updates the contract, squad and discipline records.
void sign_person(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club, std::uint32_t from_date,
                 std::uint32_t to_date);
}
