#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 5> integer_parsing_entries{
    0x1077ee20, 0x1077f180, 0x1077f570, 0x1077f670, 0x1077f6a0};
struct IntegerParsingBoundaryObserver {
    virtual ~IntegerParsingBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t entry, bool original) = 0;
    virtual void leave(State&, std::uint32_t entry, bool original) = 0;
};
bool try_modern_integer_parsing(State&, std::uint32_t entry);
void reset_integer_parsing_diagnostics(State&);
std::string integer_parsing_call_summary(const State&);
}
