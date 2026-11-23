// graph_carbon.cpp, block `remap` (graph_carbon_remap.cpp): the screen fade. Not a stack-contract leaver, but it builds the original's frame
// (r19-r31 saved) because the host's drawing view reads the stack pointer, the rectangle at sp + 0x44 and r26/r28/r30 at its observation
// points; the C++ runs native_drawing_boundary at 0x107696cc and 0x107697b0 itself (OBSERVES_INTERIOR). A void routine: r3 is whatever the
// last callee left, compared with r4-r10 in full.
GRAPH_CARBON_ROUTINE(0x10769460, 0x107697b4, R3, "881fa9aa7ebf4ccf", [](Cpu& c) {
    graph_carbon::fade_rectangle(c, std::int32_t(c.r(3)), std::int32_t(c.r(4)), std::int32_t(c.r(5)), std::int32_t(c.r(6)), std::int32_t(c.r(7)));
})
