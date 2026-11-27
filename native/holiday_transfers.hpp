#pragma once
#include <cstdint>

namespace cm::aot::guest { class Cpu; }

namespace cm::aot::holiday_transfers {
// A new 0x28-byte transfer-history row, after the original has committed it.
// On a negotiated move the incoming job comes from the offer, before it is installed.
void recorded(guest::Cpu&, std::uint32_t row, std::uint32_t offer = 0);
// Settled return from the borrowing club to the club holding the primary contract.
void returned_from_loan(guest::Cpu&, std::uint32_t person, std::uint32_t from, std::uint32_t to);
}
