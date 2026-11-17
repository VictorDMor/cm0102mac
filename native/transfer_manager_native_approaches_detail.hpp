#pragma once
// Shared by the "approaches" block: the offer record's fields, the contract-to-club lookup and the
// record copy that 0x103e25d0, 0x103e2db0 and 0x103e3240 make field by field.
#include "guest_abi.hpp"
#include "transfer_manager_native_detail.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native::approaches {
using guest::Cpu;

// Offsets in an offer record (0x54 bytes, built by TRANSFER_OFFER.CPP's 0x10411e60).
namespace offer {
inline constexpr std::uint32_t size = 0x54, list = 4, club = 8, person = 0xc, kind = 0x24, slot = 0x28, serial = 0x2c,
                               state = 0x34;
}
// Offsets in the transfer manager object.
namespace manager {
inline constexpr std::uint32_t offers = 0, offers_size = 4, offers_count = 8, approaches = 0xc, approaches_size = 0x10,
                               approaches_count = 0x14, shortlist = 0x988, next_serial = 0x98c, last_slot = 0x990,
                               status_rows = 0x9a0, reports = 0xa78;
}
inline constexpr std::uint32_t status_row_size = 10;

inline std::uint32_t unsigned_of(std::int32_t value) { return std::uint32_t(value); }
inline std::int32_t signed_of(std::uint32_t value) { return std::int32_t(value); }
inline std::uint32_t byte_signed(std::uint8_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }

// The club a contract found by 0x10082f20 belongs to, 0 for none or for a contract whose status
// field (+0x38, upper six bits) is 5. The original leaves its flag in a non-volatile register.
inline bool counts_for_club(Cpu& c, std::uint32_t contract) {
    return contract != 0 && (std::int8_t(c.u8(contract + 0x38)) >> 2) != 5;
}
inline std::uint32_t club_of(Cpu& c, std::uint32_t clubs_cell, std::uint32_t contract) {
    return c.u32(clubs_cell) + c.u32(contract + 4) * club_size;
}

// The offer fields the original copies, with its widths (a halfword at +0x14 goes through lha/sth and
// the one at +0x16 through lhz/sth: same bits).
inline void copy_offer_fields(Cpu& c, std::uint32_t to, std::uint32_t from) {
    for (const std::uint32_t at : {0x00u, 0x04u, 0x08u, 0x0cu}) c.put32(to + at, c.u32(from + at));
    c.put8(to + 0x10, c.u8(from + 0x10));
    c.put8(to + 0x11, c.u8(from + 0x11));
    c.put16(to + 0x16, c.u16(from + 0x16));
    c.put16(to + 0x14, c.u16(from + 0x14));
    c.put8(to + 0x18, c.u8(from + 0x18));
    c.put8(to + 0x1c, c.u8(from + 0x1c));
    c.put32(to + 0x20, c.u32(from + 0x20));
    c.put8(to + 0x24, c.u8(from + 0x24));
    for (const std::uint32_t at : {0x28u, 0x2cu, 0x30u}) c.put32(to + at, c.u32(from + at));
    for (const std::uint32_t at : {0x34u, 0x35u, 0x36u, 0x37u, 0x38u}) c.put8(to + at, c.u8(from + at));
    c.put32(to + 0x3c, c.u32(from + 0x3c));
    c.put32(to + 0x40, c.u32(from + 0x40));
    c.put8(to + 0x44, c.u8(from + 0x44));
    c.put32(to + 0x48, c.u32(from + 0x48));
    for (const std::uint32_t at : {0x4cu, 0x50u, 0x51u}) c.put8(to + at, c.u8(from + at));
}
}
