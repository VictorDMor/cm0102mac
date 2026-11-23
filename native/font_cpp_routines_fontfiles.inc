// FONT.CPP, block `fontfiles` (font_cpp_fontfiles.cpp): the resource file helpers, the font data finder and file opener, the name reader and
// the list builder. All declare r3 and are compared on it and on r4-r10 in full (font_cpp_volatile_*.inc are empty on purpose).
FONT_CPP_ROUTINE(0x1074d380, 0x1074d448, R3, "d7cdd3fef96cbc27", [](Cpu& c) { font_cpp::open_resource_file(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
FONT_CPP_ROUTINE(0x1074d450, 0x1074d4bc, R3, "40b3b9c5a51e0ece", [](Cpu& c) { font_cpp::close_resource_file(c, c.r(3), c.r(4)); })
FONT_CPP_ROUTINE(0x1074f170, 0x1074f2c0, R3, "c93c19a2bba8caac", [](Cpu& c) { font_cpp::find_font_data(c, c.r(3), c.r(4), c.r(5)); })
FONT_CPP_ROUTINE(0x1074f2c0, 0x1074f384, R3, "8bc1a89955310544", [](Cpu& c) { font_cpp::open_font_file(c, c.r(3), c.r(4), c.r(5)); })
FONT_CPP_ROUTINE(0x10750d30, 0x10750f00, R3, "a9274a5386d7d2fe", [](Cpu& c) { font_cpp::scan_font_folder(c); })
FONT_CPP_ROUTINE(0x10750f00, 0x10751348, R3, "b78bd64134e1204a", [](Cpu& c) { font_cpp::read_font_name(c, c.r(3), c.r(4), c.r(5)); })
