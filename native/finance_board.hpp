#pragma once
// FINANCE.CPP, block board: what the board does for a club by its wealth band. The gifts and bail-outs it books
// as income on the donations line (0xb4 of the account), and the weekly review that picks between them.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::finance {
using guest::Cpu;

// 0x100fefa0: a benefactor's gift to a club short of money; 1 when a gift was booked and announced.
std::uint32_t benefactor_gift(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t account);
// 0x100ffec0: the board's cover of a club's shortfall (a sum by reputation, capped); 1 when booked.
std::uint32_t board_cover(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t account);
// 0x101006a0: the owner's bail-out of a club in trouble, which may also replace the owner; 1 when booked.
std::uint32_t owner_bailout(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t account);
// 0x10101400: a club's board review by its wealth band; `acted` (a signed byte) is non-zero on a day the board may act.
void board_review(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t acted);
}
