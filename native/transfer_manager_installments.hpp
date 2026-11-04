#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
void register_transfer_consideration(guest::Cpu&, std::uint32_t manager, std::uint32_t offer);
void settle_transfer_installments(guest::Cpu&, std::uint32_t manager);
}
