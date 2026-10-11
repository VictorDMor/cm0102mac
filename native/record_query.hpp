#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::record_query {
inline constexpr std::array<std::uint32_t, 6> entries{
    0x1073e990, 0x1073e9b0, 0x1073ea20, 0x1073eb00, 0x1073ec00, 0x1073ecd0};
inline constexpr unsigned row_count = 16, row_stride = 0x2c0, record_stride = 44;
struct DependencyDispatch {
    virtual ~DependencyDispatch() = default;
    virtual void call(State&, std::uint32_t target, std::uint32_t continuation) = 0;
};
unsigned entry_index(std::uint32_t);
void invoke(State&, unsigned, DependencyDispatch* = nullptr);
}
