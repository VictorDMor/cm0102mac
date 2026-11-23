// Entries compared on the dead stack too (graph_carbon_checks.cpp): none yet. The stack contract is the leaver check (the twelve
// leavers of graph_carbon_audit.LEAVERS): the checker records each entry into one, on the original and on the C++ side, with the stack
// pointer relative to the checked entry's, the link register and the registers its prologue saves. It is live once a leaver is hooked.
// text: 0x1076ca50, 0x1076d1e0 and 0x1076d470 build the original's frames (the saved registers, the link register, the back chain, the
// stores into the caller's parameter area and their own locals) and call the callees (the non-leaf 0x1076d5a0 included, typed with its frame)
// at their own stack pointer with the callee-saved registers as the original holds them there.
0x1076ca50, 0x1076d1e0, 0x1076d470, 0x1076c0b0,
// colours: the eight entries build the original's frames (the saved registers, the link register, the back chain, the locals: the record
// 0x1076b020 clones from, the buffers of 0x1076b2f0 and 0x1076b450) and call the allocator, the file layer and the byte move at their own stack
// pointer with the callee-saved registers as the original holds them there. 0x1076b7e0 is the leaver (r31).
0x1076b830, 0x1076b6e0, 0x1076b7e0, 0x1076b020, 0x1076b140, 0x1076b240, 0x1076b2f0, 0x1076b450,
// surface (0x107692f0, the leaver r26): its frame (the saved r26-r31, the link register, the back chain) is what MAIN's printf reads; the C++ builds it with guest::Frame.
0x107692f0,
// frames: 0x1076d6e0 (the leaver r17) builds the original's frame (the saved r17-r31, the link register, the back chain; no locals) and calls the
// pixel primitives (0x1076a140, 0x1076a580, 0x1076ad50, 0x10769980) at its own stack pointer with r17-r31 as the original holds them.
0x1076d6e0,
// pixels: the frames of 0x1076a140 (r19), 0x10769980 (r24), 0x1076a790 (r30), 0x1076ad50, 0x1076a580 and 0x107697c0, the stack word 0x1076ad50 hands the octants at its sp + 0x38 and the
// octants' own stmw below their stack pointer are the original's; the callees run at their own stack pointer with the registers the original holds there.
0x1076a140, 0x10769980, 0x1076a790, 0x1076ad50, 0x1076a8f0, 0x1076a580, 0x107697c0,
