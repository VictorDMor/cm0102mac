#pragma once
#include "spatial_role.hpp"

namespace cm::aot {
enum class SpatialRoleOutcome { returned, fault, budget_stop, context_transfer, terminated };
struct SpatialRoleBoundaryObserver {
    virtual ~SpatialRoleBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, SpatialRoleOutcome) = 0;
};
struct SpatialRoleDiagnostics {
    bool original = false;
    std::array<bool, 23> entry_original{};
    std::array<std::uint64_t, 23> calls{}, original_calls{}, returns{}, faults{}, budget_stops{}, transfers{}, terminations{}, profile_original_calls{};
    std::uint32_t original_body = 0;
    std::uint64_t dispatch_escapes = 0;
    SpatialRoleBoundaryObserver* observer = nullptr;
};
SpatialRoleDiagnostics& spatial_role_diagnostics(State&);
const SpatialRoleDiagnostics& spatial_role_diagnostics(const State&);
bool try_modern_spatial_role(State&, std::uint32_t);
}
