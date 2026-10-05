#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 3> rng_core_entries{
    0x1077b770, 0x1077b790, 0x107474a0};
// Private in-memory contract harness. No oracle payload is recorded in the app.
struct RngBoundaryObserver {
    virtual ~RngBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t entry, bool original) = 0;
    virtual void leave(State&, std::uint32_t entry, bool original) = 0;
};
bool try_modern_rng_core(State&, std::uint32_t entry);
void reset_rng_diagnostics(State&);
std::string rng_core_call_summary(const State&);
// Validation-only storage access inventory. Aggregates by field/PC, never by
// draw, to distinguish serialization accesses from ordinary pool consumers.
std::unique_ptr<MemoryObserver> observe_rng_storage(State&);
}
