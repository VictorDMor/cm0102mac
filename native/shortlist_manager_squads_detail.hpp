#pragma once
// What shortlist_manager_squads*.cpp share: the records the club squad
// assessment (0x1035a9f0) reads and writes, its stack frame, and the phases
// it is split into. Names are from use; "(inferred)" marks meanings read
// from how the original uses a field rather than from a text.
//
// The assessment keeps its locals in r13-r31 across ~150 calls into the rest
// of the game, and each callee's prologue saves those registers into the
// stack image; so every phase keeps, in the non-volatile registers, what the
// original keeps there at each call (Kept/keep), even values the C++ does not
// need. Volatile registers are set where a later call's callee or the
// return can see them.
#include "shortlist_manager_detail.hpp"
#include "shortlist_manager_random.hpp"
#include "shortlist_manager_squads.hpp"

namespace cm::aot::shortlist_manager::squads {
using namespace detail;

constexpr std::uint32_t entry = 0x1035a9f0;
constexpr std::int32_t slot_count = 40;    // places in a club's plan: 25 for players, 15 for staff
constexpr std::int32_t player_slots = 25;  // the first 25 (kind player_place)
constexpr std::int32_t squad_size = 50;    // the squad list 0x100d06f0 fills

// The game's routines the assessment calls (names from use and from the
// older transcriptions; "(inferred)" where only the use says it).
namespace call {
constexpr std::uint32_t days_between = 0x10718130;      // (date, other date by value)
constexpr std::uint32_t human_club = 0x1014c2e0;        // (club, r4, r5, r6, r7): the club is in a human's hands
constexpr std::uint32_t club_check_7d00 = 0x100c7d00;   // (club): nonzero skips the assessment (inferred: no squad to assess)
constexpr std::uint32_t own_staff_query = 0x100ce320;   // (club, out int8, 1) (inferred)
constexpr std::uint32_t count_and_prune_rows = 0x103660b0;  // policy group: (manager, club, -1)
constexpr std::uint32_t random_below = 0x10356e40;      // storage group: the module's random(n)
constexpr std::uint32_t memset = 0x10778300;
constexpr std::uint32_t club_value = 0x102fd640;        // (club, out int32, 0) (inferred: what the club can spend)
constexpr std::uint32_t human_manager = 0x1014c5f0;     // (club): the human manager's staff (inferred)
constexpr std::uint32_t club_division = 0x100c96e0;     // (club) (inferred)
constexpr std::uint32_t option_7e70 = 0x10147e70;       // (options, r4) (inferred: a game option)
constexpr std::uint32_t option_9df0 = 0x10149df0;       // (options, 2, r5) (inferred)
constexpr std::uint32_t today_date = 0x107156b0;        // (out date, 1, 0x7cd) (inferred: a date in year 1997)
constexpr std::uint32_t qsort = 0x10768300;
constexpr std::uint32_t squad_list = 0x100d06f0;        // (club, out person*[50], 0)
constexpr std::uint32_t club_link_row = 0x1036d240;     // links group: the club's link row for the staff
constexpr std::uint32_t club_contract = 0x10082f90;     // (contracts, person, club): the person's contract with the club
constexpr std::uint32_t special_staff_offer = 0x103f43b0;  // (offers, person, 0)
constexpr std::uint32_t keep_contract = 0x1007edd0;     // (contracts, person, club) (inferred)
constexpr std::uint32_t current_injury = 0x1018c340;    // (injuries, person)
constexpr std::uint32_t risk_multiplier = 0x1018c270;   // (injuries, person)
constexpr std::uint32_t estimate = 0x10216830;          // (person, position, flags, club, -1, manager): a rating
constexpr std::uint32_t transfer_listed = 0x102de3f0;   // (list, person, 1) (inferred)
constexpr std::uint32_t release_contract = 0x1007ee50;  // (contracts, person, club) (inferred)
constexpr std::uint32_t second_contract = 0x10083110;   // (contracts, person)
constexpr std::uint32_t pick_players = 0x10210c50;      // picks players for positions (10 arguments)
constexpr std::uint32_t contains_staff_entry = 0x103e6470;  // (offers, person)
constexpr std::uint32_t player_score = 0x10219e40;      // (manager, person, club, mode, value) (inferred)
constexpr std::uint32_t collect_offer_indices = 0x103eaea0;       // (offers, person, 0, 0)
constexpr std::uint32_t collect_club_offer_indices = 0x103eb020;  // (offers, person, 0)
constexpr std::uint32_t club_mood = 0x100c9070;         // (club, 0, 0) (inferred)
constexpr std::uint32_t append_staff_entry = 0x103e6290;  // (offers, person, 0, -1)
constexpr std::uint32_t update_contract = 0x1007d800;   // (contracts, person, club, contract) (inferred)
constexpr std::uint32_t mark_primary_contract = 0x10363130;  // links group
constexpr std::uint32_t date_plus_years = 0x10717890;   // (out date, date, 0x78) (inferred)
constexpr std::uint32_t record_scale = 0x10187650;      // (injuries, person id)
constexpr std::uint32_t staff_rating = 0x101ad080;      // (staff object, person, club, manager, kind) (inferred)
constexpr std::uint32_t contract_by_id = 0x1007f720;    // (contracts, person id, club id) (inferred)
constexpr std::uint32_t drop_staff = 0x10197960;        // (staff object, person, club) (inferred)
constexpr std::uint32_t end_date_delta = 0x10374d10;    // (contract): days to the contract's end
constexpr std::uint32_t contains_staff_club_link = 0x10362d70;  // links group
constexpr std::uint32_t transition_staff_links = 0x10365590;    // links group
}

// TOC slots the assessment reads besides detail::slot's (words: addresses
// of the game's objects).
namespace toc {
constexpr std::int32_t records = -0x7bbc;        // 0x6c-byte records per nation (inferred), the word holds the table
constexpr std::int32_t options = -0x7a54;        // the game's options object (inferred)
constexpr std::int32_t injuries = -0x7ba0;       // the injury/condition object
constexpr std::int32_t offers = -0x7b40;         // the staff offers object
constexpr std::int32_t transfer_list = -0x7b28;  // (inferred)
constexpr std::int32_t home_nation = -0x7a2c;    // int32: (inferred) the nation of the game's base
constexpr std::int32_t club_nations = -0x7984;   // int32 pairs per club id (inferred)
constexpr std::int32_t staff = -0x79f0;          // the staff object 0x101ad080 and 0x10197960 take
}

// The squad plan is the club's shortlist entry (records::ClubEntry: its
// state 0 skipped, 1 assessed, 2 to refresh (inferred), target reputation,
// formation) and its places are the entry's rows (records::Row: kinds 6..10
// are staff jobs, the category is the place's role).
inline Row slot_at(Cpu& c, std::uint32_t slots, std::uint32_t offset) { return {c, slots + offset}; }

inline std::uint32_t person_address(Cpu& c, std::int32_t id) {
    return c.u32(c.toc(slot::persons)) + std::uint32_t(id) * Person::size;
}

// Quirk-exact helpers shared by the phases.
// The original's inline date ordering: 1 when date a (int16 day, uint16
// year) comes before date b, by year then with the compiler's signed
// less-than idiom on the days ((x >> 1) - (x & b) < 0 for x = a ^ b).
inline std::uint32_t before(std::int16_t day_a, std::uint16_t year_a, std::int16_t day_b, std::uint16_t year_b) {
    if (year_a < year_b) return 1;
    if (year_a > year_b) return 0;
    const std::int32_t x = std::int32_t(day_a) ^ std::int32_t(day_b);
    return std::uint32_t((x >> 1) - (x & std::int32_t(day_b))) >> 31;
}

// The assessment's frame (0xa50 bytes) and its locals, sp-relative.
namespace local {
constexpr std::int32_t stack_arguments = 0x38;  // pick_players' arguments 9-15 (0x38..0x50)
constexpr std::int32_t staff_flag = 0x58;       // int8: own_staff_query's answer
constexpr std::int32_t standard = 0x5c;         // int32: club_value's second answer (inferred: the squad's standard)
constexpr std::int32_t positions_a = 0xa0, positions_b = 0x104, positions_c = 0x168;  // int16[50] each
constexpr std::int32_t held_back = 0x1cc;       // person*[50]: older candidates, tried last
constexpr std::int32_t squad = 0x294;           // person*[50]
constexpr std::int32_t others = 0x35c;          // person*[50]: players with a second contract elsewhere
constexpr std::int32_t picked_b = 0x424, picked_c = 0x4ec, picked_a = 0x5b4;  // person*[50] each
constexpr std::int32_t staff = 0x67c;           // person*[]: the club's staff not linked yet
constexpr std::int32_t candidates = 0x80c;      // person*[100]
constexpr std::int32_t value = 0x9dc;           // int32: club_value
constexpr std::int32_t over_budget = 0x9e0;     // int8 (inferred)
constexpr std::int32_t count_c = 0x9e1, count_b = 0x9e2;  // int8
constexpr std::int32_t refreshed = 0x9e3;       // int8: a refresh of a club not in a human's hands
constexpr std::int32_t option = 0x9e4;          // int8: option_7e70's answer
constexpr std::int32_t days = 0x9e8;            // int32: days since the game's start
constexpr std::int32_t estimate_flags = 0x9ec;  // int32: 0x1802, or 0x800 for a human's club
constexpr std::int32_t count_a = 0x9f0;         // int32
constexpr std::int32_t picked_cursor = 0x9f4;   // person**
}

// The assessment's long-lived registers: r19 the plan, r20 the club, r21
// the refresh flag (the fifth argument), r26 the human manager (or 0), r29
// the contracts slot's word, r30 the constant pool.
struct Assessment {
    Cpu& c;
    Frame& frame;
    Program program;
    Fpu fpu;
    std::uint32_t manager;  // the first argument, kept at the caller's sp+0x18

    std::uint32_t at(std::int32_t offset) const { return frame.at(offset); }
    ClubEntry plan() const { return {c, c.r(19)}; }
    std::uint32_t club() const { return c.r(20); }
    bool refresh() const { return std::int8_t(c.r(21)) != 0; }
    std::uint32_t human_manager() const { return c.r(26); }
    std::uint32_t contracts() const { return c.u32(c.r(29)); }
    std::uint32_t constants() const { return c.r(30); }
    double constant(std::uint32_t offset) const { return c.f64(c.r(30) + offset); }
    std::uint32_t slots() const { return plan().rows(); }
    std::int16_t reputation() const { return std::int16_t(c.u16(club() + 0x1ec)); }
    std::uint32_t club_manager() const { return c.u32(club() + 0x90); }
    std::int8_t local8(std::int32_t offset) const { return c.i8(at(offset)); }
    std::int32_t local32(std::int32_t offset) const { return c.i32(at(offset)); }

    // human_club(club, r4, 1, 0, 0), tested as the original does (extsb.).
    bool human(std::uint32_t r4, std::uint32_t back) {
        return std::int8_t(program.call(call::human_club, back, {club(), r4, 1, 0, 0})) != 0;
    }
    // The int-to-double idiom at sp+offset (detail::stacked_integer), minus the bias.
    double integer(std::int32_t offset, std::int32_t value) {
        return fpu.fsub(stacked_integer(c, at(offset), signed_word(value)), constant(8));
    }
    // fctiwz to sp+offset (stfd), the low word read back (lwz).
    std::int32_t truncated(std::int32_t offset, double value) {
        c.put64(at(offset), fpu.fctiwz(0, value));
        return c.i32(at(offset + 4));
    }
    // A step of the game's random table (shortlist_manager_random.hpp) and
    // its draw. The assessment reloads the slots each time and keeps the
    // stepped cursor in r4, the cursor variable's address in r3.
    static constexpr StepRegisters step{.next = 4, .slot_in_r3 = true};
    std::uint32_t draw(std::uint32_t rand_back) {
        const RandomTable random{c};
        random.advance(rand_back, step);
        return random.value();
    }
};

// A random number below `range` (shortlist_manager_random.hpp), inlined
// from the module's random(n) at two sites: the rand and random_below
// call sites, the divw, and the stack temporaries.
struct RandomSite {
    std::uint32_t rand_back, random_back, division;  // after the rand and random_below calls; the narrow path's divw
    std::int32_t range_double, value_double, result;  // sp offsets
    bool second;  // the second site schedules its division differently (r4, r5, r7 at the call)
};
std::int32_t random_below(Assessment& a, std::int32_t range, const RandomSite& site, unsigned result_register);

// The phases, in the original's order.
bool opening(Assessment& a);                   // 0x1035a9f0-0x1035ae00: false when the club is skipped
void clear_lists(Assessment& a);               // 0x1035ae00-0x1035ae88
void set_target(Assessment& a);                // 0x1035ae88-0x1035b124
void lay_out_places(Assessment& a);            // 0x1035b124-0x1035b4ac
void collect_open_places(Assessment& a);       // 0x1035b4ac-0x1035b77c
void sort_squad(Assessment& a);                // 0x1035b77c-0x1035bc78
void pick_for_places(Assessment& a);           // 0x1035bc78-0x1035c2d4
void review_candidates(Assessment& a);         // 0x1035c2d4-0x1035ca60
void rate_first_choices(Assessment& a);        // 0x1035ca60-0x1035ce50
void place_choices(Assessment& a);             // 0x1035ce50-0x1035d4c0
void value_open_places(Assessment& a);         // 0x1035d4c0-0x1035de78
void assign_staff(Assessment& a);              // 0x1035de78-0x1035e9d8
}
