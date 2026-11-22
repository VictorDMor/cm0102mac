#include "paged_screen_layout.hpp"

namespace cm::aot {
// The Game menu's Credits (builder 0x105e9ad0, opener 0x105e9a40) and Web Sites (builder 0x1062c380, opener
// 0x1062c300): one column of text lines, each line or pair of text areas with measured bounds, and the Back/Next
// footer. Credits has a section menu above the table (left) and a scrollbar; Web Sites has neither. The original
// builders keep every text, link and command; these passes move the table and the footer, carry the measured
// text areas with the table's middle and keep the scrollbar's gutter at its right edge.
void layout_credits(State& s, std::uint32_t display, GuiCanvas canvas) {
    paged_screen::layout_table_screen(s, display, canvas, ScreenRect{110, 170, 780, 535}, 1);
}
void layout_web_sites(State& s, std::uint32_t display, GuiCanvas canvas) {
    paged_screen::layout_table_screen(s, display, canvas, ScreenRect{110, 145, 780, 535}, 1);
}
}
