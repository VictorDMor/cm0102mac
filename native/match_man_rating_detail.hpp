#pragma once
// The rating block's record offsets and the idioms its floating point shares.
// Meanings marked "inferred" come from how the callers and the values use
// them; the offsets are the original's.
#include "match_man_detail.hpp"

namespace cm::aot::match_man {
// The club record given to the rating (only +0x90 is read).
namespace rating_club {
constexpr std::uint32_t default_judge = 0x90;  // inferred: the judge used when none is given
}
// The value of `kind` that asks for the current ability.
constexpr std::uint32_t current_kind = 2;

}
