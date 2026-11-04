#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
void append_past_transfer(guest::Cpu&, std::uint32_t manager, std::uint32_t transfer);
void archive_past_transfers(guest::Cpu&, std::uint32_t manager);
}
