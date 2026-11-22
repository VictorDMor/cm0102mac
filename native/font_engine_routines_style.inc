// The font engine, block `style` (font_engine_style.cpp): the bold style functions the sfnt layer calls through pointers (and the
// corner intersection the outline routine calls). Each builds the original's frame. The two pointer-called ones leave r3-r10 exactly
// as the original on every path and are compared in full (their return reaches generated code with r4-r10 unwritten); the corner
// routine is volatile-free (font_engine_volatile_free.inc).
FONT_ENGINE_ROUTINE(0x1075e0c0, 0x1075e1a0, NONE, "cdbac53a67a70692", [](Cpu& c) {
    font_engine::offset_corner(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10), c.u32(c.sp() + 0x38), c.u32(c.sp() + 0x3c)); })
FONT_ENGINE_ROUTINE(0x1075e1a0, 0x1075e31c, NONE, "fc56901655b13af0", [](Cpu& c) { font_engine::embolden_advances(c, c.r(3), c.r(5), c.r(6)); })
FONT_ENGINE_ROUTINE(0x1075e320, 0x1075e958, NONE, "a75ab4dfd0386305", [](Cpu& c) { font_engine::embolden_outline(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
