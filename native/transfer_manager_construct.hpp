#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
std::uint32_t construct_transfer_manager(guest::Cpu&, std::uint32_t manager, std::uint32_t archive);
}
