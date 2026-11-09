#pragma once
// MANAGER_MANAGER.CPP transfer-deal verdict block.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x101a0f50: the board's verdict on a deal for `player` with `club` at price `fee` (called from
// another module at 0x103f5ad8). `flag` is a byte: set (the club buys), the club's staff are
// asked first, each by a draw against the rating (a refusal answers 0x2000); then the price
// is weighed against the player's value and the club's means: the row of the club in the
// object's table is lowered by fixed steps for a refusal on price (0x800) or an asking too low
// (0x200). Answers the verdict bits in r3 (0 when the deal goes ahead).
std::uint32_t deal_verdict(guest::Cpu& c, std::uint32_t object, std::uint32_t player, std::uint32_t club,
                           std::uint32_t fee, std::uint32_t flag);
}
