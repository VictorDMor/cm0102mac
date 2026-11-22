// The font engine, block `t2ksc` (font_engine_t2ksc.cpp): the scan converter. Three are stack leavers (0x107617b0, 0x10761d50, 0x10762e30: the game's
// printf reads the words their prologues leave) and every routine builds the original's frame and holds the original's registers at its calls.
// The callers read r3, r9 and r10 after the calls (tools/register_contract.py --after), so r9 and r10 are left as the original does
// (font_engine_volatile_read.inc: the mask 0x60); 0x10762e30 leaves r4-r10 as the original too.
FONT_ENGINE_ROUTINE(0x10761270, 0x107614c0, R3, "d02ccb40541c2fea", [](Cpu& c) { c.set(3, font_engine::new_edge_chunk(c, c.r(3))); })
FONT_ENGINE_ROUTINE(0x107614c0, 0x10761730, NONE, "ff8bec691aaab880", [](Cpu& c) { font_engine::bounds_from_edges(c, c.r(3)); })
FONT_ENGINE_ROUTINE(0x10761730, 0x107617ac, NONE, "667196286a31a9e5", [](Cpu& c) { font_engine::point_extents(c, c.r(3), c.r(4), c.r(5), std::int32_t(c.r(6))); })
FONT_ENGINE_ROUTINE(0x107617b0, 0x10761d44, NONE, "c2e994beca6ae3ea", [](Cpu& c) { font_engine::add_line(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
FONT_ENGINE_ROUTINE(0x10761d50, 0x10761eb8, NONE, "1fa55aa23f0a4e89", [](Cpu& c) { font_engine::add_curve(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
FONT_ENGINE_ROUTINE(0x10761ec0, 0x107620d0, NONE, "07e5f42d1eb2ad82", [](Cpu& c) { font_engine::add_contours(c, c.r(3)); })
FONT_ENGINE_ROUTINE(0x107620d0, 0x107623b8, R3, "d2f83d9ec1b6118a", [](Cpu& c) { c.set(3, font_engine::new_scan_converter(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10))); })
FONT_ENGINE_ROUTINE(0x107623c0, 0x10762e2c, NONE, "c908c5dc62381a0b", [](Cpu& c) { font_engine::render_bitmap(c, c.r(3), c.r(5)); })
FONT_ENGINE_ROUTINE(0x10762e30, 0x10762f40, NONE, "cde913d57fbfbd66", [](Cpu& c) { font_engine::delete_scan_converter(c, c.r(3)); })
