#pragma once
// What shortlist_manager_interest*.cpp share: the records evaluate_club_interest
// (0x10369080) reads, its state, and its stages, one function each.
//
// The original keeps almost everything in non-volatile registers (r13-r31,
// f25-f31) across its ~60 calls; every callee's prologue saves them into the
// stack image, so the C++ keeps them there too (Kept<R>, KeptReal<N>),
// assigned where the original assigns them. Several registers change
// meaning on the way; each stage names the register for what it holds there.
//
// Volatile registers: the checker compares r4-r10 at return, and most of the
// callees leave some of r4-r8 as they found them, so the stages leave in
// r4-r8 what the original writes there wherever that value can reach the
// return or a callee (leave()). r0, r3 (the result is f1), r11, r12, CTR,
// XER and f0/f2-f13 are not reproduced. Every compare into cr0 and every
// floating-point operation is made, in the original's order: the condition
// register at return and FPSCR (which callees accumulate into) are the
// original's.
#include "shortlist_manager_detail.hpp"
#include "shortlist_manager_interest.hpp"
#include <bit>
#include <optional>

namespace cm::aot::shortlist_manager::interest {
using namespace detail;

// A floating-point value the original keeps in non-volatile register fN
// (f14-f31) while it calls other code (the FPR counterpart of Kept<R>).
template <unsigned N>
class KeptReal {
    static_assert(N >= 14 && N <= 31, "KeptReal: not a non-volatile register");

public:
    explicit KeptReal(Cpu& c) : c(c) {}  // the register as the caller left it
    KeptReal(const KeptReal&) = delete;
    KeptReal& operator=(double value) {
        c.set_f(N, value);
        return *this;
    }
    KeptReal& operator=(const KeptReal& other) { return *this = double(other); }
    operator double() const { return c.f(N); }

private:
    Cpu& c;
};
// A register the original keeps, as the caller left it until assigned.
template <unsigned R>
struct Held : Kept<R> {
    explicit Held(Cpu& c) : Kept<R>(c, c.r(R)) {}
    using Kept<R>::operator=;
};

// The int-to-double idiom's high word, which the original also leaves in registers.
constexpr std::uint32_t magic = 0x43300000;
constexpr double unsigned_bias = 4503599627370496.0;  // constants +0x2b8: 2^52

// The game records the routine reads are shortlist_manager_records.hpp's.
// The original's inline day count: (to.year - from.year) * 365 + to.day - from.day,
// 32-bit wrapping (years unsigned, days signed halfwords).
inline std::int32_t days_from(const Date& from, const Date& to) {
    return std::int32_t((std::uint32_t(to.year()) - std::uint32_t(from.year())) * 365 +
                        (std::uint32_t(std::int32_t(to.day())) - std::uint32_t(std::int32_t(from.day()))));
}
// The original's inline date compare on the halfword days (a > b), from
// the sign of (x >> 1) - (x & a), x = a ^ b.
inline bool day_after(std::int32_t a, std::int32_t b) {
    const std::int32_t x = a ^ b;
    return (std::uint32_t((x >> 1) - (x & a)) >> 31) != 0;
}
// Contract flags (Contract::flags) the routine tests.
namespace flag {
constexpr std::uint32_t bit_1 = 1, bit_10 = 0x10, bit_20 = 0x20, bit_100 = 0x100, blocking = 0x161a8;
}

// The game's routines the evaluation calls (Program::call).
namespace routine {
constexpr std::uint32_t empty_date = 0x10715880;        // (date)
constexpr std::uint32_t date_plus = 0x10717b20;         // (out, date, days) (inferred)
constexpr std::uint32_t days_between = 0x10718130;      // (date, other date by value)
constexpr std::uint32_t pending_offer = 0x103f43b0;     // (transfers, person, 0) (inferred)
constexpr std::uint32_t primary_contract = 0x10082f20;  // (contracts, person)
constexpr std::uint32_t club_contract = 0x10082f90;     // (contracts, person, club)
constexpr std::uint32_t contract_role = 0x1007c140;     // (contracts, person, contract)
constexpr std::uint32_t contract_relation = 0x1007e820; // (contracts, person, club)
constexpr std::uint32_t nation_tie = 0x100cab10;        // (person, nation, 1)
constexpr std::uint32_t staff_test_3230 = 0x100d3230;   // (person, 0)
constexpr std::uint32_t staff_test_3150 = 0x100d3150;   // (person, 0 or club)
constexpr std::uint32_t staff_test_3040 = 0x100d3040;   // (person, 0)
constexpr std::uint32_t club_tie_a050 = 0x100da050;     // (person, club)
constexpr std::uint32_t club_tie_a0b0 = 0x100da0b0;     // (person, club)
constexpr std::uint32_t person_tie_9520 = 0x100d9520;   // (manager, person)
constexpr std::uint32_t person_tie_9580 = 0x100d9580;   // (manager, person)
constexpr std::uint32_t club_rivalry = 0x10464160;      // (club, club) (inferred)
constexpr std::uint32_t human_controlled = 0x1014c2e0;  // (club, r4, r5, 0, 0): club_control.hpp
constexpr std::uint32_t league_row = 0x10459b50;        // (league table, club): +4, +0x75 position (inferred)
constexpr std::uint32_t staff_rating = 0x100cdef0;      // (manager, person, 1)
constexpr std::uint32_t club_finances = 0x10103510;     // (finances, club) (inferred)
constexpr std::uint32_t finance_mood = 0x100f55b0;      // (club finances in r3) (inferred)
constexpr std::uint32_t regional_familiarity = 0x103092f0;  // (player stats, person, club, zone, 1)
constexpr std::uint32_t sqrt_glue = 0x10780fd8;         // sqrt(f1), a Toolbox glue
constexpr std::uint32_t contract_dates = 0x101929b0;    // (club, date, date)
constexpr std::uint32_t offer_object = 0x103721f0;      // (object): constructs it
constexpr std::uint32_t asking_price = 0x1037b6f0;      // (object, person, club, 0, role, 11, 0): f1 (inferred)
constexpr std::uint32_t transfer_budget = 0x100f2680;   // (finances, person, role) (inferred)
}
// TOC slots this routine reads besides detail::slot.
namespace toc {
constexpr std::int32_t person_flags = -0x7b9c;     // 100-byte rows per person id, +0xc bits (inferred)
constexpr std::int32_t contract_scratch = -0x7b78; // where the primary contract is published
constexpr std::int32_t transfers = -0x7b40;        // 0x103f43b0's object (inferred)
constexpr std::int32_t rival_club_a = -0x7a04, rival_club_b = -0x7a08;  // two club ids (inferred)
constexpr std::int32_t region_a = -0x74ec, region_b = -0x7590, region_c = -0x7a14, region_d = -0x7af4;
constexpr std::int32_t federation_id = -0x7a70;
constexpr std::int32_t division_a = -0x7488, division_b = -0x7a0c;  // division ids (inferred)
constexpr std::int32_t leagues = -0x7bbc;          // 0x6c-byte league records (inferred)
constexpr std::int32_t club_leagues = -0x7984;     // per club id, 8 bytes, first word a league index (inferred)
constexpr std::int32_t top_league = -0x7a30;       // a league id valued at 15000 (inferred)
constexpr std::int32_t league_special = -0x7a2c;   // a league index (inferred)
constexpr std::int32_t club_tables = -0x79f0;      // 0x4c bytes per club id: +6, +8, +0xa (inferred)
constexpr std::int32_t finances = -0x7ab8;         // 0x10103510's object (inferred)
constexpr std::int32_t cases_own_club = -0x4e10, cases_club_role = -0x4e0c, cases_current_role = -0x4e08;
}

// fcmpo cr0 as the branches read it: ble is !gt and bge is !lt, so an
// unordered pair takes both.
struct Order {
    bool lt, gt, eq;
};

// The evaluation's state and the original's idioms.
struct Evaluation {
    Cpu& c;
    Fpu fpu;
    const Frame& frame;
    Program program;

    // The registers the original keeps (as the caller left them until it
    // assigns them). Names give the main meaning; stages say when a register
    // holds something else.
    Held<13> r13;              // the person-flags slot, the clubs' league values, the club finances
    Held<14> candidate_league; // the club's league record (0: none)
    Held<15> r15;              // a rival slot, the club's role (extsb r20), the current club's value
    Held<16> constants;        // slot::constants
    Held<17> person_at;
    Held<18> club_at;          // the club considering the person
    Held<19> loan_word;        // the caller's r7, tested as a byte (inferred: a loan)
    Held<20> role_word;        // the caller's r8: the role the club has in mind (inferred)
    Held<21> reasons_at;       // where the reasons go (0: not wanted)
    Held<22> age_reg;          // extsb of the person's +0x5c halfword
    Held<23> r23;              // the contract-scratch slot, then the contract's flags
    Held<24> current_at;       // the person's club (0: none)
    Held<25> r25;              // a region slot, then the contract's happiness (or 0, -100)
    Held<26> role;             // the person's role at his club (inferred)
    Held<27> contract_at;      // his contract with his club (0: none)
    Held<28> current_league;   // the current club's league record (0: none)
    Held<29> r29;              // the club's reputation value, then the current nation's tier
    Held<30> candidate_tier;   // the club nation's tier, squared
    Held<31> r31;              // the club's values
    KeptReal<25> f25;
    KeptReal<26> f26;          // the weight sum, then the transfer budget
    KeptReal<27> fit;          // 1.0, then the career factor, then the asking price
    KeptReal<28> weight_squad; // single precision weights of the club value, then other factors
    KeptReal<29> weight_wage;
    KeptReal<30> weight_league;
    KeptReal<31> score;

    Evaluation(Cpu& c, const Frame& frame)
        : c(c), fpu(c), frame(frame), program(c), r13(c), candidate_league(c), r15(c), constants(c), person_at(c),
          club_at(c), loan_word(c), role_word(c), reasons_at(c), age_reg(c), r23(c), current_at(c), r25(c), role(c),
          contract_at(c), current_league(c), r29(c), candidate_tier(c), r31(c), f25(c), f26(c), fit(c),
          weight_squad(c), weight_wage(c), weight_league(c), score(c) {}

    Person person() const { return {c, person_at}; }
    Club club() const { return {c, club_at}; }
    Club current() const { return {c, current_at}; }
    Contract contract() const { return {c, contract_at}; }
    std::int32_t age() const { return std::int32_t(std::uint32_t(age_reg)); }
    std::uint32_t flags() const { return r23; }  // the contract's flags (0: none)
    std::uint32_t manager() const { return c.u32(frame.at(0x1e8)); }  // the caller's r3, kept in its frame
    std::int8_t club_role() const { return std::int8_t(std::uint32_t(role_word)); }
    std::int8_t current_role() const { return std::int8_t(std::uint32_t(role)); }

    // extsb. r0,r19 (inferred: the request is a loan).
    bool loan() { return nonzero(c, std::int8_t(std::uint32_t(loan_word))); }
    // `cmplwi r21,0`: whether the caller wants the reasons.
    bool wanted() { return nonzero_logical(c, reasons_at); }
    void reason(std::uint32_t bit) { c.put32(reasons_at, c.u32(reasons_at) | bit); }
    // rlwinm. r0,r23,0,b,b: a flag of the contract.
    bool flag(std::uint32_t bit) { return nonzero(c, std::int32_t(flags() & bit)); }
    // `cmplwi r27,0`.
    bool has_contract() { return nonzero_logical(c, contract_at); }
    // `cmplwi r24,0` and `cmplw r18,r24`.
    bool has_club() { return nonzero_logical(c, current_at); }
    bool own_club() { return compare_logical(c, club_at, current_at) == 0; }

    // f31 *= factor, and with the reason when the caller wants them
    // (`cmplwi r21,0; fmul; beq; ori`).
    void scale(double factor) { score = fpu.fmul(score, factor); }
    void scale(double factor, std::uint32_t bit) {
        const bool w = wanted();
        scale(factor);
        if (w) reason(bit);
    }
    void divide(double divisor) { score = fpu.fdiv(score, divisor); }

    // fcmpo cr0.
    Order order(double a, double b) {
        fpu.fcmp(0, a, b);
        return {a < b, b < a, a == b};
    }
    // cror eq,lt,eq and cror eq,gt,eq after an fcmpo.
    void fold_less_into_equal() { c.set_cr(0, std::uint8_t(c.cr(0) | ((c.cr(0) & 8) >> 2))); }
    void fold_greater_into_equal() { c.set_cr(0, std::uint8_t(c.cr(0) | ((c.cr(0) & 4) >> 1))); }

    // The int-to-double idiom: the words magic and the value in the stack
    // temporary at sp+offset (signed: xor 0x80000000), read back as a double
    // (raw); real() also subtracts the bias (fsub, or fsubs for the single
    // forms). truncated() is fctiwz into the temporary at sp+offset.
    double raw(std::int32_t offset, std::uint32_t low) {
        const auto at = frame.at(offset);
        c.put32(at, magic);
        c.put32(at + 4, low);
        return std::bit_cast<double>(std::uint64_t(magic) << 32 | low);
    }
    double raw_signed(std::int32_t offset, std::int32_t value) { return raw(offset, signed_word(value)); }
    double real(std::int32_t offset, std::int32_t value) { return fpu.fsub(raw_signed(offset, value), k::signed_bias); }
    double real_single(std::int32_t offset, std::int32_t value) {
        return fpu.fsubs(raw_signed(offset, value), k::signed_bias);
    }
    double unsigned_real(std::int32_t offset, std::uint32_t value) {
        return fpu.fsub(raw(offset, value), unsigned_bias);
    }
    std::int32_t truncated(std::int32_t offset, double value, unsigned fpr = 0) {
        const auto bits = fpu.fctiwz(fpr, value);
        c.put64(frame.at(offset), bits);
        return std::int32_t(std::uint32_t(bits));
    }
    std::int16_t halfword(std::int32_t offset) const { return c.i16(frame.at(offset)); }
    void put_halfword(std::int32_t offset, std::int32_t value) { c.put16(frame.at(offset), std::uint32_t(value)); }
    double multiply_add(std::uint32_t site, double a, double b, double addend) {
        return fpu.multiply_add(site, a, b, addend);
    }

    // Calls whose arguments are the person and 0 or a club (`or r3,r17,r17`),
    // the result tested as a byte (extsb.).
    bool test(std::uint32_t entry, std::uint32_t back, std::uint32_t second = 0) {
        return nonzero(c, std::int8_t(program.call(entry, back, {person_at, second})));
    }
    std::uint32_t contracts() const { return c.u32(c.toc(slot::contracts)); }
    std::uint32_t today() const { return c.toc(slot::today); }

    // Stages, in the original's order (each returns the original's early
    // result when it returns there).
    std::optional<double> opening();                // 0x10369080-0x103695f8
    void loan_reputation();                         // 0x103695f8-0x10369760
    void relations();                               // 0x10369760-0x10369b80
    void managers();                                // 0x10369b80-0x10369c5c
    void current_status();                          // 0x10369c5c-0x10369f1c
    void move_roles();                              // 0x10369f24-0x1036a37c (another club)
    std::optional<double> candidate_value();        // 0x1036a37c-0x1036ad00
    void location();                                // 0x1036ad00-0x1036b3e0
    std::optional<double> later_career();           // 0x1036b3e0-0x1036b710
    void staying_role();                            // 0x1036b710-0x1036b92c (his own club)
    void current_value();                           // 0x1036b92c-0x1036c408
    void value_comparison();                        // 0x1036c408-0x1036cde8
    std::optional<double> recent_contract();        // 0x1036cde8-0x1036cf48
    double finances();                              // 0x1036cf48-0x1036d218

    // date_plus(today, 150) into sp+out, copied to sp+copy, compared with
    // the manager's +0x4c date: whether the copy is later (inline operator>).
    bool manager_date_near(std::uint32_t manager, std::int32_t out, std::int32_t copy, std::uint32_t back);

    // The distance factor between the two grounds' places (0x1036ae54, 0x1036af6c).
    void distance_factor(std::uint32_t club_place, std::uint32_t current_place, std::uint32_t site, std::uint32_t back);
    // base + determination / first + adaptability / second (temporaries at sp+0xe4, 0xec).
    double personality_factor(double base, double first, double second);

    // Shared pieces of the club values (shortlist_manager_interest_values.cpp),
    // each with the stack temporaries at sp+base on (the club's at 0x10c,
    // his club's at 0xe4, in the same layout).
    // The nation's tier, 1-3 (else 3).
    std::int32_t clamped_tier(std::uint32_t nation);
    // The ground's support of the reputation, stored at sp+at (halfword).
    void ground_value(const Club& of, std::int32_t at, std::int32_t base);
    // The division's support, stored at sp+at.
    void division_value(const Club& of, std::int32_t at);
    // The club's league record (0: none; the register keeps its value then).
    template <unsigned R> void find_league(const Club& of, Kept<R>& record);
    // The league's value: 15000 for the top league, else its level x600,
    // scaled by the club's position (1.32 - position / 50); without a league
    // 0.8 x the division value, at most 7000.
    // `store` keeps each value where the original does (r13, or sp+0x12c).
    template <class Store>
    void league_value(const Club& of, std::uint32_t record, std::int32_t division_at, std::int32_t base,
                      std::uint32_t back, Store store);
    // The staff rating blended with the club table's three values.
    std::int32_t blended_rating(std::int32_t rating, std::uint32_t of, std::uint32_t tables, std::int32_t base,
                                const std::uint32_t (&sites)[6]);
    // The reputation raised by the club's finance mood: max(factor x rep, rep + add).
    std::int32_t raised_reputation(std::int32_t reputation, double factor, std::int32_t add, std::int32_t base);
    // The best of the reputation and the manager object's entry for the club.
    std::int32_t best_reputation(std::int32_t reputation, std::int32_t club_id);
    // min(v / 2, v - 2000) on the halfword.
    std::int32_t halved(std::int32_t value);
    // 0x103092f0's familiarity (fsub'ed), after f25 = (value + 5000)^2 / 15000.
    double familiarity(std::uint32_t of, std::int32_t value, std::uint32_t back);
    // max(familiarity / 10, f25 / (12000 - 200 familiarity)).
    double familiarity_share(double familiarity, std::uint32_t site);
};
}
