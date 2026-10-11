#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 26> file_controller_entries{
    0x10776690, 0x107766c0, 0x107768f0, 0x10776940, 0x10776a40, 0x10776bc0,
    0x10776c30, 0x10776cd0, 0x10776d10, 0x10776d20, 0x10776d60, 0x10776da0,
    0x10777140, 0x107772e0, 0x10777430, 0x107777d0, 0x10777990, 0x10777a60,
    0x10777c40, 0x10777cc0, 0x10777ce0, 0x10777d20, 0x10777f10, 0x10777f30,
    0x107784a0, 0x107784b0};
struct FileControllerBoundaryObserver {
    virtual ~FileControllerBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool) = 0;
    virtual void leave(State&, std::uint32_t, bool) = 0;
    virtual void import_enter(const State&, const char*) {}
    virtual void import_leave(const State&, const char*) {}
    // Explicitly attached component fixtures may supply dependency imports.
    // Production has no observer, and always uses the actual native bridge.
    virtual bool import_fixture(State&, const char*) { return false; }
};
bool try_modern_file_controller(State&, std::uint32_t);
void reset_file_controller_diagnostics(State&);
std::string file_controller_call_summary(const State&);
}
