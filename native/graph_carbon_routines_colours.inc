// graph_carbon.cpp, block `colours` (graph_carbon_colours.cpp): the colour variables and the bitmaps. Every entry builds the original's frame
// and leaves r3-r10 as the original does (the checker compares them in full). 0x1076b7e0 is a stack-contract leaver (stmw r31 at its
// frame); 0x1076b020 and 0x1076b140 run the native drawing observation at their return (0x1076b138, 0x1076b234): OBSERVES_INTERIOR.
GRAPH_CARBON_ROUTINE(0x1076b830, 0x1076bb14, R3, "f31f29ddd3259622", [](Cpu& c) { graph_carbon::colour_variables(c); })
GRAPH_CARBON_ROUTINE(0x1076b6e0, 0x1076b7dc, R3, "ac6755dbc5dfb28c", [](Cpu& c) { graph_carbon::clone_bitmap(c, c.r(3), c.r(4)); })
GRAPH_CARBON_ROUTINE(0x1076b7e0, 0x1076b824, R3, "95103b089ab2b420", [](Cpu& c) { graph_carbon::free_bitmap(c, c.r(3)); })
GRAPH_CARBON_ROUTINE(0x1076b020, 0x1076b13c, R3, "3903ee95792fd27a", [](Cpu& c) { graph_carbon::capture_rectangle(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
GRAPH_CARBON_ROUTINE(0x1076b140, 0x1076b238, R3, "083cc089b8461f76", [](Cpu& c) { graph_carbon::blit_bitmap(c, c.r(3), c.r(4), c.r(5)); })
GRAPH_CARBON_ROUTINE(0x1076b240, 0x1076b2e8, R3, "45881a05f2c66910", [](Cpu& c) { graph_carbon::save_region(c, c.r(3), c.r(4)); })
GRAPH_CARBON_ROUTINE(0x1076b2f0, 0x1076b450, R3, "3b7f8a0dc9b6a2f6", [](Cpu& c) { graph_carbon::load_region(c, c.r(3), c.r(4)); })
GRAPH_CARBON_ROUTINE(0x1076b450, 0x1076b6d4, R3, "35dfe2b129351d54", [](Cpu& c) { graph_carbon::read_bitmap(c, c.r(3), c.r(4)); })
