#pragma once
// TRANSFER_MANAGER.CPP native-first block "news_outcomes": typed entry points (guest addresses for records).
// Each posts a transfer news item (kind in the name) about an offer to the clubs 0x103fb840 lists (or to
// the clubs named below); the news items are saved with the game.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x10409200: kind 0xfab, to every club listed for the offer (kind 1, or 2 unless the offer's byte +0x34 is 0x11).
void post_offer_news_fab(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t short_raw);
// 0x104099f0: kind 0xfad, to the offer's club when a human manages it.
void post_offer_news_fad(guest::Cpu& c, std::uint32_t offer, std::uint32_t other_person, std::uint32_t value, std::uint32_t short_raw);
// 0x1040a050: kind 0xfb0, to the clubs listed for the offer except `other_person`'s when short is 11.
void post_offer_news_fb0(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t other_person, std::uint32_t short_raw);
// 0x1040c220: kind 0xfb3 with up to three listed players, to the listed clubs or to everyone when a human manages the offer's club.
void post_offer_news_fb3(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t shorts);
// 0x1040ce40: kind 0xfb4, to the listed clubs.
void post_offer_news_fb4(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t record);
// 0x1040ddc0: kind 0xfb6, to the listed clubs.
void post_offer_news_fb6(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t record);
// 0x1040e2d0: kind 0xfb7, to the listed clubs.
void post_offer_news_fb7(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t short_raw);
// 0x1040ea40: kind 0xfb9, to the listed clubs, counting the listed players the buying club may use.
void post_offer_news_fb9(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer);
// 0x1040f270: kind 0xfba about a person, to the clubs listed for an offer.
void post_person_news_fba(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t word, std::uint32_t offer);
// 0x1040f780: kind 0xfbb, to the offer's club and the person's club when a human manages either.
void post_offer_news_fbb(guest::Cpu& c, std::uint32_t offer, std::uint32_t other_person, std::uint32_t value);
// 0x10410500: kind 0xbcc about a person, to the person's club (and again when it differs from the record's).
void post_person_news_bcc(guest::Cpu& c, std::uint32_t person, std::uint32_t record);
// 0x104108c0: kind 0xfc0, to the offer's club.
void post_offer_news_fc0(guest::Cpu& c, std::uint32_t other, std::uint32_t offer);
// 0x104113e0: kind 0xfc3, pays the club's players' shares of the fee (FINANCE.CPP) and posts for each large one.
void post_offer_news_fc3(guest::Cpu& c, std::uint32_t offer);
}
