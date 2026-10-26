#pragma once
// The game records SHORTLIST_MANAGER.CPP reads and writes, one view each,
// as guest memory (big-endian; guest::Field). Fields are named by what the
// code does with them; "(inferred)" marks a meaning read off the code and
// not confirmed elsewhere, with the other groups' readings where they
// differ. A field's type is the width and sign most of the module's loads
// give it; a routine that loads it otherwise converts (the original's lbz
// then extsb, lha then a logical use).
#include "guest_values.hpp"

namespace cm::aot::shortlist_manager::records {
using guest::Cpu;
using guest::Field;

// A date as the game keeps it (5 bytes): day of the year, year, and a byte
// the date's constructor derives from the year (inferred).
struct Date {
    Cpu& c;
    std::uint32_t address;
    Field<std::int16_t> day() const { return {c, address}; }
    Field<std::uint16_t> year() const { return {c, address + 2}; }
    Field<std::uint8_t> derived() const { return {c, address + 4}; }
    // The original copies a date field by field: year, day, the byte.
    void copy_from(const Date& from) const {
        year() = from.year().get();
        day() = from.day().get();
        derived() = from.derived().get();
    }
};

// A nation (0x128 bytes; detail::slot -0x7b98 is the table, -0x7b04 its count).
struct Nation {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x128;
    Field<std::int32_t> id() const { return {c, address + 0x68}; }  // as player_stats_detail.hpp's
    // A record with its id at +8, compared with TOC ids (inferred: the
    // continent or federation).
    Field<std::uint32_t> continent() const { return {c, address + 0x74}; }
    Field<std::uint8_t> rule() const { return {c, address + 0x118}; }  // 14 switches the candidate country policy's checks on
    Field<std::uint8_t> group() const { return {c, address + 0x119}; }  // player_stats_detail.hpp's Nation::group (int8 there)
    Field<std::int8_t> level() const { return {c, address + 0x11c}; }  // 1-3, else taken as 3 (inferred: the nation's standing)
    Field<std::int8_t> ranking() const { return {c, address + 0x11f}; }  // (inferred)
};

// A club's division (Club::division): its id and reputation.
struct Division {
    Cpu& c;
    std::uint32_t address;
    Field<std::int32_t> id() const { return {c, address}; }
    Field<std::int16_t> reputation() const { return {c, address + 0x18}; }
};

// A club (detail::slot::clubs, 0x248 bytes).
struct Club {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x248;
    static constexpr unsigned staff_slots = 7;
    Field<std::int32_t> id() const { return {c, address}; }
    // Over 35000 keeps a big club's reputation; a tenth of it plus 4500 is
    // the squad's target reputation (inferred: attendance).
    Field<std::int32_t> attendance() const { return {c, address + 8}; }
    Field<std::int32_t> stadium() const { return {c, address + 0x14}; }  // index of a 0x6c-byte stadium record, -1: none (inferred)
    Field<std::uint32_t> nation() const { return {c, address + 0x2c}; }
    Field<std::uint32_t> division() const { return {c, address + 0x30}; }  // Division
    Field<std::uint32_t> ground() const { return {c, address + 0x3c}; }  // a record whose +0x10 record locates it (inferred)
    Field<std::uint32_t> affiliate(unsigned n) const { return {c, address + 0x74 + 4 * n}; }  // three clubs (inferred)
    Field<std::uint32_t> assistant() const { return {c, address + 0x80}; }  // a person (inferred)
    Field<std::uint32_t> manager() const { return {c, address + 0x90}; }  // a person
    Field<std::uint32_t> staff(unsigned slot) const { return {c, address + 0x174 + 4 * slot}; }  // persons (0: none)
    Field<std::int16_t> reputation() const { return {c, address + 0x1ec}; }
    Field<std::uint8_t> status() const { return {c, address + 0x23d}; }  // 1: professional, 3: amateur (inferred)
    Nation nation_record() const { return {c, nation()}; }
};

// A player's record (Person::player).
struct Player {
    Cpu& c;
    std::uint32_t address;
    Field<std::int16_t> ability() const { return {c, address + 4}; }     // (inferred: current ability)
    Field<std::int16_t> potential() const { return {c, address + 6}; }   // (inferred: potential ability)
    // The three reputations (inferred, in the database's order; the
    // candidate scan's thresholds of 2750-3250 read +0xa and +0xc).
    Field<std::int16_t> home_reputation() const { return {c, address + 8}; }
    Field<std::int16_t> reputation() const { return {c, address + 0xa}; }
    Field<std::int16_t> world_reputation() const { return {c, address + 0xc}; }
    Field<std::uint8_t> goalkeeping() const { return {c, address + 0xf}; }  // 20 tested (inferred: a goalkeeper)
    Field<std::int8_t> rating(std::uint32_t offset) const { return {c, address + offset}; }  // the category tests' bytes from +0xf
    Field<std::int8_t> decline() const { return {c, address + 0x2d}; }  // squared, over 6000, off the worth (inferred)
    Field<std::int8_t> versatility() const { return {c, address + 0x45}; }  // > 10 tested (meaning unknown)
};

// A non-player's (staff) record (Person::non_player, 0x44 bytes).
struct StaffRecord {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x44;
    Field<std::int16_t> ability() const { return {c, address + 0x24}; }  // over 100 rules a manager out (inferred)
    Field<std::int16_t> home_reputation() const { return {c, address + 0x28}; }
    Field<std::int16_t> reputation() const { return {c, address + 0x2a}; }
    Field<std::int16_t> world_reputation() const { return {c, address + 0x2c}; }
    Field<std::int8_t> standing() const { return {c, address + 0x3d}; }  // compared with the person's +0x63 (inferred)
    Field<std::int8_t> ambition() const { return {c, address + 0x40}; }  // (inferred)
};

// A person (detail::slot::persons, 0x70 bytes).
struct Person {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x70;
    Field<std::int32_t> id() const { return {c, address}; }
    Field<std::int32_t> value() const { return {c, address + 8}; }  // an amount (inferred: his market value)
    Field<std::uint32_t> nation() const { return {c, address + 0xc}; }  // Nation; the category-list reset draws one when null
    Field<std::uint32_t> second_nation() const { return {c, address + 0x10}; }  // 0: none
    Field<std::uint32_t> player() const { return {c, address + 0x14}; }  // Player, 0: not a player
    Field<std::uint32_t> non_player() const { return {c, address + 0x1c}; }  // StaffRecord, 0: none
    // His second employer and his job there (+0x60) (inferred: a national team).
    Field<std::uint32_t> national_team() const { return {c, address + 0x20}; }
    Field<std::uint32_t> club() const { return {c, address + 0x24}; }  // Club, 0: none
    Date joined() const { return {c, address + 0x4c}; }  // when he joined his club (inferred)
    // The original loads the halfword at +0x5c and keeps its low byte (the
    // age, inferred).
    Field<std::int16_t> age_halfword() const { return {c, address + 0x5c}; }
    std::int32_t age() const { return std::int8_t(age_halfword()); }
    Field<std::uint8_t> byte_5e() const { return {c, address + 0x5e}; }  // 0 or not (meaning unknown)
    Field<std::uint8_t> national_job() const { return {c, address + 0x60}; }  // (inferred)
    Field<std::int8_t> adaptability() const { return {c, address + 0x61}; }  // as player_stats_detail.hpp's
    // His job at his club (0x11 rules him out of the candidate context; 5
    // and 12 are manager jobs, 11 a player, inferred).
    Field<std::int8_t> job() const { return {c, address + 0x62}; }
    // Personality bytes (signed, 1-20 likely; inferred from use: ambition
    // weighs reputation, loyalty sets the days he must have served; the
    // interest group reads +0x63 as determination and +0x65 as pressure).
    Field<std::int8_t> ambition() const { return {c, address + 0x63}; }
    Field<std::int8_t> trait_64() const { return {c, address + 0x64}; }
    Field<std::int8_t> loyalty() const { return {c, address + 0x65}; }
    Field<std::int8_t> professionalism() const { return {c, address + 0x66}; }  // (inferred)
    Field<std::int8_t> sportsmanship() const { return {c, address + 0x67}; }   // (inferred)
    Player player_record() const { return {c, player()}; }
    Nation nation_record() const { return {c, nation()}; }
};
inline Person person_at(Cpu& c, std::uint32_t persons, std::int32_t index) {
    return {c, persons + std::uint32_t(index) * Person::size};
}

// A contract (0x10082f20 a person's primary one, 0x10082f90 his one with a club).
struct Contract {
    Cpu& c;
    std::uint32_t address;
    Field<std::int32_t> club() const { return {c, address + 4}; }  // the club's index
    Field<std::int32_t> wage() const { return {c, address + 0xc}; }  // -1: not valued yet (inferred)
    Date ends() const { return {c, address + 0x30}; }  // (inferred)
    Field<std::int8_t> kind_byte() const { return {c, address + 0x38}; }  // bits 7-2: the contract mode (signed)
    std::int32_t mode() const { return std::int32_t(kind_byte().get()) >> 2; }
    // Signed; the category lists' bank 1 rates by it (inferred: happiness).
    Field<std::int8_t> happiness() const { return {c, address + 0x44}; }
    Field<std::int8_t> morale() const { return {c, address + 0x4c}; }  // +-1/400 per point (inferred)
    Field<std::uint32_t> flags() const { return {c, address + 0x50}; }  // 0x20: not to a club of the same group
    Field<std::int8_t> listed() const { return {c, address + 0x54}; }  // (inferred: transfer-listed)
    // Bit 0: the primary-contract mark, bit 1 its second mark, bit 3 no
    // happiness reasons (inferred).
    Field<std::uint8_t> options() const { return {c, address + 0x5c}; }
    Field<std::uint8_t> squad_status() const { return {c, address + 0x60}; }  // low nibble: the contract type (inferred)
};

// One of a club's forty shortlist rows (0x2c bytes): a place the club wants
// filled, the person it holds and the persons around it. The groups read the
// words at +4 to +0x18 differently; the names are the best supported.
struct Row {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x2c, per_club = 40, legacy_per_club = 25;
    static constexpr std::uint8_t player = 11;  // kind 11: a player; 6-10 are staff jobs (inferred)
    Field<std::uint8_t> kind() const { return {c, address}; }
    Field<std::int16_t> positions() const { return {c, address + 2}; }  // position flags (bit 0 a key place, inferred)
    Field<std::int32_t> level() const { return {c, address + 4}; }  // the lowest score the row takes (inferred)
    Field<std::int32_t> budget() const { return {c, address + 8}; }  // (inferred)
    Field<std::int32_t> weight() const { return {c, address + 0xc}; }  // (inferred)
    Field<std::int32_t> target() const { return {c, address + 0x10}; }  // the person index it holds, -1: none
    Field<std::int32_t> previous() const { return {c, address + 0x14}; }  // a person index asked for, -1: any (inferred)
    Field<std::int32_t> approach() const { return {c, address + 0x18}; }  // -1: none (inferred: the link made)
    // The words at +4 to +0x18 by offset (the save's and the defaults' loops).
    Field<std::int32_t> word(std::uint32_t offset) const { return {c, address + offset}; }
    Date date() const { return {c, address + 0x1c}; }  // the row opens after it (inferred)
    Field<std::int8_t> rank() const { return {c, address + 0x24}; }
    // Bit 7: a loan, and the candidate scan's relaxed second pass
    // (inferred); bits 3-6 the category; bit 2 a raised weight (inferred).
    Field<std::uint8_t> flags() const { return {c, address + 0x28}; }
    static std::uint32_t loan(std::uint8_t flags) { return flags >> 7; }
    static std::uint32_t category(std::uint8_t flags) { return std::uint32_t(flags) >> 3 & 0xf; }
    std::uint32_t loan() const { return loan(flags()); }
    std::uint32_t category() const { return category(flags()); }
    // rlwimi r0,category,3,25,28.
    void set_category(std::uint32_t category) const {
        flags() = std::uint8_t((flags() & 0x87) | ((category << 3) & 0x78));
    }
};

// A club's entry in the manager's club table (12 bytes per club id).
struct ClubEntry {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0xc;
    Field<std::uint32_t> rows() const { return {c, address}; }  // Row[40]
    Field<std::uint8_t> status() const { return {c, address + 4}; }  // 1: the club keeps a shortlist
    Field<std::int16_t> reputation() const { return {c, address + 6}; }  // the club's reputation when created (inferred)
    Field<std::int8_t> formation() const { return {c, address + 8}; }  // 0..9: lay_out_formation's layouts
    // Set to 100 when the club's rows change; 100 or more stops new
    // approaches (inferred: a change counter).
    Field<std::uint8_t> changes() const { return {c, address + 9}; }
    Row row(std::uint32_t index) const { return {c, rows() + index * Row::size}; }
};

// One entry of a category list (12 bytes).
struct CategoryEntry {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0xc;
    Field<std::int32_t> person() const { return {c, address}; }  // the person's index
    Field<std::int32_t> nation_id() const { return {c, address + 4}; }  // his nation's id
    Field<std::int32_t> rating() const { return {c, address + 8}; }  // the lists are sorted by it
};

// The shortlist manager (224 bytes, owned by the transfer manager at +0x988).
struct Manager {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0xe0, banks = 2, categories = 16;
    // The staff-club links: (staff id, club id) int32 pairs sorted by staff
    // then club (as saved), and a copy sorted by club then staff;
    // capacities and counts.
    Field<std::uint32_t> links_by_staff() const { return {c, address}; }
    Field<std::uint32_t> links_by_club() const { return {c, address + 4}; }
    Field<std::int32_t> capacity_by_staff() const { return {c, address + 8}; }
    Field<std::int32_t> capacity_by_club() const { return {c, address + 0xc}; }
    Field<std::int32_t> count_by_staff() const { return {c, address + 0x10}; }
    Field<std::int32_t> count_by_club() const { return {c, address + 0x14}; }
    // Two banks of sixteen category lists (CategoryEntry[capacity]), both
    // banks' list k holding count(k) entries.
    static constexpr std::uint32_t list_offset(std::uint32_t bank, std::uint32_t category) {
        return 0x18 + bank * 0x40 + category * 4;
    }
    Field<std::uint32_t> list(std::uint32_t bank, std::uint32_t category) const {
        return {c, address + list_offset(bank, category)};
    }
    Field<std::int32_t> count(std::uint32_t category) const { return {c, address + 0x98 + category * 4}; }
    Field<std::int32_t> capacity() const { return {c, address + 0xd8}; }
    Field<std::uint32_t> clubs() const { return {c, address + 0xdc}; }  // ClubEntry per club id
    ClubEntry club_entry(std::int32_t id) const { return {c, clubs() + std::uint32_t(id) * ClubEntry::size}; }
};
}
