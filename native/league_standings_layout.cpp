#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_competition_stage(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    int body = -1;
    int group_table = -1;
    bool message = false;
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        const auto box = screen.rect(grid);
        if (box.left != 110 || box.right != 780) continue;
        const auto type = c.u32(grid + 0x18);
        if (box.top == 190 && type == 2 && screen.columns(grid) == 11 &&
            box.bottom != 500 && box.bottom != 545) {
            group_table = int(i);
            continue;
        }
        if (box.bottom != 500 && box.bottom != 545) continue;
        if (group_table >= 0 && type == 1 && screen.columns(grid) == 6 &&
            box.top == screen.rect(screen.grid(group_table)).bottom + 5) {
            body = int(i);
            break;
        }
        if (type != 2) continue;
        // Section 1 stacks the optional stage selector (25px), round title
        // (40px) and live draw controls (25px) before the nine-column list.
        const auto columns = screen.columns(grid);
        const bool stage_top = box.top == 125 || box.top == 150 || box.top == 165 || box.top == 190;
        if ((columns == 11 && box.top == 190) || (columns == 1 && stage_top) ||
            (columns == 9 && (stage_top || box.top == 175 || box.top == 215))) {
            body = int(i);
            message = columns == 1;
            group_table = -1;
            break;
        }
    }
    if (body < 0) return;
    const auto before = screen.rect(screen.grid(body));
    const auto original_rows = screen.rows(screen.grid(body));
    const bool fixtures = group_table >= 0;
    if ((!original_rows && !fixtures) || original_rows > 30) return;
    // The group fixture list uses font 2 without a frame inset. Standings
    // and knockout grids use font 3 and reserve 16 pixels for their frame.
    const int font = fixtures ? 2 : 3;
    const int inset = fixtures ? 0 : 16;
    const int line_height = c.i32(c.toc(-0x740c)) != -1 ? (fixtures ? 18 : 21)
        : c.i32(c.toc(-0x3bdc) + (font - 1) * 0x1404);
    if (line_height <= 0) return;
    const ScreenLayout shell{canvas};
    auto available = shell.content(before.top);
    if (before.bottom == 500) available.bottom -= 45;
    const int width = std::min(1600, available.width());
    available.left += (available.width() - width) / 2;
    available.right = available.left + width;
    const std::array<FormTrack, 1> tracks{{{before.height(), 1, 30 * line_height + inset}}};
    const auto panel = form_tracks(available, tracks, false).front();
    const int rows = message ? original_rows : std::clamp((panel.height() - inset) / line_height, int(original_rows), 30);
    const auto standings = screen.grid(fixtures ? group_table : body);
    const bool standings_overflow = c.i32(standings + 0x14) + 1 > (fixtures ? screen.rows(standings) : rows);
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = screen.control(i);
        const int owner = c.i16(control + 0x72);
        if (owner == body || (fixtures && owner == group_table)) {
            if (message && (c.i32(control + 0x28) || c.i32(control + 0x2c))) {
                const int shift = (panel.left + panel.right - before.left - before.right) / 2;
                c.put32(control + 0x28, c.i32(control + 0x28) + shift);
                c.put32(control + 0x2c, c.i32(control + 0x2c) + shift);
            }
            // The cup builder gives the bye label and underline measured widths.
            else if (c.i32(control + 0x28) == 115 && c.i32(control + 0x2c) > 115) {
                c.put32(control + 0x28, panel.left + 5);
                c.put32(control + 0x2c, c.i32(control + 0x2c) + panel.left - before.left);
            }
            // Promotion/relegation separators span the row through explicit
            // X overrides. AutoGrid alone would leave them at the old width.
            else if (c.i32(control + 0x28) == 112 &&
                (c.i32(control + 0x2c) == 778 || c.i32(control + 0x2c) == 756)) {
                c.put32(control + 0x28, panel.left + 2);
                c.put32(control + 0x2c, panel.right - 2 - (standings_overflow ? 22 : 0));
            }
            continue;
        }
        if (owner >= 0) continue;
        const auto old = screen.rect(control + 0x10);
        auto box = old;
        if (old == ScreenRect{100, 10, 790, 70}) box = shell.title();
        else if (old == ScreenRect{110, 150, 780, 185}) box = {panel.left, 150, panel.right, 185};
        else if (old == ScreenRect{110, 125, 780, 160}) box = {panel.left, 125, panel.right, 160};
        else if (old == ScreenRect{110, 125, 235, 145}) box = {panel.left, 125, panel.left + 125, 145};
        else if (old == ScreenRect{655, 125, 780, 145}) box = {panel.right - 125, 125, panel.right, 145};
        else if (old == ScreenRect{528, 125, 653, 145}) box = {panel.right - 252, 125, panel.right - 127, 145};
        else if (old == ScreenRect{710, 15, 785, 35}) box = shell.anchor(old, LayoutAxis::trailing, LayoutAxis::fixed);
        else continue;
        screen.write(control + 0x10, box);
        c.put32(control + 0x28, box.left); c.put32(control + 0x2c, box.right);
        c.put32(control + 0x30, box.top); c.put32(control + 0x34, box.bottom);
    }
    for (unsigned i = 0; i < grids; ++i) {
        auto box = screen.rect(screen.grid(i));
        if (int(i) == body) {
            box = panel;
            screen.show_rows(screen.grid(i), original_rows, rows);
            screen.set_rows(screen.grid(i), rows);
            if (!message) {
                screen.hide_scrollbar_if_fits(screen.grid(body), rows);
                screen.resize_rows(screen.grid(i));
            }
        } else if (int(i) == group_table) {
            // All clubs and qualification separators already have their rows.
            // Keep their height and give the extra vertical space to matches.
            box.left = panel.left;
            box.right = panel.right;
        } else if (box.left == 110 && box.right == 780 && box.height() == 20 &&
                   box.bottom + 5 == before.top && screen.rows(screen.grid(i)) == 1 &&
                   (screen.columns(screen.grid(i)) == 4 || screen.columns(screen.grid(i)) == 5)) {
            box.left = panel.left;
            box.right = panel.right;
        } else if (box == ScreenRect{100, 80, 790, 115}) box.right = canvas.width - 10;
        else if (box == ScreenRect{100, 510, 790, 545}) box = {100, canvas.height - 90, canvas.width - 10, canvas.height - 55};
        else if (box == ScreenRect{100, 555, 790, 590}) box = shell.footer();
        else continue;
        screen.write(screen.grid(i), box);
        screen.measure(screen.grid(i));
        screen.publish(screen.grid(i));
    }
}
}
