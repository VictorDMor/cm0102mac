#pragma once
// TRANSFER_MANAGER.CPP native-first block "stages": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x103e7cf0: looks at an offer's stage and the clubs' answers and sends it on to the next step (0x103e80e0,
// 0x103e8500, 0x103e44b0 or, for a bid with several people, a restart through 0x103ed090).
void advance_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer);
// 0x103e80e0: moves an offer on once `club` has answered (stage 11 to 12/13, those to 14); at 14 settles it when
// its agreed date has come, otherwise hands it back to 0x103eb4a0.
void route_answered_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t club);
// 0x103ebf50: after an agreed offer, takes its person and each listed person off the other bids and sends those
// people's other offers back as refused.
void settle_agreed_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer);
// 0x103e95a0: takes `person` off the offer's listed people (or off every other offer listing them), growing the
// fee by their value and handing the offer on (0x103e7cf0).
void remove_listed_person(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t offer,
                          std::uint32_t as_listed_person, std::uint32_t extra);
// 0x103e6cb0: the contract side's answer to an offer for `person`.
void answer_person_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t subject, std::uint32_t person,
                         std::uint32_t offer);
// 0x103ed170: re-examines an offer's people against their clubs' interest: agrees it (returns 0) or drops the
// refusing listed people (returns 1). `quiet` suppresses the refusal's side effects and the final advance.
std::uint32_t review_offer_people(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t quiet);
// 0x103e70f0: the person's club accepts the offer at `index` (fee terms `terms`, `kind` the offer's byte +0x11).
void accept_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::int32_t index, std::uint32_t terms,
                  std::uint32_t kind, std::uint32_t extra);
// 0x103e76c0: the person's club rejects the offer at `index`.
void reject_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::int32_t index, std::uint32_t listed_only);
}
