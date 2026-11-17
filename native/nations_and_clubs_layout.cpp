#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_nations_and_clubs(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const auto history = c.u32(c.toc(-0x7bb0) + 0x10);
    const auto tab = c.u32(history + 0x14);
    if (tab < 1 || tab > 6) return;
    // Each tab stores its own selection, scroll position and list index.
    const auto body_index = c.u32(history + 0x14 + 3 * tab * 8);
    const auto grid_count = std::min<unsigned>(250, c.u16(display + 0x129da4));
    if (body_index >= grid_count) return;

    const auto body = screen.grid(body_index);
    const auto before = screen.rect(body);
    const auto original_rows = screen.rows(body);
    const auto mode = c.u32(history + 0x14 + 19 * 8);
    const int top = mode == 2 ? 145 : 190;
    const int bottom = mode == 0 ? 500 : 545;
    if (before != ScreenRect{110, top, 780, bottom} ||
        screen.columns(body) != (tab <= 2 ? 4 : 6) || !original_rows || original_rows > 30) return;

    // The original builders pick font 3 through 0x1076d470.
    const int line_height = c.i32(c.toc(-0x740c)) != -1 ? 21 : c.i32(c.toc(-0x3bdc) + 2 * 0x1404);
    if (line_height <= 0) return;
    const ScreenLayout shell{canvas};
    auto available = shell.content(top);
    const int width = std::min(1400, available.width());
    available.left += (available.width() - width) / 2;
    available.right = available.left + width;
    available.bottom = shell.footer().bottom;

    const std::array<FormTrack, 5> tracks{{
        {before.height(), 1, std::max(before.height(), 30 * line_height + 16)},
        {mode == 0 ? 10 : 0}, {mode == 0 ? 35 : 0}, {10}, {35}
    }};
    const auto boxes = form_tracks(available, tracks, false);
    const auto list = boxes[0], lower_tabs = boxes[2], footer = boxes[4];
    const int rows = std::clamp((list.height() - 16) / line_height, int(original_rows), 30);
    const int filter_width = std::clamp(width / 6, 125, 165);
    const ScreenRect buttons{footer.right - 340, footer.top, footer.right, footer.bottom};

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    screen.write(body, list);
    screen.show_rows(body, original_rows, rows);
    screen.set_rows(body, rows);
    screen.hide_scrollbar_if_fits(body, rows);
    screen.resize_rows(body);

    const auto controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = screen.control(i);
        if (c.i16(control + 0x72) >= 0) continue;
        const auto old = screen.rect(control + 0x10);
        auto box = old;
        if (old == ScreenRect{100, 10, 790, 70}) box = shell.title();
        else if (old == ScreenRect{110, top - 65, 235, top - 45}) box = {list.left, old.top, list.left + filter_width, old.bottom};
        else if (old == ScreenRect{110, top - 40, 780, top - 5}) box = {list.left, old.top, list.right, old.bottom};
        else continue;
        screen.write(control + 0x10, box);
        c.put32(control + 0x28, box.left); c.put32(control + 0x2c, box.right);
        c.put32(control + 0x30, box.top); c.put32(control + 0x34, box.bottom);
    }

    for (unsigned i = 0; i < grid_count; ++i) {
        const auto area = screen.grid(i);
        const auto old = screen.rect(area);
        auto box = old;
        if (area == body) box = list;
        else if (old == ScreenRect{100, 80, 790, 115}) box = {list.left, 80, list.right, 115};
        else if (old == ScreenRect{100, 510, 790, 545}) box = lower_tabs;
        else if (old == ScreenRect{100, 555, 790, 590} || old == shell.footer()) {
            box = buttons;
            c.put8(area + 0xbb2, 1);
            c.put8(area + 0xbb3, 1);
        } else continue;
        screen.write(area, box);
        screen.measure(area);
        screen.publish(area);
    }
}
}
