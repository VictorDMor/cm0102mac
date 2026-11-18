#include "form_layout.hpp"
#include "adaptive_gui.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
namespace {
// The match builder draws the pitch and the possession strip as individual
// controls. Map their endpoints together so adjacent colors meet exactly and
// circles, stripes and the three thirds keep their proportions.
constexpr ScreenRect original_zones{185,160,705,490};

ScreenRect fit_zones(ScreenRect body) {
    const int top = body.top + 35;
    const int room = body.bottom - 10 - top;
    const int width = std::min(body.width() - 20, room * original_zones.width() / original_zones.height());
    const int height = width * original_zones.height() / original_zones.width();
    const int x = body.left + (body.width() - width) / 2;
    const int y = top + (room - height) / 2;
    return {x,y,x+width,y+height};
}

ScreenRect place_zone_control(ScreenRect box, ScreenRect target) {
    const auto x = [&](int value) {
        return target.left + ((value-original_zones.left)*target.width() + original_zones.width()/2) / original_zones.width();
    };
    const auto y = [&](int value) {
        return target.top + ((value-original_zones.top)*target.height() + original_zones.height()/2) / original_zones.height();
    };
    return {x(box.left),y(box.top),x(box.right),y(box.bottom)};
}
}

// The running match sends fresh possession bounds between screen builds.
// Read the laid-out frame so initial construction, partial repaints and
// window resizes all use the same geometry without storing it in the match.
ScreenRect live_match_update_rect(State& s, std::uint32_t display, int index, ScreenRect box) {
    if (gui_canvas(s).original() || !adaptive_screen_available(s, 0x1028c430)) return box;
    guest::Cpu c(s);
    const auto manager = c.toc(-0x7bb0);
    if (display != c.u32(manager + 0x24)) return box;
    const auto history = c.u32(manager + 0x10);
    const auto slot = [&](unsigned n) { return c.i32(history + 0x14 + n*8); };
    const GuestScreen screen{c,display};
    const int count = c.u16(display + 0x129da6);
    if (index < 0 || index >= count) return box;
    if (slot(2) == 1) {
        const int frame = slot(33);
        if (frame >= 0 && (index == frame+3 || index == frame+4)) {
            const auto bounds = screen.rect(screen.control(frame)+0x10);
            const int dx = bounds.left-150, dy = bounds.top-(slot(34)-45);
            return {box.left+dx, box.top+dy, box.right+dx, box.bottom+dy};
        }
    } else if (slot(2) == 8) {
        // 0x102988f0 refreshes the competition headings every few seconds with
        // bounds measured from the original list edge. Follow the laid-out list.
        const int owner = c.i16(screen.control(index) + 0x72);
        if (owner >= 0 && owner < int(std::min<unsigned>(250, c.u16(display + 0x129da4))) &&
            box.left == 125 && box.right > 125) {
            const auto list = screen.rect(screen.grid(owner));
            if (list.top == 165) return {box.left + list.left-110, box.top, box.right + list.left-110, box.bottom};
        }
    } else if (slot(2) == 3) {
        const int first = slot(11);
        if (first < 22 || first+19 >= count) return box;
        const int part = index-first;
        if (part >= 6 && part <= 22 && (part%5 == 1 || part%5 == 2)) {
            auto pitch = screen.rect(screen.control(first-22)+0x10);
            pitch.bottom = screen.rect(screen.control(first+19)+0x10).bottom;
            return place_zone_control(box,pitch);
        }
    }
    return box;
}

void layout_match_report(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const auto history = c.u32(c.toc(-0x7bb0) + 0x10);
    const auto tab = c.u32(history + 0x24);
    const bool live = c.u32(c.u32(history)) == 0x1028c430;
    const bool can_show_message = tab != 1 && tab != 2 && tab != 6;

    auto title = ScreenLayout{canvas}.title();
    const int width = std::min(1420, title.width());
    title.left += (title.width() - width) / 2;
    title.right = title.left + width;
    const std::array<FormTrack, 3> header_tracks{{{0,1}, {2}, {0,1}}};
    const auto teams = form_tracks(title, header_tracks, true);
    const int left = title.left + 10, right = title.right - 10;
    const auto stats = c.u32(history + 0x14 + 32*8);
    const bool detailed = live || (stats && !c.u8(stats + 0x4c));
    const bool team_data = detailed && (live ? tab == 6 || tab == 9 : tab == 5 || tab == 7);
    const bool zones = tab == 3 && detailed;
    const std::array<FormTrack, 1> body_track{{{375,1,zones ? 675 : 520}}};
    const auto body = form_tracks({left,125,right,canvas.height-100}, body_track, false).front();
    const auto diagram = zones ? fit_zones(body) : ScreenRect{};
    const ScreenRect upper{title.left,80,title.right,115};
    const ScreenRect lower{title.left,body.bottom+10,title.right,body.bottom+45};
    const ScreenRect footer{right-340,lower.bottom+10,right,lower.bottom+45};

    const int center_shift = (left + right - 890) / 2;
    const int bottom_shift = body.bottom - 500;
    const auto shift = [](ScreenRect r, int x, int y) {
        return ScreenRect{r.left+x, r.top+y, r.right+x, r.bottom+y};
    };
    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    for (unsigned i = 0, count = std::min<unsigned>(1200, c.u16(display + 0x129da6)); i < count; ++i) {
        const auto control = screen.control(i);
        const int owner = c.i16(control + 0x72);
        if (owner >= 0) {
            if (live && tab == 10 && c.i32(control + 0x28) == 112 &&
                (c.i32(control + 0x2c) == 778 || c.i32(control + 0x2c) == 756)) {
                // Full-width separator between positions in the live table;
                // 756 leaves the scrollbar gutter free (0x1029df00).
                const int gutter = c.i32(control + 0x2c) == 756 ? 22 : 0;
                c.put32(control + 0x28, left+2); c.put32(control + 0x2c, right-2-gutter);
            } else if (live && tab == 8 && c.i32(control + 0x28) == 125 && c.i32(control + 0x2c) > 125) {
                // Latest Scores competition headings (0x102988f0) carry measured X bounds.
                c.put32(control + 0x28, c.i32(control + 0x28) + left-110);
                c.put32(control + 0x2c, c.i32(control + 0x2c) + left-110);
            } else if (can_show_message && owner < 250 && screen.columns(screen.grid(owner)) == 1 &&
                screen.rect(screen.grid(owner)) == ScreenRect{110,125,780,500} &&
                c.i32(control + 0x2c) > c.i32(control + 0x28)) {
                c.put32(control + 0x28, c.i32(control + 0x28) + center_shift);
                c.put32(control + 0x2c, c.i32(control + 0x2c) + center_shift);
            }
            continue;
        }
        const auto old = screen.rect(control + 0x10);
        auto box = old;
        if (old == ScreenRect{100,10,444,70}) box = teams.front();
        else if (old == ScreenRect{446,10,790,70}) box = teams.back();
        else if (old == ScreenRect{399,15,439,65} || old == ScreenRect{745,15,785,65}) {
            const int edge = old.left == 399 ? teams.front().right : teams.back().right;
            box = {edge-45,15,edge-5,65};
        } else if (old == ScreenRect{451,55,740,65}) box = {teams.back().left+5,55,teams.back().right-50,65};
        else if (zones && old == ScreenRect{110,125,235,145}) box = {diagram.left-10,125,diagram.left+115,145};
        else if (zones && old.left >= original_zones.left && old.right <= original_zones.right &&
                 old.top >= original_zones.top && old.bottom <= original_zones.bottom)
            box = place_zone_control(old, diagram);
        else if (live && old == ScreenRect{100,555,790,590})
            box = {title.left,lower.bottom+10,title.right,lower.bottom+45};
        else if (live && tab == 1 && old.left >= 150 && old.right <= 740 && old.top >= 415 && old.bottom <= 450)
            box = shift(old,center_shift,bottom_shift);
        else if (live && tab == 1 && old.left == 140 && old.right == 750 && old.height() == 70)
            box = {left+30,old.top+bottom_shift/2,right-30,old.bottom+bottom_shift/2};
        else if (live && tab == 8 && old == ScreenRect{155,125,630,160})
            box = {left+45,125,right-150,160};
        else if (live && tab == 8 && old.left >= 656 && old.right <= 775 && old.top == 130 && old.bottom == 150)
            box = shift(old,right-780,0);
        else if ((tab == 1 || (live && (tab == 8 || tab == 10))) && old == ScreenRect{110,125,780,160}) box = {left,125,right,160};
        else if (tab == 1 && old.top == 165 && old.bottom == 185 && old.left == 110)
            box = {left,165,left+old.width(),185};
        else if (tab == 1 && old.top == 165 && old.bottom == 185 && old.right == 780)
            box = {right-old.width(),165,right,185};
        else continue;
        screen.write(control + 0x10, box);
        c.put32(control + 0x28, box.left); c.put32(control + 0x2c, box.right);
        c.put32(control + 0x30, box.top); c.put32(control + 0x34, box.bottom);
    }
    for (unsigned i = 0, count = std::min<unsigned>(250, c.u16(display + 0x129da4)); i < count; ++i) {
        const auto grid = screen.grid(i);
        const auto old = screen.rect(grid);
        auto box = old;
        if (live && old == ScreenRect{0,0,89,599}) box.bottom = canvas.height-1;
        else if (live && (tab == 8 || tab == 10) && old == ScreenRect{110,165,780,500})
            box = {left,165,right,body.bottom};
        else if (live && tab == 1 && old == ScreenRect{110,406,780,460})
            box = {left,406+bottom_shift,right,460+bottom_shift};
        else if (live && tab == 1 && old.left == 110 && old.right == 780 && old.bottom == 405)
            box = {left,old.top,right,405+bottom_shift};
        else if (old == ScreenRect{100,80,790,115}) box = upper;
        else if (zones && old == ScreenRect{110,150,780,500})
            box = {diagram.left-10,150,diagram.right+10,body.bottom};
        else if (old == ScreenRect{100,510,790,545}) box = lower;
        else if ((old == ScreenRect{100,555,790,590} || old == ScreenLayout{canvas}.footer()) &&
                 screen.columns(grid) == 2 && screen.rows(grid) == 1) {
            box = footer;
            c.put8(grid + 0xbb2, 1); c.put8(grid + 0xbb3, 1);
        } else if (old == ScreenRect{110,125,780,500}) {
            box = body;
            // Two overlapping grids share the statistic columns. Only row
            // zero of the header and rows 1..20 of the player grid have cells.
            if (team_data && c.u32(grid + 0x18) == 1 && screen.rows(grid) == 21) {
                box.left += 4; box.right -= 4;
                const int header_bottom = body.top + 4 + std::clamp(body.height()/16, 24, 32);
                if (screen.columns(grid) == 18) {
                    box.top += 4;
                    box.bottom = header_bottom;
                    screen.set_rows(grid, 1);
                    screen.resize_rows(grid);
                } else if (screen.columns(grid) == 21) {
                    box.bottom -= 4;
                    const int first_player = header_bottom + 4;
                    box.top = first_player - (box.bottom - first_player)/20;
                    // Keep shirt numbers and card markers compact. Give their
                    // spare column weight to names so the first stat stays
                    // exactly aligned with the corresponding header cell.
                    for (const int column : {0,2}) {
                        const auto at = grid + 0xbb2 + column;
                        const int weight = c.u8(at);
                        const int compact = std::min(weight, (weight * old.width() + box.width()/2) / box.width());
                        c.put8(at, compact);
                        const auto name = grid + 0xbb2 + 4;
                        c.put8(name, c.u8(name) + weight - compact);
                    }
                }
            }
            // The archive builder emits both cells of each commentary row.
            // Retain its row numbers and colors while revealing more of the log.
            if (tab == 4 && screen.columns(grid) == 2 && screen.rows(grid) == 20 && c.u32(grid + 0x18) == 1) {
                // Font 2, as the builder; the same metric the list layouts read.
                const int line = c.i32(c.toc(-0x740c)) != -1 ? 18 : c.i32(c.toc(-0x3bdc) + 0x1404);
                const int visible = line > 0 ? std::clamp(body.height() / line, 20, 30) : 20;
                screen.show_rows(grid, 20, visible);
                screen.set_rows(grid, visible);
                screen.hide_scrollbar_if_fits(grid, visible);
                screen.resize_rows(grid);
                const int gutter = c.i32(grid + 0x14) + 1 > visible ? 22 : 0;
                // AutoGrid uses integer weights. Reserve about 80 logical
                // pixels for minutes and give the rest of the row to the play.
                c.put8(grid + 0xbb2, 10);
                c.put8(grid + 0xbb3, (body.width() - gutter + 4) / 8 - 10);
            }
        } else if (tab == 1 && old == ScreenRect{110,190,780,460}) box = {left,190,right,body.bottom-40};
        else if (tab == 1 && old == ScreenRect{110,465,780,500}) box = {left,body.bottom-35,right,body.bottom};
        else continue;
        screen.write(grid, box);
        screen.measure(grid);
        screen.publish(grid);
    }
}
}
