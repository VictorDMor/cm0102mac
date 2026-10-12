#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
// Component fixtures may characterize a dependency at its actual ABI boundary.
// Production calls the shared guest dispatcher when this optional port is null.
struct SemanticDependencyDispatch {
    virtual ~SemanticDependencyDispatch() = default;
    virtual void call(State&, std::uint32_t target, std::uint32_t continuation) = 0;
};
// Optional process-local instruction/dependency evidence. An absent observer
// has no architectural effect and is never part of a guest save or setting.
struct SemanticGuestObserver {
    virtual ~SemanticGuestObserver() = default;
    virtual void before_step(State&, std::uint32_t) {}
    virtual void before_dependency(State&, std::uint32_t, std::uint32_t) {}
};
}
