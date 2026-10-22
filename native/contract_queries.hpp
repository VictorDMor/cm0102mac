#pragma once
#include "aot_runtime.hpp"
#include "semantic_dependency_dispatch.hpp"

namespace cm::aot::contract_queries {
inline constexpr std::uint32_t primary_entry = 0x10082f20;
inline constexpr std::uint32_t club_entry = 0x10082f90;
inline constexpr std::uint32_t secondary_entry = 0x10083110;
inline constexpr std::array<std::uint32_t, 3> entries{primary_entry, club_entry, secondary_entry};
void find_primary(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void find_for_club(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
void find_secondary(State&, SemanticDependencyDispatch* = nullptr, SemanticGuestObserver* = nullptr);
// Plain C++ for the app and the replays: runs the lookup at `entry` with the
// original's reads, results, register and frame effects and instruction
// count (`ran`). Has no effect when reads are observed, `entry` is not a
// lookup, a read would fault, the frame of 0x10082f90 overlaps a record it
// reads, or that routine needs the club resolver 0x100ce320; the builder
// versions above then run.
struct NativeRun {
    bool ran = false;
    bool nested_secondary = false;   // 0x10082f90 also ran 0x10083110
};
NativeRun run_native(State&, std::uint32_t entry);
}
