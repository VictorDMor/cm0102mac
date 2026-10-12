#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::match_actor {
inline constexpr std::array<std::uint32_t, 3> entries{0x101f1530, 0x101f1860, 0x101f1870};
struct DependencyDispatch {
    virtual ~DependencyDispatch() = default;
    virtual void call(State&, std::uint32_t target, std::uint32_t continuation) = 0;
};
void dismiss_player(State&, DependencyDispatch* = nullptr);
void read_position(State&);
void write_position(State&);
}
