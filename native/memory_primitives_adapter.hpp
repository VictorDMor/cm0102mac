#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 6> memory_primitives_entries{
    0x10739ef0, 0x10778300, 0x10778330, 0x10778360, 0x10778390, 0x107783e0};
struct MemoryPrimitivesBoundaryObserver {
    virtual ~MemoryPrimitivesBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t entry, bool original) = 0;
    virtual void leave(State&, std::uint32_t entry, bool original) = 0;
};
bool try_modern_memory_primitives(State&, std::uint32_t entry);
void reset_memory_primitives_diagnostics(State&);
std::string memory_primitives_call_summary(const State&);
}
