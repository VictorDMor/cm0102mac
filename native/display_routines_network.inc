// DISPLAY.CPP block "network".
DISPLAY_ROUTINE(0x10723a30, 0x10723ca4, NONE, "d0685b9a0111325c", [](Cpu& c) { display::send_ready(c, c.r(3), c.r(4)); })
DISPLAY_ROUTINE(0x10723cb0, 0x10725770, R3, "4f280e23da78f67d", [](Cpu& c) { c.set(3, display::send_gui_object(c, c.r(3), c.r(4), c.r(5)) ? 1 : 0); })
// part 1: sending.
DISPLAY_ROUTINE(0x1071c1d0, 0x1071c2cc, NONE, "6b1764be35f1c1ed", [](Cpu& c) { display::send_screen(c, c.r(3), c.r(4), c.r(5)); })
DISPLAY_ROUTINE(0x1071c2d0, 0x1071de80, R3, "59c2e76b86f03d4b", [](Cpu& c) { display::send_screen_part(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
DISPLAY_ROUTINE(0x10723650, 0x10723844, NONE, "cd2cd57daca44b5d", [](Cpu& c) { display::localize_resource_path(c, c.r(3), c.r(4)); })
DISPLAY_ROUTINE(0x10723850, 0x10723a24, NONE, "4a6472e69885c2e1", [](Cpu& c) { display::send_reply(c, c.r(4), c.r(5), c.r(6)); })
