#pragma once
#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
void mark_offer_club_response(guest::Cpu&, std::uint32_t manager, std::int32_t index);
void mark_offer_staff_response(guest::Cpu&, std::uint32_t manager, std::int32_t index, std::int32_t staff);
void clear_inactive_offer_flag(guest::Cpu&, std::uint32_t manager, std::uint32_t offer);
}
