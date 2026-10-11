#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 6> runtime_lifecycle_entries{
    0x10772df0, 0x10772e10, 0x10773fb0,
    0x10773ff0, 0x10774060, 0x1077db20};
struct RuntimeLifecycleBoundaryObserver {
    virtual ~RuntimeLifecycleBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool) = 0;
    // A normal application termination is a successful nonreturning boundary.
    virtual void leave(State&, std::uint32_t, bool, bool terminal) = 0;
    virtual void import_enter(const State&, const char*) {}
    virtual void import_leave(const State&, const char*) {}
    // Only an explicitly attached characterization fixture can make the native
    // ExitToShell dependency return; production retains ApplicationExit.
    virtual bool import_fixture(State&, const char*) { return false; }
};
bool try_modern_runtime_lifecycle(State&, std::uint32_t);
void reset_runtime_lifecycle_diagnostics(State&);
std::string runtime_lifecycle_call_summary(const State&);
}
