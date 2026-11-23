#pragma once
// graph_carbon.cpp, block `colours` (graph_carbon_colours.cpp): the bitmap record and the variables the colour and bitmap routines use.
// Kept apart from graph_carbon_detail.hpp so that blocks written at the same time cannot define a name twice.
#include "graph_carbon_detail.hpp"

namespace cm::aot::graph_carbon {
// A bitmap record (0x30 bytes, a block of the allocator's): its pixels (15-bit, two bytes each, rows of `width` pixels) are a block of
// their own at `pixels`; the 0x20 bytes of metadata from 0x10 are copied word by word and never read by this block.
namespace bitmap {
inline constexpr std::uint32_t width = 0, height = 4, bytes = 8, pixels = 0xc, metadata = 0x10, size = 0x30;
}
// The surface the drawing layer writes (small data, addressed from r2, graph_carbon_audit.SMALL_DATA).
namespace surface_data {
inline constexpr std::int32_t base = -0x3564,   // u32: the address of the 15-bit pixels
    stride_pixels = -0x3568,                    // i16: the pixels from one row to the next
    row_bytes = -0x3566,                        // i16: the bytes from one row to the next (the stride's double)
    setting_up = -0x3550;                       // byte: nonzero while the display is being created: nothing is drawn or loaded
}
}
