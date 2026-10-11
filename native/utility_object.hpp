#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::utility_object {
inline constexpr std::array<std::uint32_t, 11> entries{
    0x10738b20, 0x10738ba0, 0x10738c00, 0x10738c20, 0x10738ca0,
    0x1073ae10, 0x1073aee0, 0x1074dec0, 0x1074dee0, 0x1074dfc0, 0x1074e000};
// Explicit controlled dependencies belong only to isolated component checks.
// Production retains the allocator, memory and tree-cleanup dispatchers.
struct DependencyDispatch {
    virtual ~DependencyDispatch() = default;
    virtual void call(State&, std::uint32_t target, std::uint32_t continuation) = 0;
};
unsigned entry_index(std::uint32_t);
void invoke(State&, unsigned, DependencyDispatch* = nullptr);
}
