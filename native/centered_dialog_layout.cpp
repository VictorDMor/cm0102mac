#include "paged_screen_layout.hpp"

namespace cm::aot {
// A modal dialog the builder places at fixed coordinates inside the classic 800x600 window, such as the tournament setup
// of the friendly tour (builder 0x10046de0, opener 0x10046c80: a frame at 170,137-719,461, the option rows in a
// four-column grid and a Cancel/OK footer inside the frame). The dialog is small and self-contained, so on a larger canvas it
// is not stretched: the whole dialog keeps its size and its internal layout and moves to the middle of the canvas, every grid, standalone
// control and measured text area by the same offset, which is the canvas' extra width and height halved. It declines
// (classic geometry) a screen whose first grid is not a frame that holds all the others.
void layout_centered_dialog(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    const unsigned controls = std::min<unsigned>(1200, c.u16(display + 0x129da6));
    if (!grids) return;
    const auto frame = screen.rect(screen.grid(0));
    constexpr ScreenRect window{0, 0, 800, 600};
    if (frame.left < window.left || frame.top < window.top || frame.right > window.right || frame.bottom > window.bottom || frame.width() < 100 || frame.height() < 60) return;
    for (unsigned i = 1; i < grids; ++i) {
        const auto box = screen.rect(screen.grid(i));
        if (box.left < frame.left || box.top < frame.top || box.right > frame.right || box.bottom > frame.bottom) return;
    }
    const int dx = (canvas.width - 800) / 2, dy = (canvas.height - 600) / 2;
    if (!dx && !dy) return;

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    for (unsigned i = 0; i < controls; ++i) {
        const auto at = screen.control(i);
        // Measured text areas (x at +0x28/+0x2c, y at +0x30/+0x34) of every control, and the standalone controls' own rectangles.
        if (c.i32(at + 0x28) || c.i32(at + 0x2c)) { c.put32(at + 0x28, c.i32(at + 0x28) + dx); c.put32(at + 0x2c, c.i32(at + 0x2c) + dx); }
        if (c.i32(at + 0x30) || c.i32(at + 0x34)) { c.put32(at + 0x30, c.i32(at + 0x30) + dy); c.put32(at + 0x34, c.i32(at + 0x34) + dy); }
        if (c.i16(at + 0x72) < 0) {
            const auto box = screen.rect(at + 0x10);
            if (box.right > box.left || box.bottom > box.top) screen.write(at + 0x10, {box.left + dx, box.top + dy, box.right + dx, box.bottom + dy});
        }
    }
    for (unsigned i = 0; i < grids; ++i) {
        const auto grid = screen.grid(i);
        const auto box = screen.rect(grid);
        screen.write(grid, {box.left + dx, box.top + dy, box.right + dx, box.bottom + dy});
        screen.measure(grid);
        screen.publish(grid);
    }
}
}
