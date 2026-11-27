#pragma once

#include <cstdint>

namespace cm::aot::guest { class Cpu; }

namespace cm::aot::croatia_first {
// The 3.89 daily callback checks the completed opening table before the ordinary league update.
void maybe_split(guest::Cpu&, std::uint32_t competition, std::uint32_t context);

// The saved standings include the per-club position histories. The comparison descriptor is process state.
void restore_split_order(guest::Cpu&, std::uint32_t competition);

// At the shared six-club comparator, dispatch here only for a Croatian 3.89 root currently being sorted.
std::int32_t compare_split(guest::Cpu&, std::uint32_t left_row, std::uint32_t right_row);
}
