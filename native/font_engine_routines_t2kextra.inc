// The font engine, block `t2kextra` (font_engine_t2kextra.cpp): the colour table rendering for command bit 0x80 (dormant for the game).
// It leaves r3-r10 as the original on every path (0x10760e30 returns to FONT.CPP with r4-r10 unwritten): compared in full. It builds the
// original's frame and holds the original's r24-r31 at its calls to tsi_AllocMem and the leaver tsi_DeAllocMem.
FONT_ENGINE_ROUTINE(0x10761070, 0x10761268, NONE, "6e598cde415e7d43", [](Cpu& c) { font_engine::render_color_table(c, c.r(3)); })
