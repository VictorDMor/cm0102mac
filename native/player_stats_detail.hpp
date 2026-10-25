#pragma once
// What player_stats_*.cpp share: views of the player-stats object, the
// records it keeps and the game records it reads, the module's TOC slots and
// string pool, and the calls into the rest of the program.
//
// The routines compute with C++ values and leave what the game can observe as
// the original does (guest_values.hpp); floating point through guest::Fpu
// (guest_fpu.hpp), which leaves FPSCR as the original. r0, r11, r12, CTR and
// XER are not reproduced: nothing around the module reads them before
// writing them (tools/player_stats_audit.py --volatiles, a ctest). The
// condition register is reproduced at returns; at calls into other code it
// could only reach memory through the exception runtime's throw (its context
// save, mfcr at 0x10772d28), which no path the game takes reaches.
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "player_stats.hpp"
#include <initializer_list>

namespace cm::aot::player_stats::detail {
using guest::Cpu;
using guest::Fpu;
using guest::Frame;
using guest::Field;
using guest::Kept;
using guest::compare;
using guest::compare_logical;
using guest::enter;
using guest::extended;
using guest::keep;
using guest::leave;
using guest::nonzero;
using guest::nonzero_logical;
using guest::result;

// The season record categories (PlayerEntry::season), in the order of the
// pool's labels (text::non_competitive on). Category 6 adds up every club
// game of categories 2-4.
namespace category {
constexpr std::int32_t non_competitive = 1, league = 2, cup = 3, continental = 4, international = 5, senior_club = 6;
}

// A date as the game keeps it (8 bytes; the calendar routines take it by
// value in two words).
struct Date {
    Cpu& c;
    std::uint32_t address;
    Field<std::int16_t> day() const { return {c, address}; }  // day of the year
    Field<std::uint16_t> year() const { return {c, address + 2}; }
};

// A history row (TOC -0x74c4, count at -0x74c8): one season of a player's
// real-world career, imported when a game is created.
struct HistoryRow {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x14;
    Field<std::uint32_t> first() const { return {c, address}; }
    Field<std::uint32_t> player() const { return {c, address + 4}; }
    Field<std::uint32_t> club() const { return {c, address + 8}; }
    Field<std::int16_t> year() const { return {c, address + 0xc}; }
    Field<std::int8_t> loan() const { return {c, address + 0xe}; }
    Field<std::uint8_t> appearances() const { return {c, address + 0xf}; }
    Field<std::uint8_t> goals() const { return {c, address + 0x10}; }
};
// A relation entry (the stats object's +0x3c array, 8 bytes, sorted by
// compare_relations): a person's attachment to an affinity group.
struct Relation {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 8;
    Field<std::int32_t> person() const { return {c, address}; }
    Field<std::int8_t> target() const { return {c, address + 4}; }
    Field<std::int8_t> strength() const { return {c, address + 5}; }
};

// The game's database records the module reads (fields named by use here).
// A person (slot::persons, 0x70 bytes each); ids below slot::person_count
// are players.
struct Person {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x70;
    Field<std::int32_t> id() const { return {c, address}; }
    Field<std::uint32_t> nation() const { return {c, address + 0xc}; }
    Field<std::uint32_t> second_nation() const { return {c, address + 0x10}; }  // 0: none
    // The person's player record (0: not a player); its first word indexes
    // Stats::players.
    Field<std::uint32_t> player() const { return {c, address + 0x14}; }
    Field<std::uint32_t> non_player() const { return {c, address + 0x1c}; }  // staff record, 0: none
    Field<std::uint32_t> club() const { return {c, address + 0x24}; }
    // The date he joined his club (inferred: the module counts days at the
    // club from it, 0x10718130, and years abroad from its year).
    Date joined() const { return {c, address + 0x4c}; }
    // 200 - 10 x this is the person's relation threshold (likely
    // adaptability, not confirmed).
    Field<std::int8_t> adaptability() const { return {c, address + 0x61}; }
};
// A club (slot::clubs, 0x248 bytes each).
struct Club {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x248;
    // The first word, which CareerRow::club and the archive compare by (a
    // nation's first word is read the same way).
    Field<std::int32_t> id() const { return {c, address}; }
    Field<std::uint32_t> nation() const { return {c, address + 0x2c}; }
};
// A nation. `group` and the two codes after it are the affinity groups
// (1-64) the relations are kept for (see player_stats_relations.cpp).
struct Nation {
    Cpu& c;
    std::uint32_t address;
    Field<std::int32_t> id() const { return {c, address + 0x68}; }
    Field<std::int8_t> group() const { return {c, address + 0x119}; }
    Field<std::int8_t> first_group() const { return {c, address + 0x11a}; }
    Field<std::int8_t> second_group() const { return {c, address + 0x11b}; }
};

// One season record (Stats::seasons, 32 bytes): a player's tallies for one
// competition type. Names follow the module's stat labels (text::appearances
// on), in the order of season_value's kinds 1-17 (player_stats_queries.cpp);
// `figure` is kind 22's int32, the minutes played.
struct SeasonRecord {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x20;
    Field<std::uint8_t> appearances() const { return {c, address}; }
    // Printed after the starts as "%d (%d)".
    Field<std::uint8_t> substitute_appearances() const { return {c, address + 1}; }
    // Kinds 1-12: appearances, goals, conceded, penalties, assists, team
    // goals, team conceded, won, lost, yellow cards, red cards, man of the match.
    Field<std::uint8_t> tally(std::int32_t kind) const {
        constexpr std::uint8_t offsets[13] = {0, 0, 2, 3, 4, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd};
        return {c, address + offsets[kind]};
    }
    // The figure printed in brackets after the penalties (tally(4)).
    Field<std::uint8_t> penalties_bracketed() const { return {c, address + 5}; }
    Field<std::uint8_t> games_won() const { return {c, address + 9}; }
    Field<std::uint8_t> games_lost() const { return {c, address + 0xa}; }
    Field<std::int16_t> rating_total() const { return {c, address + 0xe}; }  // -1: a freed record
    Field<std::int16_t> passes() const { return {c, address + 0x10}; }
    Field<std::int16_t> passes_completed() const { return {c, address + 0x12}; }
    Field<std::int16_t> tackles() const { return {c, address + 0x14}; }
    Field<std::int16_t> dribbles() const { return {c, address + 0x16}; }
    Field<std::int16_t> shots() const { return {c, address + 0x18}; }
    Field<std::int16_t> shots_on_target() const { return {c, address + 0x1a}; }
    Field<std::int32_t> figure() const { return {c, address + 0x1c}; }
};
// A player's totals over several season records, as the stats screens add
// them up (0x4c bytes, built by the caller, often on its stack): the tallies
// as int32 in SeasonRecord's order, the rating total as int32, the int16
// figures, and the minutes played.
struct SeasonTotals {
    Cpu& c;
    std::uint32_t address;
    Field<std::uint32_t> appearances() const { return {c, address}; }
    Field<std::uint32_t> substitute_appearances() const { return {c, address + 4}; }
    // Kinds 2-12 as SeasonRecord::tally (kind 4: penalties, then the bracketed figure at +0x14).
    Field<std::uint32_t> tally(std::int32_t kind) const {
        return {c, address + 4 * std::uint32_t(kind + (kind > 4 ? 1 : 0))};
    }
    Field<std::uint32_t> penalties_bracketed() const { return {c, address + 0x14}; }
    Field<std::int32_t> rating_total() const { return {c, address + 0x38}; }
    Field<std::int16_t> passes() const { return {c, address + 0x3c}; }
    Field<std::int16_t> passes_completed() const { return {c, address + 0x3e}; }
    Field<std::int16_t> tackles() const { return {c, address + 0x40}; }
    Field<std::int16_t> dribbles() const { return {c, address + 0x42}; }
    Field<std::int16_t> shots() const { return {c, address + 0x44}; }
    Field<std::int16_t> shots_on_target() const { return {c, address + 0x46}; }
    Field<std::int32_t> minutes() const { return {c, address + 0x48}; }
};
// A player's entry (Stats::players, 0x28 bytes, indexed by person id): the
// index of his season record for each category 1-6 (-1: none), where his
// past seasons lie in the archive, two int16 figures (entry_figure's kinds
// 1 and 2: his league games and goals for his club) and five 4-bit ratings
// packed in the high and low nibbles of +0x24, +0x25 and the high nibble of
// +0x26.
struct PlayerEntry {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x28;
    Field<std::int32_t> season(std::int32_t category) const { return {c, address + std::uint32_t(category - 1) * 4}; }
    // Where his past seasons' CareerRows lie in Stats::archive (size -1: none).
    Field<std::int32_t> history_offset() const { return {c, address + 0x18}; }
    Field<std::int32_t> history_size() const { return {c, address + 0x1c}; }
    Field<std::int16_t> figure(std::int32_t kind) const { return {c, address + 0x1e + std::uint32_t(kind) * 2}; }
    // The figures the history import adds his real-world seasons at his
    // current club to.
    Field<std::int16_t> real_appearances() const { return figure(1); }
    Field<std::int16_t> real_goals() const { return figure(2); }
    Field<std::uint8_t> ratings(std::uint32_t byte) const { return {c, address + 0x24 + byte}; }  // byte 0-2
};
// A row of a person's career table (career_table, 0x10305c60; 0x30 bytes):
// one season's tallies for one category, six rows (categories 1-6) per
// season, newest season first; the archive's history rows have the same
// layout. Bytes 2-3 are never written by the table's builder (they keep
// what the allocator left). `other_club` and `status` are moved one season
// up by the builder; their meaning is not confirmed (status 1 is treated
// apart, likely a loan).
struct CareerRow {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x30;
    Field<std::uint16_t> year() const { return {c, address}; }     // 0: a dropped row
    Field<std::int32_t> club() const { return {c, address + 4}; }  // club id, -1: none
    SeasonRecord record() const { return {c, address + 8}; }
    Field<std::int32_t> other_club() const { return {c, address + 0x28}; }  // -1: none
    Field<std::uint8_t> kind() const { return {c, address + 0x2c}; }  // category (low 5 bits), status (high 3)
    std::uint32_t category() const { return kind() & 0x1fu; }
    std::uint32_t status() const { return std::uint32_t(kind()) >> 5; }
};

// The player-stats object (slot::player_stats, 0x48 bytes).
struct Stats {
    Cpu& c;
    std::uint32_t address;
    // The history archive the players' career rows are read from (an 8-byte
    // file object, history_file; PlayerEntry::history_offset/history_size).
    Field<std::uint32_t> archive() const { return {c, address}; }
    // The season records (SeasonRecord[]) and the per-player entries
    // (PlayerEntry[], indexed by person id; the count is slot::stats_players'
    // value when the object is built).
    Field<std::uint32_t> seasons() const { return {c, address + 4}; }
    Field<std::int32_t> season_capacity() const { return {c, address + 8}; }
    Field<std::int32_t> season_count() const { return {c, address + 0xc}; }
    // Two doubles each: the sum of the match ratings given and how many were
    // given, for games with the match's `rated` flag (+0x20) and without (+0x10).
    std::uint32_t rating_sums(bool rated) const { return address + (rated ? 0x20u : 0x10u); }
    Field<std::uint32_t> players() const { return {c, address + 0x30}; }
    Field<std::int32_t> player_count() const { return {c, address + 0x34}; }
    // Freed season records (their rating_total -1) that add_season_record reuses.
    Field<std::int32_t> free_seasons() const { return {c, address + 0x38}; }
    Field<std::uint32_t> relations() const { return {c, address + 0x3c}; }  // Relation[], 0 until the first
    Field<std::int32_t> relation_capacity() const { return {c, address + 0x40}; }
    Field<std::int32_t> relation_count() const { return {c, address + 0x44}; }
    Relation relation(std::int32_t index) const { return {c, relations() + std::uint32_t(index) * Relation::size}; }
};

// A fixture as record_match and competition_type read it.
struct Fixture {
    Cpu& c;
    std::uint32_t address;
    Field<std::int32_t> id() const { return {c, address}; }  // index of slot::competitions; -2: no competition
    Field<std::uint32_t> competition() const { return {c, address + 0x14}; }  // its first word: the competition id
    Field<std::uint32_t> home() const { return {c, address + 0x1c}; }  // clubs
    Field<std::uint32_t> away() const { return {c, address + 0x20}; }
    // Nonzero: the game counts by the competition's kind for that side,
    // zero: non-competitive for it (meaning not confirmed).
    Field<std::int8_t> home_counts() const { return {c, address + 0x40}; }
    Field<std::int8_t> away_counts() const { return {c, address + 0x41}; }
};
// A competition (slot::competitions): its kind decides the category.
struct Competition {
    Cpu& c;
    std::uint32_t address;
    Field<std::int8_t> kind() const { return {c, address + 0x42}; }  // 1 domestic, 2 continental, 4 international
};
// A played match as record_match reads it: two sides, each with its goals
// and twenty rows (eleven starters, then the substitutes).
struct Match {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t side_size = 0x664, rows = 20;
    Field<std::int32_t> man_of_match() const { return {c, address + 0x3c}; }  // player index
    Field<std::int8_t> rated() const { return {c, address + 0x4c}; }  // which Stats rating sums (meaning not confirmed)
    Field<std::int16_t> length() const { return {c, address + 0x4e}; }  // minutes
    std::uint32_t side(std::uint32_t n) const { return address + 0x120 + n * side_size; }
    Field<std::uint8_t> goals(std::uint32_t n) const { return {c, side(n)}; }
};
// A player's row of a Match side (0x4c bytes at side + 0x74). Its tallies
// go into SeasonRecord's fields of the same names.
struct MatchRow {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x4c, first = 0x74;
    Field<std::int32_t> person() const { return {c, address}; }  // -1: empty
    Field<std::int32_t> player() const { return {c, address + 4}; }  // -1: empty
    Field<std::uint8_t> goals() const { return {c, address + 0x21}; }
    Field<std::uint8_t> conceded() const { return {c, address + 0x22}; }
    Field<std::uint8_t> penalties() const { return {c, address + 0x23}; }
    Field<std::uint8_t> penalties_bracketed() const { return {c, address + 0x24}; }
    Field<std::uint8_t> shots() const { return {c, address + 0x25}; }
    Field<std::uint8_t> shots_on_target() const { return {c, address + 0x26}; }
    Field<std::uint8_t> assists() const { return {c, address + 0x27}; }
    Field<std::uint8_t> dribbles() const { return {c, address + 0x28}; }
    Field<std::uint8_t> yellow_cards() const { return {c, address + 0x2c}; }
    Field<std::uint8_t> red_cards() const { return {c, address + 0x2d}; }
    Field<std::int8_t> sent_off() const { return {c, address + 0x30}; }     // minute, -1: not
    Field<std::int8_t> came_on() const { return {c, address + 0x32}; }      // minute, -1: did not play
    Field<std::int8_t> substituted() const { return {c, address + 0x34}; }  // minute, -1: not
    Field<std::uint8_t> rating() const { return {c, address + 0x35}; }
    Field<std::uint8_t> passes() const { return {c, address + 0x3e}; }
    Field<std::uint8_t> passes_completed() const { return {c, address + 0x3f}; }
    Field<std::uint8_t> tackles() const { return {c, address + 0x42}; }
};
// A nation's season dates (slot::calendars, 0x4c bytes; 34 of them).
struct Calendar {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 0x4c, count = 0x22;
    Field<std::int16_t> start_day() const { return {c, address + 0x2c}; }
    Field<std::uint16_t> start_year() const { return {c, address + 0x2e}; }
    Field<std::int16_t> end_day() const { return {c, address + 0x3c}; }
    Field<std::uint16_t> end_year() const { return {c, address + 0x3e}; }
};

// TOC slots PLAYER_STATS.CPP reads (lwz rX,slot(r2): the address of the
// object or table; `Cpu::toc(slot)`).
namespace slot {
// The module's own.
constexpr std::int32_t pool = -0x5050;            // the module's strings (text:: offsets)
constexpr std::int32_t constants = -0x5054;       // double constants (0x10802e18)
constexpr std::int32_t constants2 = -0x5064;      // more constants (0x10802df8)
constexpr std::int32_t cd_name_a = -0x5048, cd_name_b = -0x504c;  // halves of the CD volume name, interleaved
constexpr std::int32_t compare_relations_tv = -0x5058;   // TVector of compare_relations
constexpr std::int32_t compare_by_club_tv = -0x505c;     // TVector of compare_history_by_club
constexpr std::int32_t compare_by_player_tv = -0x5060;   // TVector of compare_history_by_player
constexpr std::int32_t value_cases = -0x506c;     // season_value's jump table (23 case labels)
// The game's.
constexpr std::int32_t success = -0x7bac;         // int8: cleared by every assertion report
constexpr std::int32_t message = -0x7ba8;         // the shared message buffer
constexpr std::int32_t game = -0x7bb0;            // the game object
constexpr std::int32_t player_stats = -0x7b3c;    // the player-stats object (0x48 bytes)
constexpr std::int32_t person_count = -0x7be4;    // int32: number of person records
constexpr std::int32_t persons = -0x7be0;         // person records (0x70 bytes each)
constexpr std::int32_t clubs = -0x7be8;           // club records (0x248 bytes each)
constexpr std::int32_t club_count = -0x7b50;      // int32: entries of slot::clubs
constexpr std::int32_t player_records = -0x7b38;  // player records (0x48 bytes each; the first word indexes Stats::players)
constexpr std::int32_t stats_players = -0x74d4;   // int32*: entries of Stats::players
constexpr std::int32_t today = -0x7b80;           // the game date: int16 day of year, uint16 year
constexpr std::int32_t start_date = -0x7a9c;      // Date*: the game's start date
constexpr std::int32_t options = -0x7410;         // the start-up options text
constexpr std::int32_t history_rows = -0x74c4;    // HistoryRow* (count at history_count)
constexpr std::int32_t history_count = -0x74c8;   // int32
constexpr std::int32_t contracts = -0x7b7c;       // the contracts object (0x10082f90's first argument)
constexpr std::int32_t save_version = -0x7b74;    // int32*: the loaded save's version (its abs() is compared)
constexpr std::int32_t club_histories = -0x7afc;  // the object 0x100134b0 adds a player's totals at a club to (its first word)
constexpr std::int32_t scratch_text = -0x7bb8;    // a text buffer (the too-many-goals warning)
// Two int8 flags the history import tests: it runs when the first is 0 and
// the second is not (meaning not confirmed).
constexpr std::int32_t history_import_off = -0x6338, history_import_on = -0x6318;
// Ids of four nations whose regions count as affinity groups of their own
// (group_score in player_stats_relations.cpp).
constexpr std::int32_t regional_nation_1 = -0x7514, regional_nation_2 = -0x7504, regional_nation_3 = -0x750c,
                       regional_nation_4 = -0x7510;
constexpr std::int32_t calendars = -0x7b44;       // Calendar[] (the TOC word is the array)
// The game's random-number table: a cursor stepping two bytes through
// uint16 values up to random_end, back at random_table with a new salt
// (rand() & 0xffff) when it passes the end; a draw is the value plus the salt.
constexpr std::int32_t random_cursor = -0x7b84, random_end = -0x7b88, random_table = -0x7b8c, random_salt = -0x7b90;
constexpr std::int32_t alert_text = -0x79e0, alert_title = -0x79dc;  // buffers of the out-of-memory alert
constexpr std::int32_t competitions = -0x7b5c;    // Competition* per Fixture::id
// The object 0x10494d20 asks whether a fixture's competition is non-competitive
// (meaning not confirmed), and three competitions whose games are non-competitive.
constexpr std::int32_t fixture_rules = -0x7b54;
constexpr std::int32_t friendly_competition_1 = -0x7aa8, friendly_competition_2 = -0x6944,
                       friendly_competition_3 = -0x6940;
}

// The module's strings (offsets into its pool, slot::pool).
namespace text {
constexpr std::uint32_t insert_cd_title = 0, insert_cd = 0xa, temp_folder = 0x37, history_temp = 0x40,
                        file_name = 0x59, assert_format = 0x6a, error_title = 0x77, no_real_history = 0x7d,
                        player_histories = 0x8a, check_goalkeepers = 0x9b, too_many_goals = 0xa4, newline = 0xc5,
                        non_competitive = 0xc7, league = 0x106, cup = 0x10d, continental = 0x111,
                        international = 0x146, senior_club = 0x17f, dash = 0x18b, number_dash = 0x18d,
                        non_competitive_short = 0x191, continental_short = 0x1c9, international_short = 0x1cf,
                        club_short = 0x1d4, appearances = 0x1d9, goals = 0x1e5, conceded = 0x1eb, penalties = 0x1f4,
                        assists = 0x1fe, team_goals = 0x206, team_conceded = 0x211, games_won = 0x21f,
                        games_lost = 0x229, yellow_cards = 0x234, red_cards = 0x241, man_of_match = 0x24b,
                        average_rating = 0x258, pass_completion = 0x267, tackles_per_game = 0x277,
                        dribbles_per_game = 0x286, shots_on_target = 0x296, apps_short = 0x2ac, goals_short = 0x2b1,
                        conceded_short = 0x2b5, penalties_short = 0x2df, assists_short = 0x2e4,
                        team_goals_short = 0x2e9, team_conceded_short = 0x2ee, won_short = 0x2f3, lost_short = 0x311,
                        yellow_short = 0x316, red_short = 0x31a, man_of_match_short = 0x31e, rating_short = 0x322,
                        pass_short = 0x327, tackles_short = 0x32e, dribbles_short = 0x332, shots_short = 0x336,
                        no_value = 0x33d, count_of = 0x342, count = 0x34a, count_per = 0x34d, count_of_per = 0x355,
                        ratio2_per = 0x362, ratio2 = 0x36d, percent_per = 0x373, percent = 0x37d, ratio1_per = 0x382,
                        ratio1 = 0x38d, stats_file = 0x393, new_history_temp = 0x3a4, assert_format_detail = 0x3b4,
                        no_memory_detail = 0x3c6, no_memory_title = 0x406, no_memory = 0x41e;
// "" (the byte before "League"): localize's argument for a text's
// "<%s - COMMENT ...>" part.
constexpr std::uint32_t empty = 0x105;
}

// "player stats.dat", an entry of the save archive: the object's fields in
// this order, big-endian as in memory: season_capacity, season_count (4 bytes
// each), season_count SeasonRecords (0x20 bytes; saves older than version
// full_records_version stored their first 0x10 bytes only, the rest starting
// at 0), player_count, the PlayerEntries (0x28 bytes each), the four doubles
// at +0x10-0x2f (rating_sums), free_seasons, relation_capacity,
// relation_count, then relation_capacity Relations (8 bytes each). Loading an
// older save also rewrites the history archive: its rows were 0x20 bytes (the
// SeasonRecord's first 0x10 bytes), now CareerRows.
namespace stats_file {
constexpr std::uint32_t archive_type = 0x16, archive_flags = 4;  // the archive open's last arguments (every caller's)
constexpr std::int32_t full_records_version = 0x3e015;
constexpr std::uint32_t old_record_size = 0x10, old_row_size = 0x20;
}
// The history archive (Stats::archive): an 8-byte file object in the
// temporary folder holding each player's CareerRows as a list, where
// PlayerEntry::history_offset/history_size say.
namespace history_file {
constexpr std::uint32_t size = 8;
constexpr std::uint32_t open = 0x10748bb0;      // (object, path, mode)
constexpr std::uint32_t destroy = 0x10748c80;   // (object, 1)
constexpr std::uint32_t read = 0x10749fd0;      // (object, offset, size): a new list of the rows
constexpr std::uint32_t write = 0x107495b0;     // (object, &offset, &size, list): stores the list, sets both
constexpr std::uint32_t mode = 0x30, old_mode = 0x20;  // old_mode: an older save's file, reopened for converting
}
// The game's lists (16 bytes; the conversion sets the cursor at +0xc to the first item at +4).
namespace list {
constexpr std::uint32_t create = 0x10738c00, destroy = 0x10738c20 /* (list, flags) */, clear = 0x10738ca0,
                        append = 0x10738d10 /* (list, item, size): the stored copy */,
                        next = 0x10738e00 /* the item at the cursor, 0 at the end */;
}
// Other routines of the rest of the program the module calls (Program::call).
namespace routine {
constexpr std::uint32_t abs = 0x10775550, memcpy = 0x10739ef0, remove = 0x10776690, operator_delete = 0x107735a0;
constexpr std::uint32_t copy_file = 0x107470b0;        // (from, to)
constexpr std::uint32_t fs_make_fs_spec = 0x1077fee0;  // Toolbox glue
constexpr std::uint32_t progress = 0x10743120;         // (game, title, done, total, -1)
constexpr std::uint32_t add_club_history = 0x100134b0;  // (club_histories, person, club, appearances, goals)
constexpr std::uint32_t roles = 0x10117490;            // (player record): int16 position bits, 1: goalkeeper only
constexpr std::uint32_t append_name = 0x100caeb0;      // (person, to, size, 1)
constexpr std::uint32_t days_between = 0x10718130;     // (date, other date by value): the calendar's date difference
constexpr std::uint32_t free_history_rows = 0x100a4c00;  // frees slot::history_rows, clears its count
// For calls that set their argument registers themselves (Program::sprintf
// and Program::localize set them from their arguments).
constexpr std::uint32_t sprintf = 0x1077b720, localize = 0x10727890;
}
// The text routines assert a buffer of at least this many bytes.
constexpr std::int32_t minimum_text_buffer = 0x33;

// The compiler's int-to-double idiom: the words 0x43300000 and the integer
// (signed: signed_word, the value xor 0x80000000) in the stack temporary at
// `at`, read back as a double; the original then subtracts the pool's bias
// (fsub or fsubs). The temporary is part of the stack image.
inline double stacked_integer(Cpu& c, std::uint32_t at, std::uint32_t low) {
    c.put32(at, 0x43300000);
    c.put32(at + 4, low);
    return c.f64(at);
}
inline std::uint32_t signed_word(std::int32_t value) { return std::uint32_t(value) ^ 0x80000000u; }
// The pool's biases for it (slot::constants +0x20 and +0x30).
constexpr double signed_bias = 4503601774854144.0;    // 2^52 + 2^31
constexpr double unsigned_bias = 4503599627370496.0;  // 2^52

// The rest of the original program that PLAYER_STATS.CPP calls: the game,
// the C library and the Toolbox glues. Each call sets the argument
// registers the original's call site sets and returns r3; `back` is the
// address after the original `bl`. The caller sets the non-volatile
// registers the original holds at the call first (callees save them into
// the stack image). Calls through a Toolbox glue reload r2 from sp+0x14
// afterwards, as the original's next instruction does (toolbox()).
class Program {
public:
    explicit Program(Cpu& c) : c(c) {}
    std::uint32_t call(std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) {
        unsigned n = 3;
        for (const auto value : arguments) c.set(n++, value);
        c.call(entry, back);
        return c.r(3);
    }
    std::uint32_t toolbox(std::uint32_t glue, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) {
        const auto result = call(glue, back, arguments);
        c.set(2, c.u32(c.sp() + 0x14));
        return result;
    }
    // The C library.
    std::uint32_t malloc(std::uint32_t size, std::uint32_t back) { return call(0x10775130, back, {size}); }
    void free(std::uint32_t pointer, std::uint32_t back) { call(0x10775170, back, {pointer}); }
    std::uint32_t realloc(std::uint32_t pointer, std::uint32_t size, std::uint32_t back) {
        return call(0x107751b0, back, {pointer, size});
    }
    void memset(std::uint32_t to, std::uint32_t value, std::uint32_t size, std::uint32_t back) {
        call(0x10778300, back, {to, value, size});
    }
    std::uint32_t strlen(std::uint32_t text, std::uint32_t back) { return call(0x1077dbd0, back, {text}); }
    void strcpy(std::uint32_t to, std::uint32_t text, std::uint32_t back) { call(0x1077dbf0, back, {to, text}); }
    void strcat(std::uint32_t to, std::uint32_t part, std::uint32_t back) { call(0x1077dc60, back, {to, part}); }
    std::uint32_t strstr(std::uint32_t text, std::uint32_t part, std::uint32_t back) {
        return call(0x1077dda0, back, {text, part});
    }
    // sprintf (0x1077b720) is varargs: its callee stores r3-r10 into the
    // caller's frame, so set every one the original holds there.
    std::uint32_t sprintf(std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        return call(0x1077b720, back, arguments);
    }
    void qsort(std::uint32_t base, std::uint32_t count, std::uint32_t size, std::uint32_t compare, std::uint32_t back) {
        call(0x10768300, back, {base, count, size, compare});
    }
    std::uint32_t bsearch(std::uint32_t key, std::uint32_t base, std::uint32_t count, std::uint32_t size,
                          std::uint32_t compare, std::uint32_t back) {
        return call(0x10775600, back, {key, base, count, size, compare});
    }
    std::uint32_t operator_new(std::uint32_t size, std::uint32_t back) { return call(0x10773520, back, {size}); }
    // The game.
    void localize(std::uint32_t to, std::uint32_t text, std::uint32_t back) { call(0x10727890, back, {to, text}); }
    // An alert with `text` and `title` (and a flag the error reports pass as 0).
    void alert(std::uint32_t title, std::uint32_t text, std::uint32_t flag, std::uint32_t back) {
        call(0x10769b60, back, {title, text, flag});
    }
    void fatal_exit(std::uint32_t code, std::uint32_t back) { call(0x1076f7d0, back, {code}); }
    std::uint32_t folder_path(std::uint32_t folder, std::uint32_t name, std::uint32_t back) {
        return call(0x10746280, back, {folder, name});
    }
    // Entries of the save archive (formation_files.cpp names the same calls).
    std::uint32_t open_entry(std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        return call(0x107475b0, back, arguments);
    }
    std::uint32_t read_entry(std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        return call(0x10747a80, back, arguments);
    }
    std::uint32_t write_entry(std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        return call(0x10747cc0, back, arguments);
    }
    std::uint32_t close_entry(std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        return call(0x10747900, back, arguments);
    }

private:
    Cpu& c;
};

// The original's assertion report (guest::report_assertion) with this
// module's pool texts: `at` is the address of its first call, `name` and
// `extension` the two stack buffers, `line` the source line it prints.
// Most sites clear the success flag through r4 (`lwz r4,-0x7bac(r2); li
// r0,0; stb r0,0(r4)`), which then holds the flag's address; sites that keep
// that address in a non-volatile register pass flag_in_r4 = false.
inline void report(Cpu& c, std::uint32_t pool, std::uint32_t at, std::uint32_t name, std::uint32_t extension,
                   std::uint32_t line, bool flag_in_r4 = true) {
    guest::report_assertion(c, {at, pool + text::file_name, pool + text::assert_format, pool + text::error_title,
                                name, extension, line});
    if (flag_in_r4) c.set(4, c.toc(slot::success));
}
}
