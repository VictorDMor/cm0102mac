#pragma once
// TRANSFER_MANAGER.CPP native-first block "flow": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x103eec00: allocates and builds a candidate offer object for `person` at `club` with `fee` into the word at
// `out_offer`, sets the byte at `out_kind` to 50 and returns 1; a club the finance manager has no account for
// is reported to the player first.
std::uint32_t make_candidate_offer(guest::Cpu& c, std::uint32_t club, std::uint32_t person, std::uint32_t out_offer,
                                   std::uint32_t out_kind, std::uint32_t fee);
// 0x103e5810: for the offer in the manager slot `slot`: finds the club's contract row for its person, scales
// the row's share down in steps of ten percent until the club can pay it, renews the row's date, builds the
// answer and hands it to the contract manager; without a row or a contract the failure is reported and the
// offer is closed.
void settle_contract_fee_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t slot);
// 0x103e4740: closes the offer in manager slot `slot` after its round (`final_flag` as a byte: the first round of
// a listed offer): marks the answer stage, settles an accepted deal when the signing is worth it, and signs the
// listed people off the list one by one while the club's valuation of them exceeds the pay offered.
void conclude_offer_round(guest::Cpu& c, std::uint32_t manager, std::uint32_t slot, std::uint32_t final_flag);
// 0x103e4d70: weighs the offer in manager slot `slot` (or the one the manager keeps for it, for a first-round
// `final_flag`) against what the club can pay: takes the listed people off the offer one by one, then either
// closes the shortlist entry or, for a person the partner club can be talked out of, rates the deal and draws
// whether the partner's contract gets its 0x800000 flag.
void evaluate_listed_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t slot, std::uint32_t final_flag);
// 0x103ee1b0: one round of the offer in manager slot `slot` (`flag_word`: called from inside another round).
// A first-round offer closes (shortlists, contract manager and news updated); a later round settles the
// partner's bids, builds up to three candidate proposals and either answers with the best, closes the offer
// or hands it to 0x103e4740.
void review_offer_slot(guest::Cpu& c, std::uint32_t manager, std::uint32_t slot, std::uint32_t flag_word);
}
