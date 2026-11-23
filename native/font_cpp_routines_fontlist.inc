// FONT.CPP, block `fontlist` (font_cpp_fontlist.cpp): the font list's folder finder, its lazy-build wrappers (the loader, the selection
// setter, the count and the name getter). All five declare r3 as a result and are compared on it and on r4-r10 in full: every register
// is left as the original leaves it (the checker has no exemption for this module, font_cpp_volatile_*.inc are empty).
FONT_CPP_ROUTINE(0x10750a00, 0x10750a54, R3, "be3a203bc3b5faaa", [](Cpu& c) { font_cpp::load_font_list(c); })
FONT_CPP_ROUTINE(0x10750a60, 0x10750afc, R3, "31764446752079fd", [](Cpu& c) { font_cpp::select_font(c, std::int32_t(c.r(3))); })
FONT_CPP_ROUTINE(0x10750b00, 0x10750b5c, R3, "145c131c9d7c2a2f", [](Cpu& c) { font_cpp::font_count(c); })
FONT_CPP_ROUTINE(0x10750b60, 0x10750c9c, R3, "15f45265c366e6d0", [](Cpu& c) { font_cpp::font_name(c, std::int32_t(c.r(3)), c.r(4), std::int32_t(c.r(5))); })
FONT_CPP_ROUTINE(0x10750ca0, 0x10750d28, R3, "0b002f8b8c824187", [](Cpu& c) { font_cpp::locate_font_folder(c); })
