#pragma once
#include "screen_layout.hpp"
#include <algorithm>
#include <array>

namespace cm::aot {
// Original margins, font sizes and control styles. Extra space first keeps
// names readable, then reveals existing player views beside the selected one.
struct TacticsLayout {
    GuiCanvas canvas;
    unsigned selected_view = 1;
    bool during_match = false;
    ScreenLayout shell() const { return {canvas}; }
    ScreenRect pitch() const { return shell().proportional_right(145, 330, 400, 330, 330, 42); }
    int pitch_width() const { return pitch().width(); }
    int pitch_growth() const { return pitch_width() - 330; }
    int extra_width() const { return canvas.width - 800 - pitch_growth(); }
    int pitch_left() const { return pitch().left; }
    int pitch_right() const { return pitch().right; }
    int pitch_bottom() const { return pitch().bottom; }
    int list_bottom() const { return shell().content().bottom; }
    static constexpr int column_width(unsigned view) {
        return view == 5 ? 160 : view == 3 ? 96 : 128;
    }
    std::array<unsigned, 3> column_views() const {
        std::array<unsigned, 3> result{};
        int available = pitch_left() - 10 - 110 - 400;
        unsigned count = 0;
        for (const unsigned view : {1u, 5u, 3u, 4u}) {
            if (view == selected_view || (during_match && view != 1)) continue;
            if (count == result.size() || column_width(view) > available) break;
            result[count++] = view;
            available -= column_width(view);
        }
        return result;
    }
    int columns() const {
        const auto views = column_views();
        return int(std::count_if(views.begin(), views.end(), [](unsigned view) { return view != 0; }));
    }
    int list_right() const {
        auto right = pitch_left() - 10;
        for (const auto view : column_views()) if (view) right -= column_width(view);
        return right;
    }
    int footer_left() const { return shell().buttons().left; }
    int footer_right() const { return shell().buttons().right; }
};
}
