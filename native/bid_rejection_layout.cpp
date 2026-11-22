#include "paged_screen_layout.hpp"

namespace cm::aot {
// Rejecting an offer (builder 0x1041f8f0, opened from the offer decision's Reject and from the proposal
// screen when its offer is refused): a panel at 110,140-780,545 with a two-column grid, two lines of text over
// the full width (the question and the chosen reason), the reasons themselves as choices in the wide column
// (a radio-style label or a button each), and a footer of two buttons (No/Yes) or three when several offers
// are pending (No/All/Yes). The original builder keeps every text, choice and command; this pass makes the
// same kind of dialog as the other transfer screens: the panel centered under the title bars and at most 800
// wide, trailing empty rows dropped so it is only as tall as its choices, the choices' cells following the new
// width (the wide column takes it), full lines following both edges, and the buttons as equal 190px cells
// right-aligned under the panel.
void layout_bid_rejection(State& s, std::uint32_t display, GuiCanvas canvas) {
    using namespace paged_screen;
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    constexpr ScreenRect panel_box{110, 140, 780, 545}, footer_classic{100, 555, 790, 590};
    int panel = -1, footer = -1;
    bool ambiguous = false;
    const auto claim = [&](int& slot, unsigned i) { ambiguous |= slot >= 0; slot = int(i); };
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        const auto box = screen.rect(grid);
        if (box == panel_box && screen.columns(grid) == 2) claim(panel, i);
        else if (box == footer_classic && screen.rows(grid) == 1 && screen.columns(grid) >= 2 && screen.columns(grid) <= 3) claim(footer, i);
    }
    if (ambiguous || panel < 0 || footer < 0) return;
    const auto panel_grid = screen.grid(panel), footer_grid = screen.grid(footer);
    const unsigned rows = screen.rows(panel_grid), buttons = screen.columns(footer_grid);
    // A scrollbar narrows the columns and is not ours to move; keep that state as built.
    if (!rows || rows > 30 || c.u8(panel_grid + 0xbef)) return;
    unsigned last = 0;
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) != panel) continue;
        const unsigned row = c.u32(at + 0x24);
        if (row >= rows) return;
        last = std::max(last, row);
    }
    // The rows after the last choice stay out of the panel, with one empty row of padding.
    const unsigned count = std::min(rows, last + 2);

    const ScreenLayout shell{canvas};
    constexpr int row_height = 28, gap = 10, button_height = 35;
    auto available = shell.content(panel_box.top);
    const int width = std::min(800, available.width());
    available.left += (available.width() - width) / 2; available.right = available.left + width;
    const int panel_height = int(count) * row_height + 16;
    // The original row height is about 21 pixels; a panel that cannot keep that stays as built.
    if (int(count) * 21 + 16 + gap + button_height > available.height()) return;
    const ScreenRect body{available.left, available.top, available.right, available.top + panel_height};
    const int total = std::min(body.width(), int(buttons) * 190);
    const ScreenRect buttons_box{body.right - total, body.bottom + gap, body.right, body.bottom + gap + button_height};
    const int lead = body.left - panel_box.left, trail = body.right - panel_box.right;

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    // Read the columns the builder's rectangle produced, not the ones a previous pass left behind.
    screen.measure(panel_grid);
    ColumnMap map;
    if (!map.read(c, screen, panel_grid, panel_box.width(), body)) return;
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) != panel) continue;
        const int left = c.i32(at + 0x28), right = c.i32(at + 0x2c);
        if (left && right) {
            // A full line follows the panel's right edge; a link measured to its words keeps its width.
            c.put32(at + 0x28, left + lead);
            c.put32(at + 0x2c, right >= panel_box.right - 10 ? right + trail : right + lead);
        } else if (c.u32(at + 0x20) < map.count()) {
            const auto [first, last_x] = map.cell(c.u32(at + 0x20));
            c.put32(at + 0x28, first); c.put32(at + 0x2c, last_x);
        }
    }
    // Rows beyond the compacted table do not exist any more; the used-row range and scroll position stay.
    const auto first_used = c.u32(panel_grid + 0x10), last_used = c.u32(panel_grid + 0x14), scroll = c.u32(panel_grid + 0x1fc);
    screen.set_rows(panel_grid, count);
    screen.resize_rows(panel_grid);
    c.put32(panel_grid + 0x10, first_used); c.put32(panel_grid + 0x14, last_used); c.put32(panel_grid + 0x1fc, scroll);
    screen.write(panel_grid, body);
    screen.measure(panel_grid);
    screen.publish(panel_grid);
    place_footer(screen, c, footer_grid, buttons_box);
}
}
