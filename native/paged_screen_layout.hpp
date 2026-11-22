#pragma once
#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <algorithm>
#include <array>
#include <utility>
#include <vector>

namespace cm::aot::paged_screen {
// Helpers shared by the screens that follow the Back/Next history pattern: a subtitle, an optional
// toolbar row above the content, one content panel and a two-button footer. The original builders keep
// every text, link, command and row; these helpers only place their panels and controls.

// The columns AutoGrid measured for a grid's original rectangle (offsets from its left edge) and the
// tracks that carry them to a new width: only the widest column takes the extra width, so ranks and
// numbers keep their size and the names grow.
struct ColumnMap {
    std::vector<int> low, high, left, right;
    std::vector<ScreenRect> tracks;
    unsigned count() const { return unsigned(left.size()); }
    // `grow_mask` names the columns (bit i = column i) that share the extra width when several columns carry
    // the same kind of content (two nations side by side); 0 means only the widest one grows.
    bool read(const guest::Cpu& c, const GuestScreen& screen, std::uint32_t grid, int original_width, ScreenRect panel,
              unsigned grow_mask = 0) {
        const unsigned n = screen.columns(grid);
        if (n < 1 || n > 8) return false;
        int previous = 0, widest = 0;
        for (unsigned i = 0; i < n; ++i) {
            const int first = c.i32(grid + 0x1c + i * 4), last = c.i32(grid + 0x10c + i * 4);
            if (first < previous || last <= first || last > original_width) return false;
            low.push_back(i ? high.back() : 0); left.push_back(first); right.push_back(last);
            // Each track ends where its cell does, the last one at the grid's edge.
            high.push_back(i + 1 < n ? last : original_width);
            if (last - first > right[widest] - left[widest]) widest = int(i);
            previous = last;
        }
        std::vector<FormTrack> spans;
        for (unsigned i = 0; i < n; ++i)
            spans.push_back({high[i] - low[i], (grow_mask ? (grow_mask >> i) & 1 : i == unsigned(widest)) ? 1.0f : 0.0f});
        tracks = form_tracks(panel, spans, true);
        return true;
    }
    // Horizontal bounds of a cell: its original insets inside the track.
    std::pair<int,int> cell(unsigned column) const {
        return {tracks[column].left + left[column] - low[column], tracks[column].right - (high[column] - right[column])};
    }
};

// The content panel of such a screen: centered, at most 960 wide, as tall as its rows can use.
inline ScreenRect body(const ScreenLayout& shell, ScreenRect original, unsigned rows) {
    auto available = shell.content(original.top);
    const int width = std::min(960, available.width());
    available.left += (available.width() - width) / 2; available.right = available.left + width;
    const std::array<FormTrack,1> sections{{{original.height(), 1, int(rows) * 28 + 16}}};
    return form_tracks(available, sections, false).front();
}

// The Back/Next footer: both buttons together, right-aligned to the panel.
inline ScreenRect footer_box(const ScreenLayout& shell, ScreenRect panel) {
    auto box = shell.buttons(std::min(panel.width(), 340));
    box.left -= box.right - panel.right; box.right = panel.right;
    return box;
}
inline bool is_footer(const GuestScreen& screen, std::uint32_t grid, const ScreenLayout& shell) {
    const auto box = screen.rect(grid);
    return (box == shell.footer() || box == ScreenRect{100, 555, 790, 590}) &&
           screen.columns(grid) == 2 && screen.rows(grid) == 1;
}
// Equal-width buttons: AutoGrid divides the rectangle by the columns once they are flagged.
inline void place_footer(const GuestScreen& screen, guest::Cpu& c, std::uint32_t grid, ScreenRect box) {
    for (unsigned column = 0; column < screen.columns(grid); ++column) c.put8(grid + 0xbb2 + column, 1);
    screen.write(grid, box);
    screen.measure(grid);
    screen.publish(grid);
}

// Standalone controls (owner < 0): the subtitle follows the shell, and a toolbar between the subtitle
// and the panel keeps its side: controls on the left half move with the panel's left edge, the pager
// and other controls on the right half with its right edge. Explicit text bounds move with them.
// With `pager_band`, the small controls on the right of the subtitle's own band (a pager at 632-785 x 85-105)
// keep their offsets from each other and have their right end follow the panel's right edge.
inline bool in_pager_band(ScreenRect box) { return box.top >= 80 && box.bottom <= 125 && box.left >= 400 && box.right - box.left <= 200; }
inline void move_chrome(const GuestScreen& screen, guest::Cpu& c, unsigned controls, const ScreenLayout& shell,
                        ScreenRect panel, ScreenRect body_after, bool pager_band = false) {
    constexpr ScreenRect subtitle{100, 80, 790, 125};
    const int lead = body_after.left - panel.left, trail = body_after.right - panel.right;
    int band_right = 0;
    for (unsigned i = 0; pager_band && i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) < 0 && in_pager_band(screen.rect(at + 0x10))) band_right = std::max(band_right, screen.rect(at + 0x10).right);
    }
    const int band_shift = band_right ? body_after.right - band_right : 0;
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) >= 0) continue;
        const auto old = screen.rect(at + 0x10);
        auto box = old;
        if (old == subtitle) box = shell.subtitle();
        else if (old.top >= 125 && old.bottom <= panel.top && old.left >= panel.left && old.right <= panel.right) {
            const int shift = old.left < (panel.left + panel.right) / 2 ? lead : trail;
            box.left += shift; box.right += shift;
        } else if (band_right && in_pager_band(old)) {
            box.left += band_shift; box.right += band_shift;
        } else continue;
        // Their text bounds mirror the rectangle.
        const bool mirrored = c.i32(at + 0x28) == old.left && c.i32(at + 0x2c) == old.right;
        screen.write(at + 0x10, box);
        if (mirrored) { c.put32(at + 0x28, box.left); c.put32(at + 0x2c, box.right); }
    }
}

// A screen with one table of `columns` columns at `panel_box` (the grid the builder created for it, with or
// without a scrollbar), standalone controls above it and the Back/Next footer: the same pass for every
// such table. With `or_message` the table may instead be one column holding one centered message. It declines (classic geometry) on a missing or duplicated table or footer. Only the widest
// column takes the extra width; a scrollbar keeps its gutter at the table's right edge because the last
// column's track runs to the grid's edge, and the original's own scroll state and rows are untouched.
inline void layout_table_screen(State& s, std::uint32_t display, GuiCanvas canvas, ScreenRect panel_box, unsigned columns, bool or_message = false,
                                unsigned grow_mask = 0, bool pager_band = false) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const ScreenLayout shell{canvas};
    int table = -1, footer = -1;
    bool ambiguous = false;
    const auto claim = [&](int& slot, unsigned i) { ambiguous |= slot >= 0; slot = int(i); };
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        if (screen.rect(grid) == panel_box && (screen.columns(grid) == columns || (or_message && screen.columns(grid) == 1))) claim(table, i);
        else if (is_footer(screen, grid, shell)) claim(footer, i);
    }
    if (ambiguous || table < 0 || footer < 0) return;
    const auto table_grid = screen.grid(table), footer_grid = screen.grid(footer);
    const unsigned rows = screen.rows(table_grid);
    if (!rows || rows > 30) return;
    const auto panel = body(shell, panel_box, rows);

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    // Read the columns the builder's rectangle produced, not the ones a previous pass left behind.
    screen.measure(table_grid);
    ColumnMap map;
    if (!map.read(c, screen, table_grid, panel_box.width(), panel, grow_mask)) return;
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) != table) continue;
        // A link that spans its whole row (Manager Status) keeps spanning the panel.
        if (c.i32(at + 0x28) <= panel_box.left && c.i32(at + 0x2c) >= panel_box.right && c.i32(at + 0x28) && c.i32(at + 0x2c)) {
            c.put32(at + 0x28, panel.left); c.put32(at + 0x2c, panel.right);
            continue;
        }
        if (c.i32(at + 0x28) || c.i32(at + 0x2c)) {
            // Centered text with measured bounds (an empty table's message) stays centered in the panel.
            const int center = (panel.left + panel.right - panel_box.left - panel_box.right) / 2;
            c.put32(at + 0x28, c.i32(at + 0x28) + center); c.put32(at + 0x2c, c.i32(at + 0x2c) + center);
            continue;
        }
        const unsigned column = c.u32(at + 0x20);
        if (column >= map.count()) continue;
        const auto [first, last] = map.cell(column);
        c.put32(at + 0x28, first); c.put32(at + 0x2c, last);
    }
    move_chrome(screen, c, controls, shell, panel_box, panel, pager_band);
    screen.write(table_grid, panel);
    screen.measure(table_grid);
    screen.publish(table_grid);
    place_footer(screen, c, footer_grid, footer_box(shell, panel));
}
}
