#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::native_service {
// These are the complete framed facades around existing native imports.
enum class Service : unsigned { apple_event, new_pointer, dispose_pointer };
inline constexpr std::array<std::uint32_t, 3> entries{0x10770a80, 0x10778d50, 0x10778d80};
inline constexpr std::array<std::uint32_t, 3> import_glues{0x10780528, 0x1077fc70, 0x1077fc58};
unsigned entry_index(std::uint32_t);
void invoke(State&, Service);
}
