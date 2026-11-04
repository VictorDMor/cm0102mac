#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
std::uint32_t offer_is_filtered(guest::Cpu&, std::uint32_t offer);
std::uint32_t primary_offer_slot(guest::Cpu&, std::uint32_t manager, std::int32_t index);
std::uint32_t secondary_offer_slot(guest::Cpu&, std::uint32_t manager, std::int32_t index);
std::uint32_t collect_staff_offers(guest::Cpu&, std::uint32_t manager, std::uint32_t person,
                                   std::uint32_t output_list, std::uint32_t include_filtered);
std::uint32_t collect_primary_club_offers(guest::Cpu&, std::uint32_t manager, std::uint32_t person,
                                          std::uint32_t output_list);
std::uint32_t collect_secondary_club_offers(guest::Cpu&, std::uint32_t manager, std::uint32_t person,
                                            std::uint32_t output_list);
std::uint32_t collect_offers_from_club(guest::Cpu&, std::uint32_t manager, std::uint32_t club,
                                       std::uint32_t output_list);
}
