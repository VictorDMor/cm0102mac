#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::localization_format {
inline constexpr std::uint32_t format_phrase_entry = 0x10727890;
inline constexpr std::array entries{format_phrase_entry};
void format_phrase(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
}
