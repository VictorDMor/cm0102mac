#pragma once
// TRANSFER_MANAGER.CPP native-first block "flow": what its routines share (offer record fields, lookups,
// the original's integer and date idioms, the report of a failed allocation).
#include "guest_fpu.hpp"
#include <stdexcept>
#include "transfer_manager_native_detail.hpp"
#include "transfer_manager_native_status.hpp"

namespace cm::aot::transfer_manager_native::flow {
using guest::Cpu;
using guest::Frame;

// Fields of an offer record (the object behind a manager slot).
namespace offer_at {
inline constexpr std::uint32_t bidders = 0x00,  // list of the manager's slots of the other offers for the person
                               listed = 0x04,   // list of listed people (items with the person's index first)
                               club = 0x08, person = 0x0c, flag10 = 0x10, kind = 0x11, amount = 0x20, state = 0x24,
                               own_slot = 0x28, stage = 0x34, step35 = 0x35, step36 = 0x36;
}
inline constexpr std::uint32_t contract_status = 0x38;  // byte of a contract: bits 2.. hold the kind (5 = none)
inline constexpr std::uint32_t manager_shortlist = 0x988, manager_status_rows = 0x9a0;
inline constexpr std::uint32_t text_error_name = 0x37, text_error_message = 0x6ff, text_error_title = 0x70c;

inline std::int32_t s8(std::uint32_t v) { return std::int32_t(std::int8_t(v & 0xff)); }
inline std::int32_t s16(std::uint32_t v) { return std::int32_t(std::int16_t(v & 0xffff)); }
inline std::uint32_t word(std::int32_t v) { return std::uint32_t(v); }
// divw as the generated code has it: a zero divisor stops the program (the reference's strict lowering),
// 0x80000000 / -1 wraps.
inline std::int32_t div_s(std::int32_t a, std::int32_t b) {
    if (b == 0) throw std::runtime_error("Signed division by zero in translated code");
    return std::int32_t(std::uint32_t(std::int64_t(a) / std::int64_t(b)));
}
// An int as the original's int-to-double idiom makes it: the double with the word's bits under 0x43300000,
// less the pool's constant at +8 (through the Fpu, so FPSCR ends as the original's).
inline double int_to_double(Cpu& c, guest::Fpu& fpu, std::uint32_t pool, std::uint32_t value) {
    return fpu.fsub(std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (value ^ 0x80000000u)), c.f64(pool + 8));
}

inline std::uint32_t person_of(Cpu& c, std::uint32_t index) { return c.u32(c.toc(toc::people)) + index * person_size; }
inline std::uint32_t club_of(Cpu& c, std::uint32_t index) { return c.u32(c.toc(toc::clubs)) + index * club_size; }
// The person's contract (0 without one), also left in the shared found-contract cell.
inline std::uint32_t find_contract(Cpu& c, std::uint32_t person, std::uint32_t back) {
    const auto contract = invoke(c, 0x10082f20, back, {c.u32(c.toc(toc::contracts)), person});
    c.put32(c.toc(toc::found_contract), contract);
    return contract;
}
// The club of the contract the lookup just left in the found-contract cell, unless the contract's kind is 5
// (or there is none): 0 then.
inline std::uint32_t contract_club(Cpu& c, std::uint32_t contract) {
    if (!contract || (std::int8_t(c.u8(contract + contract_status)) >> 2) == 5) return 0;
    return club_of(c, c.u32(c.u32(c.toc(toc::found_contract)) + 4));
}
// 1 when `status` is not 11 (the original's (11 - x | x - 11) >> 31).
inline std::uint32_t not_eleven(std::uint32_t status_byte) { return std::int8_t(status_byte) != 11 ? 1u : 0u; }

// A failed allocation or lookup is reported to the player: the names of the source file are split, the
// version is formatted into the message with the line number, and the dialog shows it. The return addresses
// are those of the original's four calls.
struct ReportSites { std::uint32_t split, version, format, dialog; };
inline void report_failure(Cpu& c, std::uint32_t name_at, std::uint32_t extension_at, std::uint32_t line,
                           const ReportSites& sites) {
    const auto text = c.toc(-0x4b9c), message = c.toc(-0x7ba8);
    invoke(c, 0x10739890, sites.split, {text + text_error_name, 0, 0, name_at, extension_at});
    const auto version = invoke(c, 0x10768810, sites.version);
    invoke(c, 0x1077b720, sites.format, {message, text + text_error_message, version, name_at, extension_at, line});
    invoke(c, 0x10769b60, sites.dialog, {text + text_error_title, message, 0});
}
}
