// DATABASE.CPP tables block (database_routines.inc).
DATABASE_ROUTINE(0x100a34e0, 0x100a3da4, R3, "0b9daa001f787f05", [](Cpu& c) { database::choose_kept_clubs(c, c.r(3), c.r(4)); })
DATABASE_ROUTINE(0x100a2da0, 0x100a34e0, R3, "6a9552cac4fbfa49", [](Cpu& c) { database::compact_club_table(c, c.r(3)); })
DATABASE_ROUTINE(0x100ab5d0, 0x100abfa8, NONE, "a44e31d969b1272d", [](Cpu& c) { database::build_staff_tables(c); })
DATABASE_ROUTINE(0x100a1950, 0x100a2d98, R3, "9fe24ecd28746879", [](Cpu& c) { database::choose_kept_staff(c, c.r(3), c.r(4)); })
DATABASE_ROUTINE(0x1009efc0, 0x100a0d78, NONE, "a78191092b7b0666", [](Cpu& c) { database::compact_staff_table(c, c.r(3)); })
