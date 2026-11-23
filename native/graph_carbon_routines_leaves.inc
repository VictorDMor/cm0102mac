// graph_carbon.cpp, block `leaves` (graph_carbon_leaves.cpp): the leaf routines. Every entry is compared on r3 and r4-r10 in full: the
// hooks start from the registers the entry was called with and the C++ overwrites the ones the original writes (Leftovers), so each
// leaves the original's r3-r10 on every path (r0 is a diagnostic, cr0 is a volatile condition field).
#define GRAPH_CARBON_LEFTOVERS(c) graph_carbon::Leftovers left{c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)}
#define GRAPH_CARBON_SET_LEFTOVERS(c) c.set({{4, left.r4}, {5, left.r5}, {6, left.r6}, {7, left.r7}, {8, left.r8}, {9, left.r9}, {10, left.r10}})
GRAPH_CARBON_ROUTINE(0x10769b50, 0x10769b5c, R3, "64f4cd09be131057", [](Cpu& c) { c.set(3, graph_carbon::full_screen(c)); })
GRAPH_CARBON_ROUTINE(0x1076e910, 0x1076e918, R3, "76a58357dcf88f07", [](Cpu& c) { c.set(3, graph_carbon::current_cursor(c)); })
GRAPH_CARBON_ROUTINE(0x107698d0, 0x1076991c, R3, "ee2787a784de2f12", [](Cpu& c) {
    GRAPH_CARBON_LEFTOVERS(c);
    graph_carbon::rectangle_from_corners(c, std::int32_t(c.r(3)), std::int32_t(c.r(4)), std::int32_t(c.r(5)), std::int32_t(c.r(6)), c.r(7), left);
    GRAPH_CARBON_SET_LEFTOVERS(c);
})
GRAPH_CARBON_ROUTINE(0x10769920, 0x10769980, R3, "e7a36124f47d281e", [](Cpu& c) {
    GRAPH_CARBON_LEFTOVERS(c);
    const auto area = graph_carbon::intersection_area(
        {std::int32_t(c.r(3)), std::int32_t(c.r(4)), std::int32_t(c.r(5)), std::int32_t(c.r(6))},
        {std::int32_t(c.r(7)), std::int32_t(c.r(8)), std::int32_t(c.r(9)), std::int32_t(c.r(10))}, left);
    GRAPH_CARBON_SET_LEFTOVERS(c);
    c.set(3, area);
})
GRAPH_CARBON_ROUTINE(0x1076bb20, 0x1076bb34, R3, "0add5158886b3c2f", [](Cpu& c) { c.set(3, graph_carbon::pack_pixel(c.r(3), c.r(4), c.r(5))); })
GRAPH_CARBON_ROUTINE(0x1076bb40, 0x1076bbe8, R3, "aeaebf792535f274", [](Cpu& c) {
    GRAPH_CARBON_LEFTOVERS(c);
    const auto pixel = graph_carbon::scale_pixel(c.r(3), c.r(4), left);
    GRAPH_CARBON_SET_LEFTOVERS(c);
    c.set(3, pixel);
})
GRAPH_CARBON_ROUTINE(0x1076ca20, 0x1076ca4c, R3, "1c46c47760272e82", [](Cpu& c) {
    GRAPH_CARBON_LEFTOVERS(c);
    const auto result = graph_carbon::probe_text_position(c, c.r(3), left);
    GRAPH_CARBON_SET_LEFTOVERS(c);
    c.set(3, result);
})
#undef GRAPH_CARBON_SET_LEFTOVERS
#undef GRAPH_CARBON_LEFTOVERS
