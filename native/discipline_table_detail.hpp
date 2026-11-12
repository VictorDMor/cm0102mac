#pragma once
// DISCIPLINE.CPP table block: the records and globals its routines reach besides the manager's own
// (discipline_detail.hpp). Offsets and TOC cells are the original's; accesses go through guest::Cpu.
#include "discipline_detail.hpp"

namespace cm::aot::discipline {
namespace table_toc {
inline constexpr std::int32_t people_count = -0x7be4, people = -0x7be0, competitions = -0x7b5c,
                              fixture_filter = -0x7b54, friendly_competition = -0x7aa8, pool = -0x5608;
}
// The manager's word at +0x00 is not the 42 bodies themselves but the array holding them
// (every routine reads it as `lwz r3,0(manager); lwzx r3,r3,index*4`).
inline std::uint32_t body_object(const guest::Cpu& c, std::uint32_t manager_object, std::uint32_t index) {
    return c.u32(c.u32(manager_object + manager::bodies) + index * 4);
}
// A competition's object in the table at TOC -0x7b5c (pointers by competition id, no bounds test in the
// original). Its signed byte at +0x4a is the index of the ruling body that governs it (-1: none).
inline std::uint32_t competition_object(const guest::Cpu& c, std::uint32_t competition_id) {
    return c.u32(c.u32(c.toc(table_toc::competitions)) + competition_id * 4);
}
namespace competition {
inline constexpr std::uint32_t ruling_body = 0x4a, ruling_argument = 4;
}
inline constexpr std::int8_t no_ruling_body = -1;
// A person record (0x70 bytes, DATABASE.CPP): +0 the person id, +0x24 the human manager object when the person
// is one. A human manager's 20 pointers at +0x19c.
namespace person_record {
inline constexpr std::uint32_t id = 0, human_manager = 0x24, human_manager_list = 0x19c;
inline constexpr int human_manager_list_size = 20;
}
// The original's test of a person pointer against the people count: null passes, else the id (signed) must be
// below the count.
inline bool person_in_table(const guest::Cpu& c, std::uint32_t person_record_at) {
    return person_record_at == 0 || c.i32(person_record_at + person_record::id) < c.i32(c.toc(table_toc::people_count));
}
// A fixture record (MATCH_DAY.CPP): +0 competition id, +0x14 a word passed to 0x10494d20, +0x1c / +0x20 the two
// sides, each with a pointer at +0x30 whose first word is the competition id of the side's league.
namespace fixture {
inline constexpr std::uint32_t competition_id = 0, filter_word = 0x14, home_side = 0x1c, away_side = 0x20,
                               side_league = 0x30;
}
}
