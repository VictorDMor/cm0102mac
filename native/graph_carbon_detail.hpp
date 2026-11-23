#pragma once
// graph_carbon.cpp's record views and constants (tools/graph_carbon_audit.py, .local/rewrites/graph_carbon/brief.md). Offsets come from
// the listing and the decompilation; "?" marks one inferred from a single reader.
#include "graph_carbon.hpp"
#include "adaptive_gui.hpp"
#include "graph_carbon_adapter.hpp"
#include "guest_values.hpp"

namespace cm::aot::graph_carbon {
using namespace guest;

// Table of contents: slots that hold the address of a variable (read with Cpu::toc, then dereferenced).
namespace toc {
inline constexpr std::int32_t pool = -0x3bd0,         // the string pool ("graph_carbon.cpp" first)
    full_screen = -0x5ae8;                            // byte: the display mode flag
}
// The module's small data: variables the code reaches directly from r2 (no slot), -0x3580..-0x354f (see graph_carbon_audit.SMALL_DATA).
namespace small_data {
inline constexpr std::int32_t current_cursor = -0x3580,  // byte: the index of the cursor last set (0x1076e920)
    text_probe_captured = -0x357c,                       // int: the value the text callback kept (-1 before the drawer starts)
    text_probe_count = -0x3578,                          // int: the calls the text callback has had
    text_probe_limit = -0x3574;                          // int: the call at which it keeps its value
}
// The address of a small-data variable.
inline std::uint32_t small(const Cpu& c, std::int32_t offset) { return c.r(2) + std::uint32_t(offset); }

// An instruction the host's adaptive layout replaces (tools/adaptive_gui_audit.py ADAPTIVE_GUI_INSTRUCTIONS): the generated page calls
// adaptive_gui_instruction(s, pc) in place of it and, when the hook declines (the original canvas), runs the instruction. The C++ calls the
// same hook with the registers the instruction reads in the guest state, and when it declines does what the instruction does: `li reg,value`
// here. Returns the register after the instruction. The hook steps the instruction counter and so moves the PC; the PC is put back.
inline std::uint32_t loaded(Cpu& c, std::uint32_t pc, unsigned reg, std::uint32_t original) {
    const auto at = c.s.pc;
    const bool replaced = adaptive_gui_instruction(c.s, pc);
    c.s.pc = at;
    if (!replaced) c.set(reg, original);
    return c.r(reg);
}

// A Mac Rect in guest memory (QuickDraw): four signed halfwords.
struct Rect {
    Rect(Cpu& c, std::uint32_t at) : top(c, at), left(c, at + 2), bottom(c, at + 4), right(c, at + 6) {}
    Field<std::int16_t> top, left, bottom, right;
};
// The 15-bit pixel of the surfaces: red in bits 10-14, green in 5-9 and blue in 0-4.
namespace pixel {
inline constexpr std::uint32_t red_mask = 0x7c00, green_mask = 0x03e0, blue_mask = 0x001f;
}
}
