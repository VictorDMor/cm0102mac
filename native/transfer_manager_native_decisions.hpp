#pragma once
// TRANSFER_MANAGER.CPP native-first block "decisions": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x103eed00: builds a transfer offer by `club` (record) for `person` and chases the cheapest sellers
// among the club's players and their team-mates; returns 0 when the club has no free scouting slot.
// `terms` is the caller's offer record the sellers are checked against; the offer goes to *out_offer and
// the reaction value to *out_reaction.
std::uint32_t plan_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t person,
                         std::uint32_t terms, std::uint32_t out_offer, std::uint32_t out_reaction, std::uint32_t budget);
// 0x103ef580: starts an offer for `person` and gauges the buyer's reaction; returns 1 when the offer is kept.
std::uint32_t start_reaction_offer(guest::Cpu& c, std::uint32_t club, std::uint32_t person, std::uint32_t out_offer,
                                   std::uint32_t out_reaction, std::uint32_t fourth);
// 0x103fb630: removes an offer from the manager's tables and releases its lists and counters.
void remove_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer);
// 0x103fdc20: posts the news item (kind 4000) describing an offer to every club the query names.
void post_offer_news(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer);
// 0x10410e50: writes the reasons an offer between two clubs is refused (up to two texts) to `out`; the
// number of reasons is returned (1 for the flag bit 31 message).
std::uint32_t describe_refusal(guest::Cpu& c, double fee, std::int32_t club, std::int32_t other, std::uint32_t flags);
}
