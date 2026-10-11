#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::record_history {
inline constexpr std::array<std::uint32_t, 2> entries{0x1073e350, 0x1073e4b0};
inline constexpr unsigned row_count = 16, row_stride = 0x2c0, record_stride = 44, field_count = 60;
struct DependencyDispatch {
    virtual ~DependencyDispatch() = default;
    virtual void call(State&, std::uint32_t target, std::uint32_t continuation) = 0;
};
unsigned entry_index(std::uint32_t);
void invoke(State&, unsigned, DependencyDispatch* = nullptr);
}
