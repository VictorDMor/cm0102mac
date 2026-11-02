#pragma once
#include "media.hpp"

namespace cm::aot::media {
// 0x10672800: a press story of `kind` about a club, as seen from `side` (1..3), with `other` (the
// second club of the story): what it does to the club's two rating columns through the rating routines
// (0x1066b600, 0x1066bc50) and its flags, and finally the item it posts (0x106620b0). `reporter` is the
// caller's context, passed on to those routines.
void evaluate_club_story(guest::Cpu&, std::uint32_t reporter, std::uint32_t club, std::uint32_t kind, std::uint32_t side, std::uint32_t other);
}
