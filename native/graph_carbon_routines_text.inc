// graph_carbon.cpp, block `text` (graph_carbon_text.cpp, graph_carbon_text_fonts.cpp, graph_carbon_text_files.cpp): the text drawing and measuring
// entry points and the font loaders. Every entry declares r3 as the result and leaves r4-r10 as the original does (the routines set them
// where the original writes them: nothing is exempt from the full comparison).
GRAPH_CARBON_ROUTINE(0x1076d5a0, 0x1076d6e0, R3, "281ed6eaf553cb81", [](Cpu& c) { graph_carbon::glyph_gap(c, c.r(3), c.r(4), c.r(5)); })
GRAPH_CARBON_ROUTINE(0x1076d470, 0x1076d598, R3, "4347141db3022171", [](Cpu& c) { graph_carbon::font_height(c, c.r(3)); })
GRAPH_CARBON_ROUTINE(0x1076d1e0, 0x1076d470, R3, "724899b89b67ad5d", [](Cpu& c) { graph_carbon::text_width(c, c.r(3), c.r(4)); })
GRAPH_CARBON_ROUTINE(0x1076ca50, 0x1076d198, R3, "550366aed59dd02f", [](Cpu& c) { graph_carbon::draw_text(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)); })
GRAPH_CARBON_ROUTINE(0x1076bbf0, 0x1076c010, R3, "a64b4045fbc56640", [](Cpu& c) { graph_carbon::load_fonts(c); })
GRAPH_CARBON_ROUTINE(0x1076c010, 0x1076c04c, R3, "47698214f9ee72ac", [](Cpu& c) { graph_carbon::delete_cleanup_object(c, 0x1076c010, c.r(3), c.r(4)); })
GRAPH_CARBON_ROUTINE(0x1076c050, 0x1076c0a4, R3, "a3bfa2b2a923423e", [](Cpu& c) { graph_carbon::free_all_fonts(c); })
GRAPH_CARBON_ROUTINE(0x1076c0b0, 0x1076c974, R3, "90d0ff207d06b72f", [](Cpu& c) { graph_carbon::load_bitmap_font(c, c.r(3), c.r(4)); })
GRAPH_CARBON_ROUTINE(0x1076c980, 0x1076ca18, R3, "ce454977b33b0dfb", [](Cpu& c) { graph_carbon::free_font_pixels(c, c.r(3)); })
GRAPH_CARBON_ROUTINE(0x1076d1a0, 0x1076d1e0, R3, "17f2f0ee5a9cfecd", [](Cpu& c) { graph_carbon::delete_cleanup_object(c, 0x1076d1a0, c.r(3), c.r(4)); })
GRAPH_CARBON_ROUTINE(0x1076ea70, 0x1076eaa0, R3, "22ca3fc74ef7a65e", [](Cpu& c) { graph_carbon::destroy_font_array(c); })
// The message panel and the text box (graph_carbon_text_panel.cpp, graph_carbon_text_box.cpp): leavers (r24 and r18); the panel passes its profiling markers itself.
// DRAFT (2026-11-23): the host's HD drawing observes instructions inside this routine (tools/native_drawing_audit.py, tools/adaptive_gui_audit.py); until the C++ passes them, the game runs the original.
GRAPH_CARBON_DRAFT(0x1076e5f0, 0x1076e7c0, R3, "7a0c8ac972bb7227", [](Cpu& c) { graph_carbon::message_panel(c, c.r(3), c.r(4), c.r(5)); })
GRAPH_CARBON_ROUTINE(0x1076e160, 0x1076e5e4, R3, "dee94778e4147ffc", [](Cpu& c) { graph_carbon::text_box(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)); })
