#include "paged_screen_layout.hpp"

namespace cm::aot {
// Arranging a friendly tour (the Club menu's friendly matches; opener 0x10045800 from the club screens' handler):
// the two screens of the first steps are Back/Next tables, laid out like the other paged screens.
//
// Choose Country (builder 0x10045930): a table of sixteen rows with a scrollbar, each row two nations side by
// side (name, confederation, name, confederation), a filter control above its right edge and Cancel/Next. The
// two name columns share the extra width equally so the pairs stay balanced; the confederation codes keep their
// size and the scrollbar keeps its 24px gutter at the table's right edge.
void layout_friendly_countries(State& s, std::uint32_t display, GuiCanvas canvas) {
    paged_screen::layout_table_screen(s, display, canvas, ScreenRect{110, 170, 780, 535}, 4, false, 0b0101);
}

// Choose Dates (builder 0x10043b00): seventeen rows of six columns (date, availability, venue and so on), a pager
// of five small controls in the subtitle's band (right-aligned to the table here) and Back/Next. Only the widest
// column takes the extra width.
void layout_friendly_dates(State& s, std::uint32_t display, GuiCanvas canvas) {
    paged_screen::layout_table_screen(s, display, canvas, ScreenRect{110, 145, 780, 535}, 6, false, 0, true);
}
}
