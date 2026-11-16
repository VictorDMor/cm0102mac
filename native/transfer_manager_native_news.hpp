#pragma once
// TRANSFER_MANAGER.CPP native-first block "news": typed entry points (guest addresses for records).
// Each builds a news item in its frame (NEWS.CPP: 0x102bdf90, fields 0x102cb4b0) and posts it (0x102cc140).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x103ffe80: the signing news (item 0xfa1) of an offer to every club 0x103fb840 names; `outcome` is a halfword.
void post_signing_news(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t outcome);
// 0x104019d0: item 0xfa2, built from two offers and the people listed on the first, posted to each recipient.
void post_news_fa2(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t other);
// 0x10402b90: item 0xfac, to the offer's club and then once per person listed, to the player's club.
void post_news_fac(guest::Cpu& c, std::uint32_t offer);
// 0x10403890: item 0xfaf between two people, posted to the (up to two) recipients of 0x103fb840.
void post_news_faf(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t other, std::uint32_t word);
// 0x10403e20: the large item 0xfa4 (both clubs, managers, the shortlist's attraction scaled by a constant),
// posted to the recipients and then to the best three candidate clubs not already among them.
void post_news_fa4(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club, std::uint32_t other_club,
                   std::uint32_t outcome, std::uint32_t offer, std::uint32_t contract, std::uint32_t list);
// 0x10406910: item 0xfa7, to the offer's two clubs.
void post_news_fa7(guest::Cpu& c, std::uint32_t offer, std::uint32_t word, std::uint32_t other, std::uint32_t flag);
// 0x10406e00: item 0xfa6 (nothing when `done` is set or the player has no contract), to the offer's club and the player's.
void post_news_fa6(guest::Cpu& c, std::uint32_t offer, std::uint32_t word, std::uint32_t flag, std::uint32_t done);
// 0x10407460: item 0xfa8 with a news-table draw, posted to the recipients or filed with 0x102cc8e0.
void post_news_fa8(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer);
}
