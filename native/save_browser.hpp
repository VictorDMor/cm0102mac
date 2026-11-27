#pragma once
// The saved-game browser (Load, Save and Delete of a saved career): a list of the
// saves, most recently saved first, and a panel with the selected save's summary.
// It replaces the classic dialog's grids when New Screens is on; the dialog's
// state, its name buffer, confirmations and file actions stay the original's.
#include "file_screens.hpp"
#include "screen_layout.hpp"

namespace cm::aot::file_screens {
using guest::Frame;
// The action code of a row cell: the handler copies the row's save name into the
// dialog's name buffer, as the classic row's click does.
inline constexpr std::int32_t saved_game_pick_action = 30301;

// Where the browser's parts go on a canvas, in logical pixels. Wide canvases put the
// panel right of the list and the buttons in and under it; the classic 800x600 canvas
// (and any canvas too narrow for two columns) stacks the panel under a shorter list,
// with Cancel and the primary button side by side in the footer.
struct SavedGameGeometry {
    bool compact = false;
    int lines_per_save = 1;         // table rows one save takes in the list
    ScreenRect field{}, list{}, panel{}, cancel{}, primary{};
    int row_height = 0, visible_rows = 0;     // list table rows, a whole number of saves
    int detail_row_height = 0, detail_rows = 0;
};
inline SavedGameGeometry saved_game_geometry(GuiCanvas canvas, int line_height, bool name_field) {
    SavedGameGeometry g;
    const ScreenLayout shell{canvas};
    const auto content = shell.content();
    const auto footer = shell.footer();
    constexpr int gap = 12, field_height = 36;
    g.compact = content.width() < 900;
    g.lines_per_save = g.compact ? 2 : 1;
    int top = content.top;
    if (name_field) {
        g.field = {content.left, top, g.compact ? content.right : content.left + content.width() * 56 / 100, top + field_height};
        top += field_height + 8;
    }
    g.cancel = {content.left, footer.top, content.left + 170, footer.bottom};
    if (g.compact) {
        g.row_height = std::max(26, line_height + 5);
        // Four saves, or three beside the name field, then the panel takes the rest.
        const int saves = name_field ? 3 : 4;
        g.list = {content.left, top, content.right, top + saves * 2 * g.row_height + 4};
        g.panel = {content.left, g.list.bottom + 8, content.right, content.bottom};
        g.primary = {content.right - 300, footer.top, content.right, footer.bottom};
        g.detail_row_height = std::max(22, line_height + 2);
    } else {
        g.row_height = std::max(34, line_height + 10);
        const int list_width = std::clamp(content.width() * 56 / 100, 520, std::max(520, content.width() - gap - 420));
        g.list = {content.left, top, content.left + list_width, content.bottom};
        if (name_field) g.field.right = g.list.right;
        g.panel = {g.list.right + gap, content.top, content.right, content.bottom};
        g.primary = {g.panel.left + 10, g.panel.bottom - 56, g.panel.right - 10, g.panel.bottom - 10};
        g.detail_row_height = std::max(32, line_height + 10);
    }
    const int rows = std::max(1, (g.list.height() - 4) / g.row_height);
    g.visible_rows = std::max(g.lines_per_save, rows / g.lines_per_save * g.lines_per_save);
    const int detail_bottom = g.compact ? g.panel.bottom - 4 : g.primary.top - 10;
    g.detail_rows = std::max(1, (detail_bottom - g.panel.top - 8) / g.detail_row_height);
    return g;
}

// Whether the dialog being built is the saved-career chooser this browser replaces:
// New Screens on, a running platform, and the gate of the classic chooser's layout.
bool saved_game_browser(Cpu&);

// What the classic builder holds when it reaches its first grid.
struct SavedGameBrowserInput {
    std::uint32_t screens, label, pool, folder, extension, filter, name_buffer;
};
// Builds the browser in place of the classic grids and records the host-side
// state the pick handler reads (the names the rows stand for).
void build_saved_game_browser(Cpu&, const Frame&, const SavedGameBrowserInput&);
// The name a row cell stands for, copied into `scratch` (31 bytes at least);
// zero when `control` is not one of the browser's current row cells.
std::uint32_t saved_game_pick_name(Cpu&, std::uint32_t control, std::uint32_t scratch);
}
