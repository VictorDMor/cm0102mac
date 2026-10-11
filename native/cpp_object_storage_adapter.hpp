#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 12> cpp_object_storage_entries{
    0x10773150, 0x10773260, 0x10773320, 0x10773420, 0x107734a0,
    0x10773520, 0x107735a0, 0x107735f0, 0x10773610, 0x10773630,
    0x10773680, 0x107736e0};
struct CppObjectStorageBoundaryObserver {
    virtual ~CppObjectStorageBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool) = 0;
    // A normal application termination is a successful nonreturning boundary.
    virtual void leave(State&, std::uint32_t, bool, bool terminal) = 0;
    virtual void import_enter(const State&, const char*) {}
    virtual void import_leave(const State&, const char*) {}
    // Only an explicitly attached characterization fixture can provide native
    // dependency results; production retains its native bridges.
    virtual bool import_fixture(State&, const char*) { return false; }
};
bool try_modern_cpp_object_storage(State&, std::uint32_t);
void reset_cpp_object_storage_diagnostics(State&);
std::string cpp_object_storage_call_summary(const State&);
}
