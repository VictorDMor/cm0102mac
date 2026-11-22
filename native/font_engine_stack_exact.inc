// Entries compared on the dead stack too (font_engine_checks.cpp): the routine builds the original's frame, so every byte below its
// entry stack pointer must match. An entry is listed only if everything it calls inside the engine is generated or itself listed (a
// natively written callee that builds no frame leaves its depth of the stack different: the project's rule is per reader, not an exact
// image). The contract itself is the LEAVER CHECK (font_engine_checks.cpp, for every entry and every run): the original's and the C++'s
// entries into the five leavers (0x1075c830, 0x107617b0, 0x10761d50, 0x10762e30, 0x10767bb0) are recorded with the stack pointer relative
// to the checked entry's, the link register and the registers each leaver's prologue saves, and compared in order.
// Not listed, the leaver check covers them: 0x1075bc90 (calls 0x1075b970, 0x10762fd0, 0x10763350), 0x10765810 (the table readers, the
// stream readers, new_hint_font and the glue), 0x107664a0, 0x10766770, 0x107668d0 (all call the t2ktt_head and truetype_tables routines)
// and 0x10766ab0 (calls 0x10765810).
// One line per entry with the reason.
// tsimem: 0x10767bb0 is a stack-contract leaver (its saved r28-r31, link register and back chain are read by the game's printf); the
// others call it (or free, malloc, the reporter) at their own frame's stack pointer with the original's held registers.
0x10767460, 0x107675c0, 0x10767690, 0x10767720, 0x10767820, 0x107678a0, 0x10767a00, 0x10767bb0,
// t2ktt_head: the two routines that call the leaver 0x10767bb0 at their own frame's stack pointer (r30/r31 and r28/r29 as the original holds them).
0x107632a0, 0x107642e0,
// style: 0x1075e320 calls tsi_AllocMem and the leaver tsi_DeAllocMem at its own frame's sp with r14-r31 as the original holds them; the other two build their frames.
0x1075e0c0, 0x1075e1a0, 0x1075e320,
// t2kextra: 0x10761070 calls tsi_AllocMem and the leaver tsi_DeAllocMem at its own frame's sp with r24-r31 as the original holds them.
0x10761070,
// glyph: 0x1075c830 is a stack-contract leaver; the others build the original's frame and call it, the allocator's release (a leaver) or the reporter at their own stack pointer with the original's held registers.
0x1075bba0, 0x1075c0f0, 0x1075c830, 0x1075c8c0,
// truetype_glyph: every routine with a frame (0x10766dc0 and 0x1075b970 are leaves); they call the release leaver, the allocator, the readers and the reporter at their own stack pointer, and the two API entries setjmp on it.
0x10766430, 0x10766cc0, 0x10766d30, 0x107670e0, 0x10767190, 0x10767430,
// util (the pilot): the five leaf helpers (0x10767f20, the distance, has a frame the rewrite does not build: the leaver check covers it) are frameless that write nothing on the stack, so their dead stack matches; listed so that the entries that call them can be.
0x10767ce0, 0x10767d40, 0x10767dc0, 0x10767e40, 0x10767eb0,
