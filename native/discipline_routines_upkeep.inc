// DISCIPLINE.CPP upkeep block (discipline_routines.inc).
DISCIPLINE_ROUTINE(0x100df360, 0x100df4fc, NONE, "31c4402d1a5627c3", [](Cpu& c) { discipline::drop_out_of_scope(c, c.r(3), c.r(4), c.r(6)); c.finish(); })
DISCIPLINE_ROUTINE(0x100df500, 0x100df6c4, NONE, "50d54e4ab0302a31", [](Cpu& c) { discipline::defer_out_of_scope(c, c.r(3), c.r(4)); c.finish(); })
DISCIPLINE_ROUTINE(0x100df6d0, 0x100df950, NONE, "3e7da3469fafa486", [](Cpu& c) { discipline::restore_in_scope(c, c.r(3), c.r(4)); c.finish(); })
DISCIPLINE_ROUTINE(0x100df9f0, 0x100dfb28, NONE, "404172b9268560cb", [](Cpu& c) { discipline::clear_person(c, c.r(3), c.r(4)); c.finish(); })
DISCIPLINE_ROUTINE(0x100e0700, 0x100e0b6c, NONE, "89ba96a03b175419", [](Cpu& c) { discipline::post_ban_news(c, c.r(4), c.r(5), c.r(6)); c.finish(); })
