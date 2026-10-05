#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 14> string_core_entries{
    0x107393f0, 0x107394e0, 0x1074de10, 0x107395f0, 0x10739640,
    0x1077dbd0, 0x1077dbf0, 0x1077dc10, 0x1077dc60, 0x1077dc90,
    0x1077dcd0, 0x1077dd10, 0x1077dd50, 0x1077dda0};
struct StringBoundaryObserver {
    virtual ~StringBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t entry, bool original) = 0;
    virtual void leave(State&, std::uint32_t entry, bool original) = 0;
};
bool try_modern_string_core(State&, std::uint32_t entry);
void reset_string_core_diagnostics(State&);
std::string string_core_call_summary(const State&);
}
