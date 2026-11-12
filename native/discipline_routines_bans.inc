// DISCIPLINE.CPP bans block (discipline_routines.inc).
DISCIPLINE_ROUTINE(0x100dfb30, 0x100e0090, NONE, "afaa28853a1ad78d", [](Cpu& c) { discipline::register_ban(c, c.r(3), c.r(4), c.r(5)); c.finish(); })
DISCIPLINE_ROUTINE(0x100e0090, 0x100e024c, NONE, "f12ce4347c816737", [](Cpu& c) { discipline::clear_ban(c, c.r(3), c.r(4), std::int16_t(c.r(5))); c.finish(); })
DISCIPLINE_ROUTINE(0x100e0250, 0x100e0404, NONE, "eae8517c6b7a510a", [](Cpu& c) { discipline::serve_ban(c, c.r(3), c.r(4), std::int8_t(c.r(5)), std::int8_t(c.r(6))); c.finish(); })
DISCIPLINE_ROUTINE(0x100e0410, 0x100e05b4, NONE, "ecf4db43befdec8b", [](Cpu& c) { discipline::reduce_ban(c, c.r(3), c.r(4), std::int8_t(c.r(5)), std::int8_t(c.r(6)), std::int8_t(c.r(7)), std::int16_t(c.r(8)), std::int8_t(c.r(9)), std::int16_t(c.r(10))); c.finish(); })
DISCIPLINE_ROUTINE(0x100e05c0, 0x100e06f8, NONE, "7ccc515344391d84", [](Cpu& c) { discipline::serve_key_holders(c, c.r(3), c.r(4)); c.finish(); })
