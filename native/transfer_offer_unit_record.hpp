#pragma once
// TRANSFER_OFFER.CPP: the offer record itself. Building an empty one, destroying it, its file form,
// starting an offer for a player, and the questions asked of a finished offer.
#include "guest_abi.hpp"

namespace cm::aot::transfer_offer_unit {
// 0x10411e60: an empty record: its date built, no lists.
std::uint32_t construct(guest::Cpu& c, std::uint32_t offer);
// 0x10416d30: what is paid later: the instalments and the appearance extra, each when above zero.
std::int32_t deferred_amount(guest::Cpu& c, std::uint32_t offer);
// 0x10416d60: the status of one player of the offer: the offer's own for its player (a kind-3
// offer first becomes status 4), an exchange row's for a player in exchange, 4 for anyone else.
std::uint32_t status_of(guest::Cpu& c, std::uint32_t offer, std::uint32_t person);
// 0x10417260: the exchange row whose person is the word at `person`, or null.
std::uint32_t exchange_row_of(guest::Cpu& c, std::uint32_t offer, std::uint32_t person);
// 0x10411ea0: an empty record whose date is built, then filled by 0x104126d0 (another block) from the five
// arguments the caller gave; returns the offer.
std::uint32_t construct_with(guest::Cpu& c, std::uint32_t offer, std::uint32_t first, std::uint32_t second,
                             std::uint32_t third, std::uint32_t fourth, std::uint32_t fifth);
// 0x10411f10: releases the offer's two lists and, when `deleting` (a signed halfword) is above zero, the
// record itself. A null offer is accepted; returns the offer.
std::uint32_t destroy(guest::Cpu& c, std::uint32_t offer, std::uint32_t deleting);
// 0x10411fa0: reads an offer from a saved game's `file`: the record, its club list and its exchange list.
// `table` holds the club records by index (null reports an assertion when the file has clubs).
void read_from_file(guest::Cpu& c, std::uint32_t offer, std::uint32_t file, std::uint32_t table);
// 0x104123e0: writes the same form.
void write_to_file(guest::Cpu& c, std::uint32_t offer, std::uint32_t file);
// 0x104163d0: starts a plain offer of `club` for `person` (records); null for either reports an assertion.
void start_plain(guest::Cpu& c, std::uint32_t offer, std::uint32_t person, std::uint32_t club);
// 0x104164f0: starts an offer for `amount`; one draw of the sample table decides the flag at +0x35.
void start_with_amount(guest::Cpu& c, std::uint32_t offer, std::uint32_t person, std::uint32_t club,
                       std::uint32_t amount);
// 0x10416710: the offer's total. Each flag is the low byte of its argument, as the original tests it.
std::int32_t total(guest::Cpu& c, std::uint32_t offer, std::uint32_t fee_only, std::uint32_t without_exchange,
                   std::uint32_t by_valuation, std::uint32_t with_sell_on);
}
