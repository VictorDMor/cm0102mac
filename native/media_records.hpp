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
constexpr int standings = -0x7b7c;     // the table 0x10082f90 searches for a person's contract with a club
constexpr int tallies = -0x7b9c;       // rows of tally_row_size bytes, indexed by the person's id
}
constexpr std::uint32_t club_rating_size = 0x4c;
constexpr std::uint32_t tally_row_size = 0x64;

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
    // As shortlist_manager_records.hpp's Contract names the same bytes.
    Field<std::int32_t> wage() const { return {c, at + 0xc}; }  // the press tells a big one (over 7500) from the rest
    Field<std::int8_t> happiness() const { return {c, at + 0x44}; }  // signed; the press needs it above 0, or weighs a tenth of it
    // Matches in a row the player was left out (inferred: contract_manager_match_review.cpp clears it when he played enough
    // and counts it up to 100 when he did not); the press reports whether it is at most 1.
    Field<std::int8_t> missed_run() const { return {c, at + 0x45}; }
    Field<std::int8_t> morale() const { return {c, at + 0x4c}; }  // -100..100: the press moves it by draws after a story
    Field<std::int8_t> listed() const { return {c, at + 0x54}; }  // (inferred: transfer-listed)
    Field<std::uint8_t> squad_status() const { return {c, at + 0x60}; }  // the low nibble is the contract type (1 to 3 are the first teams)
    // The press tests it above 1 for a contract of type 3 (inferred: a count of what the person has to complain of).
    Field<std::int8_t> grievances() const { return {c, at + 0x46}; }
};
// A person's row of the press's tally table.
struct TallyRow {
    Cpu& c;
    std::uint32_t at;
    Field<std::uint8_t> tally() const { return {c, at + 0xa}; }  // grows with the reactions about him; below 50 it may still grow
    Field<std::uint8_t> marks() const { return {c, at + 0xc}; }  // the evaluation tests its bit 0
};
// A club's row of the ratings table (club_rating_size bytes, indexed by the club's id).
struct ClubRatingRow {
    Cpu& c;
    std::uint32_t at;
    Field<std::int16_t> rating() const { return {c, at + 6}; }
    Field<std::int16_t> third_column() const { return {c, at + 0xa}; }  // moved by stories, held within 1..10000
    Field<std::int16_t> flags() const { return {c, at + 0xe}; }  // the standing bits the press scales its stories by (0x20, 0x10, 0x9, 0x6, ...)
};
// A person's staff record (Person::non_player).
struct StaffRecord {
    Cpu& c;
    std::uint32_t at;
    Field<std::int16_t> reputation() const { return {c, at + 0x2a}; }  // (as shortlist_manager_records.hpp)
    Field<std::int16_t> world_reputation() const { return {c, at + 0x2c}; }
    // The attributes the manager's verdict draws against (inferred from how the press uses them; the game's names are not known).
    Field<std::int8_t> attitude() const { return {c, at + 0x34}; }
    Field<std::int8_t> man_management() const { return {c, at + 0x39}; }  // the one 0x100d2f50 scales by the ability at +0x24
    Field<std::int8_t> tactics() const { return {c, at + 0x3b}; }
    Field<std::int8_t> standing() const { return {c, at + 0x3d}; }  // (as shortlist_manager_records.hpp)
    Field<std::int8_t> discipline() const { return {c, at + 0x3f}; }
};
// A person's player record (Person::player).
struct Player {
    Cpu& c;
    std::uint32_t at;
    Field<std::int16_t> ability() const { return {c, at + 4}; }  // (inferred: current ability, as shortlist_manager_records.hpp)
    Field<std::int16_t> home_reputation() const { return {c, at + 8}; }  // (inferred, in the database's order)
    Field<std::int16_t> reputation() const { return {c, at + 0xa}; }  // the one the press ranks stories by
    Field<std::int16_t> world_reputation() const { return {c, at + 0xc}; }
    Field<std::int8_t> morale() const { return {c, at + 0x44}; }     // a draw can raise it, up to 20 (inferred)
    // Bytes the stories nudge (inferred from the stories that move them; 20 is the top of the scale).
    Field<std::int8_t> aggression() const { return {c, at + 0x1c}; }   // a dirty-play story raises it
    Field<std::int8_t> consistency() const { return {c, at + 0x21}; }  // rises together with morale after a good story
    Field<std::int8_t> dirtiness() const { return {c, at + 0x25}; }    // a negative attribute, like injury proneness at +0x2d
    Field<std::int8_t> influence() const { return {c, at + 0x2f}; }    // 50 points of reputation each in a dressing-room comparison
    Field<std::int8_t> versatility() const { return {c, at + 0x45}; }  // (as shortlist_manager_records.hpp)
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
    Field<std::uint32_t> national_team() const { return {c, at + 0x20}; }  // his second employer, 0: none (inferred: a national team, as shortlist_manager_records.hpp)
    Field<std::uint8_t> national_job() const { return {c, at + 0x60}; }    // his job there (inferred)
    Field<std::int8_t> job() const { return {c, at + 0x62}; }             // his job at his club (as shortlist_manager_records.hpp)
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
    Field<std::int8_t> trait_64() const { return {c, at + 0x64}; }  // (as shortlist_manager_records.hpp)
    Field<std::int8_t> loyalty() const { return {c, at + 0x65}; }
    Field<std::int8_t> professionalism() const { return {c, at + 0x66}; }  // (inferred, as shortlist_manager_records.hpp)
    Field<std::int8_t> sportsmanship() const { return {c, at + 0x67}; }
    Field<std::int8_t> trait_68() const { return {c, at + 0x68}; }
    Field<std::int8_t> trait_69() const { return {c, at + 0x69}; }
    // The same two bytes by the role the press gives them (inferred): +0x69 is the last of the eight personality bytes.
    Field<std::int8_t> composure() const { return {c, at + 0x68}; }
    Field<std::int8_t> temperament() const { return {c, at + 0x69}; }
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
    Field<std::uint32_t> assistant() const { return {c, at + 0x80}; }  // a Person (inferred, as shortlist_manager_records.hpp)
    // The club's people: 50 slots from +0x98 (0: empty).
    static constexpr unsigned squad_slots = 50;
    Field<std::uint32_t> squad(unsigned slot) const { return {c, at + 0x98 + 4 * slot}; }
    Field<std::int16_t> reputation() const { return {c, at + 0x1ec}; }
    Person manager_record() const { return {c, manager()}; }
    Person deputy_record() const { return {c, deputy()}; }
    Nation nation_record() const { return {c, nation()}; }
    Division division_record() const { return {c, division()}; }
    Ground ground_record() const { return {c, ground()}; }
    // The club's row of the ratings table (the rows are club_rating_size bytes; +6 is its rating).
    // The club's row of the ratings table, from its id.
    ClubRatingRow rating_row() const {
        return {c, c.u32(c.u32(c.toc(tables::club_ratings))) + id().get() * club_rating_size};
    }
    Field<std::int16_t> table_rating() const {
        return {c, c.u32(c.u32(c.toc(tables::club_ratings))) + id().get() * club_rating_size + 6};
    }
};
inline Club club_at(Cpu& c, std::uint32_t index) { return {c, c.u32(c.toc(tables::clubs)) + index * club_size}; }
inline Person person_at(Cpu& c, std::uint32_t index) { return {c, c.u32(c.toc(tables::persons)) + index * Person::size}; }
// A person's tally row, by his id.
inline TallyRow tally_row_of(Cpu& c, std::uint32_t id) { return {c, c.u32(c.toc(tables::tallies)) + id * tally_row_size}; }
}
