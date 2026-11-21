#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <algorithm>
#include <array>

namespace cm::aot {
// Previous offers (builder 0x10422600): a title control (which already follows the shell), a one-row subtitle grid with a centered
// text and its link, a one-column message grid whose centered lines the builder places in the middle
// row, and one standalone footer button. The original builder keeps every text, link and action;
// this pass moves the grids and the footer button, and carries each centered text and its
// measured links to the new center together.
void layout_previous_offers(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    constexpr ScreenRect subtitle_box{100,80,790,125}, message_box{110,140,780,545}, footer_box{100,555,790,590};
    int subtitle = -1, message = -1;
    bool ambiguous = false;
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        const auto box = screen.rect(grid);
        if (box == subtitle_box && screen.columns(grid) == 1 && screen.rows(grid) == 1) { ambiguous |= subtitle >= 0; subtitle = int(i); }
        else if (box == message_box && screen.columns(grid) == 1) { ambiguous |= message >= 0; message = int(i); }
    }
    if (ambiguous || subtitle < 0 || message < 0) return;
    const unsigned rows = screen.rows(screen.grid(message));
    // A scrollbar narrows the message column; keep such a state original.
    if (!rows || rows > 30 || c.u8(screen.grid(message) + 0xbef)) return;
    // The footer is a single standalone button, not a grid. Require exactly one.
    int footer = -1;
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) >= 0) continue;
        const auto box = screen.rect(at + 0x10);
        if (box == footer_box) { ambiguous |= footer >= 0; footer = int(i); }
    }
    if (ambiguous || footer < 0) return;

    const ScreenLayout shell{canvas};
    auto available = shell.content(message_box.top);
    const int width = std::min(960, available.width());
    available.left += (available.width() - width) / 2; available.right = available.left + width;
    const std::array<FormTrack,1> sections{{{message_box.height(), 1, int(rows) * 28 + 16}}};
    const auto body = form_tracks(available, sections, false).front();
    auto button = shell.buttons(std::min(body.width(), 190));
    button.left -= button.right - body.right; button.right = body.right;
    const int shift = (body.left + body.right - message_box.left - message_box.right) / 2;
    const auto subtitle_after = shell.subtitle();

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        const int owner = c.i16(at + 0x72), left = c.i32(at + 0x28), right = c.i32(at + 0x2c);
        if (owner == subtitle) {
            if (left && right) {
                const auto moved = DetailsLayout::centered_text({left,0,right,0}, subtitle_box, subtitle_after);
                c.put32(at + 0x28, moved.left); c.put32(at + 0x2c, moved.right);
            }
        } else if (owner == message) {
            if (left && right) { c.put32(at + 0x28, left + shift); c.put32(at + 0x2c, right + shift); }
        } else if (owner < 0 && int(i) == footer) {
            screen.write(at + 0x10, button);
            c.put32(at + 0x28, button.left); c.put32(at + 0x2c, button.right);
            c.put32(at + 0x30, button.top); c.put32(at + 0x34, button.bottom);
        }
    }
    const std::array<std::pair<int,ScreenRect>,2> panels{{{subtitle, subtitle_after}, {message, body}}};
    for (const auto& [index, box] : panels) {
        screen.write(screen.grid(index), box);
        screen.measure(screen.grid(index));
        screen.publish(screen.grid(index));
    }
}
}
