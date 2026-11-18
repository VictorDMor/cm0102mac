#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
namespace {
// Keep seasons and record categories readable while giving the remaining
// space to the original columns (including third place and the host).
void size_history_columns(const GuestScreen& screen, std::uint32_t area, int width, int leading_limit) {
    const auto first = area + 0xbb2;
    const int columns = screen.columns(area);
    int rest = 0;
    for (int col = 1; col < columns; ++col) rest += screen.c.u8(first + col);
    const int leading = screen.c.u8(first);
    if (!rest || !leading || width * leading <= leading_limit * (leading + rest)) return;
    screen.c.put8(first, (leading_limit + 4) / 8);
    for (int col = 1; col < columns; ++col) {
        const int share = (width - leading_limit) * screen.c.u8(first + col);
        screen.c.put8(first + col, std::max(1, (share + 4*rest) / (8*rest)));
    }
}
}
void layout_competition_list(State& s, std::uint32_t display, GuiCanvas canvas, unsigned section, bool historical) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const auto grid_count = std::min<unsigned>(250, c.u16(display + 0x129da4));
    int body_index = -1;
    bool message = false;
    for (unsigned i = 0; i < grid_count; ++i) {
        const auto at = screen.grid(i);
        const auto box = screen.rect(at);
        if (box.left != 110 || box.right != 780 || (box.bottom != 500 && box.bottom != 545)) continue;
        const auto columns = screen.columns(at);
        const auto type = c.u32(at + 0x18);
        if (historical) {
            if ((section == 1 && box.top == 125 && columns >= 3 && columns <= 5 && type == 2) ||
                (section != 1 && box.top == 190 && columns == 2 && type == 1))
                body_index = int(i);
        } else if (box.top == 125 && columns == 1 && type == 2) {
            body_index = int(i);
            message = true;
        } else if ((section == 4 && box.top == 125 && columns == 2 && type == 1) ||
                   (section != 4 && box.top == 190 && columns == 9 && type == 2)) {
            body_index = int(i);
            message = false;
        }
    }
    // The gate enlarges the canvas before the builder runs, so a variant
    // whose list does not match its original grid shape must still get its
    // title, tab bars and footer moved. Its own grids then stay where the
    // builder put them, at the original width.
    const bool records = historical && section != 1;
    const int font = records ? 2 : 3;
    const int inset = records ? 0 : 16;
    const int line_height = c.i32(c.toc(-0x740c)) != -1 ? (records ? 18 : 21)
        : c.i32(c.toc(-0x3bdc) + (font-1) * 0x1404);
    if (body_index >= 0) {
        const auto rows = screen.rows(screen.grid(body_index));
        if (!rows || rows > 30 || line_height <= 0) body_index = -1;
    }
    const bool recognised = body_index >= 0;
    const auto body = recognised ? screen.grid(body_index) : 0;
    const ScreenLayout window{canvas};
    const auto footer = window.footer();
    const auto original = recognised ? screen.rect(body) : ScreenRect{110, 125, 780, 545};
    const auto original_rows = recognised ? screen.rows(body) : std::uint8_t(0);
    auto list = original;
    int rows = original_rows;
    if (recognised) {
        auto available = window.content(original.top);
        if (original.bottom == 500) available.bottom -= 45;
        const int width = std::min(1600, available.width());
        available.left += (available.width() - width) / 2;
        available.right = available.left + width;
        const std::array<FormTrack, 1> tracks{{
            {original.height(), 1, std::max(original.height(), 30 * line_height + inset)}
        }};
        list = form_tracks(available, tracks, false).front();
        if (!message) rows = std::clamp((list.height() - inset) / line_height, int(original_rows), 30);
    }
    const int heading_shift = list.left - original.left;

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    // Record text and its invisible links have absolute, measured X bounds.
    // Move them together by the displacement of their value column.
    int old_value_start = 0;
    if (recognised && records) {
        screen.measure(body);
        old_value_start = original.left + c.i32(body + 0x20);
    }
    if (recognised) screen.write(body, list);
    if (recognised && !message) {
        screen.show_rows(body, original_rows, rows);
        screen.set_rows(body, rows);
        screen.hide_scrollbar_if_fits(body, rows);
        screen.resize_rows(body);
        if (historical) {
            const int gutter = c.u8(body + 0xbef) ? 22 : 0;
            size_history_columns(screen, body, list.width() - gutter, section == 2 ? 340 : 90);
            screen.measure(body);
        }
    }
    const auto controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = screen.control(i);
        const auto owner = c.i16(control + 0x72);
        if (recognised && owner == body_index) {
            const int left = c.i32(control + 0x28), right = c.i32(control + 0x2c);
            // Empty-state text is centered. Group labels and their underlines
            // use the original measured width at a fixed five-pixel inset.
            if (message && right > left) {
                const int shift = (list.left + list.right - original.left - original.right) / 2;
                c.put32(control + 0x28, left + shift); c.put32(control + 0x2c, right + shift);
            } else if (records && right > left) {
                const int shift = list.left + c.i32(body + 0x20) - old_value_start;
                c.put32(control + 0x28, left + shift);
                c.put32(control + 0x2c, right + shift);
            } else if (!historical && left == 115 && right > left) {
                c.put32(control + 0x28, left + heading_shift);
                c.put32(control + 0x2c, right + heading_shift);
            }
            continue;
        }
        if (owner >= 0) continue;
        const auto old = screen.rect(control + 0x10);
        auto box = old;
        if (old == ScreenRect{100, 10, 790, 70}) box = window.title();
        else if (old == ScreenRect{110, 150, 780, 185}) box = {list.left, 150, list.right, 185};
        else if (historical && old == ScreenRect{110,125,235,145}) box = {list.left,125,list.left+125,145};
        else if (old == ScreenRect{710, 15, 785, 35}) box = window.anchor(old, LayoutAxis::trailing, LayoutAxis::fixed);
        else continue;
        screen.write(control + 0x10, box);
        c.put32(control + 0x28, box.left); c.put32(control + 0x2c, box.right);
        c.put32(control + 0x30, box.top); c.put32(control + 0x34, box.bottom);
    }
    for (unsigned i = 0; i < grid_count; ++i) {
        const auto at = screen.grid(i);
        const auto old = screen.rect(at);
        auto box = old;
        if (recognised && at == body) box = list;
        else if (old == ScreenRect{100, 80, 790, 115}) box.right = canvas.width - 10;
        else if (old == ScreenRect{100, 510, 790, 545}) box = {100, canvas.height - 90, canvas.width - 10, canvas.height - 55};
        else if (old == ScreenRect{110, 125, 360, 145}) box = {list.left, 125, list.left + 250, 145};
        else if ((old == ScreenRect{100, 555, 790, 590} || old == footer) &&
                 screen.columns(at) == 2 && screen.rows(at) == 1) {
            box = {list.right - 340, footer.top, list.right, footer.bottom};
            c.put8(at + 0xbb2, 1); c.put8(at + 0xbb3, 1);
        } else continue;
        screen.write(at, box);
        screen.measure(at);
        screen.publish(at);
    }
}
}
