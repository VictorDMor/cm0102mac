#pragma once
// What shortlist_manager_context*.cpp share: the records the candidate context
// evaluation (0x10366470, evaluate_candidate_context) reads, the registers it
// keeps its values in, and the evaluation itself as one object whose stages
// follow the original's order (one stage per part of the routine; each file
// holds a few).
//
// The original keeps almost every value it computes in a non-volatile
// register (r13-r31, f24-f31) and calls ~20 routines of the rest of the game
// in between, whose prologues save those registers into the stack image; so
// each value lives in a Held<R> / HeldFloat<F> named for its role, written to
// its register when the original writes it. Several roles share a register
// in turn (r23: a nation id pointer, then the club's prestige, then the
// reference nation's level), as the original reuses it.
//
// What the caller can observe besides f1: r4-r10 and cr0 at return (a later
// varargs call stores r3-r10; r3 itself is overwritten by every caller),
// FPSCR (every FP operation goes through Fpu in the original's order), and
// the stack temporaries of the int-to-double idiom and of fctiwz, which stay
// in the frame. Each stage leaves r4-r10 as the original does (leave()).
#include "shortlist_manager_detail.hpp"
#include <optional>

namespace cm::aot::shortlist_manager::context {
using namespace detail;

// A value the original keeps in non-volatile register R while it calls other
// code (the callees' prologues save R into the stack image). Assigning writes
// R; reading returns the value (what R holds after each call, whose epilogue
// reloads it). Unlike Kept, it does not write R when it is declared, because
// the original assigns most of these midway (R keeps the caller's value until
// then) and several roles share one register.
template <unsigned R>
class Held {
    static_assert(R >= 13 && R <= 31, "Held: not a non-volatile register");

public:
    explicit Held(Cpu& c) : c(&c) {}
    Held(const Held&) = delete;
    Held& operator=(std::uint32_t v) {
        value = v;
        c->set(R, v);
        return *this;
    }
    Held& operator=(const Held& other) { return *this = std::uint32_t(other); }
    operator std::uint32_t() const { return value; }

private:
    Cpu* c;
    std::uint32_t value = 0;
};
// The same for a floating-point register (f14-f31).
template <unsigned F>
class HeldFloat {
    static_assert(F >= 14 && F <= 31, "HeldFloat: not a non-volatile register");

public:
    explicit HeldFloat(Cpu& c) : c(&c) {}
    HeldFloat(const HeldFloat&) = delete;
    HeldFloat& operator=(double v) {
        value = v;
        c->set_f(F, v);
        return *this;
    }
    HeldFloat& operator=(const HeldFloat& other) { return *this = double(other); }
    operator double() const { return value; }

private:
    Cpu* c;
    double value = 0;
};

// A value as the original holds it in a register (sign extensions:
// detail::byte_of, half_of; mulhw: detail::multiply_high).
inline std::uint32_t word(std::int32_t v) { return std::uint32_t(v); }
// The compiler's signed division by a constant (mulhw by the magic word,
// add, shift, add the sign): exact truncating division, kept as written
// because its intermediate words are left in registers.
inline std::int32_t sign_bit(std::int32_t v) { return std::int32_t(std::uint32_t(v) >> 31); }

// 1 when day a is after day b of the same year (both lha): the original's
// branchless sign of ((a ^ b) >> 1) - ((a ^ b) & a).
inline std::int32_t after_day(std::int32_t a, std::int32_t b) {
    const std::int32_t mixed = a ^ b;
    return std::int32_t((word(mixed >> 1) - word(mixed & a)) >> 31);
}

// The game's records this routine reads are shortlist_manager_records.hpp's.

// The game's TOC slots this group reads besides detail::slot's.
namespace slot {
using namespace detail::slot;
constexpr std::int32_t rival_nation_a = -0x74ec, rival_nation_b = -0x7590;  // int32* nation ids (0x10366e1c)
constexpr std::int32_t rival_club_a = -0x7a04, rival_club_b = -0x7a08;      // int32* club ids (0x10366c3c)
constexpr std::int32_t neighbour_nation = -0x7a14;  // int32* a nation id whose clubs weigh loyalty over 60
constexpr std::int32_t offers = -0x7b40;           // 0x103f43b0's object
constexpr std::int32_t club_stats = -0x79f0;       // object whose first word is 0x4c-byte records by club id
constexpr std::int32_t stadiums = -0x7bbc;         // 0x6c-byte records (pointer)
constexpr std::int32_t club_stadiums = -0x7984;    // per club id, 8 bytes, the first an index into stadiums
constexpr std::int32_t finances = -0x7ab8;         // 0x10103510's object (inferred: club finances)
constexpr std::int32_t shared_stadium = -0x7a2c;   // int32* a stadium index the reference club ignores under 5000
constexpr std::int32_t minor_division = -0x7a0c;   // int32* a division id
constexpr std::int32_t home_continent = -0x7a70;   // int32* a continent id
}

// The calls into the rest of the game (Program::call), named by use.
namespace call {
constexpr std::uint32_t name_is = 0x1036ff00;              // (person, text): person is that named person (policy group)
constexpr std::uint32_t is_foreigner_in = 0x100cab10;     // (person, nation, 1) (inferred: needs a permit there)
constexpr std::uint32_t is_national_team = 0x100c7d00;    // (club) (inferred)
constexpr std::uint32_t contract_with = 0x10082f90;       // (contracts, person, club)
constexpr std::uint32_t dislikes_club = 0x100da050;       // (person, club) (inferred: x0.33 to join, x2 to leave)
constexpr std::uint32_t likes_club = 0x100da0b0;          // (person, club) (inferred: x3 to join, x0.5 to leave)
constexpr std::uint32_t clubs_related = 0x10464160;       // (club, club)
constexpr std::uint32_t absolute = 0x10775540;            // abs()
constexpr std::uint32_t staff_favours = 0x100d9520;       // (club staff, person) (inferred)
constexpr std::uint32_t staff_dislikes = 0x100d9580;      // (club staff, person) (inferred)
constexpr std::uint32_t special_offer = 0x103f43b0;       // (offers, person, 0)
constexpr std::uint32_t staff_rating = 0x100cdef0;        // (club staff, person, 1)
constexpr std::uint32_t finance_record = 0x10103510;      // (finances, club)
constexpr std::uint32_t finance_state = 0x100f55b0;       // (record): -2..2
constexpr std::uint32_t is_human_managed = 0x100ce250;    // (club) (inferred)
constexpr std::uint32_t affinity = 0x103092f0;            // (player stats, person, club, group, 1): 0-10
constexpr std::uint32_t initialize_draft = 0x103721f0;    // (draft)
constexpr std::uint32_t reference_quote = 0x1037b6f0;     // (draft, person, club, 0, -1, category, 0): f1
constexpr std::uint32_t budget_for = 0x100f4790;          // (finance record, category): int
}

// The reason bits the evaluation sets in the caller's word (r8) for the
// interface; their texts are not identified, so they are named by where the
// original sets them.
namespace reason {
constexpr std::uint32_t hostile = 0x2, dislikes_reference = 0x4, likes_reference = 0x8, likes_club = 0x10,
                        local_rival = 0x20, staff_favours = 0x40, staff_dislikes = 0x80,
                        reference_staff_dislikes = 0x100, reference_staff_favours = 0x200, contract_low = 0x400,
                        contract_high = 0x800, affinity = 0x10000, step_up = 0x100000, step_down_big = 0x200000,
                        step_up_big = 0x400000, worse = 0x800000, continent = 0x2000000, national_job = 0x4000000,
                        job = 0x8000000, offer = 0x10000000;
}

// The categories (r7) and jobs (person +0x62, +0x60) the original treats
// apart: 5 and 12 go together everywhere (inferred: manager and assistant).
namespace role {
constexpr std::int32_t manager = 5, assistant = 12;
}

// Stack temporaries of one club's valuation: the considered club's start at
// sp+0xbc, the reference club's at sp+0xac, each step using them in the same
// pattern (temporary n at base + 8n).
struct Temporaries {
    std::uint32_t base;
    std::uint32_t at(std::uint32_t n) const { return base + n * 8; }
};
// The original instructions of one club's valuation (call return addresses
// and multiply-add sites), which differ between the two copies.
struct ValuationSites {
    std::uint32_t staff_blend[5];   // the staff value's multiply-adds
    std::uint32_t finance[3];       // returns of 0x100c7d00, 0x10103510, 0x100f55b0
    double finance_factors[4];      // finance state 2, 1, -1, -2
    std::int32_t finance_floors[2]; // states 2 and 1
    std::uint32_t budget[2];        // returns of 0x100c7d00, 0x100ce250
    double national_factor;
    std::uint32_t budget_blend;
    std::uint32_t weighed[4];
    std::uint32_t status;           // return of 0x100c7d00 before the status rule
    std::uint32_t outlook;          // 1.5 x value - 5000
    std::uint32_t affinity, affinity_blend;
};
// The five components of a club's value and where their temporaries are.
struct Components {
    std::uint32_t prestige, staff, division, capacity, budget, result;
};

class Evaluation {
public:
    Evaluation(Cpu& c, std::uint32_t object, std::uint32_t person, std::uint32_t club, std::uint32_t enabling,
               std::uint32_t category, std::uint32_t reasons);
    double evaluate();

private:
    // The stages, in the original's order. shortlist_manager_context.cpp:
    std::optional<double> check_candidate();      // 0x103664d4: who is never considered
    void weigh_factors();                         // 0x103667b8: the valuation's weights
    std::optional<double> find_reference_club();  // 0x103668ac: the club he would leave
    std::optional<double> base_score();           // 0x10366908: contract, base, time out of work
    void relation_factors();                      // 0x10366afc: likes and dislikes
    void neighbour_factors();                     // 0x10366bf4: rivals and related clubs
    void staff_factors();                         // 0x10366ed8: the clubs' staff's views of him
    std::optional<double> offer_check();          // 0x10366fd0
    void contract_factor();                       // 0x10367014
    // shortlist_manager_context_roles.cpp:
    std::optional<double> role_factors();         // 0x103670fc: his job there against the category
    void results_factor();                        // 0x10367434
    void foreign_factor();                        // 0x10367dc8
    std::optional<double> service_factor();       // 0x10367f00: time served at his club
    // shortlist_manager_context_valuation.cpp: the considered club's value
    // (0x103674e4-0x10367dc8), the reference club's (0x10368070-0x10368960)
    // or his own out of work (0x10367fe0).
    void value_considered_club();
    void value_reference_club();
    void value_free_agent();
    std::uint32_t level_squared(std::uint32_t nation);
    std::uint32_t prestige(Club of, Temporaries t);
    std::uint32_t division_reputation(Club of);
    std::uint32_t capacity_of(std::uint32_t stadium, std::uint32_t division_value, Temporaries t);
    std::uint32_t blended_staff(Club of, std::uint32_t stats, std::int32_t rating, Temporaries t, const ValuationSites& s);
    template <unsigned R>
    void finance_reputation(Held<R>& reputation, Club of, Temporaries t, const ValuationSites& s);
    std::uint32_t budget_reputation(Club of, std::uint32_t reputation, Temporaries t, const ValuationSites& s);
    std::uint32_t weighted(Components values, Components at, const std::uint32_t (&sites)[4]);
    std::uint32_t status_adjusted(Club of, std::uint32_t value_word, std::uint32_t back);
    template <unsigned R>
    bool outlook(Held<R>& value_word, Club of, std::uint32_t site);
    double attraction(Club of, std::uint32_t value_word, const ValuationSites& s);
    // shortlist_manager_context_verdict.cpp:
    void compare_clubs();                         // 0x10368968
    double refine_by_quote();                     // 0x10368ee8

    // The original's idioms over the frame's temporaries.
    double stacked(std::uint32_t at, std::int32_t v) { return stacked_integer(c, frame.at(at), signed_word(v)); }
    double integer(std::uint32_t at, std::int32_t v) { return fpu.fsub(stacked(at, v), k::signed_bias); }
    // fctiwz, stfd at sp+at, lwz of the low word.
    std::uint32_t truncated(std::uint32_t at, double v) {
        const auto bits = fpu.fctiwz(0, v);
        c.put64(frame.at(at), bits);
        return std::uint32_t(bits);
    }
    // fcmpo into cr0; the branches test its bits (unordered sets none).
    struct Order {
        bool less, greater, equal;
    };
    Order fcompare(double a, double b) {
        fpu.fcmp(0, a, b);
        return {a < b, b < a, a == b};
    }
    // `cror eq,lt,eq` (with_less) or `cror eq,gt,eq` after an fcmpo.
    void merge_equal(bool with_less) {
        const auto field = c.cr(0);
        c.set_cr(0, std::uint8_t(field | (((field >> (with_less ? 3 : 2)) & 1) << 1)));
    }
    // `cmplwi r18,0`, and an `ori` into the caller's reason word.
    bool has_reasons() { return nonzero_logical(c, reasons); }
    void add_reason(std::uint32_t bit) { c.put32(reasons, c.u32(reasons) | bit); }
    void note(std::uint32_t bit) {
        if (has_reasons()) add_reason(bit);
    }
    std::int32_t category8() const { return std::int8_t(category); }
    std::int32_t job8() const { return std::int8_t(reference_job); }
    // value == a || value == b as `cmpwi a; beq; cmpwi b`.
    bool either(std::int32_t v, std::int32_t a, std::int32_t b) { return compare(c, v, a) == 0 || compare(c, v, b) == 0; }
    bool manager_category() { return either(category8(), role::manager, role::assistant); }
    bool manager_job() { return either(job8(), role::manager, role::assistant); }

    Cpu& c;
    Frame frame;
    Fpu fpu;
    Program program;
    std::uint32_t constants, singles, pool;
    Person person;
    Club club;

    // The registers' roles, in the order the original first assigns them.
    Held<30> constants_register{c};
    Held<18> reasons{c};              // the caller's reason word, 0: none
    Held<23> rival_nation_b{c};       // int32* (TOC)
    Held<15> person_register{c};
    Held<16> club_register{c};
    Held<25> rival_nation_a{c};       // int32* (TOC)
    Held<17> category{c};             // the requested category (low byte, signed)
    Held<24> nation_level{c};         // 0, then the club nation's level squared
    Held<14> start_date{c};           // Date* (TOC)
    Held<22> stadium_record{c};       // 0, then the club's stadium record
    Held<13> today{c};                // Date* (TOC)
    Held<21> reference_stadium{c};    // 0, then the reference club's stadium record
    Held<19> reference_job{c};        // the pool, then his job at the reference club, or the category
    Held<20> reference{c};            // the club he would leave (his, or his national team's), 0: none
    Held<31> contract{c};             // his contract with it, 0: none
    Held<14> category_index{c};       // extsb of the category, for the jump tables
    Held<23> club_prestige{c};
    Held<29> club_division{c};        // the club's reputation weighed by its division's
    Held<27> club_capacity{c};
    Held<26> club_value{c};           // the staff's rating of him, then the club's weighed value
    Held<25> club_finance{c};         // its reputation weighed by its finances
    Held<23> reference_level{c};      // the reference nation's level squared (his own nation's out of work)
    Held<14> reference_prestige{c};
    Held<28> reference_division{c};
    Held<13> reference_capacity{c};
    Held<25> reference_staff{c};
    Held<27> reference_finance{c};
    Held<13> other_value{c};          // the reference club's weighed value, or his own out of work
    Held<13> quote_record{c};         // the club's finance record (0x10103510)
    HeldFloat<29> service{c};         // 1.0, then time served at his club / 400
    HeldFloat<28> staff_weight{c};
    HeldFloat<27> prestige_weight{c};
    HeldFloat<26> division_weight{c};
    HeldFloat<30> capacity_weight{c};
    HeldFloat<25> weight_sum{c};
    HeldFloat<31> score{c};
    HeldFloat<24> pull{c};            // the attraction's numerator
    HeldFloat<24> quote_factor{c};
    HeldFloat<25> quote{c};
    std::uint32_t last_level_square = 0;  // level_squared's mullw (r8 for the free agent)
};
}
