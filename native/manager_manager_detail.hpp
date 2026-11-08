#pragma once
// MANAGER_MANAGER.CPP's shared record views and constants. Offsets are the
// original's; meanings come from the callers and are named as far as the code
// shows them.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// TOC slots of the unit (tools/manager_manager_audit.py): -0x5368 is its
// source-name pool ("MANAGER_MANAGER.CPP", the assert texts after it).
namespace slots {
constexpr std::int32_t pool = -0x5368;
// The game's shared assert buffer and the success flag the assert clears.
constexpr std::int32_t message = -0x7ba8, success = -0x7bac;
}
// The game's linked list (the container classes of the runtime library): a
// 16-bit count, the first node and a cursor that list_next (0x10738e00) moves.
namespace list {
constexpr std::uint32_t count = 0x0, head = 0x4, cursor = 0xc;  // count: int16
}
namespace list_node {
constexpr std::uint32_t data = 0x0, next = 0x8;
}
// An epilogue's `lmw rN,-k(r1)` leaves Ghidra's effective-address temporary
// (register byte 2432, compared at imports) one word past r31's slot: the
// caller's stack pointer for the usual save area right below it.
inline void lmw_leaves(guest::Cpu& c, std::uint32_t caller_sp) { c.s.wr<4>(2432, caller_sp); }
}
