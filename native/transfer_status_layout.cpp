#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <algorithm>
#include <array>
#include <vector>

namespace cm::aot {
namespace {
// The columns AutoGrid measured for a grid's original rectangle (offsets from its left edge) and the
// tracks that carry them to a new width: only the widest column takes the extra width, so labels
// grow while the buttons that follow keep their size and move with it.
struct ColumnMap {
    std::vector<int> low, high, left, right;
    std::vector<ScreenRect> tracks;
    unsigned count() const { return unsigned(left.size()); }
    bool read(const guest::Cpu& c, const GuestScreen& screen, std::uint32_t grid, int original_width, ScreenRect panel) {
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
        for (unsigned i = 0; i < n; ++i) spans.push_back({high[i] - low[i], i == unsigned(widest) ? 1.0f : 0.0f});
        tracks = form_tracks(panel, spans, true);
        return true;
    }
    // Horizontal bounds of a cell: its original insets inside the track.
    std::pair<int,int> cell(unsigned column) const {
        return {tracks[column].left + left[column] - low[column], tracks[column].right - (high[column] - right[column])};
    }
};
}

// Player transfer status (builder 0x10428ab0): a seven-column grid for the asking-price row, a
// two-column grid over the same rectangle for the squad-status and listing choices, and a one-row
// footer. The original builder keeps every control, action, text and the price menu; this pass moves
// the grids and the footer buttons only; the title and subtitle controls already follow the shell.
void layout_transfer_status(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    constexpr ScreenRect panel_box{110,140,780,545}, footer_box_original{100,555,790,590};
    int price = -1, choices = -1, footer = -1;
    bool ambiguous = false;
    const auto claim = [&](int& slot, unsigned i) { if (slot >= 0) ambiguous = true; slot = int(i); };
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    for (unsigned i = 0; i < grids; ++i) {
        const auto box = screen.rect(screen.grid(i));
        const unsigned columns = screen.columns(screen.grid(i)), rows = screen.rows(screen.grid(i));
        if (box == panel_box && columns == 7) claim(price, i);
        else if (box == panel_box && columns == 2) claim(choices, i);
        else if (box == footer_box_original && rows == 1 && columns >= 1 && columns <= 3) claim(footer, i);
    }
    if (ambiguous || price < 0 || choices < 0 || footer < 0) return;
    const auto price_grid = screen.grid(price), choice_grid = screen.grid(choices), footer_grid = screen.grid(footer);
    const unsigned rows = screen.rows(price_grid), buttons = screen.columns(footer_grid);
    // Both grids share one row table; a scrollbar would change the column widths measured below.
    if (!rows || rows > 30 || screen.rows(choice_grid) != rows || c.u8(price_grid + 0xbef) || c.u8(choice_grid + 0xbef)) return;

    const ScreenLayout shell{canvas};
    auto available = shell.content(panel_box.top);
    const int width = std::min(960, available.width());
    available.left += (available.width() - width) / 2; available.right = available.left + width;
    const std::array<FormTrack,1> sections{{{panel_box.height(), 1, int(rows) * 28 + 16}}};
    const auto body = form_tracks(available, sections, false).front();
    auto buttons_box = shell.buttons(std::min(body.width(), int(buttons) * 190));
    buttons_box.left -= buttons_box.right - body.right; buttons_box.right = body.right;

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    // Read the columns the builder's rectangle produced, not the ones a previous pass left behind.
    screen.measure(price_grid); screen.measure(choice_grid);
    ColumnMap price_columns, choice_columns;
    if (!price_columns.read(c, screen, price_grid, panel_box.width(), body) ||
        !choice_columns.read(c, screen, choice_grid, panel_box.width(), body)) return;
    std::vector<FormTrack> equal(buttons, FormTrack{60, 1});
    const auto button_tracks = form_tracks(buttons_box, equal, true);

    const int shift = body.left - panel_box.left;
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        const int owner = c.i16(at + 0x72), left = c.i32(at + 0x28), right = c.i32(at + 0x2c);
        if (owner == price || owner == choices) {
            if (left && right) {
                // The intro paragraph and its links carry explicit text bounds. Links keep their
                // measured width; the paragraph background follows the panel's right edge.
                const int end = std::min(body.right, !c.u32(at + 8) && right == panel_box.right ? body.right : right + shift);
                c.put32(at + 0x28, std::min(left + shift, end)); c.put32(at + 0x2c, end);
            } else {
                const auto& columns = owner == price ? price_columns : choice_columns;
                const unsigned column = c.u32(at + 0x20);
                if (column < columns.count()) {
                    const auto [first, last] = columns.cell(column);
                    c.put32(at + 0x28, first); c.put32(at + 0x2c, last);
                }
            }
        } else if (owner == footer) {
            const unsigned column = c.u32(at + 0x20);
            if (column < button_tracks.size()) {
                c.put32(at + 0x28, button_tracks[column].left + (column ? 1 : 0));
                c.put32(at + 0x2c, button_tracks[column].right - (column + 1 < button_tracks.size() ? 1 : 0));
            }
        }
    }
    for (unsigned i = 0; i < buttons; ++i) c.put8(footer_grid + 0xbb2 + i, 1);
    const std::array<std::pair<int,ScreenRect>,3> panels{{{price, body}, {choices, body}, {footer, buttons_box}}};
    for (const auto& [index, box] : panels) {
        screen.write(screen.grid(index), box);
        screen.measure(screen.grid(index));
        screen.publish(screen.grid(index));
    }
}
}
