// DISPLAY.CPP block "drawing".
DISPLAY_ROUTINE(0x10718f60, 0x10718fbc, NONE, "ec0698d85e4324c6", [](Cpu& c) { display::paint_first(c, c.r(3)); })
DISPLAY_ROUTINE(0x10718fc0, 0x10719348, R3, "c97ea3d9da3f7fb0", [](Cpu& c) { c.set(3, display::draw_screen(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
DISPLAY_ROUTINE(0x10719350, 0x10719554, R3, "7164351f24c27e3d", [](Cpu& c) { c.set(3, display::screen_step(c, c.r(3))); })
DISPLAY_ROUTINE(0x1071add0, 0x1071afb8, R3, "fc2027eb23fa30e2", [](Cpu& c) { c.set(3, display::scroll_bar_drag(c, c.r(3))); })
DISPLAY_ROUTINE(0x1071afc0, 0x1071b514, R3, "579d27d3142cef28", [](Cpu& c) { c.set(3, display::area_drag(c, c.r(3))); })
DISPLAY_ROUTINE(0x10725770, 0x10725998, NONE, "e62e5ce4a25db12a", [](Cpu& c) { display::update_blinking(c, c.r(3)); })
