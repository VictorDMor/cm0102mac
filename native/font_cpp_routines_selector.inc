// FONT.CPP, block `selector` (font_cpp_selector.cpp): the style selector, a stack-contract leaver (stmw r24). It declares r3 and f1 (the
// product of the skew constant and the sine, or what the glue left when the skew is not recomputed) and leaves r4-r10 as the original.
FONT_CPP_ROUTINE(0x10750320, 0x10750580, R3 | F1, "e5009ac1e6528e08", [](Cpu& c) { font_cpp::select_style(c, c.r(3), c.r(4), c.r(5)); })
