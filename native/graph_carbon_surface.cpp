// graph_carbon.cpp, block `surface` (0x10768820, 0x10768990, 0x107689e0, 0x10768b40, 0x10768c50, 0x10768c90, 0x10768dc0, 0x10768e10,
// 0x107690f0, 0x107691b0, 0x107692f0, 0x1076e7c0, 0x1076e870, 0x1076e920, 0x1076e9d0; 908 instructions): the game's drawing surface and
// its window. The surface is a 15-bit off-screen GWorld (0x10768c90) shown in one 800x600 window or in a full screen window
// (0x107689e0, 0x10768e10), a rectangle of it is put on the window with CopyBits (0x107692f0), the window's position is a resource
// 'WinP' (0x107690f0 reads it, 0x107691b0 writes it on close), and the three cursors are resources (0x1076e7c0, 0x1076e870, 0x1076e920).
// All of it is Toolbox: every glue call below is made with `c.call` at the return address of the original's `bl` (the import observer
// sees the same arguments, the same link register and the same held registers), and the TOC is reloaded from the frame after each one
// as the glue leaves it.
//
// Registers: every routine leaves r3 and r4-r10 as the original does (the checker compares them in full, nothing is exempt): the code
// sets what the original sets before each call, in the original's order, and the registers a callee leaves stay in place. The
// non-volatile registers hold what the original holds at each call (a callee saves them in its frame, and the import log shows them).
// Each routine builds the original's frame. 0x107692f0 leaves its frame for MAIN's printf (graph_carbon_stack_exact.inc).
#include "graph_carbon_surface_detail.hpp"

namespace cm::aot::graph_carbon {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::extended;

// `extsb`, `extsh`: a byte or a halfword as the original holds it in a register.
std::int32_t byte_of(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }
std::uint32_t half_of(std::int32_t value) { return extended(std::int16_t(value)); }
// `lwz r2,0x14(r1)`: after a call into the glue the original reloads the TOC pointer the glue saved in its frame's linkage area.
void restore_toc(Cpu& c) { c.set(2, c.u32(c.sp() + 0x14)); }
// A glue call at the original's `bl` (the return address is the address after it).
void glue(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    c.call(entry, back);
    restore_toc(c);
}

// The assertion the original raises with the pool strings of this module (guest::report_assertion): `at` is the address of its first
// `bl`. The callers hold the pool and the message buffer in non-volatile registers first.
void assert_here(Cpu& c, std::uint32_t at, std::uint32_t name, std::uint32_t extension, std::uint32_t line) {
    const auto pool = c.toc(surface_toc::pool);
    guest::report_assertion(c, {at, pool, pool + surface_pool::format, pool + surface_pool::title, name, extension, line});
}
// The original's way of centring the window in `area` (a Mac Rect: top, left, bottom, right halfwords at +0, +2, +4, +6): the difference
// of the sizes (the width and height variables), halved rounding toward zero, added to the corner. Returns the two coordinates as the
// original leaves them in registers.
struct Centred { std::uint32_t horizontal, vertical; std::int32_t left, top, horizontal_gap; };
Centred centre(Cpu& c, std::uint32_t area, std::uint32_t width_variable, std::uint32_t height_variable) {
    const std::int32_t left = c.i16(area + 2), right = c.i16(area + 6), top = c.i16(area), bottom = c.i16(area + 4);
    const std::int32_t width = std::int32_t(c.u32(width_variable)), height = std::int32_t(c.u32(height_variable));
    const std::int32_t horizontal_gap = std::int32_t(std::uint32_t(right - left) - std::uint32_t(width));
    const std::int32_t vertical_gap = std::int32_t(std::uint32_t(bottom - top) - std::uint32_t(height));
    // `rlwinm r4,r5,1,31,31; add; srawi 1`: the sign bit added before the arithmetic shift is a division by two rounding toward zero.
    const auto half = [](std::int32_t gap) { return (std::int32_t(std::uint32_t(gap) + ((std::uint32_t(gap) >> 31) & 1))) >> 1; };
    const auto x = std::uint32_t(left) + std::uint32_t(half(horizontal_gap)), y = std::uint32_t(top) + std::uint32_t(half(vertical_gap));
    return {half_of(std::int32_t(x)), half_of(std::int32_t(y)), left, top, horizontal_gap};
}
}

// 0x10768dc0: gives the GWorld back (when the surface exists, that is, the PixMap variable is set) and clears the three variables.
void dispose_surface(Cpu& c) {
    Frame frame(c, 0x10768dc0, 0x40, {});
    const auto pixmap = c.u32(small(c, surface_small::pixmap));
    compare_logical(c, pixmap, 0);
    if (pixmap == 0) return;
    c.set(3, c.u32(small(c, surface_small::gworld)));
    glue(c, import::dispose_gworld, 0x10768de0);
    c.put32(small(c, surface_small::surface_base), 0);
    c.put32(small(c, surface_small::pixmap), 0);
    c.put32(small(c, surface_small::gworld), 0);
}

// 0x10768c90: the 15-bit off-screen surface the size of the window (the width and height variables): NewGWorld, the PixMap, its pixels
// locked, the base address and the row length (the PixMap's row bytes without the flag bits, and half of it in pixels). An error from
// NewGWorld raises the assertion (line 0x1c6).
void create_surface(Cpu& c) {
    Frame frame(c, 0x10768c90, 0x70, {.first_gpr = 30});
    const auto rect = frame.at(0x40);
    const auto pool = c.toc(surface_toc::pool);
    c.set({{5, c.toc(surface_toc::width)}, {4, 0}, {6, c.toc(surface_toc::height)}, {30, c.toc(surface_toc::assertion_buffer)}, {31, pool}});
    const auto width = c.u32(c.r(5)), height = c.u32(c.r(6));
    c.set({{3, rect}, {5, 0}, {6, half_of(std::int32_t(width))}, {7, half_of(std::int32_t(height))}});
    glue(c, import::set_rect, 0x10768cd4);
    c.set({{3, small(c, surface_small::gworld)}, {5, rect}, {4, 0x10}, {6, 0}, {7, 0}, {8, 0}});
    glue(c, import::new_gworld, 0x10768cf4);
    compare(c, std::int16_t(c.r(3)), 0);
    if (std::int16_t(c.r(3)) != 0) {
        assert_here(c, 0x10768d68, frame.at(0x48), frame.at(0x38), 0x1c6);
        c.set(3, c.toc(surface_toc::assertion_flag));
        return;
    }
    c.set(3, c.u32(small(c, surface_small::gworld)));
    glue(c, import::get_port_pixmap, 0x10768d08);
    c.put32(small(c, surface_small::pixmap), c.r(3));
    glue(c, import::lock_pixels, 0x10768d14);
    c.set(3, c.u32(small(c, surface_small::pixmap)));
    glue(c, import::get_pixel_base, 0x10768d20);
    c.put32(small(c, surface_small::surface_base), c.r(3));
    c.set(3, c.u32(c.u32(small(c, surface_small::pixmap))));
    const auto row_bytes = std::uint32_t(c.i16(c.r(3) + 4)) & 0x3fff;
    c.put16(small(c, surface_small::row_bytes), row_bytes);
    c.set(3, extended(c.i16(small(c, surface_small::row_bytes))));
    c.put16(small(c, surface_small::row_pixels), std::uint32_t((std::int32_t(c.r(3)) + std::int32_t((c.r(3) >> 31) & 1)) >> 1));
}

// 0x107690f0: the window's bounds into the Rect at `rect`: the current window's content (GetWindowBounds, region 0x21) when there is
// one, else a 800x600 rectangle at the position the resource 'WinP' holds. 1 when there is a bounds, 0 when there is neither a window
// nor a resource.
bool window_bounds(Cpu& c, std::uint32_t rect) {
    Frame frame(c, 0x107690f0, 0x40, {.first_gpr = 30});
    c.set(30, rect);
    c.set(4, c.toc(surface_toc::window));
    const auto window = c.u32(c.r(4));
    c.set(3, window);
    compare_logical(c, window, 0);
    if (window != 0) {
        c.set({{5, rect}, {4, 0x21}});
        glue(c, import::get_window_bounds, 0x10769124);
        c.set(3, 1);
        return true;
    }
    c.set({{3, window_resource::type}, {4, window_resource::id}});
    glue(c, import::get_resource, 0x10769140);
    c.set(31, c.r(3));
    compare(c, std::int32_t(c.r(31)), 0);
    if (c.r(31) == 0) {
        c.set(3, 0);
        return false;
    }
    const auto data = c.u32(c.r(31));
    c.set({{4, data}, {3, rect}});
    const auto vertical = c.i16(data), horizontal = c.i16(data + 2);
    c.set({{5, extended(vertical)}, {4, extended(horizontal)}});
    c.set({{6, half_of(std::int32_t(c.r(4)) + 0x320)}, {7, half_of(std::int32_t(c.r(5)) + 0x258)}});
    glue(c, import::set_rect, 0x10769170);
    c.set(3, c.r(31));
    glue(c, import::release_resource, 0x1076917c);
    c.set(3, 1);
    return true;
}

// 0x107689e0: full screen on (`mode` nonzero) or off. On: a window the size of the main screen's region (inset by 100 each side),
// with the event handler installed and the window shown, the port restored, and the display object (when there is one) shown. Off: the
// window brought to the front, the display object hidden and deleted, the window disposed, the menu bar invalidated.
void set_full_screen(Cpu& c, std::uint32_t mode) {
    Frame frame(c, 0x107689e0, 0x50, {.first_gpr = 30});
    const auto rect = frame.at(0x3c), saved_port = frame.at(0x38);
    c.set({{4, c.toc(surface_toc::full_screen)}, {30, c.toc(surface_toc::window_again)}});
    compare(c, byte_of(mode), 0);
    c.put8(c.r(4), mode);
    const auto window_variable = c.r(30);
    if (byte_of(mode) != 0) {
        glue(c, import::get_gray_region, 0x10768a0c);
        c.set(4, rect);
        glue(c, import::get_region_bounds, 0x10768a18);
        c.set({{3, rect}, {4, 0xffffff9c}, {5, 0xffffff9c}});
        glue(c, import::inset_rect, 0x10768a2c);
        c.set(3, saved_port);
        glue(c, import::get_port, 0x10768a38);
        c.set({{3, 0xd}, {6, window_variable}, {4, 0x1220000}, {5, rect}});
        glue(c, import::create_new_window, 0x10768a54);
        compare(c, std::int32_t(c.r(3)), 0);
        if (c.r(3) == 0) {
            c.set(3, c.toc(surface_toc::event_handler));
            glue(c, import::new_event_handler, 0x10768a68);
            c.set({{31, c.r(3)}, {3, c.u32(window_variable)}});
            glue(c, import::get_window_event_target, 0x10768a78);
            c.set({{6, c.toc(surface_toc::event_data)}, {4, c.r(31)}, {5, 1}, {7, 0}, {8, 0}});
            glue(c, import::install_event_handler, 0x10768a94);
            c.set(3, c.u32(window_variable));
            glue(c, import::show_window, 0x10768aa0);
        }
        c.set(3, c.u32(saved_port));
        glue(c, import::set_port, 0x10768aac);
        const auto display = c.u32(small(c, surface_small::display_object));
        c.set(3, display);
        compare_logical(c, display, 0);
        if (display != 0) c.call(outside::show_display, 0x10768ac0);
        return;
    }
    c.set(3, c.u32(window_variable));
    compare_logical(c, c.r(3), 0);
    if (c.r(3) != 0) glue(c, import::bring_to_front, 0x10768ad4);
    c.set(3, c.u32(small(c, surface_small::display_object)));
    compare_logical(c, c.r(3), 0);
    if (c.r(3) != 0) {
        c.call(outside::hide_display, 0x10768ae8);
        c.set({{3, c.u32(small(c, surface_small::display_object))}, {4, 1}});
        c.call(outside::delete_display, 0x10768af4);
        c.put32(small(c, surface_small::display_object), 0);
    }
    c.set(3, c.u32(window_variable));
    compare_logical(c, c.r(3), 0);
    if (c.r(3) != 0) {
        glue(c, import::dispose_window, 0x10768b0c);
        c.put32(window_variable, 0);
    }
    glue(c, import::invalidate_menu_bar, 0x10768b1c);
}

// 0x10768c50: leaves full screen when it is on.
void leave_full_screen(Cpu& c) {
    Frame frame(c, 0x10768c50, 0x40, {});
    c.set(3, c.toc(surface_toc::full_screen));
    const auto on = c.u8(c.r(3));
    compare(c, byte_of(on), 0);
    if (byte_of(on) != 0) {
        c.set(3, 0);
        c.link(0x10768c74);
        set_full_screen(c, 0);
    }
}

// 0x10768b40: the bounds of the display into the Rect at `bounds`, in the display mode `mode`: the first call builds the display
// object (the device from the window's bounds with GetMaxDevice, else the main device; the object 800x600, depth 16) and switches the
// full-screen mode when it differs from `mode`; the bounds are those of the device's GDevice record (the halfwords at +0x22..+0x28 of
// what the device handle points to).
void display_bounds(Cpu& c, std::uint32_t mode, std::uint32_t bounds) {
    Frame frame(c, 0x10768b40, 0x50, {.first_gpr = 29});
    const auto probe = frame.at(0x38);
    c.set({{31, bounds}, {30, mode}});
    const auto object = c.u32(small(c, surface_small::display_object));
    compare_logical(c, object, 0);
    if (object == 0) {
        c.set(3, probe);
        c.link(0x10768b74);
        const bool found = window_bounds(c, probe);
        compare(c, byte_of(c.r(3)), 0);
        if (found) {
            c.set(3, probe);
            glue(c, import::get_max_device, 0x10768b84);
            c.put32(small(c, surface_small::device), c.r(3));
        }
        const auto device = c.u32(small(c, surface_small::device));
        compare_logical(c, device, 0);
        if (device == 0) {
            glue(c, import::get_main_device, 0x10768b9c);
            c.put32(small(c, surface_small::device), c.r(3));
        }
        c.set(3, 0x18);
        c.call(outside::new_object, 0x10768bac);
        c.set(29, c.r(3));
        compare(c, std::int32_t(c.r(29)), 0);
        if (c.r(29) != 0) {
            c.set({{4, c.u32(small(c, surface_small::device))}, {5, 0}});
            c.call(outside::make_display, 0x10768bc0);
        }
        compare_logical(c, c.r(29), 0);
        c.put32(small(c, surface_small::display_object), c.r(29));
        if (c.r(29) != 0) {
            c.set({{3, c.r(29)}, {4, 0x10}});
            c.call(outside::set_display_depth, 0x10768bdc);
            c.set({{3, c.u32(small(c, surface_small::display_object))}, {4, 0x320}, {5, 0x258}});
            c.call(outside::set_display_size, 0x10768be8);
        }
    }
    c.set(3, c.toc(surface_toc::full_screen));
    const auto current = c.u8(c.r(3));
    c.set(3, std::uint32_t(byte_of(current)));
    compare(c, byte_of(current), byte_of(mode));
    if (byte_of(current) != byte_of(mode)) {
        c.set(3, mode);
        c.link(0x10768c08);
        set_full_screen(c, mode);
    }
    const auto device = c.u32(c.u32(small(c, surface_small::device)));
    c.set(3, device);
    c.put16(bounds, c.u16(device + 0x22));
    c.put16(bounds + 2, c.u16(device + 0x24));
    c.put16(bounds + 4, c.u16(device + 0x26));
    c.put16(bounds + 6, c.u16(device + 0x28));
}

// 0x10768e10: creates the game's window titled `title` over the screen rectangle `area` (left, top, right, bottom as Mac Rect halfwords):
// full screen (class 0xd, attributes 0x2000000, plus 0x200000 when MAIN's 0x10770ee0 says so) centred in the area, or windowed (class 6,
// attributes 0x2000008) at the position the resource 'WinP' remembers when that lies inside the area (PtInRect) and on the desktop
// (PtInRgn of the gray region), else centred, else at (16, 32). The window's port becomes current, the title is set from a CFString and
// the window shown. An error from CreateNewWindow raises the assertion (line 0x240).
void create_window(Cpu& c, std::uint32_t title, std::uint32_t area) {
    Frame frame(c, 0x10768e10, 0xa0, {.first_gpr = 21});
    const auto point = frame.at(0x38), port = frame.at(0x3c), rect = frame.at(0x48);
    c.set({{27, c.toc(surface_toc::window)}, {21, title}, {28, c.toc(surface_toc::assertion_buffer)}, {25, area},
           {29, c.toc(surface_toc::pool)}, {30, c.toc(surface_toc::height)}, {31, c.toc(surface_toc::width)}});
    c.set(3, port);
    glue(c, import::get_port, 0x10768e44);
    c.set(3, c.toc(surface_toc::full_screen));
    const auto full_screen = byte_of(c.u8(c.r(3)));
    compare(c, full_screen, 0);
    // The centring both branches share: r3 = left + half the horizontal gap, r4 that half, r5 the gap, r6 left, r7 top (what the original
    // leaves in the argument registers).
    const auto centred = [&] {
        const auto at = centre(c, c.r(25), c.r(31), c.r(30));
        const auto gap = std::uint32_t(at.horizontal_gap);
        const auto half = (std::int32_t(gap + ((gap >> 31) & 1))) >> 1;
        c.set({{6, std::uint32_t(at.left)}, {7, std::uint32_t(at.top)}, {5, gap}, {4, std::uint32_t(half)}, {3, std::uint32_t(at.left) + std::uint32_t(half)}});
        c.set({{26, at.horizontal}, {25, at.vertical}});
    };
    if (full_screen != 0) {
        c.set({{23, 0x2000000}, {22, 0xd}});
        centred();
        c.call(outside::full_screen_query, 0x10768eb8);
        compare(c, byte_of(c.r(3)), 0);
        if (byte_of(c.r(3)) != 0) c.set(23, c.r(23) | 0x200000);
    } else {
        c.set({{4, 0x2000000}, {23, 0x2000008}, {22, 6}, {3, window_resource::type}});
        c.set(4, window_resource::id);
        glue(c, import::get_resource, 0x10768ee4);
        c.set(24, c.r(3));
        compare(c, std::int32_t(c.r(24)), 0);
        bool remembered = false;
        if (c.r(24) != 0) {
            c.set({{3, c.u32(c.r(24))}, {4, c.r(25)}});
            c.set(3, c.u32(c.r(3)));
            glue(c, import::point_in_rect, 0x10768f00);
            compare(c, std::int32_t(c.r(3) & 0xff), 0);
            remembered = (c.r(3) & 0xff) != 0;
        }
        if (remembered) {
            c.set(3, c.u32(c.r(24)));
            c.set({{26, extended(c.i16(c.r(3) + 2))}, {25, extended(c.i16(c.r(3)))}});
        } else {
            centred();
        }
        compare_logical(c, c.r(24), 0);
        if (c.r(24) != 0) {
            c.set(3, c.r(24));
            glue(c, import::release_resource, 0x10768f7c);
        }
        c.set({{4, c.r(26)}, {5, c.r(25)}, {3, point}});
        glue(c, import::set_point, 0x10768f90);
        glue(c, import::get_gray_region, 0x10768f98);
        c.set({{4, c.r(3)}, {3, c.u32(point)}});
        glue(c, import::point_in_region, 0x10768fa8);
        compare(c, std::int32_t(c.r(3) & 0xff), 0);
        if ((c.r(3) & 0xff) == 0) c.set({{26, 0x10}, {25, 0x20}});
    }
    const auto width = c.u32(c.r(31)), height = c.u32(c.r(30));
    c.set(4, half_of(std::int32_t(c.r(26))));
    c.set(0, half_of(std::int32_t(c.r(25))));
    c.set({{5, width}, {3, height}});
    c.set({{6, width + c.r(4)}, {4, c.r(26)}});
    c.set(0, height + c.r(0));
    c.set({{5, c.r(25)}, {3, rect}, {6, half_of(std::int32_t(c.r(6)))}, {7, half_of(std::int32_t(c.r(0)))}});
    glue(c, import::set_rect, 0x10768fec);
    c.set({{3, c.r(22)}, {4, c.r(23)}, {6, c.r(27)}, {5, rect}});
    glue(c, import::create_new_window, 0x10769004);
    compare(c, std::int32_t(c.r(3)), 0);
    if (c.r(3) != 0) {
        assert_here(c, 0x10769090, frame.at(0x50), frame.at(0x40), 0x240);
        c.set(3, c.toc(surface_toc::assertion_flag));
        return;
    }
    c.set(3, c.u32(c.r(27)));
    glue(c, import::get_window_port, 0x10769018);
    glue(c, import::set_port, 0x10769020);
    glue(c, import::string_encoding, 0x10769028);
    c.set({{5, c.r(3)}, {4, c.r(21)}, {3, 0}, {6, 0}});
    glue(c, import::create_string, 0x10769040);
    c.set(21, c.r(3));
    compare(c, std::int32_t(c.r(21)), 0);
    if (c.r(21) != 0) {
        c.set({{3, c.u32(c.r(27))}, {4, c.r(21)}});
        glue(c, import::set_window_title, 0x10769058);
        c.set(3, c.r(21));
        glue(c, import::release_string, 0x10769064);
    }
    c.call(outside::window_ready, 0x1076906c);
    c.set(3, c.u32(c.r(27)));
    glue(c, import::show_window, 0x10769074);
}

// 0x107691b0: closes the window: the current port saved, the window's port made current, and, unless `keep` (the low byte) is nonzero, the
// window's position written to the resource 'WinP' (the top left corner in global coordinates, SetPt + LocalToGlobal on the resource's
// data; the resource is created with NewHandle + AddResource under the name at pool + 0x24 when there is none, else marked changed), then the
// saved port restored, the window disposed and the variable cleared.
void close_window(Cpu& c, std::uint32_t keep) {
    Frame frame(c, 0x107691b0, 0x50, {.first_gpr = 30});
    const auto saved_port = frame.at(0x38);
    c.set({{31, c.toc(surface_toc::window)}, {30, keep}});
    c.set(3, saved_port);
    glue(c, import::get_port, 0x107691d4);
    c.set(3, c.u32(c.r(31)));
    glue(c, import::get_window_port, 0x107691e0);
    glue(c, import::set_port, 0x107691e8);
    compare(c, std::int32_t(c.r(30) & 0xff), 0);
    if ((c.r(30) & 0xff) == 0) {
        c.set({{3, window_resource::type}, {4, window_resource::id}});
        glue(c, import::get_resource, 0x10769204);
        c.set(30, c.r(3));
        compare(c, std::int32_t(c.r(30)), 0);
        if (c.r(30) != 0) {
            c.set({{3, c.u32(c.r(30))}, {4, 0}, {5, 0}});
            glue(c, import::set_point, 0x10769284);
            c.set(3, c.u32(c.r(30)));
            glue(c, import::local_to_global, 0x10769290);
            c.set(3, c.r(30));
            glue(c, import::changed_resource, 0x1076929c);
            c.set(3, c.r(30));
            glue(c, import::write_resource, 0x107692a8);
        } else {
            c.set(3, 4);
            glue(c, import::new_handle, 0x10769218);
            c.set(30, c.r(3));
            compare(c, std::int32_t(c.r(30)), 0);
            if (c.r(30) != 0) {
                c.set({{3, c.u32(c.r(30))}, {4, 0}, {5, 0}});
                glue(c, import::set_point, 0x10769234);
                c.set(3, c.u32(c.r(30)));
                glue(c, import::local_to_global, 0x10769240);
                c.set({{6, c.toc(surface_toc::pool)}, {4, window_resource::type}, {3, c.r(30)}, {5, window_resource::id}});
                c.set(6, c.r(6) + window_resource::name);
                glue(c, import::add_resource, 0x10769260);
                c.set(3, c.r(30));
                glue(c, import::write_resource, 0x1076926c);
            }
        }
    }
    c.set(3, c.u32(saved_port));
    glue(c, import::set_port, 0x107692b4);
    c.set(3, c.u32(c.r(31)));
    glue(c, import::dispose_window, 0x107692c0);
    c.put32(c.r(31), 0);
}

// 0x107692f0: puts the rectangle (left, top, right, bottom inclusive) of the surface on the window: SetRect with the exclusive corners,
// the grey conversion of the rectangle first when the grey switch is set (0x107697c0), then CopyBits from the GWorld's bitmap to the
// window port's bitmap and QDFlushPortBuffer, the port restored. Nothing happens while 0x10768820 is still running; calling it before
// it has finished raises the assertion (line 0x2b2) and goes on. A leaver: it keeps r26-r31 and the frame's bytes the way MAIN's
// printf reads them.
void flush_rect(Cpu& c, std::uint32_t left, std::uint32_t top, std::uint32_t right, std::uint32_t bottom) {
    Frame frame(c, 0x107692f0, 0x90, {.first_gpr = 26});
    const auto saved_port = frame.at(0x38), rect = frame.at(0x44);
    c.set({{30, c.toc(surface_toc::assertion_buffer)}, {26, left}, {31, c.toc(surface_toc::pool)}, {27, top}, {28, right}, {29, bottom}});
    const auto initialising = c.u8(small(c, surface_small::initialising));
    compare(c, byte_of(initialising), 0);
    if (byte_of(initialising) != 0) return;
    const auto ready = c.u8(small(c, surface_small::ready));
    compare(c, byte_of(ready), 0);
    if (byte_of(ready) == 0) {
        assert_here(c, 0x10769344, frame.at(0x4c), frame.at(0x3c), 0x2b2);
        c.set(3, c.toc(surface_toc::assertion_flag));
    }
    c.set({{6, right + 1}, {3, rect}, {4, half_of(std::int32_t(left))}, {5, half_of(std::int32_t(top))}});
    c.set({{6, half_of(std::int32_t(c.r(6)))}, {7, half_of(std::int32_t(bottom + 1))}});
    glue(c, import::set_rect, 0x107693a4);
    const auto grey = c.u8(small(c, surface_small::grey));
    compare(c, byte_of(grey), 0);
    if (byte_of(grey) != 0) {
        c.set({{3, left}, {4, top}, {5, right}, {6, bottom}});
        c.call(outside::grey_rectangle, 0x107693c8);
    }
    c.set(3, saved_port);
    glue(c, import::get_port, 0x107693d0);
    c.set(3, c.u32(c.toc(surface_toc::window)));
    glue(c, import::get_window_port, 0x107693e0);
    c.set(31, c.r(3));
    glue(c, import::set_port, 0x107693ec);
    c.set(3, c.u32(small(c, surface_small::gworld)));
    glue(c, import::port_bitmap, 0x107693f8);
    c.set({{0, c.r(3)}, {3, c.r(31)}});
    c.set(30, c.r(0));
    glue(c, import::port_bitmap, 0x1076940c);
    c.set({{5, rect}, {4, c.r(3)}});
    c.set({{3, c.r(30)}, {7, 0}, {6, c.r(5)}, {8, 0}});
    glue(c, import::copy_bits, 0x1076942c);
    c.set({{3, c.r(31)}, {4, 0}});
    glue(c, import::flush_port, 0x1076943c);
    c.set(3, c.u32(saved_port));
    glue(c, import::set_port, 0x10769448);
}

// 0x1076e7c0: loads the three cursors, resources 0x80..0x82 of the table at the TOC's cursor slot: the colour cursor when there is one
// (flag 1), else the black and white one (flag 0).
void load_cursors(Cpu& c) {
    Frame frame(c, 0x1076e7c0, 0x40, {.first_gpr = 30});
    c.set({{31, c.toc(surface_toc::cursors)}, {30, 0}});
    for (;; c.set(30, c.r(30) + 1)) {
        compare(c, std::int32_t(c.r(30)), 3);
        if (std::int32_t(c.r(30)) >= 3) break;
        const auto at = c.r(31) + (c.r(30) << 3);
        c.set(3, half_of(std::int32_t(c.r(30) + 0x80)));
        glue(c, import::get_color_cursor, 0x1076e7ec);
        c.put32(c.r(31) + (c.r(30) << 3), c.r(3));
        const auto handle = c.u32(c.r(31) + (c.r(30) << 3));
        compare_logical(c, handle, 0);
        if (handle != 0) {
            c.set({{4, 1}, {3, at}});
            c.put8(at + 4, 1);
        } else {
            c.set(3, half_of(std::int32_t(c.r(30) + 0x80)));
            glue(c, import::get_cursor, 0x1076e828);
            c.set(4, c.r(30) << 3);
            c.put32(c.r(31) + (c.r(30) << 3), c.r(3));
            c.set(3, at);
            c.put8(at + 4, 0);
        }
    }
}

// 0x1076e870: InitCursor, then the three cursors given back: a colour cursor is disposed, a black and white one is a resource released.
void release_cursors(Cpu& c) {
    Frame frame(c, 0x1076e870, 0x40, {.first_gpr = 30});
    c.set(31, c.toc(surface_toc::cursors));
    glue(c, import::init_cursor, 0x1076e88c);
    for (c.set(30, 0);; c.set(30, c.r(30) + 1)) {
        compare(c, std::int32_t(c.r(30)), 3);
        if (std::int32_t(c.r(30)) >= 3) break;
        const auto slot = c.r(31) + (c.r(30) << 3);
        const auto handle = c.u32(slot);
        compare_logical(c, handle, 0);
        if (handle == 0) continue;
        c.set(3, slot);
        const auto colour = c.u8(slot + 4);
        compare_logical(c, colour, 0);
        if (colour != 0) {
            c.set(3, c.u32(slot));
            glue(c, import::dispose_color_cursor, 0x1076e8c8);
        } else {
            c.set(3, c.u32(slot));
            glue(c, import::release_resource, 0x1076e8dc);
        }
    }
}

// 0x1076e920: makes cursor `index` (a signed byte) the current one when cursors are enabled and it differs from the current: the colour
// cursor with SetCCursor, the black and white one with SetCursor (the resource's data), InitCursor (the arrow) when the table has no
// cursor of that index.
void set_cursor(Cpu& c, std::uint32_t index) {
    Frame frame(c, 0x1076e920, 0x40, {});
    c.set({{4, c.toc(surface_toc::cursors_enabled)}, {5, c.toc(surface_toc::cursors)}});
    const auto enabled = c.u8(c.r(4));
    compare(c, byte_of(enabled), 0);
    if (byte_of(enabled) == 0) return;
    const auto current = c.u8(small(c, small_data::current_cursor));
    c.set(4, extended(std::int16_t(std::int8_t(current))));
    compare(c, byte_of(current), byte_of(index));
    if (byte_of(current) == byte_of(index)) return;
    c.put8(small(c, small_data::current_cursor), index);
    const auto slot = c.r(5) + (std::uint32_t(byte_of(index)) << 3);
    const auto handle = c.u32(slot);
    compare_logical(c, handle, 0);
    if (handle == 0) {
        glue(c, import::init_cursor, 0x1076e9bc);
        return;
    }
    c.set(4, slot);
    const auto colour = c.u8(slot + 4);
    compare_logical(c, colour, 0);
    if (colour != 0) {
        c.set(3, c.u32(slot));
        glue(c, import::set_color_cursor, 0x1076e994);
    } else {
        c.set(3, c.u32(c.u32(slot)));
        glue(c, import::set_cursor, 0x1076e9b0);
    }
}

// 0x1076e9d0: the window's event handler (through the transition vector 0x108bdabc): paints the window's whole content black-on-gray
// (ForeColor 0x21, GetWindowBounds region 0x20, PaintRect) and returns 0 (noErr).
std::uint32_t paint_window(Cpu& c) {
    Frame frame(c, 0x1076e9d0, 0x40, {});
    c.set(3, 0x21);
    glue(c, import::fore_color, 0x1076e9e4);
    c.set({{3, c.toc(surface_toc::window_again)}, {5, frame.at(0x38)}, {4, 0x20}});
    c.set(3, c.u32(c.r(3)));
    glue(c, import::get_window_bounds, 0x1076e9fc);
    c.set(3, frame.at(0x38));
    glue(c, import::paint_rect, 0x1076ea08);
    c.set(3, 0);
    return 0;
}

// 0x10768990: shuts the graphics down: the "ready" byte cleared, full screen left, the surface given back, the window closed (remembering
// its position when the display mode byte was zero), the fonts freed (0x1076c050) and the cursors released.
void close_graphics(Cpu& c) {
    Frame frame(c, 0x10768990, 0x40, {.first_gpr = 31});
    c.set(3, c.toc(surface_toc::full_screen));
    c.put8(small(c, surface_small::ready), 0);
    c.set(31, c.u8(c.r(3)));
    c.link(0x107689b4);
    leave_full_screen(c);
    c.link(0x107689b8);
    dispose_surface(c);
    c.set(3, c.r(31));
    c.link(0x107689c0);
    close_window(c, c.r(31));
    c.call(outside::free_fonts, 0x107689c4);
    c.link(0x107689c8);
    release_cursors(c);
}

// 0x10768820: starts the graphics: a `width` x `height` window with its surface. `flags` bit 0x1000 asks for no initial fill; `mode` is
// the display mode (full screen) to start in; `title` the window's title; `fonts` is the argument of the font loader 0x1076bbf0. Returns
// 1 when the fonts loaded, else raises the assertion (line 0xd1) and returns 0. When the surface exists already (a second start) it is
// not created again but the window position is saved (0x107691b0, with the mode byte as it was) and the window recreated.
std::uint32_t start_graphics(Cpu& c, std::uint32_t width, std::uint32_t height, std::uint32_t flags, std::uint32_t fonts, std::uint32_t mode,
                             std::uint32_t title) {
    Frame frame(c, 0x10768820, 0x90, {.first_gpr = 24});
    const auto rect = frame.at(0x40);
    c.set({{28, width}, {29, height}, {30, flags}, {26, c.toc(surface_toc::assertion_buffer)}, {27, c.toc(surface_toc::pool)}, {31, fonts},
           {25, mode}, {24, title}});
    c.set({{3, c.toc(surface_toc::area)}, {4, 0}, {5, 0x20}});
    c.call(outside::fill_memory, 0x10768860);
    c.set({{8, c.toc(surface_toc::width)}, {6, 1}, {7, c.toc(surface_toc::height)}, {4, c.r(25)}, {5, c.toc(surface_toc::full_screen)}, {3, c.r(30)}});
    c.put32(c.r(8), c.r(28));
    c.set(25, c.u8(c.r(5)));
    c.set(5, rect);
    c.put32(c.r(7), c.r(29));
    c.put8(small(c, surface_small::initialising), c.r(6));
    c.link(0x10768890);
    display_bounds(c, c.r(4), rect);
    const auto ready = c.u8(small(c, surface_small::ready));
    compare(c, byte_of(ready), 0);
    if (byte_of(ready) == 0) {
        c.link(0x107688a0);
        create_surface(c);
    } else {
        c.set(3, c.r(25));
        c.link(0x107688ac);
        close_window(c, c.r(25));
    }
    c.set({{3, c.r(24)}, {4, rect}});
    c.link(0x107688b8);
    create_window(c, c.r(3), rect);
    c.set({{3, 0}, {0, 1}});
    c.put8(small(c, surface_small::initialising), 0);
    c.put8(small(c, surface_small::ready), 1);
    c.call(outside::colours, 0x107688cc);
    c.link(0x107688d0);
    load_cursors(c);
    c.set(3, c.r(31));
    c.call(outside::fonts, 0x107688d8);
    compare(c, byte_of(c.r(3)), 0);
    if (byte_of(c.r(3)) == 0) {
        assert_here(c, 0x107688f4, frame.at(0x48), frame.at(0x38), 0xd1);
        c.set({{4, c.toc(surface_toc::assertion_flag)}, {3, 0}});
        return 0;
    }
    compare(c, std::int32_t(c.r(30) & 0x1000), 0);
    if ((c.r(30) & 0x1000) == 0) {
        c.set({{4, c.toc(surface_toc::background)}, {5, c.r(28)}, {6, c.r(29)}, {3, 0}});
        c.set({{8, c.u16(c.r(4))}, {4, 0}, {7, 4}});
        c.call(outside::fill_rectangle, 0x10768964);
        c.set({{5, c.r(28) - 1}, {6, c.r(29) - 1}, {3, 0}, {4, 0}});
        c.link(0x10768978);
        flush_rect(c, 0, 0, c.r(5), c.r(6));
    }
    c.set(3, 1);
    return 1;
}
}
