#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 8> compiler_arithmetic_entries{
    0x10773780, 0x10773870, 0x10773960, 0x10773a90,
    0x10773b70, 0x10773c80, 0x10773d30, 0x10773de0};
struct CompilerArithmeticBoundaryObserver {
    virtual ~CompilerArithmeticBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t entry, bool original) = 0;
    virtual void leave(State&, std::uint32_t entry, bool original, bool returned) = 0;
};
bool try_modern_compiler_arithmetic(State&, std::uint32_t entry);
void invoke_compiler_arithmetic(State&, std::uint32_t entry);
void reset_compiler_arithmetic_diagnostics(State&);
std::string compiler_arithmetic_call_summary(const State&);
}
