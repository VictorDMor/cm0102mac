#pragma once
#include "presentation.hpp"
#include <algorithm>

namespace cm::aot {
// Original margins, font sizes and control styles; extra width reveals up to
// three existing player views beside the selected view.
struct TacticsLayout {
    GuiCanvas canvas;
    // Fit the original 330:400 pitch into the available space without
    // stretching it vertically on taller windows.
    int pitch_width() const { return std::min(canvas.width - 470, (canvas.height - 200) * 330 / 400); }
    int pitch_growth() const { return pitch_width() - 330; }
    int extra_width() const { return canvas.width - 800 - pitch_growth(); }
    int columns() const { return std::min(3, extra_width() / 160); }
    int list_right() const { return 440 + extra_width() - columns() * 160; }
    int pitch_left() const { return 450 + extra_width(); }
    int pitch_right() const { return canvas.width - 20; }
    int pitch_bottom() const { return 145 + pitch_width() * 400 / 330; }
    int list_bottom() const { return canvas.height - 55; }
    int footer_left() const { return canvas.width - 700; }
};
}
