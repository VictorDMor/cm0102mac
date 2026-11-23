// graph_carbon.cpp, block `lines` (graph_carbon_lines.cpp): the line, the circle's octants and the ellipse. Every entry is compared on r3 and
// r4-r10 in full (the C++ models the original's loop registers). 0x1076a140 is a leaver (stack contract) and, with 0x1076ad50, runs the
// native drawing observation at its return (graph_carbon_audit.OBSERVES_INTERIOR).
GRAPH_CARBON_ROUTINE(0x1076a140, 0x1076a574, R3, "6a01d94fc10f5354", [](Cpu& c) {
    graph_carbon::draw_line(c, {std::int32_t(c.r(3)), std::int32_t(c.r(4)), std::int32_t(c.r(5)), std::int32_t(c.r(6)), c.r(7), c.r(8)});
})
GRAPH_CARBON_ROUTINE(0x1076a8f0, 0x1076ad50, R3, "e96c24745594dbe4", [](Cpu& c) {
    // The ninth argument is the word at the caller's sp + 0x38 (this routine builds no frame).
    graph_carbon::Leftovers left{c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)};
    graph_carbon::circle_octants(c, c.r(3), std::int32_t(c.r(4)), std::int32_t(c.r(5)), std::int32_t(c.r(6)), std::int32_t(c.r(7)),
                                 std::int32_t(c.r(8)), std::int32_t(c.r(9)), c.r(10), c.u32(c.sp() + 0x38), left);
    c.set({{4, left.r4}, {5, left.r5}});
})
GRAPH_CARBON_ROUTINE(0x1076ad50, 0x1076b018, R3, "43a01da42fd3602b", [](Cpu& c) {
    graph_carbon::draw_ellipse(c, {std::int32_t(c.r(3)), std::int32_t(c.r(4)), std::int32_t(c.r(5)), std::int32_t(c.r(6)), c.r(7), c.r(8)});
})
