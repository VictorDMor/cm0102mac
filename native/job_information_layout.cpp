#include "paged_screen_layout.hpp"

namespace cm::aot {
// Job Information (builder 0x101c74e0, MANAGER_SCREENS.CPP, opener 0x101c7370 from the Manager menu): the
// managerial jobs on offer in the nation chosen in the control under the subtitle, as a five-column table
// (manager, club, nation, division, job security) of sixteen rows with a scrollbar when there are more, or
// one column holding a centered message when there are none, and the Back/Next footer. The original builder
// keeps every text, link and command; this pass moves the table, the nation control and the footer, and lets
// only the manager column take the extra width. The scrollbar keeps its gutter at the table's right edge.
void layout_job_information(State& s, std::uint32_t display, GuiCanvas canvas) {
    paged_screen::layout_table_screen(s, display, canvas, ScreenRect{110, 170, 780, 535}, 5, true);
}
}
