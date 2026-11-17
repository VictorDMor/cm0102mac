#include "form_layout.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_load_browser(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const auto grid = [display](unsigned i) { return display + 4 + i * 0xbf0; };
    const auto rect = [&](unsigned at) {
        return ScreenRect{c.i32(at), c.i32(at + 4), c.i32(at + 8), c.i32(at + 12)};
    };
    const auto write = [&](unsigned at, ScreenRect box) {
        c.put32(at, box.left); c.put32(at + 4, box.top);
        c.put32(at + 8, box.right); c.put32(at + 12, box.bottom);
    };
    const unsigned count = std::min<unsigned>(250, c.u16(display + 0x129da4));
    int field = -1, list = -1, buttons = -1;
    const ScreenLayout shell{canvas};
    for (unsigned i = 0; i < count; ++i) {
        const auto box = rect(grid(i));
        const auto columns = c.u8(grid(i) + 0xbb0), rows = c.u8(grid(i) + 0xbb1);
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
    write(grid(field), boxes[0]);
    write(grid(list), boxes[2]);
    for (int row = 6; row < rows; ++row) c.put8(grid(list) + 0xbd0 + row, 1);
    c.put8(grid(list) + 0xbb1, rows);
    // AutoGrid sets, but does not clear, its scrollbar flag. Retained scroll
    // is clamped locally without rewriting the browser's history or filename.
    if (c.i32(grid(list) + 0x14) + 1 <= rows) c.put8(grid(list) + 0xbef, 0);
    c.set({{3, grid(list)}, {4, c.u32(grid(list) + 0x1fc)}}); c.call(0x107145f0, 0xfffffff0);
    write(grid(buttons), {available.right - 510, canvas.height - 45, available.right, canvas.height - 10});
    for (unsigned column = 0; column < 3; ++column) c.put8(grid(buttons) + 0xbb2 + column, 1);
    for (int index : {field,list,buttons}) {
        c.set(3, grid(index)); c.call(0x10714630, 0xfffffff0);
        c.set({{3, grid(index)}, {4, 0}}); c.call(0x10714110, 0xfffffff0);
    }
}
}
