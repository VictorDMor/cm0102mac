#pragma once
#include "form_layout.hpp"
#include <span>
#include <vector>

namespace cm::aot {
// The Debugging Options screen: switches grouped under headings, one or two columns of groups, as many
// pages as the canvas needs, a utility row (All On / All Off and a pager) and a Cancel/OK footer.
// This is the pure geometry in the game's own coordinates (the right and bottom edges are what the
// game's control calls take); the drawing, the texts and the actions stay with match_debugging.cpp.
struct DebugOptionsMetrics {
    int row = 24, heading = 26, group_gap = 8, column_gap = 24;
    int minimum_two_columns = 960;  // the available width that gets two columns of groups
    int one_column_width = 640, two_column_width = 1000;
};
struct DebugOptionsPlacement {
    unsigned group;
    ScreenRect heading, grid;  // the grid holds the group's rows, `rows_per_group[group]` of them
};
struct DebugOptionsLayout {
    ScreenRect background, panel, title, body;
    ScreenRect all_on, all_off;                // the left cluster, two cells, as one rectangle each
    ScreenRect previous, page, next;           // empty (all zero) when there is a single page
    ScreenRect footer;                         // Cancel and OK, two equal cells in one rectangle
    unsigned columns = 1, pages = 1;
    std::vector<std::vector<DebugOptionsPlacement>> placements;  // by page
};
DebugOptionsLayout debug_options_layout(GuiCanvas canvas, std::span<const unsigned> rows_per_group,
                                        DebugOptionsMetrics metrics = {});
}
