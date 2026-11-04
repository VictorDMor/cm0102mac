#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
std::uint32_t rejection_reason_from_squad_status(guest::Cpu&, std::uint32_t offer);
std::uint32_t rejection_reason_allowed(guest::Cpu&, std::uint32_t reason, std::uint32_t offer);
void describe_rejection_reason(guest::Cpu&, std::uint32_t reason,
                               std::uint32_t buffer, std::int32_t capacity);
std::uint32_t squad_status_for_rejection_reason(guest::Cpu&, std::uint32_t person, std::uint32_t reason);
}
