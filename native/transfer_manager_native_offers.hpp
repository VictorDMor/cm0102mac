#pragma once
// TRANSFER_MANAGER.CPP native-first block "offers": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x103e3860: a draw in [0, n) (in (n, 0] for negative n) from the sample table; ranges past 0xffff use two draws.
std::int32_t random_below(guest::Cpu& c, std::int32_t n);
// 0x103e4390: moves the offer behind `slot` to stage 2 (flag: marks it), tells news and the shortlists.
void close_offer_stage_2(guest::Cpu& c, std::uint32_t manager, std::uint32_t slot, std::uint32_t flag, std::uint32_t reason);
// 0x103e4130: the same with the contract and club checks first, then notifies the other bidders' shortlists.
void close_offer_stage_2_with_bidders(guest::Cpu& c, std::uint32_t manager, std::uint32_t slot, std::uint32_t flag, std::uint32_t reason);
// 0x103e44b0: moves the offer behind `slot` to stage 3, then notifies the other bidders' shortlists.
void close_offer_stage_3(guest::Cpu& c, std::uint32_t manager, std::uint32_t slot, std::uint32_t flag, std::uint32_t reason);
// 0x103e5b70: after an offer for a person: the player's own verdict (a contract review, an AWOL, a file or release).
void assess_player_after_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer);
// 0x103e8500: an offer answered by a club: stage 0x11 or 0x12, then the counter-offer draw or the news.
void answer_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t club_slot, std::uint32_t reason);
// 0x103e8bb0: the pending review of an offer (`link`): whether the offer went through.
bool review_pending_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t link);
// 0x103e9910: moves an offer to stage 0x10 (clearing the people it blocked), news and shortlists.
void withdraw_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t news_a, std::uint32_t news_b, std::uint32_t flag);
// 0x103e9ac0: withdraws every offer of a club (and those of the people linked to it) that has not been through.
void withdraw_offers_of_club(guest::Cpu& c, std::uint32_t manager, std::uint32_t club);
}
