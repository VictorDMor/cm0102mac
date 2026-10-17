#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::injury_record {
inline constexpr std::uint32_t registration_entry=0x10188cc0;
inline constexpr std::uint32_t notice_entry=0x1018aed0;
inline constexpr std::uint32_t propagation_entry=0x1018b6e0;
inline constexpr std::array<std::uint32_t,3> entries{registration_entry,notice_entry,propagation_entry};
void register_injury(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void prepare_injury_notice(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void propagate_injury_risk(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
