#include "paged_screen_layout.hpp"

namespace cm::aot {
// The two text entry dialogs of the person screens: the player's nickname (builder 0x103ac1c0, opener 0x103ac0d0:
// Cancel, Remove, OK) and Send Message to a person or the manager (builder 0x102d0500, opener 0x102d02b0: Cancel,
// Send). Each is a title, a subtitle, one edit control at 150,310-740,370 and a footer grid of two or three buttons
// at 100,555-790,590. The original builders keep every text, the editing control and each command; this pass makes
// the same kind of dialog as the message screens: the edit control centered under the title bars and at most 800
// wide, in the upper middle of the content area, with equal 190px buttons right-aligned under it. It declines
// (classic geometry) any other arrangement.
void layout_text_entry(State& s, std::uint32_t display, GuiCanvas canvas) {
    using namespace paged_screen;
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    constexpr ScreenRect input_box{150, 310, 740, 370}, footer_classic{100, 555, 790, 590};
    const ScreenLayout shell{canvas};
    int footer = -1, input = -1;
    bool ambiguous = false;
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        const auto box = screen.rect(grid);
        if ((box == footer_classic || box == shell.footer()) && screen.rows(grid) == 1 && screen.columns(grid) >= 2 && screen.columns(grid) <= 3) {
            ambiguous |= footer >= 0; footer = int(i);
        }
    }
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) < 0 && screen.rect(at + 0x10) == input_box) { ambiguous |= input >= 0; input = int(i); }
    }
    if (ambiguous || footer < 0 || input < 0) return;
    const auto footer_grid = screen.grid(footer);
    const unsigned buttons = screen.columns(footer_grid);

    constexpr int gap = 10, button_height = 35;
    auto available = shell.content(145);
    const int width = std::min(800, available.width());
    available.left += (available.width() - width) / 2; available.right = available.left + width;
    const int block = input_box.height() + gap + button_height;
    if (block > available.height()) return;
    const int top = available.top + std::max(0, (available.height() - block) / 3);
    const ScreenRect box{available.left, top, available.right, top + input_box.height()};
    const int total = std::min(box.width(), int(buttons) * 190);
    const ScreenRect buttons_box{box.right - total, box.bottom + gap, box.right, box.bottom + gap + button_height};

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    const auto at = screen.control(unsigned(input));
    screen.write(at + 0x10, box);
    c.put32(at + 0x28, box.left); c.put32(at + 0x2c, box.right);
    c.put32(at + 0x30, box.top); c.put32(at + 0x34, box.bottom);
    place_footer(screen, c, footer_grid, buttons_box);
}
}
