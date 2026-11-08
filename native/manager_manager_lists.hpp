#pragma once
// MANAGER_MANAGER.CPP lists block.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x101b1620: whether the list at container+8 holds an element whose word at
// +8 equals *key (the recruitment candidate scan asks it for every candidate).
// The list's cursor is restored on every exit. A null element is reported
// through the game's assert and answers false.
std::uint32_t list_contains(guest::Cpu& c, std::uint32_t container, std::uint32_t key);
}
