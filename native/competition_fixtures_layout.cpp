#include "form_layout.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_competition_fixtures(State& s, std::uint32_t display, GuiCanvas canvas, unsigned section) {
    guest::Cpu c(s);
    const auto grid = [display](unsigned index) { return display + 4 + index * 0xbf0; };
    const auto rect = [&](std::uint32_t at) {
        return ScreenRect{c.i32(at), c.i32(at + 4), c.i32(at + 8), c.i32(at + 12)};
    };
    const auto write = [&](std::uint32_t at, ScreenRect box) {
        c.put32(at, box.left); c.put32(at + 4, box.top);
        c.put32(at + 8, box.right); c.put32(at + 12, box.bottom);
    };
    const auto grid_count = std::min<unsigned>(250, c.u16(display + 0x129da4));
    int body_index = -1;
    bool message = false;
    for (unsigned i = 0; i < grid_count; ++i) {
        const auto at = grid(i);
        const auto box = rect(at);
        if (box.left != 110 || box.right != 780 || (box.bottom != 500 && box.bottom != 545)) continue;
        const auto columns = c.u8(at + 0xbb0);
        const auto type = c.u32(at + 0x18);
        if (box.top == 125 && columns == 1 && type == 2) {
            body_index = int(i);
            message = true;
        } else if ((section == 4 && box.top == 125 && columns == 2 && type == 1) ||
                   (section != 4 && box.top == 190 && columns == 9 && type == 2)) {
            body_index = int(i);
            message = false;
        }
    }
    // The gate enlarges the canvas before the builder runs, so a variant
    // whose list is not one of the three shapes above must still get its
    // title, tab bars and footer moved. Its own grids then stay where the
    // builder put them, at the original width.
    const int line_height = c.i32(c.toc(-0x740c)) != -1 ? 21 : c.i32(c.toc(-0x3bdc) + 2 * 0x1404);
    if (body_index >= 0) {
        const auto rows = c.u8(grid(body_index) + 0xbb1);
        // The three builders use font 3 for row height.
        if (!rows || rows > 30 || line_height <= 0) body_index = -1;
    }
    const bool recognised = body_index >= 0;
    const auto body = recognised ? grid(body_index) : 0;
    const ScreenLayout window{canvas};
    const auto footer = window.footer();
    const auto original = recognised ? rect(body) : ScreenRect{110, 125, 780, 545};
    const auto original_rows = recognised ? c.u8(body + 0xbb1) : std::uint8_t(0);
    auto list = original;
    int rows = original_rows;
    if (recognised) {
        auto available = window.content(original.top);
        if (original.bottom == 500) available.bottom -= 45;
        const int width = std::min(1600, available.width());
        available.left += (available.width() - width) / 2;
        available.right = available.left + width;
        const std::array<FormTrack, 1> tracks{{
            {original.height(), 1, std::max(original.height(), 30 * line_height + 16)}
        }};
        list = form_tracks(available, tracks, false).front();
        if (!message) rows = std::clamp((list.height() - 16) / line_height, int(original_rows), 30);
    }
    const int heading_shift = list.left - original.left;

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    if (recognised) write(body, list);
    if (recognised && !message) {
        for (int row = original_rows; row < rows; ++row) c.put8(body + 0xbd0 + row, 1);
        c.put8(body + 0xbb1, rows);
        if (c.i32(body + 0x14) + 1 <= rows) c.put8(body + 0xbef, 0);
        c.set({{3, body}, {4, c.u32(body + 0x1fc)}}); c.call(0x107145f0, 0xfffffff0);
    }
    const auto controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = display + 0xba864 + i * 0x17c;
        const auto owner = c.i16(control + 0x72);
        if (recognised && owner == body_index) {
            const int left = c.i32(control + 0x28), right = c.i32(control + 0x2c);
            // Empty-state text is centered. Group labels and their underlines
            // use the original measured width at a fixed five-pixel inset.
            if (message && right > left) {
                const int shift = (list.left + list.right - original.left - original.right) / 2;
                c.put32(control + 0x28, left + shift); c.put32(control + 0x2c, right + shift);
            } else if (left == 115 && right > left) {
                c.put32(control + 0x28, left + heading_shift);
                c.put32(control + 0x2c, right + heading_shift);
            }
            continue;
        }
        if (owner >= 0) continue;
        const auto old = rect(control + 0x10);
        auto box = old;
        if (old == ScreenRect{100, 10, 790, 70}) box = window.title();
        else if (old == ScreenRect{110, 150, 780, 185}) box = {list.left, 150, list.right, 185};
        else if (old == ScreenRect{710, 15, 785, 35}) box = window.anchor(old, LayoutAxis::trailing, LayoutAxis::fixed);
        else continue;
        write(control + 0x10, box);
        c.put32(control + 0x28, box.left); c.put32(control + 0x2c, box.right);
        c.put32(control + 0x30, box.top); c.put32(control + 0x34, box.bottom);
    }
    for (unsigned i = 0; i < grid_count; ++i) {
        const auto at = grid(i);
        const auto old = rect(at);
        auto box = old;
        if (recognised && at == body) box = list;
        else if (old == ScreenRect{100, 80, 790, 115}) box.right = canvas.width - 10;
        else if (old == ScreenRect{100, 510, 790, 545}) box = {100, canvas.height - 90, canvas.width - 10, canvas.height - 55};
        else if (old == ScreenRect{110, 125, 360, 145}) box = {list.left, 125, list.left + 250, 145};
        else if ((old == ScreenRect{100, 555, 790, 590} || old == footer) &&
                 c.u8(at + 0xbb0) == 2 && c.u8(at + 0xbb1) == 1) {
            box = {list.right - 340, footer.top, list.right, footer.bottom};
            c.put8(at + 0xbb2, 1); c.put8(at + 0xbb3, 1);
        } else continue;
        write(at, box);
        c.set(3, at); c.call(0x10714630, 0xfffffff0);
        c.set({{3, at}, {4, 0}}); c.call(0x10714110, 0xfffffff0);
    }
}
}
