#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
namespace {
struct AttributePair {
    std::array<std::uint32_t, 2> controls{};
};

// Original numeric attributes use decimal text; coaching style and other
// descriptions are localized strings. Empty reserved pairs stay in the grid.
bool is_rating(const guest::Cpu& c, std::uint32_t value) {
    const auto text = value + 0x78;
    const auto digit = [](unsigned ch) { return ch >= '0' && ch <= '9'; };
    return digit(c.u8(text)) && (!c.u8(text + 1) || (digit(c.u8(text + 1)) && !c.u8(text + 2)));
}
}

void layout_staff_profile(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const ScreenLayout shell{canvas};
    const auto count = std::min<unsigned>(250, c.u16(display + 0x129da4));
    unsigned attributes = count;
    for (unsigned i = 0; i + 1 < count; ++i) {
        const auto grid = screen.grid(i), next = screen.grid(i + 1);
        if (screen.rect(grid).left == 110 && screen.rect(grid).top == 160 && screen.rect(grid).right == 780 &&
            screen.columns(grid) == 4 && screen.columns(next) == 2 &&
            screen.rect(next).top == screen.rect(grid).bottom + 5 && screen.rect(next).right == 780) { attributes = i; break; }
    }
    if (attributes == count) return;
    const auto stats = screen.grid(attributes), prefs = screen.grid(attributes + 1);
    const int rows = screen.rows(stats), pref_rows = screen.rows(prefs);
    if (!rows || rows > 30 || !pref_rows || pref_rows > 30) return;
    const auto original_stats = screen.rect(stats), original_prefs = screen.rect(prefs);
    const auto controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    std::array<AttributePair, 60> pairs{};
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) != int(attributes)) continue;
        const auto col = c.u32(at + 0x20), row = c.u32(at + 0x24);
        if (col >= 4 || row >= unsigned(rows)) return;
        pairs[(col / 2) * rows + row].controls[col % 2] = at;
    }
    int descriptions = 0;
    for (int i = 0; i < 2 * rows; ++i) {
        const auto [label, value] = pairs[i].controls;
        if (!label || !value) return;
        if (!is_rating(c, value)) ++descriptions;
    }

    auto panel = shell.content(original_stats.top);
    const int width = std::min(1500, panel.width());
    panel.left += (panel.width() - width) / 2;
    panel.right = panel.left + width;
    panel.bottom -= 35; // Leave space for the original job title below the preferences.
    auto stats_box = panel, prefs_box = panel;
    const bool wide = width >= 1020;
    const int lines = wide ? std::max(2 * rows - descriptions, descriptions + pref_rows) : rows + pref_rows;
    const int line = std::min(32, (panel.height() - 5) / lines);
    const int split = wide ? descriptions : rows;
    prefs_box.top += split * line + 5;
    prefs_box.bottom = prefs_box.top + pref_rows * line;
    stats_box.bottom = wide ? panel.top + (2 * rows - descriptions) * line : prefs_box.top - 5;
    const int role_top = std::max(stats_box.bottom, prefs_box.bottom) + 5;

    if (wide) {
        // Keep the preferences value column at least as wide as in the original.
        const std::array<FormTrack, 3> tracks{{{334,1},{16},{670,1}}};
        const auto columns = form_tracks(panel, tracks, true);
        prefs_box.left = columns[2].left;
        const int pref_value = prefs_box.left + prefs_box.width() * 3 / 10;
        const int rating_value = columns[0].right - 68;
        int number_row = 0, text_row = 0;
        for (int i = 0; i < 2 * rows; ++i) {
            const auto [label, value] = pairs[i].controls;
            const bool numeric = is_rating(c, value);
            const auto side = numeric ? columns[0] : columns[2];
            const int top = panel.top + line * (numeric ? number_row++ : text_row++);
            const int divider = numeric ? rating_value : pref_value;
            // Explicit bounds are used by the original renderer and picker.
            // Preserve the grid's ordered cells and their command payloads.
            c.put32(label + 0x28, side.left); c.put32(label + 0x2c, divider - 1);
            c.put32(value + 0x28, divider + 1); c.put32(value + 0x2c, side.right);
            for (const auto at : {label, value}) {
                c.put32(at + 0x30, top); c.put32(at + 0x34, top + line - 2);
            }
        }
        stats_box.bottom = std::max(stats_box.bottom, prefs_box.top - 5);
    }

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    for (unsigned i = 0; i < count; ++i) {
        const auto area = screen.grid(i);
        const auto old = screen.rect(area);
        auto box = old;
        if (area == stats) box = stats_box;
        else if (area == prefs) box = prefs_box;
        else if (old == ScreenRect{100,10,790,70} || old == ScreenRect{100,25,790,55} ||
                 old == ScreenRect{100,80,790,115} || old == ScreenRect{110,125,780,155})
            box = {panel.left,old.top,panel.right,old.bottom};
        else if (old == shell.footer() || old == ScreenRect{100,555,790,590}) {
            box = {panel.right - 340, role_top + 40, panel.right, role_top + 75};
            c.put8(area + 0xbb2, 1); c.put8(area + 0xbb3, 1);
        } else continue;

        if (old == ScreenRect{100,25,790,55} || old == ScreenRect{110,125,780,155}) {
            const int shift = (box.left + box.right - old.left - old.right) / 2;
            for (unsigned j = 0; j < controls; ++j) {
                const auto at = screen.control(j);
                if (c.i16(at + 0x72) != int(i) || !c.i32(at + 0x28)) continue;
                c.put32(at + 0x28, c.i32(at + 0x28) + shift);
                c.put32(at + 0x2c, c.i32(at + 0x2c) + shift);
            }
        }
        screen.write(area, box);
        screen.measure(area);
        screen.publish(area);
    }
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        if (c.i16(at + 0x72) >= 0) continue;
        const auto old = screen.rect(at + 0x10);
        ScreenRect box;
        if (old == ScreenRect{660,4,785,24}) box = {panel.right - 130,4,panel.right - 5,24};
        else if (old == ScreenRect{110,original_prefs.bottom + 5,780,original_prefs.bottom + 35})
            box = {panel.left,role_top,panel.right,role_top + 30};
        else continue;
        screen.write(at + 0x10, box);
        c.put32(at + 0x28, box.left); c.put32(at + 0x2c, box.right);
        c.put32(at + 0x30, box.top); c.put32(at + 0x34, box.bottom);
    }
}
}
