#pragma once
// TRANSFER_OFFER.CPP: building a club's offer for a player (0x104126d0).
#include "guest_abi.hpp"

namespace cm::aot::transfer_offer_unit {
// 0x104126d0: fills `offer` with `club`'s bid for `player` (both records; null reports an assertion and
// answers 0). `wanted` is the shortlist row behind the bid (kind 11 for a player) or null; `budget` the most
// the club means to pay, -1 for no bound; `fixed` (its low byte) what the offer starts with at +0x37; `deal`, when not
// null, the word that receives the contract record (operator new'd, 100 bytes) of a player the club signs
// without a fee. A human club repeats its earlier offer for the player when it has one. A computer club
// asks the seller's price and its own ceiling, may scale them by the clubs' finances, may split the fee
// into instalments, an appearance extra and a sell-on share, and revises the fee toward what it can pay.
// Answers 1 when the offer stands, 0 when the club gives up (the deal, if any, freed again).
std::uint32_t build_offer(guest::Cpu& c, std::uint32_t offer, std::uint32_t player, std::uint32_t club,
                          std::uint32_t wanted, std::int32_t budget, std::uint32_t fixed, std::uint32_t deal);
}
