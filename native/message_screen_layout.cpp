#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "paged_screen_layout.hpp"
#include "gui_guest_scope.hpp"
#include <algorithm>
#include <array>
#include <vector>

namespace cm::aot {
// Message screens (the offer decision from the news, transfer completion, transfer-list confirmation, an
// offer's restriction and feedback messages, the staff release, the manager's job confirmations: apply for
// a job, rescind the contract, retire, and the questions and notices opened from the news or a person: scouting,
// choosing two players, the Intertoto place, a job offer, a transfer request, an invalid national squad): the title and subtitle, one wrapped or centered message in a
// one-column grid at 110,140-780,545 (110,145-780,535 for the manager's confirmations), and a row of
// buttons that is either a grid of one to six columns or one standalone control at 100,555-790,590 (110,555-780,590 for the national team notices). Their builders place the message by
// row (a short message in the middle row of nineteen) and size the buttons by a column specification, so
// the original shape is a tall panel with a few words in it and unequal buttons.
//
// The trial period selection (one row of a label, a minus button, the weeks and a plus button) has the same
// panel as a grid of seven columns and no explicit text bounds: its cells follow the new width with only the
// label column growing, like a table's.
//
// Here the screen becomes a dialog: a centered panel only as tall as its message needs plus a row of
// padding on each side, with equal buttons under it, right-aligned to its edge, the whole block in the
// upper middle of the content area. Only positions change: the grid's rectangle and row count, the rows
// of its controls (kept in order and spacing), their text bounds and the button cells.
void layout_message_screen(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    // The classic button strip, and the narrower one of the national team notices (its one Ok button is panel-wide).
    constexpr ScreenRect footer_box{100,555,790,590}, notice_box{110,555,780,590};
    constexpr std::array<ScreenRect,2> message_boxes{{{110,140,780,545}, {110,145,780,535}}};
    ScreenRect message_box = message_boxes[0];
    int message = -1, footer = -1;
    bool ambiguous = false;
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        const auto box = screen.rect(grid);
        if (std::find(message_boxes.begin(), message_boxes.end(), box) != message_boxes.end() && (screen.columns(grid) == 1 || screen.columns(grid) == 7)) {
            ambiguous |= message >= 0; message = int(i); message_box = box;
        }
        else if (box == footer_box && screen.rows(grid) == 1 && screen.columns(grid) >= 1 && screen.columns(grid) <= 6) {
            ambiguous |= footer >= 0; footer = int(i);
        }
    }
    if (ambiguous || message < 0) return;
    const auto message_grid = screen.grid(message);
    const unsigned rows = screen.rows(message_grid);
    // A scrollbar narrows the message column and the rows are not ours to compact.
    if (!rows || rows > 30 || c.u8(message_grid + 0xbef)) return;
    unsigned first = rows, last = 0;
    std::vector<unsigned> standalone;
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        const int owner = c.i16(at + 0x72);
        if (owner == message) {
            const unsigned row = c.u32(at + 0x24);
            if (row >= rows) return;
            first = std::min(first, row); last = std::max(last, row);
        } else if (owner < 0 && (screen.rect(at + 0x10) == footer_box || screen.rect(at + 0x10) == notice_box)) standalone.push_back(i);
    }
    // A message screen without a message, or with both kinds of footer, is not one this recipe knows.
    if (first > last || standalone.size() > 1 || (footer >= 0 && !standalone.empty())) return;
    const unsigned buttons = footer >= 0 ? screen.columns(screen.grid(footer)) : unsigned(standalone.size());

    const ScreenLayout shell{canvas};
    auto available = shell.content(message_box.top);
    const int width = std::min(800, available.width());
    available.left += (available.width() - width) / 2; available.right = available.left + width;
    const unsigned used = last - first + 1, count = std::max(used + 2, 5u);
    constexpr int row_height = 28, gap = 10, button_height = 35;
    const int panel_height = int(count) * row_height + 16;
    const int block = panel_height + (buttons ? gap + button_height : 0);
    // The original row height is about 21 pixels; a message that cannot keep that stays as built.
    if (count > 30 || int(count) * 21 + 16 + (buttons ? gap + button_height : 0) > available.height()) return;
    const int top = available.top + std::max(0, (available.height() - block) / 3);
    const ScreenRect panel{available.left, top, available.right, top + panel_height};
    ScreenRect buttons_box{};
    if (buttons) {
        const int total = std::min(panel.width(), int(buttons) * 190);
        buttons_box = {panel.right - total, panel.bottom + gap, panel.right, panel.bottom + gap + button_height};
    }
    const int shift = (panel.left + panel.right - message_box.left - message_box.right) / 2;
    const int row_delta = int((count - used) / 2) - int(first);
    std::vector<FormTrack> equal(buttons, FormTrack{60, 1});
    const auto cells = buttons ? form_tracks(buttons_box, equal, true) : std::vector<ScreenRect>{};

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    // The columns of a seven-column panel, as AutoGrid measured them for the builder's rectangle.
    paged_screen::ColumnMap map;
    const bool columned = screen.columns(message_grid) != 1;
    if (columned) {
        screen.measure(message_grid);
        if (!map.read(c, screen, message_grid, message_box.width(), panel)) return;
    }
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        const int owner = c.i16(at + 0x72), left = c.i32(at + 0x28), right = c.i32(at + 0x2c);
        if (owner == message) {
            c.put32(at + 0x24, unsigned(int(c.u32(at + 0x24)) + row_delta));
            // Text and links carry explicit bounds measured at the original wrapping; they move together.
            if (left && right) { c.put32(at + 0x28, left + shift); c.put32(at + 0x2c, right + shift); }
            else if (columned && c.u32(at + 0x20) < map.count()) {
                const auto [first_x, last_x] = map.cell(c.u32(at + 0x20));
                c.put32(at + 0x28, first_x); c.put32(at + 0x2c, last_x);
            }
        } else if (footer >= 0 && owner == footer) {
            const unsigned column = c.u32(at + 0x20);
            if (column < cells.size()) {
                c.put32(at + 0x28, cells[column].left + (column ? 1 : 0));
                c.put32(at + 0x2c, cells[column].right - (column + 1 < cells.size() ? 1 : 0));
            }
        } else if (owner < 0 && !standalone.empty() && i == standalone.front()) {
            screen.write(at + 0x10, buttons_box);
            c.put32(at + 0x28, buttons_box.left); c.put32(at + 0x2c, buttons_box.right);
            c.put32(at + 0x30, buttons_box.top); c.put32(at + 0x34, buttons_box.bottom);
        }
    }
    // Rows beyond the compacted table do not exist any more.
    // The grid also keeps its first and last used rows and a scroll position that resizing resets.
    const auto scroll = c.u32(message_grid + 0x1fc);
    screen.set_rows(message_grid, count);
    c.put32(message_grid + 0x10, first + unsigned(row_delta));
    c.put32(message_grid + 0x14, last + unsigned(row_delta));
    screen.resize_rows(message_grid);
    c.put32(message_grid + 0x1fc, scroll);
    screen.write(message_grid, panel);
    screen.measure(message_grid);
    screen.publish(message_grid);
    if (footer >= 0) {
        const auto grid = screen.grid(footer);
        for (unsigned i = 0; i < buttons; ++i) c.put8(grid + 0xbb2 + i, 1);
        screen.write(grid, buttons_box);
        screen.measure(grid);
        screen.publish(grid);
    }
}
}
