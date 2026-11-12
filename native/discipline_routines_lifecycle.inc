// DISCIPLINE.CPP lifecycle block (discipline_routines.inc).
DISCIPLINE_ROUTINE(0x100de410, 0x100de658, NONE, "b430ce5f240e237a", [](Cpu& c) { discipline::construct_manager(c, c.r(3), c.r(4)); c.finish(); })
DISCIPLINE_ROUTINE(0x100de660, 0x100de754, NONE, "dfd237296a306227", [](Cpu& c) { discipline::destroy_manager(c, c.r(3), std::int16_t(c.r(4))); c.finish(); })
DISCIPLINE_ROUTINE(0x100de760, 0x100ded98, R3, "28c7050822e192d1", [](Cpu& c) { c.set(3, discipline::load_manager(c, c.r(3), c.r(4)) ? 1 : 0); c.finish(); })
DISCIPLINE_ROUTINE(0x100deda0, 0x100df25c, NONE, "186a133c0bde67f5", [](Cpu& c) { discipline::save_manager(c, c.r(3), c.r(4)); c.finish(); })
DISCIPLINE_ROUTINE(0x100e7cd0, 0x100e8660, NONE, "ae62cbe43db2bed3", [](Cpu& c) { discipline::create_bodies(c, c.r(3), c.r(4)); c.finish(); })
