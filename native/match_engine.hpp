#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::match_engine {
inline constexpr std::array<std::uint32_t, 3> entries{0x101ccdb0, 0x101cd5c0, 0x101d3f60};
// Guest match records, event queues and random consumers retain their original
// storage. This seam is used by the independent component contract harness.
struct DependencyDispatch {
    virtual ~DependencyDispatch() = default;
    virtual void call(State&, std::uint32_t target, std::uint32_t continuation) = 0;
};
void advance_watched(State&, DependencyDispatch* = nullptr);
void advance_background(State&, DependencyDispatch* = nullptr);
void advance_shared(State&, DependencyDispatch* = nullptr);
}
