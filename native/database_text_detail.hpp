#pragma once
// DATABASE.CPP's text block: the library routines it calls, the string pool
// slots it loads, and the arithmetic with XER[CA] that PowerPC compilers use.
#include "guest_abi.hpp"
#include "guest_values.hpp"

namespace cm::aot::database::text {
using guest::Cpu;
using guest::compare;
using guest::compare_logical;

// C library and other modules' routines (the generated code or its C++ owner runs).
constexpr std::uint32_t strlen_entry = 0x1077dbd0, strcpy_entry = 0x1077dbf0, sprintf_entry = 0x1077b720,
                        compare_text = 0x107465f0;  // (a, b, 1, 0): the comparator behind every sort of names
// The string pool of DATABASE.CPP's assertions (TOC -0x5618): file name at +0, the format of the
// assertion text at +0xd, its title at +0x1a.
constexpr std::int32_t pool_slot = -0x5618;

inline std::int32_t extsb(std::uint32_t value) { return std::int8_t(value); }
inline std::int32_t extsh(std::uint32_t value) { return std::int16_t(value); }

// addic / addic.: the carry goes to XER[CA]; the record form compares the sum with 0.
inline std::uint32_t addic(Cpu& c, std::uint32_t a, std::uint32_t immediate) {
    const auto sum = std::uint32_t(a + immediate);
    c.s.wr<1>(guest::reg::xer_ca, sum < a);
    return sum;
}
// srawi: XER[CA] is set when a negative value loses one bits.
inline std::uint32_t srawi(Cpu& c, std::uint32_t value, unsigned count) {
    c.s.wr<1>(guest::reg::xer_ca, std::int32_t(value) < 0 && (value & ((1u << count) - 1)) != 0);
    return std::uint32_t(std::int32_t(value) >> count);
}

// The assert macro's failure at one site of the original: report_assertion (which clears the shared
// success flag once, as the original's single `stb` does), then r0 = r3 = 0. `flag_in_r4` for the
// sites that load the flag's address into r4 before that store. Answers false.
inline bool assertion_failure(Cpu& c, const guest::Frame& frame, std::uint32_t pool, std::uint32_t at,
                              std::int32_t name_at, std::int32_t extension_at, std::uint32_t line,
                              bool flag_in_r4 = false) {
    guest::report_assertion(c, {at, pool, pool + 0xd, pool + 0x1a, frame.at(name_at), frame.at(extension_at), line});
    if (flag_in_r4) c.set(4, c.toc(-0x7bac));
    c.set(0, 0);
    c.set(3, 0);
    return false;
}
}
