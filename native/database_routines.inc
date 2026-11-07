// DATABASE.CPP routines in C++ (tools/database_audit.py): entry, end, the
// results the routine checker compares, the digest of the original
// instructions, and the hook that runs it at its guest ABI edge.
DATABASE_ROUTINE(0x100d2d70, 0x100d2e60, R3|F1, "e885887ed502d6d1", [](Cpu& c) { database::attribute_rating_tenth(c, c.r(3), std::int8_t(c.r(4))); })
DATABASE_ROUTINE(0x100d2e60, 0x100d2f48, R3|F1, "2f6f8414fb98ef00", [](Cpu& c) { database::attribute_rating_04(c, c.r(3), std::int8_t(c.r(4))); })
DATABASE_ROUTINE(0x100d2f50, 0x100d3038, R3|F1, "6914557306ffa899", [](Cpu& c) { database::attribute_rating(c, c.r(3), std::int8_t(c.r(4))); })
DATABASE_ROUTINE(0x100d9480, 0x100d94c4, R3, "cde347d9b4ddfc1c", [](Cpu& c) { database::club_favours(c, c.r(3), c.r(4)); })
DATABASE_ROUTINE(0x100d94d0, 0x100d9514, R3, "431bebd2e47b3cfb", [](Cpu& c) { database::club_dislikes(c, c.r(3), c.r(4)); })
DATABASE_ROUTINE(0x100d9520, 0x100d957c, R3, "fccec09107ab1bb6", [](Cpu& c) { database::favours_person(c, c.r(3), c.r(4)); })
DATABASE_ROUTINE(0x100d9580, 0x100d95dc, R3, "29afb71dfc9817be", [](Cpu& c) { database::dislikes_person(c, c.r(3), c.r(4)); })
DATABASE_ROUTINE(0x100da050, 0x100da0ac, R3, "128f6e0798f4c745", [](Cpu& c) { database::dislikes_club(c, c.r(3), c.r(4)); })
DATABASE_ROUTINE(0x100da0b0, 0x100da10c, R3, "058df4843f653432", [](Cpu& c) { database::favours_club(c, c.r(3), c.r(4)); })
DATABASE_ROUTINE(0x100d3150, 0x100d3228, R3, "459b7587b0c6bcec", [](Cpu& c) { database::contract_flag_09(c, c.r(3), c.r(4)); })
DATABASE_ROUTINE(0x100d3230, 0x100d32fc, R3, "1f529d4f42765d2f", [](Cpu& c) { database::contract_flag_02(c, c.r(3), c.r(4)); })
DATABASE_ROUTINE(0x100d3300, 0x100d33cc, R3, "ec3dcf0b224aaa72", [](Cpu& c) { database::contract_flag_08(c, c.r(3), c.r(4)); })
DATABASE_ROUTINE(0x100db1b0, 0x100db404, R3, "f6e1662be75555b1", [](Cpu& c) { database::position_mask(c, c.r(3)); })
DATABASE_ROUTINE(0x100de3b0, 0x100de408, NONE, "0812290479c4cc8f", [](Cpu& c) { database::initialize_season_date(c); })
#include "database_routines_lookups.inc"
#include "database_routines_records.inc"
#include "database_routines_text.inc"
#include "database_routines_generation.inc"
#include "database_routines_people.inc"
#include "database_routines_search.inc"
#include "database_routines_loading.inc"
#include "database_routines_tables.inc"
#include "database_routines_resolve.inc"
#include "database_routines_load.inc"
#include "database_routines_save.inc"
#include "database_routines_init.inc"
#include "database_routines_languages.inc"
