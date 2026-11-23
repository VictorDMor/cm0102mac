// FONT.CPP, block `drawer` (font_cpp_drawer.cpp): the text drawer, a stack-contract leaver (stmw r13) that calls itself for the outline
// style. It declares r3 as the result and leaves r4-r10 as the original does on every path (no exemption). The ninth argument is the
// stack word at the caller's sp + 0x38 (the callback descriptor or 0), which the routine reads itself.
// The host's HD text layer observes four instructions of this routine (tools/native_drawing_audit.py: 0x1074f800, 0x1074fb58, 0x107500b4,
// 0x107500e8): the C++ passes the last three itself and the outline recursion's entry (font_cpp_drawer.cpp), the page runs the entry for
// every other caller (FONT_CPP HANDLED_POINTS / HANDLED_CALLS, tools/font_cpp_audit.py). Routed again 2026-11-23 after the checker's boundary log
// (native/native_drawing_log.hpp) showed the same events with the same observed state on both sides.
FONT_CPP_ROUTINE(0x1074f800, 0x107500ec, R3, "9c2376c35dbc123e", [](Cpu& c) {
    font_cpp::draw_text(c, {c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8) & 0xff, c.r(9), c.r(10)});
})
