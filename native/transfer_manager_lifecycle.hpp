#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
std::uint32_t destroy_transfer_manager(guest::Cpu&, std::uint32_t manager, std::uint32_t delete_flag);
}
