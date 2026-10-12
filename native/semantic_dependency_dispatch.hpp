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
}
