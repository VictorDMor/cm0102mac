#include "paged_screen_layout.hpp"

namespace cm::aot {
// Manager Status (builder 0x10338b40, opener 0x10338a90, dispatcher action 0x3fc): the managers of the game as a
// three-column table (name, club, status) of seventeen rows with the Back/Next footer; each row also owns an
// invisible link that spans it. The original builder keeps every text, link and command; this pass moves the
// table and the footer, spans each row link over the panel and lets only the widest column take the extra width.
void layout_manager_status(State& s, std::uint32_t display, GuiCanvas canvas) {
    paged_screen::layout_table_screen(s, display, canvas, ScreenRect{110, 145, 780, 535}, 3);
}
}
