#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_load_browser(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const unsigned count = std::min<unsigned>(250, c.u16(display + 0x129da4));
    int field = -1, list = -1, buttons = -1;
    const ScreenLayout shell{canvas};
    for (unsigned i = 0; i < count; ++i) {
        const auto box = screen.rect(screen.grid(i));
        const auto columns = screen.columns(screen.grid(i)), rows = screen.rows(screen.grid(i));
        if (box == ScreenRect{190,195,700,231} && columns == 2 && rows == 1) field = int(i);
        else if (box == ScreenRect{190,234,700,510} && columns == 1 && rows == 6) list = int(i);
        else if ((box == ScreenRect{100,555,790,590} || box == shell.footer()) && columns == 3 && rows == 1) buttons = int(i);
    }
    if (field < 0 || list < 0 || buttons < 0) return;
    const int line_height = c.i32(c.toc(-0x740c)) != -1 ? 21 : c.i32(c.toc(-0x3bdc) + 3 * 0x1404);
    if (line_height <= 0) return;
    const int row_height = std::max(32, line_height + 8);
    auto available = shell.content();
    available.bottom -= 10;
    const int width = std::min(1120, available.width());
    available.left += (available.width() - width) / 2;
    available.right = available.left + width;
    const std::array<FormTrack,3> tracks{{{36},{8},{276,1,30 * row_height + 4}}};
    const auto boxes = form_tracks(available, tracks, false);
    const int rows = std::clamp((boxes[2].height() - 4) / row_height, 6, 30);
    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    screen.write(screen.grid(field), boxes[0]);
    screen.write(screen.grid(list), boxes[2]);
    screen.show_rows(screen.grid(list), 6, rows);
    screen.set_rows(screen.grid(list), rows);
    // AutoGrid sets, but does not clear, its scrollbar flag. Retained scroll
    // is clamped locally without rewriting the browser's history or filename.
    screen.hide_scrollbar_if_fits(screen.grid(list), rows);
    screen.resize_rows(screen.grid(list));
    screen.write(screen.grid(buttons), {available.right - 510, canvas.height - 45, available.right, canvas.height - 10});
    for (unsigned column = 0; column < 3; ++column) c.put8(screen.grid(buttons) + 0xbb2 + column, 1);
    for (int index : {field,list,buttons}) {
        screen.measure(screen.grid(index));
        screen.publish(screen.grid(index));
    }
}
}
