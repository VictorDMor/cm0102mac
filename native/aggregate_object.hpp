#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::aggregate_object {
inline constexpr std::array<std::uint32_t, 6> entries{
    0x10718b40, 0x10718ba0, 0x1073c1b0, 0x1073c460, 0x1073c4e0, 0x1073f4e0};
inline constexpr unsigned table_extent = 0x12a948, aggregate_extent = 0x2581b8;
struct DependencyDispatch {
    virtual ~DependencyDispatch() = default;
    virtual void call(State&, std::uint32_t target, std::uint32_t continuation) = 0;
};
unsigned entry_index(std::uint32_t);
void invoke(State&, unsigned, DependencyDispatch* = nullptr);
}
