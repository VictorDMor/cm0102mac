#include "form_layout.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_startup(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const auto grid = [display](unsigned i) { return display + 4 + i * 0xbf0; };
    const auto rect = [&](unsigned at) {
        return ScreenRect{c.i32(at), c.i32(at + 4), c.i32(at + 8), c.i32(at + 12)};
    };
    const auto write = [&](unsigned at, ScreenRect box) {
        c.put32(at, box.left); c.put32(at + 4, box.top);
        c.put32(at + 8, box.right); c.put32(at + 12, box.bottom);
    };
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    int body = -1;
    for (unsigned i = 0; i < grids; ++i)
        if (rect(grid(i)) == ScreenRect{110,145,780,535} && c.u32(grid(i) + 0x18) == 1 &&
            c.u8(grid(i) + 0xbb0) == 2 && c.u8(grid(i) + 0xbb1) == 6) body = int(i);
    if (body < 0) return;

    const ScreenLayout shell{canvas};
    auto available = shell.content();
    available.bottom -= 10;
    const int width = std::min(1120, available.width());
    available.left += (available.width() - width) / 2;
    available.right = available.left + width;
    const std::array<FormTrack,1> panel_track{{{325,1,360}}};
    auto panel = form_tracks(available, panel_track, false).front();
    const int shift = (available.height() - panel.height()) / 2;
    panel.top += shift; panel.bottom += shift;
    const std::array<FormTrack,3> column_tracks{{{0,1},{10},{0,1}}};
    const auto columns = form_tracks(panel, column_tracks, true);
    const std::array<FormTrack,9> row_tracks{{{50,1},{8},{50,1},{8},{50,1},{8},{50,1},{8},{50,1}}};
    const auto rows = form_tracks(panel, row_tracks, false);

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    write(grid(body), panel);
    c.put8(grid(body) + 0xbb1, 5);
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = display + 0xba864 + i * 0x17c;
        if (c.i16(control + 0x72) == body) {
            const int column = c.i32(control + 0x20), row = c.i32(control + 0x24);
            if (column < 0 || column > 1 || row < 0 || row > 4) continue;
            auto box = columns[column * 2];
            box.top = rows[row * 2].top; box.bottom = rows[row * 2].bottom;
            // The final odd choice has its own horizontal overrides in the
            // original helper. Keep it centered at the same width as a column.
            if (row == 4) {
                const int left = panel.left + (panel.width() - box.width()) / 2;
                box.right = left + box.width(); box.left = left;
            }
            c.put32(control + 0x28, box.left); c.put32(control + 0x2c, box.right);
            c.put32(control + 0x30, box.top); c.put32(control + 0x34, box.bottom);
        } else if (c.i16(control + 0x72) < 0) {
            // Shared header helpers already use the canvas in production;
            // also accept a previously built classic screen.
            const auto before = rect(control + 0x10);
            if (before == ScreenRect{100,10,790,70}) write(control + 0x10, shell.title());
            else if (before == ScreenRect{100,80,790,125}) write(control + 0x10, shell.subtitle());
        }
    }
    c.set(3, grid(body)); c.call(0x10714630, 0xfffffff0);
    c.set({{3, grid(body)}, {4, 0}}); c.call(0x10714110, 0xfffffff0);
    for (unsigned i = 0; i < grids; ++i) {
        const auto before = rect(grid(i));
        if ((before != shell.footer() && before != ScreenRect{100,555,790,590}) ||
            c.u8(grid(i) + 0xbb0) != 2 || c.u8(grid(i) + 0xbb1) != 1) continue;
        write(grid(i), {panel.right - 340, canvas.height - 45, panel.right, canvas.height - 10});
        c.put8(grid(i) + 0xbb2, 1); c.put8(grid(i) + 0xbb3, 1);
        c.set(3, grid(i)); c.call(0x10714630, 0xfffffff0);
        c.set({{3, grid(i)}, {4, 0}}); c.call(0x10714110, 0xfffffff0);
    }
}
}
