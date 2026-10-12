#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::spatial_role {
using DependencyDispatch = SemanticDependencyDispatch;
enum class Entry : unsigned {
    current_position,
    column,
    row,
    marking_target,
    set_marking_target,
    clear_marking_list,
    marking_count,
    current_role,
    secondary_role,
    previous_role,
    previous_position,
    register_center_cell,
    set_column,
    set_row,
    remove_from_cell,
    marking_member,
    clear_goalkeeper,
    resolve_goalkeeper,
    set_previous_position,
    cached_role,
    actor_current_role,
    actor_previous_role,
    actor_secondary_role,
};
inline constexpr std::array<std::uint32_t,23> entries{0x101d3b50,0x101d5680,0x101d5690,0x101d56c0,0x101d5730,0x101d5740,0x101d7db0,0x101e3d50,0x101e3de0,0x101e3e70,0x101e3f00,0x101f1880,0x101f1930,0x101f1940,0x101f1950,0x101f1dc0,0x101f1de0,0x101f1e10,0x101f21c0,0x10289550,0x10289560,0x102895a0,0x102895e0};
void invoke(State&, Entry, DependencyDispatch* = nullptr);
}
