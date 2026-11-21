#include "paged_screen_layout.hpp"

namespace cm::aot {
// UEFA Coefficients (builder 0x10074d80, COMP_SCREENS.CPP, opener 0x10074d00 from the Competitions menu): a
// six-column table (rank, nation, TCE, UEFA, INTER, coefficient) whose header row and sixteen nations the
// builder fills, with a scrollbar for the rest, and the Back/Next footer. The original builder keeps every
// text, sort link and command; this pass moves the table and the footer, and lets only the nation column
// take the extra width. The scrollbar stays on the table's right edge, where the original draws it.
void layout_uefa_coefficients(State& s, std::uint32_t display, GuiCanvas canvas) {
    paged_screen::layout_table_screen(s, display, canvas, ScreenRect{110, 145, 780, 535}, 6);
}
}
