#pragma once
// graph_carbon.cpp (tools/graph_carbon_audit.py): the game's drawing layer over the Mac, rewritten as typed C++. Each function is one
// routine of the original; the adapter (graph_carbon_adapter.cpp, graph_carbon_routines*.inc) reads the guest ABI's arguments, calls it
// and leaves the results the original leaves. The routines are native inside and keep the guest ABI at their edge: guest memory is the
// only truth, and the registers they leave (r3 and r4-r10) are those of the original's last instruction or callee.
#include "guest_abi.hpp"

namespace cm::aot::graph_carbon {
using guest::Cpu;

// The argument registers r4-r10 as a routine leaves them: the hook starts from the registers it was entered with and the routine
// overwrites the ones the original writes (callers' next calls store them into the stack image, and the checker compares them).
struct Leftovers { std::uint32_t r4, r5, r6, r7, r8, r9, r10; };
// A rectangle as the routines take it in registers: left, top, right, bottom (signed 32-bit words).
struct Corners { std::int32_t left, top, right, bottom; };

// Block `leaves` (graph_carbon_leaves.cpp): the leaf routines of the module.
// 0x10769b50: the display mode flag (a byte; nonzero while the game runs full screen: the initialization saves it, GAME_CONFIG.CPP and
// MAIN.CPP ask for it).
std::uint32_t full_screen(Cpu&);
// 0x1076e910: the index of the cursor 0x1076e920 last set (a byte kept in the TOC's small data).
std::uint32_t current_cursor(Cpu&);
// 0x107698d0: the Mac Rect {top, left, bottom, right} at `rect` that holds the two corner points (x1, y1) and (x2, y2) in order; the
// larger coordinates are left in r5 and r6.
void rectangle_from_corners(Cpu&, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint32_t rect, Leftovers&);
// 0x10769920: the area of the intersection of two rectangles, 0 when it is empty (a negative width or height counts as none; the
// product wraps at 32 bits). The intersection's edges are left in r7-r10, its height in r4 and its width in r5.
std::uint32_t intersection_area(Corners a, Corners b, Leftovers&);
// 0x1076bb20: the 15-bit pixel of an 8-bit red, green and blue (the top five bits of each).
std::uint32_t pack_pixel(std::uint32_t red, std::uint32_t green, std::uint32_t blue);
// 0x1076bb40: a 15-bit pixel with each component scaled by `percent` (the low byte, a percentage) and clamped at 255; the divide by 100
// is the compiler's multiply by 0x51eb851f.
std::uint32_t scale_pixel(std::uint32_t pixel, std::uint32_t percent, Leftovers&);
// 0x1076ca20: the callback the text drawer 0x1074f800 calls through its ninth argument (descriptor 0x108bdaac): it counts its calls and,
// when the count reaches the limit the caller 0x1076ca50 stored, keeps the low halfword of `value` (sign-extended). Returns 0.
std::uint32_t probe_text_position(Cpu&, std::uint32_t value, Leftovers&);

// Block `remap` (graph_carbon_remap.cpp): the screen fade.
// 0x10769460: fades the rectangle (x1, y1, x2, y2) of the surface from black (steps > 0) or to black (steps < 0) in |steps| + 1 frames through a
// 64K-entry remap table built with the allocator, putting each frame on the window (Toolbox glue); nothing happens for steps == 0. Builds
// the original's frame (r19-r31) and runs the native drawing observation at 0x107696cc and at its return, 0x107697b0.
void fade_rectangle(Cpu&, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::int32_t steps);

// Block `frames` (graph_carbon_frames.cpp): the framed, shaded or filled rectangle.
// 0x1076d6e0: draws the rectangle (left, top, right, bottom) as the style word says (fill, gradient, dim, bevelled frame, outlines, shadow and
// highlight lines; bits in graph_carbon_frames.cpp) in `colour` (a 15-bit pixel; 0 means the module's default colour where the shading needs one),
// `accent` being the frame colour of the ellipse style, by calling the pixel primitives. A stack-contract leaver: builds the original's frame.
void framed_rectangle(Cpu&, std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom, std::uint32_t style, std::uint32_t colour,
                      std::uint32_t accent);

// Block `surface` (graph_carbon_surface.cpp): the drawing surface, the window and the cursors, all Toolbox. Each routine builds the original's
// frame, calls the glue at the original's return addresses and leaves r3 and r4-r10 as the original does.
// 0x10768820: starts the graphics: a `width` x `height` window over a new surface; `flags` bit 0x1000 asks for no initial fill, `fonts` goes
// to the font loader, `mode` is the display mode, `title` the window's title. 1 when the fonts loaded, else the assertion and 0.
std::uint32_t start_graphics(Cpu&, std::uint32_t width, std::uint32_t height, std::uint32_t flags, std::uint32_t fonts, std::uint32_t mode, std::uint32_t title);
// 0x10768990: shuts the graphics down (full screen left, surface disposed, window closed, fonts freed, cursors released).
void close_graphics(Cpu&);
// 0x107689e0: full screen on (`mode` nonzero) or off: creates or disposes the window and the display object.
void set_full_screen(Cpu&, std::uint32_t mode);
// 0x10768b40: the bounds of the display into the Rect at `bounds`, creating the display object and switching to `mode` when needed.
void display_bounds(Cpu&, std::uint32_t mode, std::uint32_t bounds);
// 0x10768c50: leaves full screen when it is on.
void leave_full_screen(Cpu&);
// 0x10768c90: the 15-bit off-screen GWorld the size of the window, its base address and row length (an error raises the assertion).
void create_surface(Cpu&);
// 0x10768dc0: gives the GWorld back and clears its variables.
void dispose_surface(Cpu&);
// 0x10768e10: the game's window titled `title` over the screen rectangle `area` (full screen centred, or windowed at the remembered position).
void create_window(Cpu&, std::uint32_t title, std::uint32_t area);
// 0x107690f0: the window's bounds (or the 800x600 rectangle at the remembered position) into the Rect at `rect`; false when there is neither.
bool window_bounds(Cpu&, std::uint32_t rect);
// 0x107691b0: closes the window, remembering its position in the resource 'WinP' unless the low byte of `keep` is nonzero.
void close_window(Cpu&, std::uint32_t keep);
// 0x107692f0: puts the rectangle (left, top, right, bottom inclusive) of the surface on the window. A leaver (stack contract).
void flush_rect(Cpu&, std::uint32_t left, std::uint32_t top, std::uint32_t right, std::uint32_t bottom);
// 0x1076e7c0, 0x1076e870, 0x1076e920: the three cursors: load, release, and make cursor `index` current.
void load_cursors(Cpu&);
void release_cursors(Cpu&);
void set_cursor(Cpu&, std::uint32_t index);
// 0x1076e9d0: the window's event handler (descriptor 0x108bdabc): paints the window's content and returns 0.
std::uint32_t paint_window(Cpu&);

// Block `colours` (graph_carbon_colours.cpp): the colour variables and the bitmaps (0x30-byte records with a pixel block of their own). Each
// builds the original's frame, calls the allocator and the file layer at the original's return addresses and leaves r3 and r4-r10 as the
// original does; the ones that return a record or a result also leave it in r3.
// 0x1076b830: sets the 25 colour variables (15-bit pixels) unless the display is being created.
void colour_variables(Cpu&);
// 0x1076b6e0: a copy of the bitmap record `source` with a pixel block of its own (the pixels are not copied), or `reuse` itself when it has
// the same width and height; 0 when an allocation fails. Copies the source's 0x10-0x2f bytes whatever they hold.
std::uint32_t clone_bitmap(Cpu&, std::uint32_t source, std::uint32_t reuse);
// 0x1076b7e0: frees a bitmap record (its pixel block when it has one, then the record); nothing for 0. A leaver (stack contract).
void free_bitmap(Cpu&, std::uint32_t bitmap);
// 0x1076b020: copies the rectangle with corners (x1, y1) and (x2, y2) (inclusive) of the surface into a bitmap (a clone of the record it builds
// on its own frame, so `reuse` is handed back when it is that size); returns it. Runs the native drawing observation at its return, 0x1076b138.
std::uint32_t capture_rectangle(Cpu&, std::uint32_t x1, std::uint32_t y1, std::uint32_t x2, std::uint32_t y2, std::uint32_t reuse);
// 0x1076b140: copies the bitmap onto the surface with its top left corner at (x, y); without a bitmap, the assertion report. Runs the native
// drawing observation at its return, 0x1076b234.
void blit_bitmap(Cpu&, std::uint32_t x, std::uint32_t y, std::uint32_t bitmap);
// 0x1076b240: writes the bitmap as a `.rgm` file at `path` (the 0x30-byte record, then its pixels), unless the display is being created.
void save_region(Cpu&, std::uint32_t bitmap, std::uint32_t path);
// 0x1076b2f0: loads the region at `path`: its cached `.rgm` if there is one, else the bitmap file itself, which is converted and cached as a
// `.rgm`; into `reuse` when that has the same size. 0 on any failure (the failure tail gives back what the call allocated).
std::uint32_t load_region(Cpu&, std::uint32_t path, std::uint32_t reuse);
// 0x1076b450: reads a bitmap file (little-endian width, height, size, a format word, 0x20 bytes of metadata, pixels; 5-6-5 converted to 5-5-5) from
// the open stream into a clone of the record it builds (or `reuse`), and closes the stream; 0 on any failure.
std::uint32_t read_bitmap(Cpu&, std::uint32_t stream, std::uint32_t reuse);

// Block `text` (graph_carbon_text*.cpp): the bitmap and outline fonts' text entry points. Each builds the original's frame, calls what the
// original calls at its return addresses and leaves r3 and r4-r10 as the original does.
// 0x1076d5a0: the gap a bitmap font leaves after text[index - 1] before text[index] (index counts from 1; 0 at the end of the text).
std::int32_t glyph_gap(Cpu&, std::uint32_t font, std::uint32_t text, std::uint32_t index);
// 0x1076d470: the height of a font number's glyphs (a leaver).
std::uint32_t font_height(Cpu&, std::uint32_t font);
// 0x1076d1e0: the width of a text in pixels for a font number (a leaver; calls FONT.CPP's string width for the outline fonts).
std::int32_t text_width(Cpu&, std::uint32_t font, std::uint32_t text);
// 0x1076ca50: draws a text at (x, y) in a 15-bit colour; `limit` (-1: none) is a position whose pen x gets a vertical line (a leaver; observes
// the native drawing exit point 0x1076d194).
void draw_text(Cpu&, std::uint32_t x, std::uint32_t y, std::uint32_t font, std::uint32_t colour, std::uint32_t text, std::uint32_t limit);
// 0x1076bbf0: loads the eight fonts (four outline records through FONT.CPP, seven .fnt files); 1 when all loaded, else 0.
std::uint32_t load_fonts(Cpu&);
// 0x1076c0b0: loads the .fnt file `path` into bitmap font `font` (1-7) (a leaver); 1 when loaded, else 0 (the assertion of the failing read).
std::uint32_t load_bitmap_font(Cpu&, std::uint32_t font, std::uint32_t path);
// 0x1076c980 and 0x1076c050: free the glyph pixels of one bitmap font and of all seven.
void free_font_pixels(Cpu&, std::uint32_t font);
void free_all_fonts(Cpu&);
// 0x1076c010 and 0x1076d1a0 (`entry`): the cleanup destructors of two compiler-generated objects (descriptors only): delete a non-null object when
// the halfword `flags` is positive; return the object.
std::uint32_t delete_cleanup_object(Cpu&, std::uint32_t entry, std::uint32_t object, std::uint32_t flags);
// 0x1076ea70: the destructor of the array of eight font slots (descriptor only).
void destroy_font_array(Cpu&);

// Blocks `lines` (graph_carbon_lines.cpp) and `fills` (graph_carbon_fills.cpp): the pixel primitives on the 15-bit surface. Each builds the
// original's frame where it has one, leaves r3 and r4-r10 as the original's last instruction does and runs the native drawing observation
// at its return (0x107698c4, 0x10769b48, 0x1076a570, 0x1076a784, 0x1076b014).
// A line, an ellipse's box or a rectangle's corners (x1, y1) and (x2, y2) with the style bits and the 15-bit colour.
struct LineArguments { std::int32_t x1, y1, x2, y2; std::uint32_t style, colour; };
// 0x1076a140: the line (style bit 2: every pixel, clear: four pixels of each six). A leaver (stack contract): the caller sets the link register.
void draw_line(Cpu&, const LineArguments&);
// A `bl 0x1076a140` made by this module's C++: the arguments and the link register (`back_to`, the address after the original's `bl`) are set
// and the native drawing observation of the entry runs, as the generated page does before a routed entry's hook.
void call_draw_line(Cpu&, const LineArguments&, std::uint32_t back_to);
// 0x1076a8f0: the octants of a circle centred in the rectangle (left, top, right, bottom) painted at `surface`, each row's extremes recorded in
// `table` (the ninth argument, the word at sp + 0x38, 0 for none). r4 and r5 are left as the original's last loop counters leave them.
void circle_octants(Cpu&, std::uint32_t surface, std::int32_t i, std::int32_t j, std::int32_t left, std::int32_t top, std::int32_t right,
                    std::int32_t bottom, std::uint32_t colour, std::uint32_t table, Leftovers&);
// 0x1076ad50: the ellipse (a circle of the box's height) with the style bits 4 (filled through a table of rows from the allocator), 2 and clear.
void draw_ellipse(Cpu&, const LineArguments&);
// 0x107697c0: the rectangle with corners (x1, y1) and (x2, y2) (inclusive, any order) turned to grey. Observed at 0x107698c4.
void shade_to_grey(Cpu&, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2);
// 0x10769980: the same rectangle through the table of pixels at 60% (built on first use). A leaver; observed at 0x10769b48.
void shade_rectangle(Cpu&, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2);
// 0x1076a580: style bits 1 or 2 outline the rectangle with lines of that style, otherwise [x1, x2) x [y1, y2) is filled. Observed at 0x1076a784.
void fill_rectangle(Cpu&, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint32_t style, std::uint32_t colour);
// 0x1076a790: paints `colour` at the pixels where the bitmap {width, height, width * height bytes} is not zero, its upper left at (x, y)
// (a null bitmap raises the assertion). A leaver.
void draw_mask(Cpu&, std::int32_t x, std::int32_t y, std::uint32_t colour, std::uint32_t bitmap);
}
