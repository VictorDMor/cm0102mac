#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
void announce_offer_response(guest::Cpu&, std::uint32_t manager, std::uint32_t offer);
void announce_offer_termination(guest::Cpu&, std::uint32_t manager, std::uint32_t offer, std::uint32_t reason);
void conclude_offer_response(guest::Cpu&, std::uint32_t manager, std::uint32_t offer_slot, std::uint32_t firm);
}
