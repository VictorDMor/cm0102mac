#pragma once
#include "aot_runtime.hpp"
namespace cm::aot::actor_construction {
inline constexpr std::array<std::uint32_t, 2> entries{0x1020e8f0, 0x10223ac0};
struct DependencyDispatch {
    virtual ~DependencyDispatch() = default;
    virtual void call(State&, std::uint32_t target, std::uint32_t continuation) = 0;
};
void prepare_actor(State&, DependencyDispatch* = nullptr);
void initialize_actor(State&, DependencyDispatch* = nullptr);
}
