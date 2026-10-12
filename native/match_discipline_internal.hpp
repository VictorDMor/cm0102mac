#pragma once
#include "semantic_guest_call.hpp"

namespace cm::aot::match_discipline {
using DisciplineCall = SemanticGuestCall;
// Original stream cursor, replenishment, seed and limit remain live guest data.
void advance_discipline_random(DisciplineCall&, std::uint32_t site,
                               unsigned cursor = 31, unsigned limit = 30,
                               unsigned buffer = 29, unsigned seed = 28);
void time_scaled_booking_rating(DisciplineCall&, std::uint32_t site,
                               unsigned context, unsigned actor, unsigned constants,
                               unsigned wide_constants, bool load_wide_constants,
                               unsigned buffer, unsigned result_buffer);
}
