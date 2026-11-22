// The font engine, block `stream` (font_engine_stream.cpp): the input stream's readers. Each leaves r4/r5 as the original does
// inside the routine (read_unsigned_32 and read_signed_16 return the value in r3; read_bytes returns nothing).
FONT_ENGINE_ROUTINE(0x10762f40, 0x10762fd0, R3, "b444e9cd48b3889d", [](Cpu& c) { c.set(3, font_engine::read_unsigned_32(c, c.r(3))); })
FONT_ENGINE_ROUTINE(0x10762fd0, 0x1076304c, R3, "c56002caaa3c7b98", [](Cpu& c) { c.set(3, std::uint32_t(font_engine::read_signed_16(c, c.r(3)))); })
FONT_ENGINE_ROUTINE(0x10763050, 0x107630d8, NONE, "df4fe86c15a27f8f", [](Cpu& c) { font_engine::read_bytes(c, c.r(3), c.r(4), std::int32_t(c.r(5))); })
