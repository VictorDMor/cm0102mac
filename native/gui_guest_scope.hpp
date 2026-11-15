#pragma once
#include "guest_abi.hpp"

namespace cm::aot {
// Extra presentation queries must not leave scratch registers or stack bytes
// for later original readers. No game record or RNG operation belongs here.
class GuiGuestScope {
    State& s;
    decltype(State::registers) registers;
    decltype(State::unique) unique;
    std::uint32_t pc, base;
    std::vector<std::uint8_t> stack;
public:
    explicit GuiGuestScope(State& state, std::size_t frame_size = 0) : s(state), registers(s.registers), unique(s.unique), pc(s.pc),
        base(std::uint32_t(s.r<4>(4)) - 0x8000), stack(s.memory.copy_out(base, 0x8000 + frame_size)) {}
    ~GuiGuestScope() {
        s.memory.copy_in(base, stack);
        s.registers = registers; s.unique = unique; s.pc = pc;
    }
};
}
