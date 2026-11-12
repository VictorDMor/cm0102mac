#pragma once
// DISCIPLINE.CPP's records, shared by its blocks (tools/discipline_audit.py). Offsets are the
// original's; every access goes through guest::Cpu's checked big-endian accessors.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::discipline {
// The discipline manager (global at TOC -0x7aa4), built by 0x100de410:
//   +0x00 a pointer to the array of the 42 ruling bodies (RULING_BODY.CPP objects; null where a slot
//         is unused; body_object() in discipline_table_detail.hpp reads one)
//   +0x04 / +0x0c buffers freed by the destructor, with the halfword counts at +0x08/+0x0a and
//         +0x10/+0x12
//   +0x14 the person table: one 0x2c-byte row per person (TOC -0x7be4 people), see Row
//   +0x18 the ban history file (0x10748bb0 object, 0x18-byte records; ban_history.tmp)
//   +0x1c a list (0x10738c00 list object; head at +0x20, cursor at +0x28) of 0x10-byte items:
//         person id, slot (byte at +4), ruling body index (byte at +8), value (word at +0xc)
namespace manager {
inline constexpr std::uint32_t bodies = 0x00, people = 0x14, history = 0x18, pending = 0x1c,
                               pending_head = 0x20, pending_cursor = 0x28;
inline constexpr int body_count = 42;
}
// A person's row of the table at manager+0x14 (0x2c bytes):
//   +0x00 five slots of 8 bytes: a signed byte ruling body index (-1: empty) and a word at +4
//   +0x20 / +0x24 the person's history position (two words passed to the history file)
//   +0x28 a signed byte: how many of the person's items wait in the pending list
namespace row {
inline constexpr std::uint32_t size = 0x2c, slot_size = 8, slot_value = 4, history_first = 0x20,
                               history_second = 0x24, pending_count = 0x28;
inline constexpr int slots = 5;
}
// The row of person `person` (wrapping 32-bit arithmetic, as `mulli`).
inline std::uint32_t row_address(const guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person) {
    return c.u32(manager_object + manager::people) + person * row::size;
}
// The slot `slot` (a signed byte, sign-extended as `extsb` then shifted by 3) of a row.
inline std::uint32_t slot_address(std::uint32_t row_at, std::int8_t slot) {
    return row_at + std::uint32_t(std::int32_t(slot)) * row::slot_size;
}
}
