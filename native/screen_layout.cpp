#include "screen_layout.hpp"
#include "adaptive_gui.hpp"
#include "gui_guest_scope.hpp"
#include "form_layout.hpp"
#include <array>
#include <span>

namespace cm::aot {
namespace {
struct GridAnchor { int left, right; LayoutAxis horizontal; };
struct ControlAnchor {
    ScreenRect original;
    LayoutAxis horizontal;
    bool any_top = false;
    LayoutAxis vertical = LayoutAxis::fixed;
};

// Shared table panels. Popup grids keep their original links and bounds;
// their placement is calculated when the original opens them.
constexpr GridAnchor club_grids[]{
    {100, 790, LayoutAxis::stretch},      // section tabs
    {110, 780, LayoutAxis::stretch},      // roster, non-scrolling headings
    {110, 758, LayoutAxis::stretch},      // heading with 22px scrollbar gutter
};
constexpr GridAnchor squad_grids[]{
    {530, 780, LayoutAxis::trailing},     // Clear Team / Filter
    {558, 780, LayoutAxis::trailing},     // same toolbar with Lineup present
    {334, 556, LayoutAxis::centered},     // Lineup
    {334, 444, LayoutAxis::centered},     // Lineup with Debugging
    {446, 556, LayoutAxis::centered},     // Debugging
};
constexpr ControlAnchor club_controls[]{
    {{110, 0, 780, 35}, LayoutAxis::stretch, true}, // roster heading
    {{110, 0, 780, 20}, LayoutAxis::stretch, true}, // optional bottom note
    {{710, 15, 785, 35}, LayoutAxis::trailing}, // Print
    {{660, 4, 785, 24}, LayoutAxis::trailing},  // contextual club action
    {{620, 4, 785, 24}, LayoutAxis::trailing},  // Force Results
};
constexpr ControlAnchor transfer_controls[]{
    {{655, 0, 780, 20}, LayoutAxis::trailing, true}, // View menu
};
constexpr GridAnchor calendar_grids[]{
    {530, 780, LayoutAxis::trailing}, // Arrange / Cancel
};
constexpr ControlAnchor calendar_controls[]{
    {{0, 0, 799, 599}, LayoutAxis::stretch, false, LayoutAxis::stretch}, // Cancel hit area
};
constexpr GridAnchor calendar_cells[]{
    {110, 758, LayoutAxis::stretch}, // explicit full-row fixture/action hit areas
};
constexpr GridAnchor search_grids[]{
    {530, 780, LayoutAxis::trailing}, // Filter / Clear
};
constexpr ControlAnchor search_controls[]{
    {{100, 10, 790, 70}, LayoutAxis::stretch}, // search title
    {{0, 0, 799, 599}, LayoutAxis::stretch, false, LayoutAxis::stretch}, // selection dismissal
};
struct TableRecipe {
    std::span<const GridAnchor> grids;
    std::span<const ControlAnchor> controls;
    std::span<const GridAnchor> cells;
    bool search_report = false;
    bool search_shell = false;
};
void apply_recipe(State& s, std::uint32_t display, const ScreenLayout& layout, TableRecipe recipe) {
    GuiGuestScope preserve(s);
    guest::Cpu c(s);
    // AutoGrid writes its caller's linkage area before allocating its frame.
    // Keep those extra calls below the original caller's stack pointer too.
    c.set(1, c.sp() - 0x400);
    const auto read_rect = [&](std::uint32_t at) {
        return ScreenRect{c.i32(at), c.i32(at + 4), c.i32(at + 8), c.i32(at + 12)};
    };
    const auto write_rect = [&](std::uint32_t at, ScreenRect rect) {
        c.put32(at, rect.left); c.put32(at + 4, rect.top);
        c.put32(at + 8, rect.right); c.put32(at + 12, rect.bottom);
    };
    const auto grid_rule = [&](ScreenRect rect) -> const GridAnchor* {
        for (const auto rules : {std::span<const GridAnchor>{club_grids}, recipe.grids})
            for (const auto& rule : rules) if (rect.left == rule.left && rect.right == rule.right) return &rule;
        return nullptr;
    };
    int search_description_top = -1;
    if (recipe.search_shell) {
        for (unsigned i = 0, count = std::min<unsigned>(250, c.u16(display + 0x129da4)); i < count; ++i) {
            const auto area = display + 4 + i * 0xbf0;
            if (c.i32(area) == 110 && c.i32(area + 8) == 485 && c.u8(area + 0xbb0) == 3)
                search_description_top = c.i32(area + 12) + 5;
        }
    }
    // Some calendar actions span a whole row with explicit X overrides.
    // AutoGrid recomputes their Y from the row but honors these X coordinates.
    for (unsigned i = 0, count = std::min<unsigned>(1200, c.u16(display + 0x129da6)); i < count; ++i) {
        const auto control = display + 0xba864 + i * 0x17c;
        if (c.i16(control + 0x72) < 0) continue;
        for (const auto& rule : recipe.cells) {
            if (c.i32(control + 0x28) != rule.left || c.i32(control + 0x2c) != rule.right) continue;
            const auto bounds = layout.anchor({rule.left,0,rule.right,0}, rule.horizontal, LayoutAxis::fixed);
            c.put32(control + 0x28, bounds.left); c.put32(control + 0x2c, bounds.right);
            break;
        }
    }
    for (unsigned i = 0, count = std::min<unsigned>(250, c.u16(display + 0x129da4)); i < count; ++i) {
        const auto area = display + 4 + i * 0xbf0;
        const auto original = read_rect(area);
        // A scout's club assignment uses a linked description grid. Its
        // right edge depends on the pager, so it has no fixed-width recipe.
        const bool description = recipe.search_shell && search_description_top >= 0 &&
            original.left == 110 && original.right <= 780 && original.top == search_description_top &&
            original.height() == 20 && c.u8(area + 0xbb0) == 1 && c.u8(area + 0xbb1) == 1;
        const auto* rule = grid_rule(original);
        if (rule || description) {
            const bool report = recipe.search_report && original.left == 110 && original.right == 780 &&
                                c.u8(area + 0xbb0) == 13;
            int report_left = 0, report_right = 0;
            if (report) {
                c.set(3, area); c.call(0x10714630, 0xfffffff0);
                report_left = c.i32(area + 0x1c + 7 * 4);
                report_right = c.i32(area + 0x10c + 7 * 4);
            }
            // Some builders emit the footer directly rather than using the
            // common footer helper. Already positioned footers stay intact.
            // At height 645 the lower search tabs have this same rectangle;
            // only the two-button footer owns a two-column, one-row grid.
            const auto vertical = recipe.search_shell && original == ScreenRect{100,555,790,590} &&
                                  c.u8(area + 0xbb0) == 2 && c.u8(area + 0xbb1) == 1
                ? LayoutAxis::trailing : LayoutAxis::fixed;
            const auto adjusted = layout.anchor(original, description ? LayoutAxis::stretch : rule->horizontal, vertical);
            write_rect(area, adjusted);
            // Let the existing grid publish identical drawing/hit rectangles
            // to its child controls, preserving its data and scroll position.
            c.set(3, area); c.call(0x10714630, 0xfffffff0);
            if (report || description) {
                const int left_delta = description ? (adjusted.left + adjusted.right - original.left - original.right) / 2
                    : c.i32(area + 0x1c + 7 * 4) - report_left;
                const int right_delta = description ? left_delta : c.i32(area + 0x10c + 7 * 4) - report_right;
                for (unsigned j = 0, controls = std::min<unsigned>(1200, c.u16(display + 0x129da6)); j < controls; ++j) {
                    const auto control = display + 0xba864 + j * 0x17c;
                    if (c.i16(control + 0x72) != int(i)) continue;
                    const int left = c.i32(control + 0x28), right = c.i32(control + 0x2c);
                    if (left <= 0 || right <= 0) continue;
                    // The original report starts 5px before column seven.
                    // Its full text grows with that column; club links keep
                    // their measured widths and the same left alignment.
                    // Scout descriptions instead retain their measured text
                    // and link widths, moving together around the new center.
                    const bool base = report && !c.u32(control + 8) && left == 500 && right == 710;
                    c.put32(control + 0x28, left + left_delta);
                    c.put32(control + 0x2c, right + (base ? right_delta : left_delta));
                }
            }
            c.set({{3, area}, {4, 0}}); c.call(0x10714110, 0xfffffff0);
        }
    }
    for (unsigned i = 0, count = std::min<unsigned>(1200, c.u16(display + 0x129da6)); i < count; ++i) {
        const auto control = display + 0xba864 + i * 0x17c;
        // Grid-owned controls were already positioned above. A resized cell
        // can happen to match a legacy standalone rectangle at narrow widths.
        if (c.i16(control + 0x72) >= 0) continue;
        const auto bounds = control + 0x10;
        const auto original = read_rect(bounds);
        bool matched = false;
        for (const auto rules : {std::span<const ControlAnchor>{club_controls}, recipe.controls}) {
            for (const auto& rule : rules) if (original == rule.original ||
                (rule.any_top && original.left == rule.original.left && original.right == rule.original.right &&
                 original.height() == rule.original.height())) {
                write_rect(bounds, layout.anchor(original, rule.horizontal, rule.vertical));
                matched = true; break;
            }
            if (matched) break;
        }
    }
}
void apply_search_controls(State& s, std::uint32_t display, const ScreenLayout& layout) {
    guest::Cpu c(s);
    int pager_top = -1;
    for (unsigned i = 0, count = std::min<unsigned>(250, c.u16(display + 0x129da4)); i < count; ++i) {
        const auto area = display + 4 + i * 0xbf0;
        // The leading toolbar stays fixed even when the tabs wrap to a
        // second row; its bottom defines the adjacent pagination row.
        if (c.i32(area) == 110 && c.i32(area + 8) == 485 && c.u8(area + 0xbb0) == 3)
            pager_top = c.i32(area + 12) + 5;
    }
    for (unsigned i = 0, count = std::min<unsigned>(1200, c.u16(display + 0x129da6)); i < count; ++i) {
        const auto control = display + 0xba864 + i * 0x17c;
        if (c.i16(control + 0x72) >= 0) continue;
        const ScreenRect before{c.i32(control + 0x10), c.i32(control + 0x14),
                                c.i32(control + 0x18), c.i32(control + 0x1c)};
        const bool funds = before.right == 785 && before.top == 15 && before.bottom == 35;
        const bool pagination = pager_top >= 0 && before.top == pager_top && before.height() == 20 &&
                                before.left >= 110 && before.right <= 780;
        if (!funds && !pagination) continue;
        const auto horizontal = pagination && before.left == 110 ? LayoutAxis::stretch : LayoutAxis::trailing;
        const auto after = layout.anchor(before, horizontal, LayoutAxis::fixed);
        c.put32(control + 0x10, after.left); c.put32(control + 0x18, after.right);
    }
}
void apply_player_details(State& s, std::uint32_t display, const ScreenLayout& screen) {
    GuiGuestScope preserve(s);
    guest::Cpu c(s); c.set(1, c.sp() - 0x400);
    const DetailsLayout layout{screen};
    const auto history = c.u32(c.toc(-0x7bb0) + 0x10);
    const auto section = c.u32(history + 0x14 + 8);
    const bool playing_attributes = section == 1, player_history = section == 5;
    const auto history_view = c.u32(history + 0x14 + 5 * 8);
    const unsigned body_columns = !player_history ? (playing_attributes ? 6 : 2) :
        history_view == 2 ? 14 : history_view == 4 ? 4 : history_view == 16 ? 2 : 3;
    const auto read_rect = [&](std::uint32_t at) {
        return ScreenRect{c.i32(at), c.i32(at + 4), c.i32(at + 8), c.i32(at + 12)};
    };
    const auto write_rect = [&](std::uint32_t at, ScreenRect rect) {
        c.put32(at, rect.left); c.put32(at + 4, rect.top);
        c.put32(at + 8, rect.right); c.put32(at + 12, rect.bottom);
    };
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    unsigned body = grids, summary = grids;
    for (unsigned i = 0; i < grids; ++i) {
        const auto at = display + 4 + i * 0xbf0;
        if (c.i32(at) != 110 || c.i32(at + 8) != 780) continue;
        if (c.u8(at + 0xbb0) == body_columns) body = i;
        if (c.u8(at + 0xbb0) == 12 && c.u8(at + 0xbb1) == 7) summary = i;
    }
    if (body == grids || summary == grids) return;
    const auto body_area = display + 4 + body * 0xbf0;
    const auto body_rect = read_rect(body_area);
    const auto summary_rect = read_rect(display + 4 + summary * 0xbf0);
    const unsigned body_rows = c.u8(body_area + 0xbb1), total_rows = body_rows + 7;
    for (unsigned i = 0; i < grids; ++i) {
        const auto area = display + 4 + i * 0xbf0;
        const auto before = read_rect(area);
        const bool career_header = player_history && history_view == 2 &&
            c.u8(area + 0xbb0) == 12 && before.left == 110 && before.bottom == body_rect.top &&
            (before.right == 780 || before.right == 760);
        if (!((before.left == 100 && before.right == 790) || (before.left == 110 && before.right == 780) ||
              career_header)) continue;
        auto after = screen.anchor(before, LayoutAxis::stretch, LayoutAxis::fixed);
        if (i == body) after = layout.rows(before, 0, body_rows, total_rows);
        else if (before == summary_rect) after = layout.rows(before, body_rows, 7, total_rows);
        int previous_value_start = 0;
        const unsigned text_column = player_history ? 2 : 1;
        if (i == body && !playing_attributes && (!player_history || history_view == 1)) {
            // Details and achievement text use explicit left edges relative
            // to their value column, including its scrollbar gutter.
            c.set(3, area); c.call(0x10714630, 0xfffffff0);
            previous_value_start = before.left + c.i32(area + 0x1c + text_column * 4);
        }
        write_rect(area, after);
        c.set(3, area); c.call(0x10714630, 0xfffffff0);
        // The title, biography and loan message contain individually clickable
        // rich-text fragments with explicit X overrides. Keep each fragment's
        // width and move the whole group with its original alignment.
        for (unsigned j = 0; j < controls; ++j) {
            const auto control = display + 0xba864 + j * 0x17c;
            if (c.i16(control + 0x72) != int(i)) continue;
            auto text = ScreenRect{c.i32(control + 0x28), 0, c.i32(control + 0x2c), 0};
            if (text.left <= 0 || text.right <= 0) continue;
            if (i == body && playing_attributes) {
                const auto left = after.left + c.i32(area + 0x1c + 4 * 4);
                const auto right = after.left + c.i32(area + 0x10c + 5 * 4);
                if (text.left == 558 && text.right == 780) text = {left,0,right,0};
                else text = DetailsLayout::centered_text(text, {568,0,780,0}, {left+10,0,right,0});
            } else if (i == body && (!player_history || history_view == 1)) {
                const int shift = after.left + c.i32(area + 0x1c + text_column * 4) - previous_value_start;
                // Achievements retain their original wrapping and explicit
                // 22px text gutter; links keep their individual widths.
                const int text_right = player_history && text.right == before.right - 22
                    ? before.right - 22 : before.right;
                const int text_left = player_history ? before.left + (text_right - before.left) * 8 / 23 : 258;
                const bool full_value = !c.u32(control + 8) && text.left == text_left && text.right == text_right;
                text.left += shift;
                text.right = full_value ? after.right - (before.right - text_right) : text.right + shift;
            } else text = DetailsLayout::centered_text(text, before, after);
            c.put32(control + 0x28, text.left); c.put32(control + 0x2c, text.right);
        }
        c.set({{3, area}, {4, 0}}); c.call(0x10714110, 0xfffffff0);
    }
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = display + 0xba864 + i * 0x17c;
        if (c.i16(control + 0x72) >= 0) continue;
        const auto before = read_rect(control + 0x10);
        if (before == ScreenRect{660,4,785,24})
            write_rect(control + 0x10, screen.anchor(before, LayoutAxis::trailing, LayoutAxis::fixed));
        else if (player_history && history_view == 1 && before.top == body_rect.top - 60 &&
                 before.height() == 20 && before.left >= 360 && before.right <= 780)
            // The original >100-event pager is three standalone controls,
            // including its counter and disabled arrows with action zero.
            write_rect(control + 0x10, screen.anchor(before, LayoutAxis::trailing, LayoutAxis::fixed));
        else if (before.left == 110 && before.right == 780 && before.top == summary_rect.bottom + 5 && before.height() == 30)
            write_rect(control + 0x10, screen.anchor(before, LayoutAxis::stretch, LayoutAxis::trailing));
        else if (player_history && before.left == 110 && before.right == 780 &&
                 before.height() == 30 && before.bottom + 5 <= body_rect.top)
            write_rect(control + 0x10, screen.anchor(before, LayoutAxis::stretch, LayoutAxis::fixed));
    }
}
}
void layout_built_screen(State& s, std::uint32_t display, std::uint32_t builder) {
    const ScreenLayout layout{gui_canvas(s)};
    if (layout.canvas.original() || !adaptive_screen_available(s, builder)) return;
    switch (screen_template(builder)) {
    case ScreenTemplate::startup: layout_startup(s, display, layout.canvas); break;
    // Season controls and their layout are applied together after this pass.
    case ScreenTemplate::league_selection: break;
    case ScreenTemplate::load_browser: layout_load_browser(s, display, layout.canvas); break;
    case ScreenTemplate::club_tables:
        switch (adaptive_club_section(s)) {
        case 1: apply_recipe(s, display, layout, {squad_grids, {}, {}}); break;
        case 2: apply_recipe(s, display, layout, {{}, transfer_controls, {}}); break;
        case 4: apply_recipe(s, display, layout, {calendar_grids, calendar_controls, calendar_cells}); break;
        case 3: layout_next_match(s, display, layout.canvas); break;
        case 5: layout_club_information(s, display, layout.canvas); break;
        }
        break;
    case ScreenTemplate::player_details: apply_player_details(s, display, layout); break;
    case ScreenTemplate::search_filters: layout_search_filters(s, display, layout.canvas); break;
    case ScreenTemplate::transfer_proposal: layout_transfer_proposal(s, display, layout.canvas); break;
    case ScreenTemplate::contract_negotiation: layout_contract_negotiation(s, display, layout.canvas); break;
    case ScreenTemplate::loan_terms: layout_negotiation_dialog(s, display, layout.canvas, true); break;
    case ScreenTemplate::negotiation_confirmation: layout_negotiation_dialog(s, display, layout.canvas, false); break;
    case ScreenTemplate::news_inbox: layout_news_inbox(s, display, layout.canvas); break;
    case ScreenTemplate::league_standings: layout_league_standings(s, display, layout.canvas); break;
    case ScreenTemplate::daily_fixtures: layout_daily_fixtures(s, display, layout.canvas); break;
    case ScreenTemplate::nations_and_clubs: layout_nations_and_clubs(s, display, layout.canvas); break;
    case ScreenTemplate::search_tables: {
        guest::Cpu c(s);
        const auto history = c.u32(c.toc(-0x7bb0) + 0x10), engine = c.u32(history + 0x14);
        const auto settings = engine + 4 + c.u8(engine) * 0x2b0 + 0x29c;
        const bool report = (c.u32(settings) & 0x207f) == 0x2000;
        apply_recipe(s, display, layout, {search_grids, search_controls, {}, report, true});
        apply_search_controls(s, display, layout);
        break;
    }
    case ScreenTemplate::legacy: case ScreenTemplate::tactics: break;
    }
}
}
