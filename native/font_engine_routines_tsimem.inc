// The font engine, block `tsimem` (font_engine_tsimem.cpp): the allocator over the game's malloc. The routines write r3 and the
// volatile registers the original leaves themselves; they call each other directly and the game's malloc, free, realloc,
// the assertion reporter and longjmp at the original's stack pointer (0x10767bb0 is a stack-contract leaver: its frame is read).
FONT_ENGINE_ROUTINE(0x10767460, 0x107675c0, R3, "90befeead1eba8b5", [](Cpu& c) { font_engine::new_memory_handler(c, c.r(3)); })
FONT_ENGINE_ROUTINE(0x107675c0, 0x10767684, R3, "c613c13378ecf033", [](Cpu& c) { font_engine::scratch_allocate(c, c.r(3), c.r(4), std::int32_t(c.r(5))); })
FONT_ENGINE_ROUTINE(0x10767690, 0x1076771c, NONE, "d76521fc64f5520f", [](Cpu& c) { font_engine::scratch_release_all(c, c.r(3)); })
FONT_ENGINE_ROUTINE(0x10767720, 0x10767814, NONE, "b06e611c8ba1c1e8", [](Cpu& c) { font_engine::delete_memory_handler(c, c.r(3)); })
FONT_ENGINE_ROUTINE(0x10767820, 0x10767894, NONE, "c5ee7bee86336866", [](Cpu& c) { font_engine::free_blocks(c, c.r(3)); })
FONT_ENGINE_ROUTINE(0x107678a0, 0x107679f4, R3, "f206cfcb70a47e20", [](Cpu& c) { font_engine::allocate_block(c, c.r(3), c.r(4)); })
FONT_ENGINE_ROUTINE(0x10767a00, 0x10767bb0, R3, "d59d13a3ec035d7c", [](Cpu& c) { font_engine::reallocate_block(c, c.r(3), c.r(4), c.r(5)); })
FONT_ENGINE_ROUTINE(0x10767bb0, 0x10767ce0, NONE, "2dce1fe08de3d6e0", [](Cpu& c) { font_engine::release_block(c, c.r(3), c.r(4)); })
