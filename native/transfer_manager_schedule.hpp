#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
void schedule_transfer_offer(guest::Cpu&, std::uint32_t manager, std::uint32_t offer,
                             std::uint32_t days, std::uint32_t phases);
}
