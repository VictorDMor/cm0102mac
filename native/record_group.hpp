#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::record_group {
inline constexpr std::array<std::uint32_t, 11> entries{
    0x1073edb0, 0x1073ede0, 0x1073ee30, 0x1073ef70, 0x1073f0a0, 0x1073f1e0, 0x1073f530, 0x1073f700, 0x1073f7a0, 0x1073f860, 0x1073f900};
inline constexpr unsigned row_count = 16, row_stride = 0x2c0, record_stride = 44, field_count = 60;
struct DependencyDispatch {
    virtual ~DependencyDispatch() = default;
    virtual void call(State&, std::uint32_t target, std::uint32_t continuation) = 0;
};
unsigned entry_index(std::uint32_t);
void invoke(State&, unsigned, DependencyDispatch* = nullptr);
}
