// FONT.CPP, block `record` (font_cpp_record.cpp): the font record's constructor and destructor (reached through the descriptors of the
// array the start-up builds), its release and the loader. All declare r3 and are compared on it and on r4-r10 in full.
FONT_CPP_ROUTINE(0x1074f390, 0x1074f404, R3, "355f199a76b04e11", [](Cpu& c) { font_cpp::new_font_record(c, c.r(3)); })
FONT_CPP_ROUTINE(0x1074f410, 0x1074f468, R3, "b9a0a85123bfcc14", [](Cpu& c) { font_cpp::delete_font_record(c, c.r(3), c.r(4)); })
FONT_CPP_ROUTINE(0x1074f470, 0x1074f800, R3, "79058f936de15667", [](Cpu& c) { font_cpp::load_font(c, c.r(3), c.r(4)); })
FONT_CPP_ROUTINE(0x10750200, 0x10750320, R3, "9a0017e4f42254bb", [](Cpu& c) { font_cpp::release_font(c, c.r(3)); })
