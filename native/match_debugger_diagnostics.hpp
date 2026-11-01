#pragma once
// What original/C++ comparisons of a whole match leave out: instruction counts and the
// registers the checks treat as diagnostics (r0, r11, r12, f0, f2-f13, CTR, XER, Ghidra's temporaries).
#include "guest_abi.hpp"
#include <map>

namespace cm::aot {
inline auto observable_registers(const State& s) {
    auto registers = s.registers;
    const auto clear = [&](std::size_t at, std::size_t size) { for (std::size_t n = 0; n < size; ++n) registers[at + n] = 0; };
    for (const unsigned n : {0u, 11u, 12u}) clear(guest::reg::gpr(n), 4);
    for (const unsigned n : {0u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u}) clear(guest::reg::fpr(n), 16);
    clear(guest::reg::ctr, 4);
    clear(2432, 12);  // Ghidra's effective-address temporary (stmw, lmw) and the TOC save
    clear(guest::reg::xer_so, 8);
    return registers;
}
inline void clear_diagnostic_registers(State& s) { s.registers = observable_registers(s); }
// The bytes an effect trace leaves: rewritten code may store a field or a frame in another
// order or width, which nothing in the game can see. kind 0 maps, 1 clears, 2 writes.
template<class Effects> auto final_bytes(const Effects& effects) {
    std::map<std::uint32_t, std::uint8_t> bytes;
    for (const auto& effect : effects) {
        if (effect.kind == 1) for (std::uint64_t n = 0; n < effect.length; ++n) bytes[effect.address + std::uint32_t(n)] = 0;
        else if (effect.kind == 2) for (std::size_t n = 0; n < effect.bytes.size(); ++n) bytes[effect.address + std::uint32_t(n)] = effect.bytes[n];
    }
    return bytes;
}
}
