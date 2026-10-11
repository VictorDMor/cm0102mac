#pragma once
#include "aot_runtime.hpp"
namespace cm::aot {
inline constexpr std::array<std::uint32_t, 4> stdlib_core_entries{0x10775540, 0x10775550, 0x10775560, 0x10775600};
enum class StdlibCoreOutcome { returned, transferred, fault, budget_return };
struct StdlibCoreBoundaryObserver {
    virtual ~StdlibCoreBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, StdlibCoreOutcome) = 0;
};
bool try_modern_stdlib_core(State&, std::uint32_t);
void reset_stdlib_core_diagnostics(State&);
std::string stdlib_core_call_summary(const State&);
}
