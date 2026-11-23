// graph_carbon.cpp, block `text` (the message panel): 0x1076e5f0 draws the game's one-line message panel with an optional progress bar.
//
// It pumps the events (MAIN.CPP 0x1076fe80), asks the text of the current club's window to be redrawn when `step` is 0 (0x10718f60 of the club record
// at the table of TOC -0x7bb0: 0x2c0 bytes a club from the club index at +0x2c58), draws the framed 100,555 .. 790,590 panel (0x1076d6e0) and
// its text through the text box 0x1076e160, and, with a total above 0 and a step not above it, a bar (0x1076a580) of `step` / `total` of the
// 0x1f0 pixels, then puts the panel on the window (0x107692f0). 1 when drawn; 0 when the graphics are not initialised, and the assertion of line 0xe06
// (then 0) for no text.
//
// STACK CONTRACT: a leaver (stmw r24): the frame is the original's (0x90) with the text box's ninth argument at 0x38; every call is made at its own stack
// pointer with r24-r31 as the original holds them (the leaver check compares them for the nested leavers 0x1076d6e0, 0x1076e160 and 0x107692f0).
//
// PROFILING MARKERS. The original's entry (0x1076e5f0, "Progress draw begin"), its redraw call (0x1076e7a4, "Progress submit") and its `blr`
// (0x1076e7bc, "Progress draw end") carry profiling events (tools/profiling_boundaries.py). The generated page emits the entry's before the hook
// runs; this C++ emits the other two where the original passes them: "submit" just before the flush, "end" after the epilogue (every path that
// returns, not the ones a context transfer abandons).
#include "graph_carbon_text_detail.hpp"
#include "profiling.hpp"

namespace cm::aot::graph_carbon {
namespace {
std::uint32_t panel_body(Cpu& c, std::uint32_t text, std::uint32_t step, std::uint32_t total) {
    Frame frame(c, 0x1076e5f0, 0x90, {.first_gpr = 24});
    const auto pool = c.toc(toc::pool);
    const auto clubs = c.toc(-0x7bb0), text_colour = c.toc(-0x7bc8), frame_colour = c.toc(-0x7bc4), buffer = c.toc(text_toc::message_buffer);
    c.set({{27, clubs}, {24, text}, {28, text_colour}, {25, step}, {29, frame_colour}, {26, total}, {30, buffer}, {31, pool}});
    if (std::int8_t(c.u8(small(c, text_small::initialised))) != 0) { c.set(3, 0); return 0; }
    c.call(0x1076fe80, 0x1076e638);
    if (text == 0) {
        guest::report_assertion(c, {0x1076e654, pool, pool + text_pool::format, pool + text_pool::title, frame.at(0x48), frame.at(0x40), 0xe06},
                                [&](std::uint32_t entry, std::uint32_t back) { c.call(entry, back); });
        c.set({{4, c.toc(text_toc::message_flag)}, {3, 0}});
        return 0;
    }
    if (step == 0) {
        c.set(3, c.u32(clubs + std::uint32_t(c.i16(clubs + 0x2c58)) * 0x2c0 + 0x24));
        c.call(0x10718f60, 0x1076e6b8);
    }
    // The panel: a shaded frame, then the text and (with a total) the bar's frame and fill. Each coordinate below is a `li` the host's adaptive
    // layout replaces (17 instructions, the bottom bar: tools/adaptive_gui_audit.py): `loaded` runs the hook at the original's address.
    // `right_pc` is the address of the `li r5` of a right edge that is adaptive too (0: the edge is the fixed 0xfa of the bar's case).
    const auto text_box = [&](std::uint32_t right, std::uint32_t back, std::uint32_t top_pc, std::uint32_t right_pc, std::uint32_t bottom_pc) {
        c.put32(frame.at(0x38), 0xffffffffu);  // the text box's ninth argument: no limit
        c.set({{10, text}, {3, 0x78}, {9, c.u16(text_colour)}, {7, 0xc}, {8, 1}});
        loaded(c, top_pc, 4, 0x235);
        if (right_pc != 0) loaded(c, right_pc, 5, right);
        else c.set(5, right);
        loaded(c, bottom_pc, 6, 0x244);
        c.call(0x1076e160, back);
    };
    c.set({{8, c.u16(frame_colour)}, {3, 0x64}, {7, 0x30}, {9, 0}});
    loaded(c, 0x1076e6c0, 4, 0x22b);
    loaded(c, 0x1076e6c4, 5, 0x316);
    loaded(c, 0x1076e6c8, 6, 0x24e);
    c.call(0x1076d6e0, 0x1076e6d8);
    if (std::int32_t(total) > 0 && std::int32_t(step) <= std::int32_t(total)) {
        text_box(0xfa, 0x1076e714, 0x1076e6f8, 0, 0x1076e704);
        c.set({{8, c.u16(frame_colour)}, {3, 0x10e}, {7, 0x60}, {9, 0}});
        loaded(c, 0x1076e71c, 4, 0x235);
        loaded(c, 0x1076e720, 5, 0x302);
        loaded(c, 0x1076e724, 6, 0x244);
        c.call(0x1076d6e0, 0x1076e734);
        c.set(3, 0x110);
        const auto bar_end = loaded(c, 0x1076e738, 0, 0x300);
        const auto span = bar_end - 0x110u;
        const auto fill = c.toc(-0x7bdc);
        const auto share = std::int32_t(step * span) / std::int32_t(total);
        c.set({{5, 0x110 + std::uint32_t(share)}, {8, c.u16(fill)}, {7, 4}});
        loaded(c, 0x1076e748, 4, 0x237);
        loaded(c, 0x1076e750, 6, 0x242);
        c.call(0x1076a580, 0x1076e764);
    } else {
        text_box(0x302, 0x1076e794, 0x1076e778, 0x1076e77c, 0x1076e784);
    }
    c.set(3, 0x64);
    loaded(c, 0x1076e798, 4, 0x22b);
    loaded(c, 0x1076e79c, 5, 0x316);
    loaded(c, 0x1076e7a0, 6, 0x24e);
    CM_PROFILE_EVENT("Progress submit");
    c.call(0x107692f0, 0x1076e7a8);
    c.set(3, 1);
    return 1;
}
}

std::uint32_t message_panel(Cpu& c, std::uint32_t text, std::uint32_t step, std::uint32_t total) {
    const auto drawn = panel_body(c, text, step, total);  // its Frame has left: the epilogue is done
    CM_PROFILE_EVENT("Progress draw end");
    return drawn;
}
}
