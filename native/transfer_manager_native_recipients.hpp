#pragma once
// TRANSFER_MANAGER.CPP native-first block "recipients": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x103fc710: collects the clubs a transfer news item about `person` goes to into `out` (up to `capacity`
// words; an assertion report past it) and returns how many, each once. `kinds` picks them: 2 the person's own
// club (when the news manager takes news for it), 8 the shortlist's clubs, 0x10 clubs of the last 16 people by
// the person's reputation tier (0x200 the higher tier; 0x80 and 0x40 add the value and class tests).
std::uint32_t news_recipients(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t kinds,
                              std::uint32_t out, std::uint32_t capacity);
// 0x10410000: when `buyer` and `person` are given, the person has a contract and the recipients (news_recipients
// with kinds 10) are not empty, posts news item 0xfbf (person, three name words, the person's club, the buyer's
// word, `kind`, the offer's word +0x24) to each recipient club. Declines when the person is on the shortlist
// (0x102de3c0) or already announced (0x103f43b0).
void announce_signing(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t buyer, std::uint32_t kind,
                      std::uint32_t offer);
// 0x103e68c0: records a completed move of `person` to the history rows (0x28 bytes each, grown by 100) as a
// transfer (3) or, with `loan`, a loan (5) and, for a transfer, settles it: the buyer's `fee` (-1 asks for a
// staff contract's pay) to the finance account, the shortlist, the person's links and contract rows; the person's
// transfer terms are cleared. Declines when the contract manager says so (0x10083110).
void record_completed_transfer(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t buyer,
                               std::uint32_t signing, std::uint32_t fee, std::uint32_t loan);
// 0x1040b470: closes the offers for a person who has just moved (status 0x10, contract rows, clubs told through
// news item 0xfb1) and tells the other recipients (news_recipients, kinds 10) who were not told yet.
void close_offers_for_person(guest::Cpu& c, std::uint32_t manager, std::uint32_t person);
// 0x103e64e0: for each person on the manager's pending list (+0xa5c) whose club takes news: settles the move
// (0x101a0c20 and friends; a refusal closes it with 0x101b24d0), with the person's form byte +0x67 deciding in
// two generator draws whether the club's status entry is applied, then closes the offers (close_offers_for_person)
// and records the move (record_completed_transfer). Empties the list afterwards (0x10738ca0).
void complete_due_moves(guest::Cpu& c, std::uint32_t manager);
}
