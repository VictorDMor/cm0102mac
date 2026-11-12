// DISCIPLINE.CPP news block (discipline_routines.inc).
DISCIPLINE_ROUTINE(0x100e0b70, 0x100e59b0, NONE, "35d67936db7bcfc1", [](Cpu& c) { discipline::ban_news_text(c, c.r(3), c.r(4), std::int8_t(c.r(5)), c.r(6)); c.finish(); })
