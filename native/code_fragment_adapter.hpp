#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 5> code_fragment_entries{
    0x10771150, 0x107712b0, 0x107713e0, 0x107715f0, 0x10771620};
struct CodeFragmentBoundaryObserver {
    virtual ~CodeFragmentBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool) = 0;
    virtual void leave(State&, std::uint32_t, bool) = 0;
    virtual void import_enter(const State&, const char*) {}
    virtual void import_leave(const State&, const char*) {}
    // An explicitly attached component fixture may provide native results.
    // Without an observer, production always uses the native bridge.
    virtual bool import_fixture(State&, const char*) { return false; }
};
bool try_modern_code_fragment(State&, std::uint32_t);
void reset_code_fragment_diagnostics(State&);
std::string code_fragment_call_summary(const State&);
}
