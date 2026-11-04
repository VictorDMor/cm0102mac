#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
void append_staff_registration(guest::Cpu&, std::uint32_t manager, std::uint32_t person,
                               std::uint32_t kind, std::uint32_t details);
std::uint32_t count_staff_registrations_for_club(guest::Cpu&, std::uint32_t manager, std::uint32_t club);
std::uint32_t has_staff_registration(guest::Cpu&, std::uint32_t manager, std::uint32_t person);
std::uint32_t best_qualifying_offer(guest::Cpu&, std::uint32_t manager,
                                    std::uint32_t person, std::uint32_t club);
std::uint32_t find_special_offer(guest::Cpu&, std::uint32_t manager,
                                 std::uint32_t person, std::uint32_t out_detail);
std::uint32_t club_passes_division_gate(guest::Cpu&, std::uint32_t club);
}
