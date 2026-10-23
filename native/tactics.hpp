#pragma once
// TACTICS.CPP of the original: the CTACTICS object, which holds the formation
// records (formation.hpp), one 0x94-byte record per team (its formation, its
// 20-man squad, captains, set-piece takers and line-up), the human managers'
// backups of a squad, and the results each formation has had. Each routine
// leaves what the original leaves (see guest_abi.hpp).
#include "guest_abi.hpp"

namespace cm::aot::tactics {
namespace field {
// CTACTICS.
constexpr std::uint32_t results = 0x0;            // -> 0x28-byte results, one per formation
constexpr std::uint32_t backups = 0x30;           // backup[16], one per human manager
constexpr std::uint32_t backup_count = 0x5b0;     // int8: backups in use
constexpr std::uint32_t extra_formations = 0x5b4; // -> formation records after the first ones
constexpr std::uint32_t extra_count = 0x5b8;
constexpr std::uint32_t formations = 0x5bc;       // -> formation records, from index 0
constexpr std::uint32_t formation_count = 0x5c0;
constexpr std::uint32_t teams = 0x5c4;            // -> team records
constexpr std::uint32_t team_count = 0x5c8;
// Team record (0x94 bytes). Staff are kept by id (a staff record's first
// word, its index in the staff table), -1 for none.
constexpr std::uint32_t team_formation = 0x0;     // int16: formation index
constexpr std::uint32_t squad = 0x4;              // int32[20]: staff ids
constexpr std::uint32_t pick = 0x54;              // int32: a staff id picked from the squad
constexpr std::uint32_t captains = 0x58;          // int32[2]: captain, vice-captain
constexpr std::uint32_t takers = 0x60;            // int32[5][2]: set-piece takers, first and second
constexpr std::uint32_t lineup = 0x88;            // int8[11]: squad slot of each formation place, -1 none
// Backup (0x58 bytes).
constexpr std::uint32_t backup_club = 0x0;
constexpr std::uint32_t backup_managed = 0x4;     // int8: the team's `managed` flag
constexpr std::uint32_t backup_active = 0x5;      // int8
constexpr std::uint32_t backup_squad = 0x8;       // int32[20]
// Formation results (0x28 bytes): int16 played +2, won +4, drawn +6, lost +8,
// goals for +0xa, against +0xc, points +0xe; three ratings (double) +0x10.
constexpr std::uint32_t played = 0x2, won = 0x4, drawn = 0x6, lost = 0x8;
constexpr std::uint32_t scored = 0xa, conceded = 0xc, points = 0xe, ratings = 0x10;
}
constexpr std::uint32_t team_size = 0x94, backup_size = 0x58, results_size = 0x28;
constexpr std::uint32_t formation_size = 0x18e0, staff_size = 0x70;
constexpr std::int32_t squad_size = 20, human_count = 16, set_pieces = 5;

// Typed views of the records in guest memory. Indexes are the original's
// (validated by the routines); staff ids are int32, -1 for none.

// A team record (0x94 bytes) and its index among the team records.
class Team {
public:
    Team(guest::Cpu& cpu, std::uint32_t at, std::int32_t slot) : c(cpu), address(at), index(slot) {}
    std::int16_t formation() const { return c.i16(address + field::team_formation); }
    void set_formation(std::uint32_t value) const { c.put16(address + field::team_formation, value); }
    // The squad: 20 staff ids.
    std::uint32_t squad_entry(std::int32_t slot) const { return address + field::squad + std::uint32_t(slot) * 4; }
    std::int32_t player(std::int32_t slot) const { return c.i32(squad_entry(slot)); }
    void set_player(std::int32_t slot, std::uint32_t id) const { c.put32(squad_entry(slot), id); }
    std::int32_t pick() const { return c.i32(address + field::pick); }
    void set_pick(std::uint32_t id) const { c.put32(address + field::pick, id); }
    // Captain (0) and vice-captain (1).
    std::int32_t captain(std::int32_t which) const { return c.i32(captain_entry(which)); }
    void set_captain(std::int32_t which, std::uint32_t id) const { c.put32(captain_entry(which), id); }
    // Set-piece takers: 5 kinds, first (0) and second (1).
    std::int32_t taker(std::int32_t kind, std::int32_t order) const { return c.i32(taker_entry(kind, order)); }
    void set_taker(std::int32_t kind, std::int32_t order, std::uint32_t id) const { c.put32(taker_entry(kind, order), id); }
    // The squad slot playing each of the formation's 11 places (a byte).
    std::uint8_t lineup(std::int32_t place) const { return c.u8(address + field::lineup + std::uint32_t(place)); }
    void set_lineup(std::int32_t place, std::uint32_t slot) const { c.put8(address + field::lineup + std::uint32_t(place), slot); }
    guest::Cpu& c;
    std::uint32_t address;
    std::int32_t index;

private:
    std::uint32_t captain_entry(std::int32_t which) const { return address + field::captains + std::uint32_t(which) * 4; }
    std::uint32_t taker_entry(std::int32_t kind, std::int32_t order) const {
        return address + field::takers + std::uint32_t(kind) * 8 + std::uint32_t(order) * 4;
    }
};

// A human manager's backup of a team's squad (0x58 bytes).
class Backup {
public:
    Backup(guest::Cpu& cpu, std::uint32_t at) : c(cpu), address(at) {}
    bool active() const { return c.i8(address + field::backup_active) != 0; }
    void set_active(bool value) const { c.put8(address + field::backup_active, value); }
    // An active backup of this team.
    bool holds(std::uint32_t club, std::uint32_t managed) const {
        return active() && c.u32(address + field::backup_club) == club &&
               c.i8(address + field::backup_managed) == std::int8_t(managed);
    }
    void set_team(std::uint32_t club, std::uint32_t managed) const {
        c.put32(address + field::backup_club, club);
        c.put8(address + field::backup_managed, managed);
    }
    std::uint32_t player(std::int32_t slot) const { return c.u32(address + field::backup_squad + std::uint32_t(slot) * 4); }
    void set_player(std::int32_t slot, std::uint32_t id) const {
        c.put32(address + field::backup_squad + std::uint32_t(slot) * 4, id);
    }
    guest::Cpu& c;
    std::uint32_t address;
};

// One formation's results (0x28 bytes): int16 counts at field::played ..
// field::points, three ratings (double) at field::ratings.
class Results {
public:
    Results(guest::Cpu& cpu, std::uint32_t at) : c(cpu), address(at) {}
    std::int16_t count(std::uint32_t at) const { return c.i16(address + at); }
    // `sth` of the sum: wraps at 16 bits.
    void add(std::uint32_t at, std::int32_t amount) const { c.put16(address + at, std::uint32_t(count(at) + amount)); }
    void set_rating(std::int32_t which, double value) const { c.put_f64(address + field::ratings + std::uint32_t(which) * 8, value); }
    guest::Cpu& c;
    std::uint32_t address;
};

// The CTACTICS object.
class Tactics {
public:
    Tactics(guest::Cpu& cpu, std::uint32_t at) : c(cpu), address(at) {}
    Team team(std::int32_t slot) const {
        return {c, c.u32(address + field::teams) + std::uint32_t(slot) * team_size, slot};
    }
    std::uint32_t teams() const { return c.u32(address + field::teams); }
    std::int32_t team_count() const { return c.i32(address + field::team_count); }
    Backup backup(std::int32_t human) const { return {c, address + field::backups + std::uint32_t(human) * backup_size}; }
    std::int8_t backup_count() const { return c.i8(address + field::backup_count); }
    void add_backup_count(std::int32_t amount) const {
        c.put8(address + field::backup_count, std::uint32_t(c.u8(address + field::backup_count) + amount));
    }
    Results results(std::int32_t formation) const {
        return {c, c.u32(address + field::results) + std::uint32_t(formation) * results_size};
    }
    std::uint32_t formation_count() const { return c.u32(address + field::formation_count); }
    std::uint32_t extra_count() const { return c.u32(address + field::extra_count); }
    // A formation record by index: the first ones, then the extra ones.
    std::uint32_t formation(std::int32_t index) const {
        const auto first = std::int32_t(formation_count());
        if (index < first) return c.u32(address + field::formations) + std::uint32_t(index) * formation_size;
        return c.u32(address + field::extra_formations) + std::uint32_t(index - first) * formation_size;
    }
    guest::Cpu& c;
    std::uint32_t address;
};

// Char and short arguments are taken as the register holds them: the
// original keeps them in non-volatile registers, whose full value callees
// save into the stack image. `managed`, taken by most routines: for a club a
// human manages, use the manager's own team record (see team_slot).

// 0x103af060: the destructor; frees the arrays and, when `dispose` > 0, the object.
std::uint32_t destroy(guest::Cpu& c, std::uint32_t tactics, std::uint32_t dispose);
// 0x103af130: counts a played fixture in the results of both teams'
// formations, unless a human controls either club; 1 when counted.
std::uint32_t record_result(guest::Cpu& c, std::uint32_t tactics, std::uint32_t fixture);
// 0x103af480: a formation record by index (only the first ones when
// `first_only`); asserts and returns 0 when out of range.
std::uint32_t formation_at(guest::Cpu& c, std::uint32_t tactics, std::int16_t index, std::uint32_t first_only);
// 0x103af570: the formation a club plays: its human manager's choice (asking
// the human manager table, `setting` passed on), or its team record's.
std::uint32_t team_formation(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t setting);
// 0x103af6c0: sets a team's formation index; 1 when set.
std::uint32_t set_team_formation(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t index,
                                 std::uint32_t managed);
// 0x103af800: the staff record in a squad slot, or 0; empties the slot when
// its player is gone or no longer qualifies for a national team.
std::uint32_t squad_player(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t slot,
                           std::uint32_t managed);
// 0x103af9d0: puts a staff record in a squad slot; 1 when set.
std::uint32_t set_squad_player(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t slot,
                               std::uint32_t staff, std::uint32_t managed);
// 0x103afb90: removes a player from a team's squad and, with `roles`, from
// its captaincies and set pieces; 1 when he held a squad slot or set piece.
std::uint32_t remove_player(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t staff,
                            std::uint32_t managed, std::uint32_t roles);
// 0x103afe90: empties a squad slot; 1 when done.
std::uint32_t clear_squad_player(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t slot,
                                 std::uint32_t managed);
// 0x103afff0: with `restore` 0, empties the squad, first saving it in the
// current human's backup when `keep`; otherwise puts that backup back (with
// `keep`) or drops the team's backups. 1 unless the arguments are bad.
std::uint32_t backup_squad(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t managed,
                           std::uint32_t keep, std::uint32_t restore);
// 0x103b04a0: whether the current human holds a backup of this team.
std::uint32_t has_backup(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t managed);
// 0x103b0540: drops every backup of this team; true when it looked at them
// (the backup count was not 0).
bool forget_backup(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t managed);
// 0x103b05c0: the squad slot of a staff record, -1 when not in the squad
// (0 when the team has no record).
std::int32_t squad_slot_of(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t staff,
                           std::uint32_t managed);
// 0x103b07c0: swaps two squad slots and drops the team's backups; 1 when done.
std::uint32_t swap_squad_players(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t first,
                                 std::uint32_t second, std::uint32_t managed);
// 0x103b0a00: the squad slot holding the pick; otherwise -1 when `missing`
// is set, 0 when not.
std::int32_t pick_slot(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t managed,
                       std::uint32_t missing);
// 0x103b0b20: the pick's staff id (0 when the team has no record).
std::uint32_t pick(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t managed);
// 0x103b0bf0: makes the player in a squad slot the pick; 1 when set.
std::uint32_t set_pick_slot(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t slot,
                            std::uint32_t managed);
// 0x103b0d40: makes a staff record the pick; 1 when set.
std::uint32_t set_pick(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t staff,
                       std::uint32_t managed);
// 0x103b0e70: the staff record taking a set piece (`order` 0 first, 1
// second), or 0; drops a taker who is gone or may no longer take it.
std::uint32_t set_piece_taker(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t kind,
                              std::uint32_t order, std::uint32_t managed);
// 0x103b10f0: sets a set-piece taker; clearing the first moves the second up.
std::uint32_t set_set_piece_taker(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t kind,
                                  std::uint32_t order, std::uint32_t staff, std::uint32_t managed);
// 0x103b1370: the captain (`which` 0) or vice-captain (1) staff record, or 0.
std::uint32_t captain(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t which,
                      std::uint32_t managed);
// 0x103b14d0: sets the captain or vice; clearing the captain moves the vice up.
std::uint32_t set_captain(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t which,
                          std::uint32_t staff, std::uint32_t managed);
// 0x103b1670: the squad slot playing a formation place (the byte, 0xff none).
std::uint32_t lineup_slot(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t place,
                          std::uint32_t managed);
// 0x103b17b0: sets the squad slot playing a formation place; 1 when set.
std::uint32_t set_lineup_slot(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t place,
                              std::uint32_t slot, std::uint32_t managed);
// 0x103b1960: the index of a club's team record: the club's id, or with
// `managed` for a club a human controls, one of 32 records past the clubs
// (16 for the humans' clubs, 16 for their national teams), from the
// manager's id; -1 for a club 0x100ce250 accepts. Asserts and falls back to
// the club's id when the manager is missing or the index is out of range.
std::int32_t team_slot(guest::Cpu& c, std::uint32_t club, std::uint32_t managed);
// 0x103b1b10: copies a team record out; 1 when done.
std::uint32_t copy_team(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t destination,
                        std::uint32_t managed);
// 0x103b1c00: copies a team record in, when its index is below the team
// count; 1 when done.
std::uint32_t restore_team(guest::Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t source,
                           std::uint32_t managed);
}
