#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::engine_initialization {
inline constexpr std::array<std::uint32_t, 20> entries{
    0x10000000, 0x10064770, 0x100939b0, 0x100de3b0, 0x1013f000,
    0x10143240, 0x101935b0, 0x102aac90, 0x1030d470, 0x10315050,
    0x103563e0, 0x103810d0, 0x1048eb80, 0x106b6e60, 0x106bbe10,
    0x107259a0, 0x10739f30, 0x10744640, 0x1074d340, 0x1076ea20};
// Used only by isolated component fixtures. Production calls existing guest
// constructors and lifecycle helpers through the ordinary dispatcher.
struct DependencyDispatch {
    virtual ~DependencyDispatch() = default;
    virtual void call(State&, std::uint32_t target, std::uint32_t continuation) = 0;
};
unsigned entry_index(std::uint32_t);
void invoke(State&, unsigned index, DependencyDispatch* = nullptr);
}
