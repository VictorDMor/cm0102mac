#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 5> collection_lookup_entries{
    0x1074dd40, 0x1074df60, 0x1074df90, 0x1074e150, 0x1074e5b0};

// Optional private contract-harness boundaries. Production leaves this null.
struct CollectionBoundaryObserver {
    virtual ~CollectionBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t entry, bool original) = 0;
    virtual void leave(State&, std::uint32_t entry, bool original) = 0;
};
bool try_modern_collection_lookup(State&, std::uint32_t entry);
void reset_collection_lookup_diagnostics(State&);
std::string collection_lookup_call_summary(const State&);
}
