// DATABASE.CPP load block (database_routines.inc).
DATABASE_ROUTINE(0x100a55a0, 0x100ab5cc, R3, "ae346aa9076e7a5f", [](Cpu& c) { database::load_database(c, c.r(3), c.r(4), c.r(5)); })
