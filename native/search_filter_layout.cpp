#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <algorithm>
#include <array>

namespace cm::aot {
void layout_search_filters(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    // SEARCH_SCREENS::BuildFilter always emits this shell and seven overlay
    // grids (general, nationality, base, value, two attribute groups, text).
    // Validate it before changing any coordinates; nested popups are excluded.
    if (c.u16(display + 0x129da4) < 11) return;
    const auto panel = screen.rect(screen.grid(1)), fields = screen.rect(screen.grid(4));
    if (panel.left != 107 || panel.right != 781 || (panel.top != 50 && panel.top != 125) ||
        panel.bottom != 599 - panel.top || fields != ScreenRect{132,panel.top+105,756,panel.bottom-65} ||
        screen.rect(screen.grid(0)) != ScreenRect{0,0,799,599} ||
        screen.rect(screen.grid(2)) != ScreenRect{117,panel.top+65,367,panel.top+85} ||
        screen.rect(screen.grid(3)) != ScreenRect{117,panel.bottom-40,771,panel.bottom-10}) return;
    unsigned rows = 0;
    for (unsigned i = 4; i < 11; ++i) {
        if (screen.rect(screen.grid(i)) != fields) return;
        rows = std::max(rows, unsigned(screen.rows(screen.grid(i))));
    }
    const ScreenRect body{fields.left-5,fields.top-5,fields.right+5,fields.bottom+5};
    // Finite forms gain breathing room, with a cap on row height and width so
    // an ultrawide window does not spread a handful of fields across the desk.
    FormMetrics metrics;
    metrics.maximum_height = std::min(panel.height() + canvas.height - 600, int(rows) * 32 + 170);
    const auto layout = form_layout({107,50,canvas.width-19,canvas.height-51}, metrics);
    const auto transform = [](ScreenRect rect, ScreenRect from, ScreenRect to) {
        const auto x = [&](int value) { return to.left + (value - from.left) * to.width() / from.width(); };
        const auto y = [&](int value) { return to.top + (value - from.top) * to.height() / from.height(); };
        return ScreenRect{x(rect.left),y(rect.top),x(rect.right),y(rect.bottom)};
    };
    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    const ScreenRect title{117,panel.top+10,771,panel.top+55};
    const ScreenRect clear{646,panel.top+65,771,panel.top+85};
    for (unsigned i = 0, count = std::min<unsigned>(1200, c.u16(display + 0x129da6)); i < count; ++i) {
        const auto control = screen.control(i);
        const auto before = screen.rect(control + 0x10);
        const int owner = c.i16(control + 0x72);
        if (owner < 0 && (before == title || before == clear)) {
            const auto after = before == title ? layout.title : layout.toolbar_trailing;
            screen.write(control + 0x10, after);
            c.put32(control + 0x28, after.left); c.put32(control + 0x2c, after.right);
            c.put32(control + 0x30, after.top); c.put32(control + 0x34, after.bottom);
        } else if ((owner == 3 && !c.u32(control + 8) && !c.u8(control + 0x78)) || (owner >= 4 && owner <= 10)) {
            // Pitch decoration lives on the footer grid with explicit bounds.
            // Text/range fields can likewise override their row/column bounds.
            // Zero is AutoGrid's sentinel: never turn it into an override.
            const auto from = owner == 3 ? body : fields;
            const auto to = owner == 3 ? layout.body : layout.fields;
            const auto explicit_rect = ScreenRect{c.i32(control+0x28),c.i32(control+0x30),
                                                  c.i32(control+0x2c),c.i32(control+0x34)};
            const auto after = transform(explicit_rect, from, to);
            if (explicit_rect.left && explicit_rect.right) {
                c.put32(control + 0x28, after.left); c.put32(control + 0x2c, after.right);
            }
            if (explicit_rect.top && explicit_rect.bottom) {
                c.put32(control + 0x30, after.top); c.put32(control + 0x34, after.bottom);
            }
        }
    }
    const std::array<ScreenRect,11> rectangles{
        ScreenRect{0,0,canvas.width-1,canvas.height-1},layout.panel,layout.toolbar_leading,layout.footer,
        layout.fields,layout.fields,layout.fields,layout.fields,layout.fields,layout.fields,layout.fields};
    for (unsigned i = 0; i < rectangles.size(); ++i) {
        screen.write(screen.grid(i), rectangles[i]);
        screen.measure(screen.grid(i));
        // Publish exactly the same positions to drawing and hit testing.
        // These calls retain each original control and its popup ownership.
        screen.publish(screen.grid(i));
    }
}
}
