#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
std::uint32_t compare_priority_and_index(guest::Cpu&, std::uint32_t left, std::uint32_t right);
std::uint32_t compare_index(guest::Cpu&, std::uint32_t left, std::uint32_t right);
std::uint32_t compare_ranked_transfers(guest::Cpu&, std::uint32_t left, std::uint32_t right);
std::uint32_t compare_linked_dates(guest::Cpu&, std::uint32_t left_slot, std::uint32_t right_slot);
std::uint32_t compare_dated_entries(guest::Cpu&, std::uint32_t left, std::uint32_t right);
std::uint32_t compare_linked_priorities(guest::Cpu&, std::uint32_t left_slot, std::uint32_t right_slot);
std::uint32_t compare_optional_index(guest::Cpu&, std::uint32_t left, std::uint32_t right);
std::uint32_t compare_ranked_staff(guest::Cpu&, std::uint32_t left, std::uint32_t right);
void initialize_record_sentinels(guest::Cpu&, std::uint32_t destination);
}
