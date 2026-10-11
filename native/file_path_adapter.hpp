#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 7> file_path_entries{
    0x107784c0, 0x10778520, 0x107786f0, 0x10778820,
    0x10778870, 0x107788d0, 0x10778a80};
struct FilePathBoundaryObserver {
    virtual ~FilePathBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool) = 0;
    virtual void leave(State&, std::uint32_t, bool) = 0;
    virtual void import_enter(const State&, const char*) {}
    virtual void import_leave(const State&, const char*) {}
    // An explicitly attached component fixture may provide native results.
    // Without an observer, production always uses the native bridge.
    virtual bool import_fixture(State&, const char*) { return false; }
};
bool try_modern_file_path(State&, std::uint32_t);
void reset_file_path_diagnostics(State&);
std::string file_path_call_summary(const State&);
}
