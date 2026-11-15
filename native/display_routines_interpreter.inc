// DISPLAY.CPP block "interpreter".
DISPLAY_ROUTINE(0x1071de80, 0x10723650, R3, "71d88348d3337988", [](Cpu& c) { c.set(3, display::interpret(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
