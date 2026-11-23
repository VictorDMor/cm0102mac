// graph_carbon.cpp, block `fills` (graph_carbon_fills.cpp): the rectangle primitives. Every entry is compared on r3 and r4-r10 in full.
// 0x10769980 and 0x1076a790 are leavers (stack contract); 0x107697c0, 0x10769980 and 0x1076a580 run the native drawing observation at their
// return (graph_carbon_audit.HANDLED_POINTS).
GRAPH_CARBON_ROUTINE(0x107697c0, 0x107698c8, R3, "e61581f5f79439b5", [](Cpu& c) {
    graph_carbon::shade_to_grey(c, std::int32_t(c.r(3)), std::int32_t(c.r(4)), std::int32_t(c.r(5)), std::int32_t(c.r(6)));
})
GRAPH_CARBON_ROUTINE(0x10769980, 0x10769b4c, R3, "1eec4bbbbb7b148f", [](Cpu& c) {
    graph_carbon::shade_rectangle(c, std::int32_t(c.r(3)), std::int32_t(c.r(4)), std::int32_t(c.r(5)), std::int32_t(c.r(6)));
})
GRAPH_CARBON_ROUTINE(0x1076a580, 0x1076a788, R3, "bd277c818bf68a33", [](Cpu& c) {
    graph_carbon::fill_rectangle(c, std::int32_t(c.r(3)), std::int32_t(c.r(4)), std::int32_t(c.r(5)), std::int32_t(c.r(6)), c.r(7), c.r(8));
})
GRAPH_CARBON_ROUTINE(0x1076a790, 0x1076a8ec, R3, "bc1e5a50f43e18ca", [](Cpu& c) {
    graph_carbon::draw_mask(c, std::int32_t(c.r(3)), std::int32_t(c.r(4)), c.r(5), c.r(6));
})
