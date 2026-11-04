#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
void remove_offer_pending_contracts(guest::Cpu&, std::uint32_t offer);
void forget_offer_approaches(guest::Cpu&, std::uint32_t manager, std::uint32_t offer);
void mark_offer_contracts_for_withdrawal(guest::Cpu&, std::uint32_t offer);
std::uint32_t offer_requires_regional_approval(guest::Cpu&, std::uint32_t manager, std::uint32_t offer);
}
