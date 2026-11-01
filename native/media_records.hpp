#pragma once
// Views of the game's records as the press reads them: a club (0x248 bytes), a person
// (the 0x70-byte rows) with the player and contract records behind him, and the tables
// that hold them. Names follow shortlist_manager_records.hpp and player_stats_detail.hpp where
// the same offsets appear; "(inferred)" marks a meaning read from how the code uses the field.
// A view is a pair (Cpu, address): cheap to build, nothing is cached, and each accessor is
// one load or store of the width the original uses.
#include "media_detail.hpp"

namespace cm::aot::media::detail::records {
namespace tables {
// The tables: a slot of the TOC holds the address of the word that points to the rows.
constexpr int clubs = -0x7be8, club_total = -0x7b50, excluded_clubs = -0x7b04;
constexpr int persons = -0x7be0, person_count = -0x7be4;
constexpr int club_ratings = -0x79f0;  // rows of club_rating_size bytes, indexed by the club's id
constexpr int today = -0x7b80;         // the game date: day (halfword), year (halfword), hour (byte)
}
constexpr std::uint32_t club_rating_size = 0x4c;

// A nation, as far as the press reads it.
struct Nation {
    Cpu& c;
    std::uint32_t at;
    Field<std::int32_t> id() const { return {c, at + 0x68}; }
    Field<std::int16_t> rating() const { return {c, at + 0xac}; }  // what the press weighs a person's nation by (inferred)
};
// A division (a club's +0x30 record).
struct Division {
    Cpu& c;
    std::uint32_t at;
    Field<std::int16_t> level() const { return {c, at + 0x18}; }  // the press compares it with 12 (inferred)
};
// The record behind a club's +0x3c link: its +0x10 record is what makes a ground usable (inferred).
struct Ground {
    Cpu& c;
    std::uint32_t at;
    Field<std::uint32_t> location() const { return {c, at + 0x10}; }
};
// A contract (0x10082f90: the person's with a club).
struct Contract {
    Cpu& c;
    std::uint32_t at;
    Field<std::uint32_t> flags() const { return {c, at + 0x50}; }  // 8 and 0x2000 are the "flagged states" the press tests
};
// A person's staff record (Person::non_player).
struct StaffRecord {
    Cpu& c;
    std::uint32_t at;
    Field<std::int16_t> reputation() const { return {c, at + 0x2a}; }  // (as shortlist_manager_records.hpp)
    Field<std::int16_t> world_reputation() const { return {c, at + 0x2c}; }
};
// A person's player record (Person::player).
struct Player {
    Cpu& c;
    std::uint32_t at;
    Field<std::int16_t> reputation() const { return {c, at + 0xa}; }  // the one the press ranks stories by
    Field<std::int8_t> morale() const { return {c, at + 0x44}; }     // a draw can raise it, up to 20 (inferred)
};

// A person: a player, a manager or other staff (the table at tables::persons, rows of 0x70 bytes; not the
// 0x128-byte records of detail::person_size).
struct Person {
    Cpu& c;
    std::uint32_t at;
    static constexpr std::uint32_t size = 0x70;
    Field<std::uint32_t> id() const { return {c, at}; }
    Field<std::uint32_t> nation() const { return {c, at + 0xc}; }
    Field<std::uint32_t> player() const { return {c, at + 0x14}; }      // Player; 0: not a player
    Field<std::uint32_t> non_player() const { return {c, at + 0x1c}; }  // StaffRecord; 0: none
    Field<std::uint32_t> club() const { return {c, at + 0x24}; }        // 0: unattached
    // The ids behind the three name records the press prints (inferred: first, second and common name).
    std::uint32_t first_name() const { return name_id(0x28); }
    std::uint32_t second_name() const { return name_id(0x2c); }
    std::uint32_t common_name() const { return name_id(0x30); }
    // The two words of the date he joined his club, as 0x10718130 takes them (inferred).
    Field<std::uint32_t> joined_first() const { return {c, at + 0x4c}; }
    Field<std::uint32_t> joined_second() const { return {c, at + 0x50}; }
    // The original loads the halfword at +0x5c and keeps its low byte (the age, inferred).
    std::int32_t age() const { return std::int32_t(std::int8_t(c.u16(at + 0x5c))); }
    Field<std::uint8_t> ability() const { return {c, at + 0x5e}; }  // (inferred)
    // Personality bytes, signed (as shortlist_manager_records.hpp: ambition, loyalty, sportsmanship).
    Field<std::int8_t> ambition() const { return {c, at + 0x63}; }
    Field<std::int8_t> loyalty() const { return {c, at + 0x65}; }
    Field<std::int8_t> sportsmanship() const { return {c, at + 0x67}; }
    Field<std::int8_t> trait_68() const { return {c, at + 0x68}; }
    Field<std::int8_t> trait_69() const { return {c, at + 0x69}; }
    Nation nation_record() const { return {c, nation()}; }
    Player player_record() const { return {c, player()}; }
    StaffRecord staff_record() const { return {c, non_player()}; }

private:
    // A record at `link` leads to the record whose word at +0x20 is the id.
    std::uint32_t name_id(std::uint32_t link) const { return c.u32(c.u32(at + link) + 0x20); }
};

// A club (detail::slot::clubs rows of club_size bytes).
struct Club {
    Cpu& c;
    std::uint32_t at;
    Field<std::uint32_t> id() const { return {c, at}; }
    Field<std::uint32_t> nation() const { return {c, at + 0x2c}; }
    Field<std::uint32_t> division() const { return {c, at + 0x30}; }
    Field<std::uint32_t> ground() const { return {c, at + 0x3c}; }
    // The three affiliated clubs (as shortlist_manager_records.hpp).
    Field<std::uint32_t> affiliate(unsigned n) const { return {c, at + 0x74 + 4 * n}; }
    Field<std::uint32_t> manager() const { return {c, at + 0x90}; }  // a Person; 0: none
    Field<std::uint32_t> deputy() const { return {c, at + 0x94}; }   // a Person; 0: none (inferred)
    Field<std::int16_t> reputation() const { return {c, at + 0x1ec}; }
    Person manager_record() const { return {c, manager()}; }
    Person deputy_record() const { return {c, deputy()}; }
    Nation nation_record() const { return {c, nation()}; }
    Division division_record() const { return {c, division()}; }
    Ground ground_record() const { return {c, ground()}; }
    // The club's row of the ratings table (the rows are club_rating_size bytes; +6 is its rating).
    Field<std::int16_t> table_rating() const {
        return {c, c.u32(c.u32(c.toc(tables::club_ratings))) + id().get() * club_rating_size + 6};
    }
};
inline Club club_at(Cpu& c, std::uint32_t index) { return {c, c.u32(c.toc(tables::clubs)) + index * club_size}; }
inline Person person_at(Cpu& c, std::uint32_t index) { return {c, c.u32(c.toc(tables::persons)) + index * Person::size}; }
}
