#pragma once
#include "native_service.hpp"

namespace cm::aot {
enum class NativeServiceOutcome { returned, fault, budget_stop, context_transfer };
struct NativeServiceBoundaryObserver {
    virtual ~NativeServiceBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, NativeServiceOutcome) = 0;
};
bool try_modern_native_service(State&, std::uint32_t);
void reset_native_service_diagnostics(State&);
std::string native_service_call_summary(const State&);
}
