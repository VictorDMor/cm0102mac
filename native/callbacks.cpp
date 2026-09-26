#include "callbacks.hpp"
#include "aot_runtime.hpp"

namespace cm::aot {
std::uint32_t call_original(State& s, std::uint32_t transition_vector, std::span<const std::uint32_t> arguments) {
    if (arguments.size() > 8) throw std::runtime_error("Original callback requires stack argument marshalling");
    struct Saved {
        decltype(State::registers) registers;
        decltype(State::unique) unique;
        std::uint32_t pc;
    };
    auto saved = std::make_unique<Saved>(Saved{s.registers, s.unique, s.pc});
    const auto previous_stack = std::uint32_t(s.r<4>(4));
    if (previous_stack < 0x100) throw std::runtime_error("Invalid original callback stack");
    const auto stack = (previous_stack - 0x100) & ~15u;
    s.memory.clear(stack, previous_stack - stack);
    s.memory.write<4>(stack, previous_stack);
    s.wr<4>(4, stack);
    s.wr<4>(8, s.memory.read<4>(transition_vector + 4));
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        s.wr<4>((3 + i) * 4, arguments[i]);
        s.memory.write<4>(stack + 24 + i * 4, arguments[i]);
    }
    s.wr<4>(4128, return_sentinel);
    s.pc = s.memory.read<4>(transition_vector);
    // Re-enter the same statically compiled native dispatcher. No instruction
    // bytes are interpreted and no executable code is generated at runtime.
    run(s);
    const auto result = std::uint32_t(s.r<4>(12));
    s.registers = saved->registers;
    s.unique = saved->unique;
    s.pc = saved->pc;
    return result;
}
}
