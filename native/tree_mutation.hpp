#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::tree_mutation {
inline constexpr std::array<std::uint32_t, 3> entries{0x1074df40, 0x1074e030, 0x1074ea30};
// Explicit controlled dependencies belong only to isolated component checks.
// Production retains actual scalar allocation, recursive and CFM dispatch.
struct DependencyDispatch {
    virtual ~DependencyDispatch() = default;
    virtual void call(State&, std::uint32_t target, std::uint32_t continuation) = 0;
};
unsigned entry_index(std::uint32_t);
void invoke(State&, unsigned, DependencyDispatch* = nullptr);
}
