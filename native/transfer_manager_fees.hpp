#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
enum class FeeKind { sell_on, first_team_appearance, international_appearance };

void append_transfer_fee(guest::Cpu&, FeeKind, std::uint32_t manager, std::uint32_t record);
void remove_transfer_fee(guest::Cpu&, FeeKind, std::uint32_t manager, std::uint32_t person);
std::uint32_t find_sell_on_fee(guest::Cpu&, std::uint32_t manager, std::uint32_t person);
std::uint32_t find_appearance_fee(guest::Cpu&, FeeKind, std::uint32_t manager, std::uint32_t person);
}
