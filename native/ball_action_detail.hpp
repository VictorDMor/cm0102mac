#pragma once
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_action::detail {
// Preserve the common word-interval sampling schedule. Callers own the zero
// interval gate and keep their interval and result in nonvolatile registers.
void sample_action_interval(SemanticGuestCall&, std::uint32_t start, unsigned interval, unsigned result);

// The original rectangles repeatedly read the actor's live signed coordinates.
// The right rectangle has one extra comparison and jumps to the shared join.
void area_coordinates(SemanticGuestCall&,std::uint32_t start,std::uint32_t join,
                      bool right,unsigned coordinate,unsigned truth,unsigned result);
}
