#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
std::uint32_t transfer_price(guest::Cpu&, std::uint32_t person, std::uint32_t destination,
                             std::uint32_t mode);
std::uint32_t transfer_eligibility(guest::Cpu&, std::uint32_t person);
}
