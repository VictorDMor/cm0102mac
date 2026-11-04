#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
void append_regional_approval(guest::Cpu&, std::uint32_t manager, std::uint32_t person,
                              std::uint32_t nation, std::uint32_t first_year, std::uint32_t sort);
std::uint32_t find_regional_approval(guest::Cpu&, std::uint32_t manager, std::uint32_t person);
std::uint32_t find_regional_transfer_pair(guest::Cpu&, std::uint32_t manager, std::uint32_t person,
                                          std::uint32_t destination);
void prune_regional_approvals(guest::Cpu&, std::uint32_t manager);
}
