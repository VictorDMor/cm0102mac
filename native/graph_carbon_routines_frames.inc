// graph_carbon.cpp, block `frames` (graph_carbon_frames.cpp): the framed rectangle 0x1076d6e0, a stack-contract leaver (stmw r17, frame 0x80)
// that calls the pixel primitives with r17-r31 as the original holds them. A void routine: r3-r10 are what the last callee or the inline
// shading left, compared in full.
GRAPH_CARBON_ROUTINE(0x1076d6e0, 0x1076e158, R3, "679813c80923d8bb", [](Cpu& c) {
    graph_carbon::framed_rectangle(c, std::int32_t(c.r(3)), std::int32_t(c.r(4)), std::int32_t(c.r(5)), std::int32_t(c.r(6)), c.r(7), c.r(8), c.r(9));
})
