#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
void enqueue_future_transfer(guest::Cpu&, std::uint32_t manager, std::uint32_t offer, std::uint32_t date);
void remove_future_transfer(guest::Cpu&, std::uint32_t manager, std::uint32_t offer);
}
