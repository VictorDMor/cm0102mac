#include "paged_screen_layout.hpp"

namespace cm::aot {
// Hall of Fame (builder 0x1033fcf0, opener 0x1033fc60 from the Game menu): the best managers of the nation
// chosen in the menu above the table, five columns (rank, manager, club, date and score) of twenty rows,
// the year-range and nation controls above its two edges and the Back/Next footer. The original builder
// keeps every text, menu and command; this pass moves the table, the two controls and the footer, and lets
// only the widest text column take the extra width. The menus keep the original popups, placed when they open.
void layout_hall_of_fame(State& s, std::uint32_t display, GuiCanvas canvas) {
    paged_screen::layout_table_screen(s, display, canvas, ScreenRect{110, 170, 780, 535}, 5);
}
}
