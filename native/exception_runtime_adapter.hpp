#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 21> exception_runtime_entries{
    0x10771640, 0x107717f0, 0x10771b30, 0x10772430, 0x107724c0,
    0x107725a0, 0x10772600, 0x10772650, 0x10772780, 0x10772810,
    0x10772ad0, 0x10772b10, 0x10772ca0, 0x10772d80, 0x10772dd0,
    0x10772de0, 0x10772e70, 0x10772e90, 0x10772eb0, 0x10772ee0, 0x10772f10};
enum class ExceptionRuntimeOutcome { returned, transferred, terminated };
struct ExceptionRuntimeBoundaryObserver {
    virtual ~ExceptionRuntimeBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ExceptionRuntimeOutcome) = 0;
    virtual void landing_enter(const State&, bool) {}
    virtual void landing_leave(State&, bool, ExceptionRuntimeOutcome) {}
    virtual void import_enter(const State&, const char*) {}
    virtual void import_leave(const State&, const char*) {}
    // Only an explicitly attached component fixture may supply import results.
    virtual bool import_fixture(State&, const char*) { return false; }
};
bool try_modern_exception_runtime(State&, std::uint32_t);
bool try_modern_exception_unexpected_landing(State&);
// Semantic children use the same selection, counters and boundaries as hooks.
void invoke_exception_runtime(State&, std::uint32_t);
// A computed destination outside the reconstructed cases is separate coverage.
void resume_exception_unknown_transfer(State&, std::uint32_t target, std::uint32_t caller);
void reset_exception_runtime_diagnostics(State&);
std::string exception_runtime_call_summary(const State&);
}
