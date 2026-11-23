// Entries compared on the dead stack too (font_cpp_checks.cpp): none yet. The stack contract is the leaver check (the four leavers
// 0x1074f800, 0x107500f0, 0x10750320 and 0x10750580): the checker records each entry into one, on the original and on the C++
// side, with the stack pointer relative to the checked entry's, the link register and the registers its prologue saves.
// drawer: 0x1074f800 builds the original's frame (the saved registers, the slots it stores into its caller's frame and its own locals at
// 0x38, 0x40 and 0x44) and calls the style selector, the glyph cache, the assertion reporter and the callback's glue at its own stack
// pointer with r13-r31 as the original holds them, and itself for the outline style.
0x1074f800,
// selector: 0x10750320 builds the original's frame (stmw r24, 0xa0) with every local the original stores (the callbacks record, the matrix,
// the converted word and the conversion's double) and calls the engine through the dispatcher at its own stack pointer with r24-r31 as the
// original holds them.
0x10750320,
// glyphs: 0x10750580 and 0x107500f0 build the original's frames (stmw r23 / r26) and call the engine, the allocator, the assertion reporter and
// each other (the string width calls the selector and the glyph cache) at their own stack pointer with the registers the original holds there.
0x10750580, 0x107500f0,
