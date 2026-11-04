#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
std::uint32_t requires_regional_transfer_approval(guest::Cpu&, std::uint32_t manager,
                                                  std::uint32_t person, std::uint32_t club,
                                                  std::uint32_t mode);
std::uint32_t find_nation_transfer_record(guest::Cpu&, std::uint32_t manager, std::uint32_t club);
std::uint32_t has_visible_offer(guest::Cpu&, std::uint32_t manager, std::uint32_t person,
                                 std::uint32_t viewer_mode);
}
