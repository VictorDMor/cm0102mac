#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
std::uint32_t collect_candidate_clubs(guest::Cpu&, std::uint32_t manager, std::uint32_t offer,
                                      std::uint32_t sources, std::uint32_t output, std::uint32_t capacity);
}
