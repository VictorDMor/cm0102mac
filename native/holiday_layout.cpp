#include "paged_screen_layout.hpp"

namespace cm::aot {
// Holiday (builder 0x101c9eb0, opener 0x101c9e70, the Manager menu's "Go on holiday"): a panel at
// 110,145-780,535 with an intro paragraph above a one-column list of thirteen rows at 130,250-760,535 (seven
// settings, each a text and a link that carries its current value, in two groups), and a Yes/No footer. The
// original builder keeps every text, link and command; this pass turns it into the same kind of dialog as the
// message screens: the panel centered under the title bars and at most 800 wide, the settings' text and their
// links carried with its left edge (a full line follows its right edge), and the two buttons as equal 190px
// cells right-aligned under the panel. The rows keep their height, so the panel keeps its classic height.
void layout_holiday(State& s, std::uint32_t display, GuiCanvas canvas) {
    using namespace paged_screen;
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    constexpr ScreenRect panel_box{110, 145, 780, 535}, list_box{130, 250, 760, 535}, intro_box{110, 165, 780, 200},
                         footer_box_classic{100, 555, 790, 590};
    int panel = -1, list = -1, footer = -1;
    bool ambiguous = false;
    const auto claim = [&](int& slot, unsigned i) { ambiguous |= slot >= 0; slot = int(i); };
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        const auto box = screen.rect(grid);
        if (box == panel_box && screen.columns(grid) == 1 && screen.rows(grid) == 1) claim(panel, i);
        else if (box == list_box && screen.columns(grid) == 1) claim(list, i);
        else if (box == footer_box_classic && screen.columns(grid) == 2 && screen.rows(grid) == 1) claim(footer, i);
    }
    if (ambiguous || panel < 0 || list < 0 || footer < 0) return;
    const auto list_grid = screen.grid(list);
    const unsigned rows = screen.rows(list_grid);
    // A scrollbar narrows the column and is not ours to move; keep that state as built.
    if (!rows || rows > 30 || c.u8(list_grid + 0xbef)) return;

    const ScreenLayout shell{canvas};
    constexpr int gap = 10, button_height = 35;
    auto available = shell.content(panel_box.top);
    const int width = std::min(800, available.width());
    available.left += (available.width() - width) / 2; available.right = available.left + width;
    const ScreenRect body{available.left, panel_box.top, available.right, panel_box.bottom};
    // The dialog and its buttons must fit under the title bars; otherwise it stays as built.
    if (body.bottom + gap + button_height > canvas.height - 10) return;
    const ScreenRect list_after{body.left + (list_box.left - panel_box.left), list_box.top,
                                body.right - (panel_box.right - list_box.right), list_box.bottom};
    const int lead = body.left - panel_box.left, trail = body.right - panel_box.right;
    const int total = std::min(body.width(), 2 * 190);
    const ScreenRect buttons{body.right - total, body.bottom + gap, body.right, body.bottom + gap + button_height};

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        const int owner = c.i16(at + 0x72), left = c.i32(at + 0x28), right = c.i32(at + 0x2c);
        if (owner == list) {
            // A full line follows the list's right edge; a link measured to its words keeps its width.
            if (left && right) {
                c.put32(at + 0x28, left + lead);
                c.put32(at + 0x2c, right >= list_box.right - 10 ? right + trail : right + lead);
            }
        } else if (owner < 0 && screen.rect(at + 0x10) == intro_box && left == intro_box.left && right == intro_box.right) {
            // The intro paragraph spans the panel.
            screen.write(at + 0x10, ScreenRect{body.left, intro_box.top, body.right, intro_box.bottom});
            c.put32(at + 0x28, body.left); c.put32(at + 0x2c, body.right);
        }
    }
    screen.write(screen.grid(panel), body);
    screen.measure(screen.grid(panel));
    screen.publish(screen.grid(panel));
    screen.write(list_grid, list_after);
    screen.measure(list_grid);
    screen.publish(list_grid);
    place_footer(screen, c, screen.grid(footer), buttons);
}
}
