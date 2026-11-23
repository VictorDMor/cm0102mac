// graph_carbon.cpp, block `box` (graph_carbon_box.cpp): the assertion box 0x10769b60 and the version-text getter 0x10768810 (6,402 callers: hooking it
// changes the call sites of every module's assertion paths). Both are compared on r3-r10 in full on every path.
GRAPH_CARBON_ROUTINE(0x10768810, 0x10768818, R3, "b9902cc980c9bd4d", [](Cpu& c) { c.set(3, graph_carbon::version_text(c)); })
GRAPH_CARBON_ROUTINE(0x10769b60, 0x1076a140, R3, "6c4d68e0be4bb501", [](Cpu& c) { graph_carbon::assertion_box(c, c.r(3), c.r(4), c.r(5)); })
