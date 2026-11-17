#include "form_layout.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_daily_fixtures(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const auto grid = [display](unsigned i) { return display + 4 + i * 0xbf0; };
    const auto rect = [&](std::uint32_t at) {
        return ScreenRect{c.i32(at), c.i32(at + 4), c.i32(at + 8), c.i32(at + 12)};
    };
    const auto write = [&](std::uint32_t at, ScreenRect box) {
        c.put32(at, box.left); c.put32(at + 4, box.top);
        c.put32(at + 8, box.right); c.put32(at + 12, box.bottom);
    };
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    int body = -1;
    for (unsigned i = 0; i < grids; ++i) {
        const auto box = rect(grid(i));
        if (box.left == 110 && box.right == 780 && (box.top == 145 || box.top == 170) &&
            box.bottom == 535 && c.u32(grid(i) + 0x18) == 2 && c.u8(grid(i) + 0xbb0) == 9) body = int(i);
    }
    if (body < 0) return;
    const auto before = rect(grid(body));
    const auto original_rows = c.u8(grid(body) + 0xbb1);
    const int line_height = c.i32(c.toc(-0x740c)) != -1 ? 21 : c.i32(c.toc(-0x3bdc) + 2 * 0x1404);
    if (!original_rows || original_rows > 30 || line_height <= 0) return;
    const ScreenLayout shell{canvas};
    auto available = shell.content(before.top);
    available.bottom -= 10; // Preserve the original 20px gap above navigation.
    const int width = std::min(1440, available.width());
    available.left += (available.width() - width) / 2;
    available.right = available.left + width;
    const std::array<FormTrack,1> tracks{{{before.height(),1,30 * line_height + 16}}};
    const auto panel = form_tracks(available, tracks, false).front();
    const int rows = std::clamp((panel.height() - 16) / line_height, int(original_rows), 30);
    const bool old_overflow = c.i32(grid(body) + 0x14) + 1 > original_rows;
    // The first two original column weights total 5 of 52. AutoGrid includes
    // both endpoints, removes its 2px frame and optional 22px scroll gutter.
    const int old_home_left = before.left + 3 + 5 * (before.width() - 3 - (old_overflow ? 22 : 0)) / 52;
    const auto footer = ScreenRect{panel.right - 340, canvas.height - 45, panel.right, canvas.height - 10};
    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    // AutoGrid owns both the visible cells and invisible right-click result
    // cells. Move their common grid once, preserving every action and payload.
    write(grid(body), panel);
    for (int row = original_rows; row < rows; ++row) c.put8(grid(body) + 0xbd0 + row, 1);
    c.put8(grid(body) + 0xbb1, rows);
    c.set({{3, grid(body)}, {4, c.u32(grid(body) + 0x1fc)}}); c.call(0x107145f0, 0xfffffff0);
    c.set(3, grid(body)); c.call(0x10714630, 0xfffffff0);
    const int venue_shift = panel.left + c.i32(grid(body) + 0x1c + 2 * 4) - old_home_left;
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = display + 0xba864 + i * 0x17c;
        if (c.i16(control + 0x72) == body) {
            const int left = c.i32(control + 0x28);
            // Stage labels/underlines and neutral venues have measured text
            // bounds. Venues follow the home column; stage labels the panel.
            if (left == 115 || left == 180) {
                const int shift = left == 180 ? venue_shift : panel.left - before.left;
                c.put32(control + 0x28, left + shift);
                c.put32(control + 0x2c, c.i32(control + 0x2c) + shift);
            }
            continue;
        }
        if (c.i16(control + 0x72) >= 0) continue;
        const auto old = rect(control + 0x10);
        auto box = old;
        if (old == ScreenRect{100,10,790,70}) box = shell.title();
        else if (old == ScreenRect{125,10,790,70}) box.right = canvas.width - 10;
        else if (old == ScreenRect{100,80,790,125}) box = shell.subtitle();
        else if (old == ScreenRect{655,145,780,165}) box = {panel.right - 125,145,panel.right,165};
        else continue;
        write(control + 0x10, box);
        c.put32(control + 0x28, box.left); c.put32(control + 0x2c, box.right);
        c.put32(control + 0x30, box.top); c.put32(control + 0x34, box.bottom);
    }
    c.set({{3, grid(body)}, {4, 0}}); c.call(0x10714110, 0xfffffff0);
    for (unsigned i = 0; i < grids; ++i) {
        if (int(i) == body) continue;
        const auto old = rect(grid(i));
        auto box = old;
        if (old == ScreenRect{528,145,653,165}) box = {panel.right - 252,145,panel.right - 127,165};
        else if ((old == shell.footer() || old == ScreenRect{100,555,790,590}) &&
                 c.u8(grid(i) + 0xbb0) == 2 && c.u8(grid(i) + 0xbb1) == 1) {
            box = footer;
            c.put8(grid(i) + 0xbb2, 1); c.put8(grid(i) + 0xbb3, 1);
        } else continue;
        write(grid(i), box);
        c.set(3, grid(i)); c.call(0x10714630, 0xfffffff0);
        c.set({{3, grid(i)}, {4, 0}}); c.call(0x10714110, 0xfffffff0);
    }
}
}
