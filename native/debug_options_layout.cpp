#include "debug_options_layout.hpp"
#include <algorithm>
#include <array>

namespace cm::aot {
namespace {
struct Slot { unsigned page, column; int top; DebugOptionsPlacement placement; };
}

DebugOptionsLayout debug_options_layout(GuiCanvas canvas, std::span<const unsigned> rows_per_group,
                                        DebugOptionsMetrics m) {
    DebugOptionsLayout out;
    out.background = {0, 0, canvas.width - 1, canvas.height - 1};
    // The area right of the career menu and under the screen's top edge, as the other dialogs use.
    const ScreenRect area{110, 30, canvas.width - 20, canvas.height - 30};
    out.columns = area.width() >= m.minimum_two_columns ? 2 : 1;
    const int panel_width = std::min(area.width(), out.columns == 2 ? m.two_column_width : m.one_column_width);
    constexpr int panel_height_limit = 660;
    const int panel_height = std::min(area.height(), panel_height_limit);
    const std::array<FormTrack, 3> across{{{0, 1}, {panel_width}, {0, 1}}};
    const std::array<FormTrack, 3> down{{{0, 1}, {panel_height}, {0, 1}}};
    const auto sides = form_tracks(area, across, true);
    const auto bands = form_tracks(area, down, false);
    out.panel = {sides[1].left, bands[1].top, sides[1].right, bands[1].bottom};

    // Title, body and the two footer rows, inside a 10px margin.
    constexpr int title_height = 45, button_height = 30, margin = 10, row_gap = 6;
    const std::array<FormTrack, 3> inset{{{margin}, {0, 1}, {margin}}};
    const auto inner_columns = form_tracks(out.panel, inset, true);
    const ScreenRect inner{inner_columns[1].left, out.panel.top, inner_columns[1].right, out.panel.bottom};
    const std::array<FormTrack, 9> stack{{{margin}, {title_height}, {margin}, {m.heading + m.row, 1}, {margin},
        {button_height}, {row_gap}, {button_height}, {margin}}};
    const auto rows = form_tracks(inner, stack, false);
    out.title = rows[1];
    out.body = rows[3];

    // Utility row: All On and All Off on the left, the pager on the right.
    constexpr int button_width = 120, page_width = 100, gap = 6;
    const std::array<FormTrack, 7> utility{{{button_width}, {gap}, {button_width}, {0, 1}, {button_width - 10}, {page_width},
        {button_width - 10}}};
    const auto cells = form_tracks(rows[5], utility, true);
    out.all_on = cells[0];
    out.all_off = cells[2];
    // Cancel and OK are equal cells at the right.
    constexpr int footer_width = 300;
    const std::array<FormTrack, 2> foot{{{0, 1}, {footer_width}}};
    out.footer = form_tracks(rows[7], foot, true)[1];

    // Pack the groups into columns and pages, in order; a group is never split.
    const auto height = [&](unsigned group) { return m.heading + int(rows_per_group[group]) * m.row; };
    std::vector<Slot> slots;
    unsigned page = 0, column = 0;
    int used = 0;
    for (unsigned group = 0; group < rows_per_group.size(); ++group) {
        const int needed = height(group) + (used ? m.group_gap : 0);
        if (used && used + needed > out.body.height()) {
            used = 0;
            if (++column == out.columns) { column = 0; ++page; }
        }
        slots.push_back({page, column, used, {group, {}, {}}});
        used += height(group) + (used ? m.group_gap : 0);
    }
    out.pages = slots.empty() ? 1 : slots.back().page + 1;
    out.placements.resize(out.pages);
    // Yoga stacks each column's headings, grids and gaps and divides the body into columns.
    const std::array<FormTrack, 3> pair{{{0, 1}, {m.column_gap}, {0, 1}}};
    const auto lanes = out.columns == 2 ? form_tracks(out.body, pair, true)
                                        : std::vector<ScreenRect>{out.body};
    for (unsigned p = 0; p < out.pages; ++p) {
        for (unsigned c = 0; c < out.columns; ++c) {
            std::vector<unsigned> members;
            std::vector<FormTrack> tracks;
            for (unsigned i = 0; i < slots.size(); ++i) {
                if (slots[i].page != p || slots[i].column != c) continue;
                if (!members.empty()) tracks.push_back({m.group_gap});
                members.push_back(i);
                tracks.push_back({m.heading});
                tracks.push_back({int(rows_per_group[slots[i].placement.group]) * m.row});
            }
            if (members.empty()) continue;
            const ScreenRect lane = lanes[c == 1 ? 2 : 0];
            const auto boxes = form_tracks(lane, tracks, false);
            unsigned at = 0;
            for (const auto member : members) {
                if (at && at + 1 < boxes.size()) ++at;  // skip the gap before every group but the first
                auto placement = slots[member].placement;
                placement.heading = boxes[at];
                placement.grid = boxes[at + 1];
                out.placements[p].push_back(placement);
                at += 2;
            }
        }
    }
    if (out.pages > 1) {
        out.previous = cells[4];
        out.page = cells[5];
        out.next = cells[6];
    }
    return out;
}
}
