// FONT.CPP, block `drawer` (font_cpp_drawer.cpp): the text drawer, a stack-contract leaver (stmw r13) that calls itself for the outline
// style. It declares r3 as the result and leaves r4-r10 as the original does on every path (no exemption). The ninth argument is the
// stack word at the caller's sp + 0x38 (the callback descriptor or 0), which the routine reads itself.
// DRAFT (2026-11-23): the host's HD drawing observes instructions inside this routine (tools/native_drawing_audit.py, tools/adaptive_gui_audit.py); until the C++ passes them, the game runs the original.
FONT_CPP_DRAFT(0x1074f800, 0x107500ec, R3, "9c2376c35dbc123e", [](Cpu& c) {
    font_cpp::draw_text(c, {c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8) & 0xff, c.r(9), c.r(10)});
})
