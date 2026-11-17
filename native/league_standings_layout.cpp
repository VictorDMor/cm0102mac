#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_league_standings(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    int body = -1;
    bool message = false;
    for (unsigned i = 0; i < grids; ++i) {
        const auto box = screen.rect(screen.grid(i));
        if (box.left == 110 && box.right == 780 && box.top == 190 &&
            (box.bottom == 500 || box.bottom == 545) &&
            c.u32(screen.grid(i) + 0x18) == 2 && screen.columns(screen.grid(i)) == 11) body = int(i);
        else if (box.left == 110 && box.right == 780 && (box.top == 125 || box.top == 165) &&
                 (box.bottom == 500 || box.bottom == 545) && c.u32(screen.grid(i) + 0x18) == 2 &&
                 screen.columns(screen.grid(i)) == 1) { body = int(i); message = true; }
    }
    if (body < 0) return;
    const auto before = screen.rect(screen.grid(body));
    const auto original_rows = screen.rows(screen.grid(body));
    if (!original_rows || original_rows > 30) return;
    // Read the same font-3 line metric as 0x1076d470, without changing the
    // selected font or entering the game's stage/sorting routines again.
    const int line_height = c.i32(c.toc(-0x740c)) != -1 ? 21 : c.i32(c.toc(-0x3bdc) + 2 * 0x1404);
    if (line_height <= 0) return;
    const ScreenLayout shell{canvas};
    auto available = shell.content(before.top);
    if (before.bottom == 500) available.bottom -= 45;
    const int width = std::min(1600, available.width());
    available.left += (available.width() - width) / 2;
    available.right = available.left + width;
    const std::array<FormTrack, 1> tracks{{{before.height(), 1, 30 * line_height + 16}}};
    const auto panel = form_tracks(available, tracks, false).front();
    const int rows = message ? original_rows : std::clamp((panel.height() - 16) / line_height, int(original_rows), 30);
    const bool overflow = c.i32(screen.grid(body) + 0x14) + 1 > rows;
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = screen.control(i);
        if (c.i16(control + 0x72) == body) {
            if (message && (c.i32(control + 0x28) || c.i32(control + 0x2c))) {
                const int shift = (panel.left + panel.right - before.left - before.right) / 2;
                c.put32(control + 0x28, c.i32(control + 0x28) + shift);
                c.put32(control + 0x2c, c.i32(control + 0x2c) + shift);
            }
            // Promotion/relegation separators span the row through explicit
            // X overrides. AutoGrid alone would leave them at the old width.
            else if (c.i32(control + 0x28) == 112 &&
                (c.i32(control + 0x2c) == 778 || c.i32(control + 0x2c) == 756)) {
                c.put32(control + 0x28, panel.left + 2);
                c.put32(control + 0x2c, panel.right - 2 - (overflow ? 22 : 0));
            }
            continue;
        }
        if (c.i16(control + 0x72) >= 0) continue;
        const auto old = screen.rect(control + 0x10);
        auto box = old;
        if (old == ScreenRect{100, 10, 790, 70}) box = shell.title();
        else if (old == ScreenRect{110, 150, 780, 185}) box = {panel.left, 150, panel.right, 185};
        else if (message && old == ScreenRect{110, 125, 780, 160}) box = {panel.left, 125, panel.right, 160};
        else if (old == ScreenRect{110, 125, 235, 145}) box = {panel.left, 125, panel.left + 125, 145};
        else if (old == ScreenRect{655, 125, 780, 145}) box = {panel.right - 125, 125, panel.right, 145};
        else if (old == ScreenRect{528, 125, 653, 145}) box = {panel.right - 252, 125, panel.right - 127, 145};
        else if (old == ScreenRect{710, 15, 785, 35}) box = shell.anchor(old, LayoutAxis::trailing, LayoutAxis::fixed);
        else continue;
        screen.write(control + 0x10, box);
        c.put32(control + 0x28, box.left); c.put32(control + 0x2c, box.right);
        c.put32(control + 0x30, box.top); c.put32(control + 0x34, box.bottom);
    }
    for (unsigned i = 0; i < grids; ++i) {
        auto box = screen.rect(screen.grid(i));
        if (int(i) == body) {
            box = panel;
            screen.show_rows(screen.grid(i), original_rows, rows);
            screen.set_rows(screen.grid(i), rows);
            if (!message) {
                screen.resize_rows(screen.grid(i));
            }
        } else if (box == ScreenRect{100, 80, 790, 115}) box.right = canvas.width - 10;
        else if (box == ScreenRect{100, 510, 790, 545}) box = {100, canvas.height - 90, canvas.width - 10, canvas.height - 55};
        else if (box == ScreenRect{100, 555, 790, 590}) box = shell.footer();
        else continue;
        screen.write(screen.grid(i), box);
        screen.measure(screen.grid(i));
        screen.publish(screen.grid(i));
    }
}
}
