// FONT.CPP, block `glyphs` (font_cpp_glyphs.cpp): the glyph cache (stmw r23) and the string width (stmw r26), both stack-contract leavers.
// Both return in r3 (the glyph record's address, the width as a sign-extended halfword) and leave r4-r10 as the original.
// The host's HD text layer observes the cache's entry 0x10750580 (the page runs it for every caller that reaches the entry through the
// dispatcher), 0x10750740 (the `bl` of the engine's render) and 0x107509f0 (the `blr`): the C++ passes the last two itself
// (tools/font_cpp_audit.py HANDLED_POINTS). Routed again 2026-11-23 on the checker's boundary log.
FONT_CPP_ROUTINE(0x10750580, 0x107509f4, R3, "195c9e9be340eb63", [](Cpu& c) { c.set(3, font_cpp::glyph_record(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
// The string width calls the glyph cache typed: it passes the cache's entry observation itself (HANDLED_CALLS).
FONT_CPP_ROUTINE(0x107500f0, 0x10750200, R3, "271adda3ba688ac9", [](Cpu& c) { c.set(3, std::uint32_t(font_cpp::string_width(c, c.r(3), c.r(4), c.r(5), c.r(6)))); })
