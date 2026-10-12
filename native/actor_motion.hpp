#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::actor_motion {
enum class Entry { set_coordinates, set_heading, heading, can_face_destination, has_possession };
inline constexpr std::array<std::uint32_t, 5> entries{0x102374f0, 0x10237830, 0x10237b00, 0x10245050, 0x102775c0};
void invoke(State&, Entry, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
