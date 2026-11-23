#pragma once
// graph_carbon.cpp, block `surface` (graph_carbon_surface.cpp): the surface, the window and the cursors. Names of the variables, the Toolbox
// glue and the routines outside the block that these fifteen routines use; addresses are those of the original (the listing).
#include "graph_carbon_detail.hpp"

namespace cm::aot::graph_carbon {
// TOC slots (each holds the address of a variable, read with Cpu::toc).
namespace surface_toc {
inline constexpr std::int32_t height = -0x7470,         // the window's height (word)
    width = -0x746c,                                    // the window's width (word)
    assertion_buffer = -0x7ba8,                         // the shared message buffer of the assertions
    assertion_flag = -0x7bac,                           // the shared success flag the assertions clear
    background = -0x7bd4,                               // halfword: the colour the surface is filled with at start-up
    pool = -0x3bd0,                                     // the string pool (graph_carbon.cpp)
    cursors = -0x3bf4,                                  // the table of three cursors {handle, colour flag byte} of eight bytes
    full_screen = -0x5ae8,                              // byte: the display mode (nonzero: full screen)
    area = -0x5ae4,                                     // 0x20 bytes of display state the start-up clears
    window = -0x5af8,                                   // the window pointer variable
    window_again = -0x5aec,                             // the same variable, as the event handler and the full-screen branch read it
    event_handler = -0x5af0,                            // the transition vector of the window's event handler (0x1076e9d0)
    event_data = -0x5af4,                               // the handler's user data
    cursors_enabled = -0x5b14;                          // byte: nonzero when the cursors may be changed
}
// Small data (r2-relative, no slot): see graph_carbon_audit.SMALL_DATA.
namespace surface_small {
inline constexpr std::int32_t surface_base = -0x3564,   // pointer to the surface's pixels
    row_pixels = -0x3568,                               // halfword: the surface's row length in pixels
    row_bytes = -0x3566,                                // halfword: the surface's row length in bytes
    pixmap = -0x3560,                                   // the PixMap handle
    gworld = -0x355c,                                   // the GWorld
    grey = -0x3558,                                     // byte: nonzero when the flushed rectangle is converted to grey first
    device = -0x3554,                                   // the display device handle
    display_object = -0x356c,                           // the display mode object (0x18 bytes)
    initialising = -0x3550,                             // byte: set while 0x10768820 runs
    ready = -0x354f;                                    // byte: set once it has finished
}
// The Toolbox glue: a `bl` to one of these goes through the import observer (graph_carbon_audit.GLUE_NAMES).
namespace import {
inline constexpr std::uint32_t get_gray_region = 0x10780750, get_region_bounds = 0x10780768, inset_rect = 0x10780c60, get_port = 0x107803f0,
    create_new_window = 0x10780c78, new_event_handler = 0x10780cc0, get_window_event_target = 0x10780cd8, install_event_handler = 0x10780cf0,
    show_window = 0x107806a8, set_port = 0x107804f8, bring_to_front = 0x10780d38, dispose_window = 0x10780d50, invalidate_menu_bar = 0x10780270,
    get_max_device = 0x10780c30, get_main_device = 0x10780c48, set_rect = 0x10780588, new_gworld = 0x10780d68, get_port_pixmap = 0x10780d80,
    lock_pixels = 0x10780d98, get_pixel_base = 0x10780db0, dispose_gworld = 0x10780f90, get_resource = 0x10780150, point_in_rect = 0x10780e28,
    release_resource = 0x10780198, set_point = 0x10780de0, point_in_region = 0x10780e40, get_window_port = 0x107805a0,
    string_encoding = 0x10780e58, create_string = 0x10780e70, set_window_title = 0x10780e88, release_string = 0x10780ea0,
    get_window_bounds = 0x10780c18, new_handle = 0x10780dc8, local_to_global = 0x10780df8, add_resource = 0x107804b0, write_resource = 0x107804c8,
    changed_resource = 0x10780e10, port_bitmap = 0x107805b8, copy_bits = 0x107805d0, flush_port = 0x107805e8, get_color_cursor = 0x10780f00,
    get_cursor = 0x10780f18, init_cursor = 0x1077fb38, dispose_color_cursor = 0x10780fa8, set_color_cursor = 0x10780840, set_cursor = 0x10780858,
    fore_color = 0x10780c90, paint_rect = 0x10780ca8;
}
// The routines outside the block these call (all `c.call`ed at the original's return addresses).
namespace outside {
inline constexpr std::uint32_t new_object = 0x10773520, fill_memory = 0x10778300, make_display = 0x1074d8d0, set_display_depth = 0x1074daa0,
    set_display_size = 0x1074da90, show_display = 0x1074dab0, hide_display = 0x1074daf0, delete_display = 0x1074d910, colours = 0x1076b830,
    fonts = 0x1076bbf0, free_fonts = 0x1076c050, fill_rectangle = 0x1076a580, grey_rectangle = 0x107697c0, window_ready = 0x1076fd90,
    full_screen_query = 0x10770ee0;
}
// Window resource 'WinP' id 0x80 (the window's position), and the pool strings the assertions use.
namespace window_resource {
inline constexpr std::uint32_t type = 0x57696e50, id = 0x80, name = 0x24;
}
namespace surface_pool {
inline constexpr std::uint32_t format = 0x11, title = 0x1e;
}
}
