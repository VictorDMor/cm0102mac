// DISPLAY.CPP block "input".
DISPLAY_ROUTINE(0x1071a280, 0x1071aa10, R3, "597178760b770d3e", [](Cpu& c) { c.set(3, display::pointer_press(c, c.r(3))); })
DISPLAY_ROUTINE(0x1071aa10, 0x1071add0, R3, "a2d8f4e6b9b707d8", [](Cpu& c) { c.set(3, display::pointer_nested(c, c.r(3))); })
DISPLAY_ROUTINE(0x1071b520, 0x1071bcf8, R3, "28539945da46cfe5", [](Cpu& c) { c.set(3, display::text_field_key(c, c.r(3), c.r(4))); })
DISPLAY_ROUTINE(0x1071bd00, 0x1071c070, R3, "cbe2fd14cb76ac9c", [](Cpu& c) { c.set(3, display::scroll_bar(c, c.r(3), c.r(4))); })
DISPLAY_ROUTINE(0x1071c190, 0x1071c1d0, R3, "47c3f6eb1c1cc994", [](Cpu& c) { c.set(3, display::pointer_inside(c, c.r(4), c.r(5), c.r(6), c.r(7))); })
