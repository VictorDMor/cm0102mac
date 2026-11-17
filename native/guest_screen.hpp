#pragma once
#include "guest_abi.hpp"
#include "screen_layout.hpp"
#include <algorithm>
#include <cstdint>

namespace cm::aot {
// DISPLAY's screen object as the layouts see it: its AutoGrid areas, the
// controls they own, and the original routines that recompute and publish an
// area after its rectangle or row count changed. Calls run on the caller's
// guest stack, inside the caller's GuiGuestScope.
struct GuestScreen {
    guest::Cpu& c;
    std::uint32_t display;

    static constexpr std::uint32_t grid_size = 0xbf0, control_size = 0x17c;
    std::uint32_t grid(unsigned index) const { return display + 4 + index * grid_size; }
    std::uint32_t control(unsigned index) const { return display + 0xba864 + index * control_size; }

    ScreenRect rect(std::uint32_t at) const { return {c.i32(at), c.i32(at + 4), c.i32(at + 8), c.i32(at + 12)}; }
    void write(std::uint32_t at, ScreenRect box) const {
        c.put32(at, box.left); c.put32(at + 4, box.top);
        c.put32(at + 8, box.right); c.put32(at + 12, box.bottom);
    }

    std::uint8_t columns(std::uint32_t area) const { return c.u8(area + 0xbb0); }
    std::uint8_t rows(std::uint32_t area) const { return c.u8(area + 0xbb1); }
    void set_rows(std::uint32_t area, unsigned rows) const { c.put8(area + 0xbb1, rows); }
    // Rows the builder did not create start hidden; mark [first, last) shown.
    void show_rows(std::uint32_t area, int first, int last) const {
        for (int row = first; row < last; ++row) c.put8(area + 0xbd0 + row, 1);
    }
    // The area keeps a scrollbar flag from its original row count.
    void hide_scrollbar_if_fits(std::uint32_t area, int rows) const {
        if (c.i32(area + 0x14) + 1 <= rows) c.put8(area + 0xbef, 0);
    }
    // 0x107145f0: rebuild the area's row table for its current row count.
    void resize_rows(std::uint32_t area) const {
        c.set({{3, area}, {4, c.u32(area + 0x1fc)}}); c.call(0x107145f0, 0xfffffff0);
    }
    // 0x10714630: recompute cell geometry from the area's rectangle.
    void measure(std::uint32_t area) const { c.set(3, area); c.call(0x10714630, 0xfffffff0); }
    // 0x10714110: publish the area's cells to its controls.
    void publish(std::uint32_t area) const { c.set({{3, area}, {4, 0}}); c.call(0x10714110, 0xfffffff0); }
};
}
