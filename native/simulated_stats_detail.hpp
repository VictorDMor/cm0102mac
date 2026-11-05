#pragma once

#include "guest_values.hpp"

namespace cm::aot::simulated_stats_detail {

struct RandomPool {
    std::uint32_t constants, salt, begin, end, cursor;
};

struct RangeDraw {
    std::uint32_t site;
    unsigned result, bound_scratch, remainder_scratch, result_scratch;
};

// The constructor and daily update inline the same sampler with distinct
// scratch locations. The caller handles the zero bound and pool advance.
void draw_range(guest::Cpu& c, const RandomPool& pool, RangeDraw draw, std::int32_t bound);

}
