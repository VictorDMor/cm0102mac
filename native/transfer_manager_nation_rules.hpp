#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
std::uint32_t nation_allows_staff_transfer(guest::Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t person);
std::uint32_t check_nation_offer_rules(guest::Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t context,
                                      std::uint32_t person, std::uint32_t offer, std::uint32_t retry, std::uint32_t reason);
void dispatch_nation_offer_notification(guest::Cpu&, std::uint32_t manager, std::uint32_t offer);
void dispatch_club_nation_rule(guest::Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t subject,
                               std::uint32_t output);
}
