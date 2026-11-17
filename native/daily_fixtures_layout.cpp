#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_daily_fixtures(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    int body = -1;
    for (unsigned i = 0; i < grids; ++i) {
        const auto box = screen.rect(screen.grid(i));
        if (box.left == 110 && box.right == 780 && (box.top == 145 || box.top == 170) &&
            box.bottom == 535 && c.u32(screen.grid(i) + 0x18) == 2 && screen.columns(screen.grid(i)) == 9) body = int(i);
    }
    if (body < 0) return;
    const auto before = screen.rect(screen.grid(body));
    const auto original_rows = screen.rows(screen.grid(body));
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
    const bool old_overflow = c.i32(screen.grid(body) + 0x14) + 1 > original_rows;
    // The first two original column weights total 5 of 52. AutoGrid includes
    // both endpoints, removes its 2px frame and optional 22px scroll gutter.
    const int old_home_left = before.left + 3 + 5 * (before.width() - 3 - (old_overflow ? 22 : 0)) / 52;
    const auto footer = ScreenRect{panel.right - 340, canvas.height - 45, panel.right, canvas.height - 10};
    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    // AutoGrid owns both the visible cells and invisible right-click result
    // cells. Move their common grid once, preserving every action and payload.
    screen.write(screen.grid(body), panel);
    screen.show_rows(screen.grid(body), original_rows, rows);
    screen.set_rows(screen.grid(body), rows);
    screen.resize_rows(screen.grid(body));
    screen.measure(screen.grid(body));
    const int venue_shift = panel.left + c.i32(screen.grid(body) + 0x1c + 2 * 4) - old_home_left;
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = screen.control(i);
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
        const auto old = screen.rect(control + 0x10);
        auto box = old;
        if (old == ScreenRect{100,10,790,70}) box = shell.title();
        else if (old == ScreenRect{125,10,790,70}) box.right = canvas.width - 10;
        else if (old == ScreenRect{100,80,790,125}) box = shell.subtitle();
        else if (old == ScreenRect{655,145,780,165}) box = {panel.right - 125,145,panel.right,165};
        else continue;
        screen.write(control + 0x10, box);
        c.put32(control + 0x28, box.left); c.put32(control + 0x2c, box.right);
        c.put32(control + 0x30, box.top); c.put32(control + 0x34, box.bottom);
    }
    screen.publish(screen.grid(body));
    for (unsigned i = 0; i < grids; ++i) {
        if (int(i) == body) continue;
        const auto old = screen.rect(screen.grid(i));
        auto box = old;
        if (old == ScreenRect{528,145,653,165}) box = {panel.right - 252,145,panel.right - 127,165};
        else if ((old == shell.footer() || old == ScreenRect{100,555,790,590}) &&
                 screen.columns(screen.grid(i)) == 2 && screen.rows(screen.grid(i)) == 1) {
            box = footer;
            c.put8(screen.grid(i) + 0xbb2, 1); c.put8(screen.grid(i) + 0xbb3, 1);
        } else continue;
        screen.write(screen.grid(i), box);
        screen.measure(screen.grid(i));
        screen.publish(screen.grid(i));
    }
}
}
