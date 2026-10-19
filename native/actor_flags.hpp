#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"
namespace cm::aot::actor_flags {
inline constexpr std::array<std::uint32_t,7> entries{0x102862b0,0x10286320,0x10286390,0x10286400,0x10286470,0x102864b0,0x102864f0};
// Complete actor strategy queries. Both flag words, signed side/slot bytes,
// raw team override bytes, carry residues and ordered live reads are preserved.
void test_bit21(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void test_bit25(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void test_bit29(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void test_bit27(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void read_flags(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void read_flags_alternate(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
void team_or_slot_status(State&,SemanticDependencyDispatch* = nullptr,SemanticGuestObserver* = nullptr);
}
