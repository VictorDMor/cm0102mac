#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
void initialize_staff_approvals(guest::Cpu&, std::uint32_t manager);
}
