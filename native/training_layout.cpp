#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_training(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const auto history = c.u32(c.toc(-0x7bb0) + 0x10);
    const auto body_index = c.u32(history + 0x14 + 5 * 8);
    const auto count = std::min<unsigned>(250, c.u16(display + 0x129da4));
    if (body_index >= count) return;
    const auto body = screen.grid(body_index);
    const auto original = screen.rect(body);
    // The builder puts a description above some schedules. Locate the frame
    // it actually emitted to preserve the heading and the active font height.
    ScreenRect frame{};
    for (unsigned i = 0; i < count; ++i) {
        const auto area = screen.grid(i);
        const auto box = screen.rect(area);
        if (box.left == 110 && box.right == 780 && box.bottom == 500 && c.u32(area + 0x18) == 2)
            frame = box;
    }
    if (!frame.width()) return;
    const bool populated = original.bottom == frame.bottom - 8;
    if (original.left != frame.left || original.right != frame.right ||
        (!populated && original != ScreenRect{frame.left,frame.top,frame.right,frame.top + 20})) return;
    // Empty schedules store the index of the strip above the backdrop.
    const int line_height = c.i32(c.toc(-0x740c)) != -1 ? 18 : c.i32(c.toc(-0x3bdc) + 0x1404);
    if (line_height < 1) return;
    const int list_top = populated ? original.top : frame.top + 9 + line_height;
    const ScreenLayout shell{canvas};
    auto available = shell.content(list_top);
    const int width = std::min(1440, available.width());
    available.left += (available.width() - width) / 2;
    available.right = available.left + width;
    available.bottom = shell.footer().bottom;
    const int old_rows = screen.rows(body);
    if (populated && (old_rows < 1 || old_rows > 30)) return;
    const std::array<FormTrack, 5> tracks{{
        {frame.bottom - 8 - list_top, 1, 30 * line_height + 8}, {18}, {35}, {10}, {35}
    }};
    const auto boxes = form_tracks(available, tracks, false);
    const auto list = boxes[0], lower_tabs = boxes[2], footer = boxes[4];
    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    if (populated) {
        const int rows = std::clamp((list.height() - 8) / line_height, old_rows, 30);
        screen.write(body, list);
        screen.show_rows(body, old_rows, rows);
        screen.set_rows(body, rows);
        screen.hide_scrollbar_if_fits(body, rows);
        screen.resize_rows(body);
    }

    for (unsigned i = 0; i < count; ++i) {
        const auto area = screen.grid(i);
        const auto old = screen.rect(area);
        auto box = old;
        if (area == body && populated) box = list;
        else if (old == ScreenRect{100,80,790,115}) box = {list.left,80,list.right,115};
        else if (old == ScreenRect{100,510,790,545}) box = lower_tabs;
        else if (old == ScreenRect{110,125,485,145}) box = {list.left,125,list.left + 375,145};
        else if (old == ScreenRect{530,125,780,145}) box = {list.right - 250,125,list.right,145};
        else if (old == ScreenRect{110,frame.top,780,frame.top + 20}) box = {list.left,old.top,list.right,old.bottom};
        else if (old == frame) box = {list.left,frame.top,list.right,list.bottom + 8};
        else if (populated && i + 1 == body_index && old.left == 110 && old.top == frame.top + 8 && old.bottom == list.top - 1)
            box = {list.left,old.top,list.right - (c.i32(body + 0x14) + 1 > screen.rows(body) ? 22 : 0),old.bottom};
        else if (old == ScreenRect{100,555,790,590} || old == shell.footer()) {
            box = {footer.right - 340,footer.top,footer.right,footer.bottom};
            c.put8(area + 0xbb2, 1);
            c.put8(area + 0xbb3, 1);
        } else continue;
        screen.write(area, box);
        screen.measure(area);
        screen.publish(area);
    }

    const auto controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = screen.control(i);
        if (c.i16(control + 0x72) >= 0) continue;
        const auto old = screen.rect(control + 0x10);
        ScreenRect box;
        if (old == ScreenRect{100,10,790,70}) box = shell.title();
        else if (old == ScreenRect{655,125,780,145}) box = {list.right - 125,125,list.right,145};
        else if (old == ScreenRect{110,150,780,185} && frame.top == 190) box = {list.left,150,list.right,185};
        else continue;
        screen.write(control + 0x10, box);
        c.put32(control + 0x28, box.left); c.put32(control + 0x2c, box.right);
        c.put32(control + 0x30, box.top); c.put32(control + 0x34, box.bottom);
    }
}
}
