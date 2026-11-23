// FONT.CPP, block `glyphs` (font_cpp_glyphs.cpp): the glyph cache (stmw r23) and the string width (stmw r26), both stack-contract leavers.
// Both return in r3 (the glyph record's address, the width as a sign-extended halfword) and leave r4-r10 as the original.
// DRAFT (2026-11-23): the host's HD drawing observes instructions inside this routine (tools/native_drawing_audit.py, tools/adaptive_gui_audit.py); until the C++ passes them, the game runs the original.
FONT_CPP_DRAFT(0x10750580, 0x107509f4, R3, "195c9e9be340eb63", [](Cpu& c) { c.set(3, font_cpp::glyph_record(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
// DRAFT with it: its typed call of the glyph cache would skip the cache's observation points.
FONT_CPP_DRAFT(0x107500f0, 0x10750200, R3, "271adda3ba688ac9", [](Cpu& c) { c.set(3, std::uint32_t(font_cpp::string_width(c, c.r(3), c.r(4), c.r(5), c.r(6)))); })
