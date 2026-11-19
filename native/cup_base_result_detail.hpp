#pragma once
// CUP.CPP, block `result` (cup_base_result*.cpp): the records and frame offsets of CUP::process_fixture
// (0x1047bfb0, vt +0xc). Everything here is private to the block: the shared names stay in cup_base_detail.hpp.
// Field meanings come from how this routine and its callers use them; "(inferred)" marks a name no reader has
// confirmed.
#include "cup_base_detail.hpp"
#include <array>
#include <initializer_list>
#include <optional>
#include <utility>

namespace cm::aot::cup_base::result {

// The played fixture the routine is given (0x50 bytes; the game's match record), and its local copy.
namespace fixture {
inline constexpr std::uint32_t
    competition_id = 0x00,  // word: the competition's id (the copy gets it from the cup's record)
    word_04 = 0x04,         // word: the id the copy takes from word_18's record, -1 without one (inferred)
    word_08 = 0x08,         // word, -1 in the copy
    home_id = 0x0c,         // word: the home club's id (a stack argument of FIX_MAN 0x1048aea0) (inferred)
    away_id = 0x10,         // word: the away club's id (inferred)
    record = 0x14,          // word: the cup's competition record (the copy's)
    word_18 = 0x18,         // word: a club record's +0x3c word (the copy follows the clubs swapping sides) (inferred)
    home = 0x1c,            // word: the home club
    away = 0x20,            // word: the away club
    half_24 = 0x24, half_26 = 0x26,  // the next round's date in the copy (0x1047dd30: year, day)
    half_36 = 0x36,         // halfword, the round's +0xc in the copy
    year = 0x28,            // halfword: the year of the match's date
    day = 0x2a,             // halfword: its day (DATE.CPP's pair (day, year) are +0x2a, +0x28)
    half_2c = 0x2c,         // halfword, the competition's +0x40 in the copy (inferred)
    half_2e = 0x2e,         // halfword
    order = 0x30,           // halfword: 0x104648c0 of it plus 1, 2 or 5 (inferred: the slot in the day)
    half_32 = 0x32,         // halfword, the competition's +0x3a in the copy
    half_34 = 0x34,         // halfword: the round (the copy: the round's +0xa)
    flags = 0x38,           // halfword: 0x10 and 0x2000 (swap the goals / sides), 0x800, 0x1000 (copy: the round's +0xe)
    byte_3a = 0x3a,         // byte, the round's +0x18 in the copy
    byte_3b = 0x3b,         // byte, 0xff in the copy
    home_goals = 0x3d,      // byte
    away_goals = 0x3e,      // byte
    day_kind = 0x3f,        // byte: 1 when the date's weekday (0x10715b20) is 5 or 6, else 2 (inferred)
    byte_40 = 0x40, byte_41 = 0x41,
    stage = 0x42,           // byte: the stage of the competition the fixture belongs to
    score = 0x43,           // 10 bytes: the score block copied to the match row's leg slot (the copy: 0xff)
    score_size = 10,
    bits = 0x4f,            // byte: its top nibble holds flags the copy sets (0x20 among them)
    size = 0x50;
// The new fixture FIX_MAN 0x1048aea0 returns (written by this routine).
inline constexpr std::uint32_t new_goals_home = 0x3d, new_goals_away = 0x3e, new_score_a = 0x4b, new_score_b = 0x4c;
}

// A row of the round's match table (cup::round_matches: one 0x36-byte row per match).
namespace match {
inline constexpr std::uint32_t
    home = 0x00,         // halfword: the entrant index of the first club
    away = 0x02,         // halfword: the second club's
    first_goals = 0x04,  // byte: the goals of the club that is the fixture's home
    second_goals = 0x05, // byte
    slot = 0x06,         // halfword: the match slot of the winner's next tie (-1: the final) (inferred)
    side = 0x08,         // halfword: 0 writes the next tie's +2, else its +0 (inferred)
    next_round = 0x12,   // halfword: the round the winner goes to (inferred)
    first_leg = 0x14,    // 10 bytes: the score block of the first leg
    second_leg = 0x1e,   // 10 bytes
    third_leg = 0x28,    // 10 bytes (a replay)
    legs_played = 0x32,  // byte (inferred)
    replay = 0x33,       // byte: the leg index is legs_played + this (the real two-legged rows hold 1; outcome 3 stores 1) (inferred)
    state = 0x34;        // byte: 3 when the next tie is waiting for its clubs (inferred)
}

// A row of cup::rounds (cup_base_detail.hpp's Round names the few the other blocks use).
namespace round_row {
inline constexpr std::uint32_t
    start_day = 0x04,     // halfword
    year_offset = 0x06,   // signed byte, added to the competition's year
    byte_07 = 0x07,
    label = 0x08,         // halfword: 0x8c is the final
    half_0a = 0x0a, half_0c = 0x0c,
    flags = 0x0e,         // halfword: 0x800 asks which side is stronger
    half_10 = 0x10, half_12 = 0x12,
    word_14 = 0x14,       // word: STADIUM.CPP's argument
    byte_18 = 0x18,
    entrants = 0x1e,      // halfword: summed over the rounds
    legs_needed = 0x22,   // signed byte
    rule = 0x23,          // signed byte: 2 is two-legged
    days = 0x24,          // byte: days between two legs
    prize = 0x6c;         // word: prize money
}

// The cup object's other fields (cup_base_detail.hpp's cup:: and comp's competition::).
inline constexpr std::uint32_t cup_last_stage = 0x30, cup_stages = 0x0c, cup_stats = 0x08,
                               cup_byte_a9 = 0xa9, cup_finished = 0xb0, cup_word_24 = 0x24;

// What the cup's virtual +0xa0/+0xa4 answers (inferred from the branches below).
inline constexpr std::uint32_t outcome_error = 0xffffffffu, outcome_level = 0, outcome_home_through = 1,
                               outcome_away_through = 2, outcome_replay = 3, outcome_postponed = 4;

// Frame offsets (the original's frame is 0x4e0 bytes, r14-r31 saved).
namespace at {
inline constexpr std::int32_t copy = 0x360,  // the fixture's local copy (0x50 bytes)
                              news = 0x3b0;  // the news item
}
// Slots of the cup's vtable this routine calls.
namespace slot {
inline constexpr std::uint32_t process_stage = 0x0c, entrant_result = 0x48, round_data = 0x84, tie_decider = 0xa0,
                               leg_decider = 0xa4;
}
// TOC slots this routine reads beyond cup_base_detail.hpp's toc::.
namespace cell {
inline constexpr std::int32_t finance = -0x7ab8,    // word: the finance object (FINANCE.CPP)
                              fixtures = -0x7a60,   // word: the fixture manager (FIX_MAN.CPP)
                              today = -0x7b80,      // word: the game's current date (DATE.CPP's pair)
                              stadium = -0x742c;    // word: a cell holding the stadium manager (STADIUM.CPP)
}
// Callees of this routine.
namespace callee {
inline constexpr std::uint32_t
    comp_stats = 0x104537b0, finance_club = 0x10103510, finance_pay = 0x100f71f0, find_match = 0x1047ba50,
    block_move = 0x1077fc28, date_from = 0x107156b0, date_add = 0x10717890, date_weekday = 0x10715b20,
    fixtures_new = 0x1048aea0, fixtures_add = 0x10489900, fixtures_update = 0x104888f0, match_flags = 0x10485340,
    entrant_note = 0x10484fd0, prize_money = 0x10485930, pairings = 0x10481ad0, stadium_book = 0x104aff20,
    order_rank = 0x104648c0, news_on = 0x102cdbf0, news_start = 0x102bdf90, news_set = 0x102cb4b0,
    news_file = 0x102cc140, format = 0x1077b720;
}
// The assertion sites: the address of the first call, the frame offsets of the two buffers and the source line.
namespace site {
inline constexpr cup_base::CheckSite
    no_fixture{0x1047c018, 0x340, 0x1d8, 0x467}, bad_stage{0x1047c0a0, 0x320, 0x1d0, 0x473},
    no_match{0x1047c240, 0x300, 0x1c8, 0x4a3}, bad_legs{0x1047c348, 0x2e0, 0x1c0, 0x4be},
    no_outcome{0x1047c4a4, 0x2c0, 0x1b8, 0x4e6}, no_new_fixture{0x1047c5d4, 0x2a0, 0x1a8, 0x4fe},
    drawn_level{0x1047c6f8, 0x280, 0x1a0, 0x51f}, third_replay{0x1047ca98, 0x260, 0x190, 0x5aa},
    drawn_postponed{0x1047cc40, 0x240, 0x180, 0x5c7}, unknown_outcome{0x1047da30, 0x220, 0x168, 0x78c},
    no_first_club{0x1047dd8c, 0x200, 0x160, 0x804}, no_second_club{0x1047de20, 0x1e0, 0x158, 0x80f};
}

// The comparison the original inlines for "is date a before date b": the years as unsigned halfwords, then the
// days as the sign-bit expression CodeWarrior emits for a signed halfword compare (0x1047cb0c..0x1047cb54).
inline bool date_before(std::uint16_t a_day, std::uint16_t a_year, std::uint16_t b_day, std::uint16_t b_year) {
    if (a_year < b_year) return true;
    if (a_year > b_year) return false;
    const auto a = std::uint32_t(std::int32_t(std::int16_t(a_day))), b = std::uint32_t(std::int32_t(std::int16_t(b_day)));
    const auto x = b ^ a;
    const auto difference = std::uint32_t(std::int32_t(x) >> 1) - (x & b);
    return (difference >> 31) != 0;
}

// The rlwimi the original uses on the copy's flag byte (+0x4f): the bits of `mask` take `value`'s.
inline std::uint32_t with_bits(std::uint32_t byte, std::uint32_t mask, std::uint32_t value) { return (byte & ~mask) | (value & mask); }

// The routine's state while it runs: the object, the fixture and the context it is given, the frame, the
// non-volatile registers the original holds (they reach every callee's save area and every import's
// observer, so `sync` writes them before each call) and one method per phase of the original.
// A phase that ends the routine early returns the value (r3) it returns.
class Process {
public:
    Process(Cpu& cpu, std::uint32_t cup, std::uint32_t fixture, std::uint32_t context);
    std::uint32_t run();

private:
    using Leftovers = std::initializer_list<std::pair<unsigned, std::uint32_t>>;
    // The calls a "single round" check and its ending make (0x1047c678.. for outcome 0, 0x1047cbb4.. for 4).
    struct SingleRound { cup_base::CheckSite assertion; std::uint32_t format_bl, note_first_bl, note_second_bl; };
    // The calls a clubs-through ending makes: one vt +0x48 call per club and the two notes.
    struct Ending { std::uint32_t entrant_first_bl, entrant_second_bl, note_first_bl, note_second_bl, prize_bl; };

    // cup_base_result.cpp
    std::optional<std::uint32_t> check_arguments();
    std::optional<std::uint32_t> record_statistics();
    std::optional<std::uint32_t> locate_match();
    std::optional<std::uint32_t> store_leg_score();
    std::uint32_t ask_outcome();
    void note_goals();
    std::uint32_t schedule_replay();
    std::uint32_t entrants_decided();
    // cup_base_result_through.cpp
    std::uint32_t go_through(bool home);
    std::uint32_t final_through(bool home);
    void tie_through(bool home);
    void tell_entrant(std::uint32_t entrant, std::uint32_t state_as_word, std::uint32_t bl);
    // cup_base_result_play_on.cpp
    std::optional<std::uint32_t> single_round_decided(const SingleRound& where);
    std::uint32_t play_on();
    std::uint32_t postpone();
    std::pair<std::uint32_t, std::uint32_t> swap_sides_in_copy();
    void adjust_for_today(std::int32_t compare_buffer, std::uint32_t compare_bl, std::int32_t add_buffer, std::uint32_t add_bl, Leftovers leftovers);
    std::uint32_t frame_extra(std::int32_t buffer) const { return c.u8(frame.at(buffer + 4)); }  // a DATE pair's byte
    std::uint32_t today_word(unsigned index) const { return c.u32(held[24] + index * 4); }
    void refresh_record_word();
    void clear_score();
    std::uint32_t weekday_kind(std::uint32_t date, std::uint32_t bl, Leftovers leftovers = {});
    // cup_base_result_next.cpp
    std::uint32_t next_round();
    void schedule_next_tie();
    void file_news();
    void second_leg();

    // Calls and the registers they see.
    void sync();
    // `leftovers`: registers the original sets for no argument of the callee but which the callee may leave (or the
    // checker's answer does not touch), written after the arguments.
    std::uint32_t call(std::uint32_t entry, std::uint32_t bl, std::initializer_list<std::uint32_t> arguments, Leftovers leftovers = {});
    std::uint32_t vcall(std::uint32_t slot, std::uint32_t bl, std::initializer_list<std::uint32_t> arguments);
    std::uint32_t fail(const cup_base::CheckSite& where, std::uint32_t value);
    // Reads of the object's tables, fresh each time as the original loads them.
    std::uint32_t round_of_fixture() const { return sext16(c.u16(fixture + fixture::half_34)); }
    std::uint32_t rounds() const { return c.u32(cup + cup::rounds); }
    std::uint32_t round_row_at(std::uint32_t round) const { return rounds() + round * cup::round_row; }
    std::uint32_t matches_of(std::uint32_t round) const { return c.u32(c.u32(cup + cup::round_matches) + round * 4); }
    std::uint32_t entrants() const { return c.u32(cup + cup::entrants); }
    std::uint32_t entrant_row(std::uint32_t index) const { return entrants() + (index << 3); }
    std::uint32_t copy(std::uint32_t offset) const { return frame.at(at::copy) + offset; }
    std::uint32_t lha(std::uint32_t where) const { return sext16(c.u16(where)); }
    std::uint32_t lbz_extsb(std::uint32_t where) const { return sext8(c.u8(where)); }

    Cpu& c;
    std::uint32_t cup, fixture, context;
    std::array<std::uint32_t, 32> held{};  // r14..r31 as the original holds them at each call
    Frame frame;
    // The locals the original keeps in r16-r22 (held[] mirrors them for the calls).
    std::uint32_t match_index = 0, legs = 0, next_slot = 0, next_target = 0, stage_byte = 0;
};
}
