// graph_carbon.cpp, block `box` (0x10768810 and 0x10769b60): the assertion box. Every assertion of the game (6,479 call sites in every module)
// ends in 0x10769b60 with two message texts and a flag; 0x10768810 is the getter of the version text the report formats (6,402 callers).
//
// 0x10769b60 shows a framed 0x(w)-wide panel at the bottom of the screen: the two texts, an "OK" button and, when the flag is set, a "Quit"
// button; it saves the surface under the panel (0x1076b020), runs the event loop (MAIN's pump 0x1076fe80) with the hot-button state machine
// (the mouse variables, the button byte and the key code are MAIN's), and on the return key or a release over OK puts the saved surface
// back (0x1076b140) and frees it (0x1076b7e0). A release over Quit calls MAIN's exit (0x1076f7d0, which does not return in the game).
// While the graphics are being created, or when the game's "show dialogs" byte is zero, the two texts go to printf instead
// ("MESSAGE %s %s\n"). Two bytes of small data guard it against re-entry: -0x3570 ("has run once": its first run clears -0x356f) and -0x356f
// ("is showing"): while the box is showing, a nested assertion (the pump's) returns at once.
//
// STACK AND REGISTERS. The routine saves r13-r31 and builds the original's frame (0x100 bytes: the two argument words in its caller's
// parameter area, the ninth-argument word -1 at sp + 0x38 before each text box, the saved cursor at sp + 0xa8 and the saved surface at
// sp + 0xac, the two string buffers at sp + 0x40 and sp + 0x60). Every callee is called with `c.call` at the original's return address and
// at the box's own stack pointer, with the registers the original holds there: r13 = 1, r14 the button byte's address, r15 the flag,
// r16 / r17 the hot states of OK / Quit, r18-r27 the layout (set before the first call that follows it), r28-r31 the mouse y and x addresses
// and the two colour words' addresses. r3-r10 are those the last callee leaves, with the arguments the original sets.
#include "graph_carbon_detail.hpp"

namespace cm::aot::graph_carbon {
namespace {
// The routines the box calls (MAIN.CPP, the C library, LANGLIB, and this module's own, called through the dispatcher).
namespace callee {
inline constexpr std::uint32_t print = 0x1077b590, text_width = 0x1076d1e0, capture = 0x1076b020, current_cursor = 0x1076e910, localized = 0x10727890,
    copy_string = 0x1077dbf0, framed_rectangle = 0x1076d6e0, text_box = 0x1076e160, flush = 0x107692f0, pump_events = 0x1076fe80, set_cursor = 0x1076e920,
    exit_game = 0x1076f7d0, blit = 0x1076b140, free_bitmap = 0x1076b7e0;
}
// The slots of the TOC the box reads (each holds the address of a variable) and its two small-data bytes.
namespace box_toc {
inline constexpr std::int32_t mouse_y = -0x5c14, mouse_x = -0x5c10, key_code = -0x5c1c, text_colour = -0x7bc8, frame_colour = -0x7bc4,
    background = -0x7bdc,                  // the colour of the text area
    button = -0x7bac,                      // the byte the pointer's button is read from (the assertions clear it after the box)
    show_dialogs = -0x7418,                // byte: zero sends the texts to printf
    version_text = -0x5b20;                // what 0x10768810 returns
}
namespace box_small {
inline constexpr std::int32_t has_run = -0x3570, showing = -0x356f, initialising = -0x3550;
}
// The pool's strings the box uses (offsets from the pool): the printf format "MESSAGE %s %s\n", the OK caption's LANGLIB key "Ok", the Quit
// caption's key "Quit <%s - COMMENT - Option to quit the game>" and the empty string its %s is given.
namespace box_pool {
inline constexpr std::uint32_t print_format = 0x34, ok_key = 0x43, quit_key = 0x46, empty = 0x74;
}
// The panel's geometry: it is at least 200 and at most 600 pixels wide (the first text's width plus 0x18), centred on the 800-pixel
// screen; its 0x165 - 0xf2 rows are fixed. The buttons sit in a row of 0x13e - 0x156.
constexpr std::int32_t panel_top = 0xf2, panel_bottom = 0x165, button_top = 0x13e, button_bottom = 0x156, key_return = 0x24;
struct Button {
    std::int32_t left, top, right, bottom;
    bool holds(std::int32_t x, std::int32_t y) const { return x >= left && y >= top && x <= right && y <= bottom; }
};
// Where the original keeps the box's locals, relative to its stack pointer.
constexpr std::int32_t ninth_argument = 0x38, ok_caption = 0x40, quit_caption = 0x60, saved_cursor = 0xa8, saved_surface = 0xac;
}

// 0x10768810: the address of the version text (a TOC word) the assertion report formats.
std::uint32_t version_text(Cpu& c) { return c.toc(box_toc::version_text); }

void assertion_box(Cpu& c, std::uint32_t message, std::uint32_t detail, std::uint32_t offer_quit) {
    Frame frame(c, 0x10769b60, 0x100, {.first_gpr = 13});
    const auto pool = c.toc(toc::pool);
    const auto mouse_y = c.toc(box_toc::mouse_y), mouse_x = c.toc(box_toc::mouse_x), text_colour = c.toc(box_toc::text_colour),
               frame_colour = c.toc(box_toc::frame_colour), button = c.toc(box_toc::button);
    c.set({{28, mouse_y}, {29, mouse_x}, {30, text_colour}, {31, frame_colour}, {14, button}, {15, offer_quit}});
    c.put32(frame.caller() + 0x18, message);
    c.put32(frame.caller() + 0x1c, detail);
    const auto has_run = small(c, box_small::has_run), showing = small(c, box_small::showing);
    if (c.i8(has_run) == 0) {
        c.put8(showing, 0);
        c.put8(has_run, 1);
    }
    const auto show_dialogs = c.toc(box_toc::show_dialogs);
    c.set({{3, show_dialogs}, {13, 1}, {17, 0}, {16, 0}});
    if (c.i8(show_dialogs) == 0 || c.i8(small(c, box_small::initialising)) != 0) {
        c.set({{3, pool + box_pool::print_format}, {4, message}, {5, detail}});
        c.call(callee::print, 0x10769be8);
        return;
    }
    // Showing already (the pump's nested assertion, or a second report): nothing.
    if (c.i8(showing) != 0) return;
    c.put8(showing, 1);

    // The panel: as wide as the first text and 0x18 (200 to 600), centred.
    c.set({{3, 1}, {4, detail}});
    c.call(callee::text_width, 0x10769c0c);
    const auto measured = std::int32_t(c.r(3) + 0x18);
    const std::int32_t width = measured < 200 ? 200 : measured < 600 ? measured : 600;
    const std::int32_t left = (800 - width) / 2, right = left + width - 1;
    const bool quit_offered = std::int8_t(offer_quit & 0xff) != 0;
    // OK sits left of centre when Quit is offered; both captions are 0x4a wide (OK) and 0x55 apart.
    const std::int32_t ok_left = quit_offered ? left + (width - 0x8c) / 2 : left + (width - 0x4b) / 2;
    const Button ok{ok_left, button_top, ok_left + 0x4a, button_bottom};
    const Button quit = quit_offered ? Button{ok_left + 0x55, button_top, ok_left + 0x9f, button_bottom} : Button{-1, -1, -1, -1};
    c.set({{27, std::uint32_t(left)}, {26, std::uint32_t(right)}, {24, std::uint32_t(ok.top)}, {22, std::uint32_t(ok.bottom)},
           {25, std::uint32_t(ok.left)}, {23, std::uint32_t(ok.right)}, {21, std::uint32_t(quit.left)}, {20, std::uint32_t(quit.top)},
           {19, std::uint32_t(quit.right)}, {18, std::uint32_t(quit.bottom)}});

    // Save the surface under the panel and the cursor, and fetch the captions.
    c.set({{3, std::uint32_t(left)}, {5, std::uint32_t(right)}, {4, panel_top}, {6, panel_bottom}, {7, 0}});
    c.call(callee::capture, 0x10769cdc);
    c.put32(frame.at(saved_surface), c.r(3));
    c.call(callee::current_cursor, 0x10769ce4);
    c.put8(frame.at(saved_cursor), c.r(3));
    c.set({{3, frame.at(quit_caption)}, {4, pool + box_pool::ok_key}});
    c.call(callee::localized, 0x10769cfc);
    c.set({{3, frame.at(ok_caption)}, {4, frame.at(quit_caption)}});
    c.call(callee::copy_string, 0x10769d08);

    bool ok_hot = false, quit_hot = false;
    const auto text_box = [&](std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint32_t style, std::uint32_t text,
                              std::uint32_t back) {
        c.put32(frame.at(ninth_argument), 0xffffffffu);
        c.set({{3, std::uint32_t(x1)}, {4, std::uint32_t(y1)}, {5, std::uint32_t(x2)}, {6, std::uint32_t(y2)}, {7, style}, {8, 1},
               {9, c.u16(text_colour)}, {10, text}});
        c.call(callee::text_box, back);
    };
    const auto framed = [&](std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint32_t style, std::uint32_t colour_word,
                            std::uint32_t back) {
        c.set({{3, std::uint32_t(x1)}, {4, std::uint32_t(y1)}, {5, std::uint32_t(x2)}, {6, std::uint32_t(y2)}, {7, style}, {8, c.u16(colour_word)}, {9, 0}});
        c.call(callee::framed_rectangle, back);
    };
    const auto pressed = [&] { return c.i8(button) != 0; };
    // The pointer is read where the original reads it (the variables are MAIN's, written by its pump only).
    const auto over = [&](const Button& box) { return box.holds(c.i32(mouse_x), c.i32(mouse_y)); };
    for (;;) {
        // Draw the frame, the two texts and the buttons (a hot button is drawn pressed: style 0x70, its caption inset by 2).
        framed(left, panel_top, right, panel_bottom, 0x30, frame_colour, 0x10769d30);
        framed(left + 2, 0xf4, right - 2, 0x108, 0x210, c.toc(box_toc::background), 0x10769d54);
        text_box(left + 0xa, 0xf4, right - 0xa, 0x108, 9, c.u32(frame.caller() + 0x18), 0x10769d80);
        text_box(left + 0xc, 0x10a, right - 0xc, 0x13b, 0x10c, c.u32(frame.caller() + 0x1c), 0x10769dac);
        if (ok_hot) {
            framed(ok.left, ok.top, ok.right, ok.bottom, 0x70, frame_colour, 0x10769dd4);
            text_box(ok.left + 2, ok.top + 2, ok.right + 2, ok.bottom + 2, 0xc, frame.at(ok_caption), 0x10769e00);
        } else {
            framed(ok.left, ok.top, ok.right, ok.bottom, 0x30, frame_colour, 0x10769e24);
            text_box(ok.left, ok.top, ok.right, ok.bottom, 0xc, frame.at(ok_caption), 0x10769e50);
        }
        if (quit_offered) {
            c.set({{3, frame.at(quit_caption)}, {4, pool + box_pool::quit_key}, {5, pool + box_pool::empty}});
            c.call(callee::localized, 0x10769e70);
            if (quit_hot) {
                framed(quit.left, quit.top, quit.right, quit.bottom, 0x70, frame_colour, 0x10769e98);
                text_box(quit.left + 2, quit.top + 2, quit.right + 2, quit.bottom + 2, 0xc, frame.at(quit_caption), 0x10769ec4);
            } else {
                framed(quit.left, quit.top, quit.right, quit.bottom, 0x30, frame_colour, 0x10769ee8);
                text_box(quit.left, quit.top, quit.right, quit.bottom, 0xc, frame.at(quit_caption), 0x10769f14);
            }
        }
        c.set({{3, std::uint32_t(left)}, {5, std::uint32_t(right)}, {4, panel_top}, {6, panel_bottom}});
        c.call(callee::flush, 0x10769f28);
        c.call(callee::pump_events, 0x10769f2c);

        // The cursor is the hand over a button, the arrow elsewhere.
        const bool over_ok = over(ok), over_quit = !over_ok && quit_offered && over(quit);
        c.set(3, over_ok || over_quit ? 2 : 0);
        c.call(callee::set_cursor, over_ok ? 0x10769f5c : over_quit ? 0x10769f94 : 0x10769fa0);

        // OK: pressed over it, it is hot; released over it, the box is done; released elsewhere, it is cold again.
        if (!ok_hot) {
            if (pressed() && over(ok)) {
                ok_hot = true;
                c.set(16, 1);
            }
        } else if (!pressed()) {
            if (over(ok)) break;
            ok_hot = false;
            c.set(16, 0);
        }
        // Quit: the same, and a release over it leaves the game.
        if (quit_offered && !quit_hot && pressed() && over(quit)) {
            quit_hot = true;
            c.set(17, 1);
            continue;
        }
        if (quit_offered && quit_hot && !pressed()) {
            if (over(quit)) {
                c.set(3, 0xffffffffu);
                c.call(callee::exit_game, 0x1076a0bc);
            } else {
                quit_hot = false;
                c.set(17, 0);
            }
            continue;
        }
        // The return key ends the box too (and is consumed).
        const auto key = c.toc(box_toc::key_code);
        if (c.i32(key) != key_return) continue;
        c.put32(key, 0);
        break;
    }

    // Put the cursor and the surface back, show them and give the saved surface up.
    c.set(3, c.u8(frame.at(saved_cursor)));
    c.call(callee::set_cursor, 0x1076a0f8);
    c.set({{5, c.u32(frame.at(saved_surface))}, {3, std::uint32_t(left)}, {4, panel_top}});
    c.call(callee::blit, 0x1076a108);
    c.set({{3, std::uint32_t(left)}, {5, std::uint32_t(right)}, {4, panel_top}, {6, panel_bottom}});
    c.call(callee::flush, 0x1076a11c);
    c.set(3, c.u32(frame.at(saved_surface)));
    c.call(callee::free_bitmap, 0x1076a124);
    c.put8(showing, 0);
}
}
