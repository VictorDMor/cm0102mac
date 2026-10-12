#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::marking_pressure {
using DependencyDispatch=SemanticDependencyDispatch;
enum class Entry:unsigned{trait_10a,action_code,append_marker,set_pressure_adjustment,trait_10d,assess_cell,pressure_adjustment,current_position,eligible_marker,tactical_marking_trait,contains_marker,remove_marker,team};
inline constexpr std::array<std::uint32_t,13> entries{0x101d5660,0x101d56a0,0x101d5700,0x101d57d0,0x101d7da0,0x101d7dc0,0x101d8100,0x101d8110,0x101d8120,0x102373c0,0x10283540,0x10283590,0x10283690};
void invoke(State&,Entry,DependencyDispatch* = nullptr);
}
