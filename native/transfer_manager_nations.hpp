#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
void initialize_nation_transfer_records(guest::Cpu&, std::uint32_t manager);
}
