#include "paged_screen_layout.hpp"

namespace cm::aot {
// Pages with a tab bar: the transfer rules article of the news (builder 0x106ff680, opener 0x106ff550: four tabs at 140,
// then the article's wrapped lines), the Historial (0x10145290, opener 0x101451d0: five tabs at 80 and a one-column body that
// the tab's sub-builder fills), the game database (0x10106840, opener 0x10106530: four tabs, a filter pair, the page
// controls, a summary line and a three-column list of nations, clubs or staff) and a club's Historial (0x1003b7f0,
// opener 0x1003b3a0: tabs above, a second bar of four tabs under the body). Each is the tab bar (two grids over one
// rectangle: a backdrop and the tabs), one body grid at 110,<top>-780,<bottom>, optional controls between them and the Back/Next
// footer.
//
// Here they become one centered panel of at most 960: the tab bar spans it with equal tabs, the body takes the height
// its rows can use (the second bar, when there is one, under it), the filter pair and the controls of the row above the
// body keep their side of the panel (a summary line spans it) and the footer is right-aligned to it. The original builders
// keep every text, link, command and row; the body's columns keep their size except the widest, and a line with measured
// text bounds (the article's) is carried with the panel's left edge. A page whose shape differs (a second body, a
// scrollbar with an unusual column map) stays as built.
void layout_tabbed_page(State& s, std::uint32_t display, GuiCanvas canvas) {
    using namespace paged_screen;
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const ScreenLayout shell{canvas};
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    struct Bar { int backdrop = -1, tabs = -1; };
    Bar upper, lower;
    int body = -1, footer = -1, filter = -1;
    bool ambiguous = false;
    const auto claim = [&](int& slot, unsigned i) { ambiguous |= slot >= 0; slot = int(i); };
    int bar_top = 0;
    // Tab bars: the grid pairs over 100,T-790,T+35 (T 80 or 140 above the body, 510 or 511 under it).
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        const auto box = screen.rect(grid);
        if (is_footer(screen, grid, shell)) {
            claim(footer, i);
        } else if (box.left == 100 && box.right == 790 && box.height() == 35 && screen.rows(grid) == 1) {
            const bool above = box.top == 80 || box.top == 140;
            const bool below = box.top == 510 || box.top == 511;
            if (!above && !below) { ambiguous = true; continue; }
            auto& bar = above ? upper : lower;
            if (above) bar_top = box.top;
            if (screen.columns(grid) == 1) claim(bar.backdrop, i);
            else claim(bar.tabs, i);
        } else if (box.left == 110 && box.right == 780 && box.height() >= 100) {
            claim(body, i);
        } else if (box.left == 110 && box.height() == 20 && box.right <= 400 && box.top == 125 && screen.rows(grid) == 1) {
            claim(filter, i);
        } else if (box.left >= 100) {
            ambiguous = true;
        }
    }
    if (ambiguous || upper.backdrop < 0 || upper.tabs < 0 || body < 0 || footer < 0) return;
    if ((lower.backdrop < 0) != (lower.tabs < 0)) return;
    const bool has_lower = lower.tabs >= 0;
    const auto body_grid = screen.grid(body), footer_grid = screen.grid(footer);
    const auto body_box = screen.rect(body_grid);
    const unsigned rows = screen.rows(body_grid);
    if (!rows || rows > 30) return;
    const int lower_top = has_lower ? screen.rect(screen.grid(lower.tabs)).top : 0;
    // The body sits right under the bar (or the controls of the row above it) and, with a second bar, above that.
    if (body_box.top < bar_top + 35 || (has_lower && body_box.bottom > lower_top)) return;

    // The panel: the content area's width up to 960, the body's rows as tall as they can be, the second bar under it.
    auto available = shell.content(body_box.top);
    const int width = std::min(960, available.width());
    available.left += (available.width() - width) / 2; available.right = available.left + width;
    const int bar_height = 35, gap = 10;
    const int reserved = has_lower ? gap + bar_height : 0;
    available.bottom -= reserved;
    const std::array<FormTrack, 1> sections{{{body_box.height(), 1, int(rows) * 28 + 16}}};
    const auto panel = form_tracks(available, sections, false).front();
    const ScreenRect lower_box{panel.left, panel.bottom + gap, panel.right, panel.bottom + gap + bar_height};
    const ScreenRect upper_box{panel.left, bar_top, panel.right, bar_top + bar_height};
    const int lead = panel.left - body_box.left, trail = panel.right - body_box.right;

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    // Read the columns the builder's rectangle produced, not the ones a previous pass left behind.
    screen.measure(body_grid);
    ColumnMap map;
    if (!map.read(c, screen, body_grid, body_box.width(), panel)) return;
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        const int owner = c.i16(at + 0x72);
        const auto old = screen.rect(at + 0x10);
        const int left = c.i32(at + 0x28), right = c.i32(at + 0x2c);
        if (owner == body) {
            if (left && right) {
                // A line with measured bounds keeps its distance from the left edge; one that reached the right edge follows it.
                c.put32(at + 0x28, left + lead);
                c.put32(at + 0x2c, right >= body_box.right - 10 ? right + trail : right + lead);
            } else if (c.u32(at + 0x20) < map.count()) {
                const auto [first, last] = map.cell(c.u32(at + 0x20));
                c.put32(at + 0x28, first); c.put32(at + 0x2c, last);
            }
        } else if (owner < 0 && old.top >= bar_top + 35 && old.bottom <= body_box.top && old.left >= body_box.left && old.right <= body_box.right) {
            // The row between the tabs and the body: a line across the panel spans it, other controls keep their side.
            ScreenRect box = old;
            if (old.left == body_box.left && old.right == body_box.right) { box.left = panel.left; box.right = panel.right; }
            else {
                const int shift = old.left < (body_box.left + body_box.right) / 2 ? lead : trail;
                box.left += shift; box.right += shift;
            }
            const bool mirrored = left == old.left && right == old.right;
            screen.write(at + 0x10, box);
            if (mirrored) { c.put32(at + 0x28, box.left); c.put32(at + 0x2c, box.right); }
        }
    }
    screen.write(body_grid, panel);
    screen.measure(body_grid);
    screen.publish(body_grid);
    // The tabs: equal cells across the bar, the backdrop under them.
    const auto bar = [&](const Bar& which, ScreenRect box) {
        screen.write(screen.grid(which.backdrop), box);
        screen.measure(screen.grid(which.backdrop));
        screen.publish(screen.grid(which.backdrop));
        place_footer(screen, c, screen.grid(which.tabs), box);
    };
    bar(upper, upper_box);
    if (has_lower) bar(lower, lower_box);
    if (filter >= 0) {
        // The filter pair keeps its size at the panel's left edge.
        const auto old = screen.rect(screen.grid(filter));
        screen.write(screen.grid(filter), {old.left + lead, old.top, old.right + lead, old.bottom});
        screen.measure(screen.grid(filter));
        screen.publish(screen.grid(filter));
    }
    place_footer(screen, c, footer_grid, footer_box(shell, panel));
}
}
