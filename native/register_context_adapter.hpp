#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr auto register_context_entries = [] {
    std::array<std::uint32_t, 30> entries{};
    for (unsigned n = 0; n < 14; ++n) {
        entries[n] = 0x107737e0 + n * 4; entries[n + 14] = 0x10773830 + n * 4;
    }
    entries[28] = 0x10773eb0; entries[29] = 0x10773f30; return entries;
}();
enum class RegisterContextOutcome { returned, transferred, fault };
struct RegisterContextBoundaryObserver {
    virtual ~RegisterContextBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t entry, bool original) = 0;
    virtual void leave(State&, std::uint32_t entry, bool original, RegisterContextOutcome) = 0;
};
bool try_modern_register_context(State&, std::uint32_t entry);
void invoke_register_context(State&, std::uint32_t entry);
void reset_register_context_diagnostics(State&);
std::string register_context_call_summary(const State&);
}
