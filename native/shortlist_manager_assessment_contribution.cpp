#include "shortlist_manager_detail.hpp"
#include "shortlist_manager_random.hpp"
#include "shortlist_manager_assessment.hpp"
#include <algorithm>
#include <bit>
#include <optional>

// SHORTLIST_MANAGER.CPP 0x10361490: what a shortlisted candidate would be
// worth to a club. It fills in the candidate's valuation (the fee the club's
// judge estimates, through 0x10216830) and returns the contribution: the
// worth scaled by the candidate's position, age, reputation and the club's
// circumstances, or 0 when the valuation is out of reach. Called about half a
// million times a game day.
//
// Floating point goes through guest::Fpu in the original's order (FPSCR is
// observable). Integers reach doubles through the stack temporaries the
// original uses (stacked_integer); those bytes stay in the stack image.
namespace cm::aot::shortlist_manager {
using namespace detail;
namespace {
constexpr std::uint32_t entry_point = 0x10361490;

// The double pool's values this routine loads (slot::constants +offset).
constexpr double unsigned_bias = 4503599627370496.0;  // +0x2b8: 2^52, the unsigned int-to-double idiom's
static_assert(std::bit_cast<std::uint64_t>(unsigned_bias) == 0x4330000000000000);
static_assert(std::bit_cast<std::uint64_t>(0.32) == 0x3fd47ae147ae147b && std::bit_cast<std::uint64_t>(2.5e-05) == 0x3efa36e2eb1c432d &&
              std::bit_cast<std::uint64_t>(5e-05) == 0x3f0a36e2eb1c432d && std::bit_cast<std::uint64_t>(0.575) == 0x3fe2666666666666 &&
              std::bit_cast<std::uint64_t>(0.00025) == 0x3f30624dd2f1a9fc && std::bit_cast<std::uint64_t>(1666.67) == 0x409a0aae147ae148 &&
              std::bit_cast<std::uint64_t>(0.025) == 0x3f9999999999999a && std::bit_cast<std::uint64_t>(1.05) == 0x3ff0cccccccccccd &&
              std::bit_cast<std::uint64_t>(0.15) == 0x3fc3333333333333 && std::bit_cast<std::uint64_t>(0.9) == 0x3feccccccccccccd &&
              std::bit_cast<std::uint64_t>(0.65) == 0x3fe4cccccccccccd && std::bit_cast<std::uint64_t>(0.6) == 0x3fe3333333333333 &&
              std::bit_cast<std::uint64_t>(0.3) == 0x3fd3333333333333 && std::bit_cast<std::uint64_t>(0.7) == 0x3fe6666666666666 &&
              std::bit_cast<std::uint64_t>(1.2) == 0x3ff3333333333333 && std::bit_cast<std::uint64_t>(1.1) == 0x3ff199999999999a &&
              std::bit_cast<std::uint64_t>(0.8) == 0x3fe999999999999a && std::bit_cast<std::uint64_t>(0.2) == 0x3fc999999999999a &&
              std::bit_cast<std::uint64_t>(0.1) == 0x3fb999999999999a,
              "the pool's doubles (.local/shortlist/constants.txt)");

// TOC slots only this routine reads.
namespace own {
constexpr std::int32_t person_details = -0x7b9c;  // the address of a pointer to 100-byte records per person id (inferred)
constexpr std::int32_t staff_service = -0x7b1c;   // the object 0x1030fbf0 takes
constexpr std::int32_t staff_records = -0x79f8;   // the address of the first 0x44-byte staff record
constexpr std::int32_t staff_record_count = -0x79f4;
constexpr std::int32_t favoured_nation = -0x79fc;  // the address of a nation id (inferred)
constexpr std::int32_t frugal_nation = -0x7a00;    // the address of a nation id (inferred)
constexpr std::int32_t to_unsigned_bounds = -0x3b7c;  // __dtou's three doubles
}

// A non-volatile floating register the original keeps a value in while it
// calls other code (as Kept for the integer registers): callees save it in
// their frames and the pow import compares it.
template <unsigned F>
class KeptF {
public:
    explicit KeptF(Cpu& c) : c(c) {}
    KeptF& operator=(double value) {
        c.set_f(F, value);
        return *this;
    }
    operator double() const { return c.f(F); }

private:
    Cpu& c;
};

constexpr std::uint32_t club_staff = 0x174, club_staff_count = Club::staff_slots;  // Club::staff's words

// The game's routines this one calls (each sets that call's arguments).
namespace game {
constexpr std::uint32_t human_controlled = 0x1014c2e0;  // (club, 1, 1, 0, 0): club_control.hpp
constexpr std::uint32_t familiarity = 0x100ce0a0;       // (candidate, person): 0-4 (inferred)
constexpr std::uint32_t loan_check = 0x100d3230;        // (candidate, 0) (inferred)
constexpr std::uint32_t staff_check = 0x100d3150;       // (candidate, 0) (inferred)
constexpr std::uint32_t club_nation = 0x100c96e0;       // (club) (inferred)
constexpr std::uint32_t staff_predicate = 0x1030fbf0;   // (*staff_service, candidate, nation)
constexpr std::uint32_t club_judge = 0x1014c5f0;        // (club): the person whose judgement the estimate takes (inferred)
constexpr std::uint32_t estimate = 0x10216830;          // (candidate, position, qualifications, club, offer, judge): the valuation
constexpr std::uint32_t judging = 0x100d2f50;           // (staff record, attribute): a signed byte
constexpr std::uint32_t potential = 0x10219e40;         // (manager, candidate, club, kind, valuation / 4): int16 (inferred)
constexpr std::uint32_t knows_nation = 0x100cab10;      // (candidate, nation, 1) (inferred)
constexpr std::uint32_t pow_glue = 0x10780fc0;
}

class Contribution {
public:
    Contribution(Cpu& c, const Frame& frame, std::uint32_t shortlists, std::uint32_t club, std::uint32_t candidate,
                 std::uint32_t entry, std::uint32_t valuation, std::uint32_t budget, std::uint32_t capped)
        : c(c), fpu(c), sp(frame.base()), shortlists_at(c, shortlists), club_at(c, club), candidate_at(c, candidate),
          entry_at(c, entry), valuation_at(c, valuation), budget(c, budget), capped(c, capped),
          constants(c, c.toc(slot::constants)), judge(c, Club{c, club}.manager()),
          cap(c, std::uint32_t(std::max<std::int32_t>(Row{c, entry}.budget(), 1))), dear(c, 0),
          decline(c), decline_floor(c), ceiling(c), factor(c) {
        // r13, r15 and r16 hold these until the original reuses them.
        keep(c, {{13, c.toc(own::person_details)}, {15, c.toc(slot::random_cursor)}, {16, c.toc(slot::random_salt)},
                 {22, 4}});
    }
    std::uint32_t run();

private:
    Club club() const { return {c, club_at}; }
    Person candidate() const { return {c, candidate_at}; }
    Row entry() const { return {c, entry_at}; }
    Field<std::int32_t> valuation() const { return {c, valuation_at}; }

    // The int-to-double idioms through the stack temporary at sp+offset.
    double integer(std::int32_t offset, std::int32_t value) {
        return fpu.fsub(stacked_integer(c, sp + std::uint32_t(offset), signed_word(value)), k::signed_bias);
    }
    double natural(std::int32_t offset, std::uint32_t value) {
        return fpu.fsub(stacked_integer(c, sp + std::uint32_t(offset), value), unsigned_bias);
    }
    void stack(std::int32_t offset, std::int32_t value) { stacked_integer(c, sp + std::uint32_t(offset), signed_word(value)); }
    // fctiwz into f`to`, stored (stfd) at sp+offset; the low word.
    std::int32_t truncated(std::int32_t offset, double value, unsigned to = 0) {
        const auto bits = fpu.fctiwz(to, value);
        c.put64(sp + std::uint32_t(offset), bits);
        return std::int32_t(std::uint32_t(bits));
    }
    double fused(std::uint32_t site, double a, double c_, double b) { return fpu.multiply_add(site, a, c_, b); }
    std::uint32_t to_unsigned(double value);
    double power(double base, std::uint32_t back) {
        c.set_f(1, base);
        c.set_f(2, 2.0);
        Program{c}.toolbox(game::pow_glue, back);
        return c.f(1);
    }
    std::uint32_t call(std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        return Program{c}.call(entry, back, arguments);
    }
    // A routine's int8 result tested as the original does (extsb.).
    bool yes(std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        return nonzero(c, std::int8_t(call(entry, back, arguments)));
    }
    std::int32_t familiarity(std::uint32_t person, std::uint32_t back) {
        return std::int8_t(call(game::familiarity, back, {candidate_at, person}));
    }

    void draw_manager_staff_record();
    std::int32_t draw_below(std::int32_t count);
    std::uint32_t qualifications();
    void estimate_valuation(std::uint32_t qualifications);
    void choose_judge(std::uint32_t qualifications);
    std::uint32_t budget_offer();
    bool out_of_reach();
    void apply_premium();
    std::uint32_t base_worth(std::uint32_t& age_limit, std::int32_t& fee);
    void young_prospect(Kept<15>& worth, std::int32_t age, bool second_copy);
    void decline_with_age(Kept<15>& worth, std::int32_t age, std::uint32_t age_limit);
    std::int32_t contribution(Kept<15>& worth, std::int32_t age);

    Cpu& c;
    Fpu fpu;
    const std::uint32_t sp;
    const Kept<25> shortlists_at;
    const Kept<31> club_at;
    const Kept<29> candidate_at;
    const Kept<27> entry_at;
    const Kept<28> valuation_at;
    const Kept<26> budget;   // the club's offer budget (inferred), signed
    const Kept<14> capped;   // int8: the contribution is capped at the entry's limit
    const Kept<30> constants;
    Kept<20> judge;          // the club's manager, or the staff member who judges best
    const Kept<21> cap;      // the entry's limit, at least 1
    Kept<24> dear;           // set when the valuation takes the dear formula
    KeptF<28> decline;       // the age decline's factor
    KeptF<29> decline_floor;
    KeptF<30> ceiling;       // the position's ceiling, then the contribution itself
    KeptF<31> factor;        // the position's factor
};

// __dtou (0x10773780), the compiler's double-to-unsigned conversion, as the
// original runs it: below its first bound 0, from its second 0xffffffff,
// else fctiwz (less its third bound, 2^31, added back above it). Leaves r3,
// r4 (its table), cr0, cr6, cr7, FPSCR and the word below sp as it does.
std::uint32_t Contribution::to_unsigned(double value) {
    const auto table = c.toc(own::to_unsigned_bounds);
    c.set(4, table);
    const double low = c.f64(table), high = c.f64(table + 8), half = c.f64(table + 16);
    fpu.fcmp(0, value, low);
    fpu.fcmp(6, value, high);
    if (value < low) return result(c, 0);
    if (!(value < high)) return result(c, 0xffffffff);
    fpu.fcmp(7, value, half);
    const bool below_half = value < half;
    const auto bits = fpu.fctiwz(2, below_half ? value : fpu.fsub(value, half));
    c.put64(c.sp() - 8, bits);
    const auto word = std::uint32_t(bits);
    return result(c, below_half ? word : word + 0x80000000u);
}

// 0x10361524: a draw in [0, count) from the game's random table, the storage
// group's random_range (0x10356e40) inlined by the compiler, into r15; r15
// and r16 hold the cursor's and the salt's addresses until then.
std::int32_t Contribution::draw_below(std::int32_t count) {
    const RandomTable random{c, c.r(15), c.toc(slot::random_end), c.toc(slot::random_table), c.r(16)};
    const DrawSite site{0x1036154c, 0x1036165c, 0x10361614, sp + 0x68, sp + 0x60, sp + 0x58,
                        false, {.previous_in_r4 = true}};
    return detail::draw_below(c, fpu, random, site, count, 15, [&](const WideDraw& d) { d.leave_a(c); });
}

// 0x103614f0: a manager without a staff record gets one drawn at random.
void Contribution::draw_manager_staff_record() {
    if (judge == 0) return;
    const Person manager{c, judge};
    if (manager.non_player() != 0) return;
    const Kept<17> count{c, Field<std::uint32_t>{c, c.toc(own::staff_record_count)}};
    const std::int32_t drawn = draw_below(std::int32_t(count));  // into r15
    manager.non_player() = Field<std::uint32_t>{c, c.toc(own::staff_records)} + std::uint32_t(drawn) * 0x44;
}

// 0x10361688: the qualification mask handed to the estimate: by category,
// with the familiarity of the candidate and the club's manager.
std::uint32_t Contribution::qualifications() {
    const auto by_familiarity = [&](std::uint32_t back) -> std::uint32_t {
        switch (familiarity(club().manager(), back)) {
        case 0: return 0x40;
        case 1: return 0x50;
        case 2: return 0x10;
        case 3: return 0x30;
        case 4: return 0x20;
        default: return 0;
        }
    };
    const auto category = entry().category();
    if (category == 2 || category == 3) {
        Kept<23> mask{c, 0x5882};
        if (entry().target() == candidate().id() && candidate().club() == club_at &&
            yes(game::human_controlled, 0x103616d8, {club_at, 1, 1, 0, 0}))
            mask = mask | 0x810000;
        mask = mask | by_familiarity(0x103616f0);
        return mask;
    }
    if (category == 4) {
        Kept<23> mask{c, 0x5982};
        mask = mask | by_familiarity(0x10361760);
        return mask;
    }
    const Kept<23> mask{c, 0x5902};
    return mask;
}

// The offer the estimate gets: budget squared times 2.5e-5, over r22's
// divisor (4, or 0x10/6 for a loan).
std::uint32_t Contribution::budget_offer() {
    const double first = integer(0x50, budget);
    const double scaled = fpu.fmul(2.5e-05, first);
    const double second = integer(0x58, budget);
    const double squared = fpu.fmul(scaled, second);
    c.set_f(1, squared);  // the original computes it into f1 (the estimate may leave it)
    const double divisor = integer(0x60, std::int8_t(c.r(22)));
    return std::uint32_t(truncated(0x68, fpu.fdiv(squared, divisor)));
}

// 0x10361948: the club's best judge of the candidate. Mask bit 0x100 picks by
// staff attribute 6, bit 0x80 by attribute 5 (with 5 more for the manager);
// a staff member the candidate is familiar with (3 or 4) counts 5 or 10 less,
// and under attribute 6 only at clubs of reputation 3500 or more.
void Contribution::choose_judge(std::uint32_t qualifications) {
    const auto adjusted = [&](std::uint32_t rating, std::int32_t known, bool reputable) -> std::optional<std::uint32_t> {
        if (known == 4) return reputable ? std::optional(rating - 10) : std::nullopt;
        if (known == 3) return reputable ? std::optional(rating - 5) : std::nullopt;
        return rating;
    };
    if (qualifications & 0x100) {
        Kept<19> best{c, 0};
        if (judge != 0) {
            if (const auto record = Person{c, judge}.non_player().get(); record != 0) {
                best = std::uint32_t(std::int32_t(std::int8_t(call(game::judging, 0x10361970, {record, 6}))));
                best = *adjusted(best, familiarity(judge, 0x10361980), true);
            }
        }
        Kept<16> n{c, 0};
        Kept<18> offset{c, 0};
        for (; std::int32_t(n) < std::int32_t(club_staff_count); n = n + 1, offset = offset + 4) {
            const Kept<17> at{c, offset + club_staff};
            const auto person = club().staff(n).get();
            if (person == 0) continue;
            const auto record = Person{c, person}.non_player().get();
            if (record == 0) continue;
            Kept<15> rating{c, std::uint32_t(std::int32_t(std::int8_t(call(game::judging, 0x103619d4, {record, 6}))))};
            const auto known = familiarity(club().staff(n), 0x103619e4);
            const auto counted = adjusted(rating, known, club().reputation() >= 0xdac);
            if (!counted) continue;
            rating = *counted;
            if (std::int8_t(rating) > std::int8_t(best)) {
                best = rating;
                judge = club().staff(n);
            }
        }
    } else if (qualifications & 0x80) {
        Kept<15> best{c, 0};
        if (judge != 0) {
            // Quirk: the manager's staff record is not checked for null here.
            const auto rating = call(game::judging, 0x10361a70, {Person{c, judge}.non_player(), 5});
            best = std::uint32_t(std::int32_t(std::int8_t(rating + 5)));
            best = *adjusted(best, familiarity(judge, 0x10361a84), true);
        }
        Kept<19> n{c, 0};
        Kept<18> offset{c, 0};
        for (; std::int32_t(n) < std::int32_t(club_staff_count); n = n + 1, offset = offset + 4) {
            const Kept<17> at{c, offset + club_staff};
            const auto person = club().staff(n).get();
            if (person == 0) continue;
            const auto record = Person{c, person}.non_player().get();
            if (record == 0) continue;
            Kept<16> rating{c, std::uint32_t(std::int32_t(std::int8_t(call(game::judging, 0x10361adc, {record, 5}))))};
            rating = *adjusted(rating, familiarity(club().staff(n), 0x10361aec), true);
            if (std::int8_t(rating) > std::int8_t(best)) {
                best = rating;
                judge = club().staff(n);
            }
        }
    }
}

// 0x103617e8: the valuation, through the estimate (0x10216830).
void Contribution::estimate_valuation(std::uint32_t qualifications) {
    const auto position = extended(entry().positions());
    if (yes(game::human_controlled, 0x10361800, {club_at, 1, 1, 0, 0})) {
        const auto nation = call(game::club_nation, 0x10361810, {club_at});
        const auto service = c.toc(own::staff_service);
        c.set(6, service);
        if (yes(game::staff_predicate, 0x10361824, {Field<std::uint32_t>{c, service}.get(), candidate_at, nation})) {
            const auto by = call(game::club_judge, 0x10361834, {club_at});
            valuation() = std::int32_t(
                call(game::estimate, 0x10361850, {candidate_at, position, qualifications, club_at, 0xffffffff, by}));
            // The larger of 1.5 times the estimate and the estimate plus 15000.
            const std::int32_t estimate = valuation();
            const double raised = fpu.fmul(1.5, integer(0x60, estimate));
            stack(0x58, estimate + 15000);
            double value = fpu.fsub(c.f64(sp + 0x58), k::signed_bias);
            fpu.fcmp(0, value, raised);
            if (value > raised) value = integer(0x68, estimate + 15000);
            else value = raised;
            c.set_f(1, value);  // f1 at return when the valuation is out of reach
            valuation() = truncated(0x50, value);
            leave(c, {{4, std::uint32_t(estimate + 15000)}});
            return;
        }
        const auto by = call(game::club_judge, 0x103618c8, {club_at});
        const auto offer = budget_offer();
        valuation() =
            std::int32_t(call(game::estimate, 0x10361940, {candidate_at, position, qualifications, club_at, offer, by}));
        return;
    }
    choose_judge(qualifications);
    const auto offer = std::int32_t(budget) > -1 ? budget_offer() : 0xffffffff;
    valuation() =
        std::int32_t(call(game::estimate, 0x10361bc4, {candidate_at, position, qualifications, club_at, offer, judge}));
    // 5% more for a candidate at another club, when 0x100d3150 says so.
    if (candidate().club() != club_at && yes(game::staff_check, 0x10361be0, {candidate_at, 0})) {
        c.set_f(1, k::signed_bias);  // the original loads the bias into f1 (lfd f1)
        valuation() = truncated(0x58, fpu.fmul(1.05, integer(0x50, valuation())));
    }
}

// 0x10361c20: a negative valuation, one more than 10000 below the budget, or
// a loan entry gets no contribution (a loan's valuation is raised first to the
// larger of 1.2 times it and it plus 5000, when 0x100d3230 says so).
bool Contribution::out_of_reach() {
    const Kept<15> fee{c, std::uint32_t(valuation().get())};
    const auto value = std::int32_t(fee);
    if (value >= 0 && value >= std::int32_t(std::uint32_t(budget) - 10000) && entry().loan() == 0) return false;
    if (!nonzero(c, std::int32_t(entry().loan()))) return true;
    if (!yes(game::loan_check, 0x10361c5c, {candidate_at, 0})) return true;
    const std::int32_t estimate = valuation();
    const double raised = fpu.fmul(1.2, integer(0x58, estimate));
    stack(0x50, estimate + 5000);
    double raised_by = fpu.fsub(c.f64(sp + 0x50), k::signed_bias);
    fpu.fcmp(0, raised_by, raised);
    double chosen = raised;
    if (raised_by > raised) chosen = integer(0x60, estimate + 5000);
    c.set_f(1, chosen);  // the original computes it into f1, left at return
    valuation() = truncated(0x68, chosen);
    leave(c, {{4, std::uint32_t(estimate + 5000)}});
    return true;
}

// 0x10361cd4: a candidate whose details carry a premium level n (the high
// nibble at +0xc) is valued 1 + n^2/100 times as much, at most 5000 more.
void Contribution::apply_premium() {
    const auto level = [&] {
        const auto details = Field<std::uint32_t>{c, c.r(13)}.get();
        return std::uint32_t(c.u8(details + std::uint32_t(candidate().id()) * 100 + 0xc) >> 4);
    };
    const auto premium = level();
    if (premium == 0) return;
    const double first = power(natural(0x50, premium), 0x10361d10);
    // The idiom's high word, which the original builds in r5 here; nothing
    // after it writes r5 before pow (an import) on most paths, so the
    // contribution's caller can see it.
    c.set(5, 0x43300000);
    const double scale = fpu.fadd(1.0, fpu.fdiv(first, 100.0));
    const std::int32_t fee = std::int32_t(c.r(15));
    stack(0x58, fee);
    stack(0x60, fee + 5000);
    const double raised = fpu.fmul(fpu.fsub(c.f64(sp + 0x58), k::signed_bias), scale);
    const double capped_value = fpu.fsub(c.f64(sp + 0x60), k::signed_bias);
    fpu.fcmp(0, capped_value, raised);
    double value;
    if (capped_value < raised) {
        value = integer(0x68, fee + 5000);
    } else {
        // Computed again from scratch.
        const double again = power(natural(0x48, level()), 0x10361dac);
        const double rescale = fpu.fadd(1.0, fpu.fdiv(again, 100.0));
        value = fpu.fmul(integer(0x40, valuation()), rescale);
    }
    valuation() = truncated(0x38, value);
}

// 0x10361df4: the position's ceiling and factor (f30, f31), the age the
// candidate peaks at (32 for goalkeepers, 28 otherwise), and the worth: 0.32
// of a mix of the valuation and the entry's asking value.
std::uint32_t Contribution::base_worth(std::uint32_t& age_limit, std::int32_t& fee_now) {
    const auto position = entry().positions().get();
    enter_direct(c, 0x10361e00, {shortlists_at, extended(position)});
    const auto group = result(c, std::uint32_t(position_group(c, position)));
    Kept<23> limit{c, 0x1c};
    // The jump table at slot::cases_10361490_b, by group.
    switch (std::int8_t(group)) {
    case 14: case 20: ceiling = 13000000.0; factor = 1.0; break;
    case 11: case 12: case 13: case 21: ceiling = 11000000.0; factor = 0.8; break;
    case 8: case 9: case 10: case 18: ceiling = 10000000.0; factor = 0.75; break;
    case 5: case 6: case 7: case 19: ceiling = 9000000.0; factor = 0.65; break;
    case 1: case 2: case 3: case 4: case 17: ceiling = 8000000.0; factor = 0.6; break;
    default:  // 0 (goalkeepers), 15, 16 and the rest
        ceiling = 6000000.0;
        limit = 0x20;
        factor = 0.575;
        break;
    }
    age_limit = limit;
    const Kept<17> fee{c, std::uint32_t(valuation().get())};
    const auto value = fee_now = std::int32_t(fee);
    if (value < 35000 || entry().weight() < value / 8 || position < 0x80) {
        const double asking = fpu.fmul(0.25, integer(0x40, entry().weight()));
        stack(0x38, valuation());
        const double from_fee = fpu.fsub(c.f64(sp + 0x38), k::signed_bias);
        return to_unsigned(fpu.fmul(0.32, fused(0x10361ee8, 0.75, from_fee, asking)));
    }
    // Dear: the fee weighs fee * (5e-5 * fee - 1), in single precision.
    dear = 1;
    const auto asking = entry().weight().get();
    const double single_fee = fpu.fsubs(stacked_integer(c, sp + 0x48, signed_word(value)), k::signed_bias);
    stack(0x40, value);
    const double weight = fused(0x10361f34, 5e-05, single_fee, 1.0);
    const double single_again = fpu.fsubs(c.f64(sp + 0x40), k::signed_bias);
    stack(0x38, asking);
    const double rounded = fpu.frsp(weight);
    const double from_asking = fpu.fsub(c.f64(sp + 0x38), k::signed_bias);
    const double weighed = fpu.fmuls(single_again, rounded);
    return to_unsigned(fpu.fmul(0.32, fused(0x10361f5c, 0.25, from_asking, weighed)));
}

// 0x10362024 and its copy at 0x103622a0: a prospect under 27 valued over
// 20000 gains by its potential (0x10219e40) against the valuation, at most
// 0.6 more than the position's factor, tempered at 24-26; when the valuation
// is beyond 1.05 times the budget or dear, a gain over 1 is scaled by the
// club's target reputation over its reputation (at least 1000), within 1-1.2.
// A gain still over 1 raises the factor, the valuation and the worth.
void Contribution::young_prospect(Kept<15>& worth, std::int32_t age, bool second_copy) {
    struct Sites {
        std::uint32_t back, at_25, at_26, at_24, raise;
    };
    constexpr Sites first{0x10362048, 0x103620d8, 0x103620cc, 0x103620e8, 0x103621ec};
    constexpr Sites second{0x103622c4, 0x10362358, 0x10362368, 0x1036234c, 0x1036246c};
    const Sites& at = second_copy ? second : first;
    const auto quarter = std::int16_t(valuation() / 4);
    const auto rated =
        std::int16_t(call(game::potential, at.back, {club().manager(), candidate_at, club_at, 1, extended(quarter)}));
    const std::int32_t now = valuation();
    const double bound = fpu.fadd(0.6, factor);
    const double raised = fpu.fmul(220.0, integer(0x38, rated));
    double gain = fpu.fdiv(raised, integer(0x40, now));
    fpu.fcmp(0, gain, bound);
    if (!(gain < bound)) gain = bound;
    if (age == 25) gain = fused(at.at_25, 0.5, gain, 0.5);
    else if (age == 26) gain = fused(at.at_26, 0.3, gain, 0.7);
    else if (age == 24) gain = fused(at.at_24, 0.7, gain, 0.3);
    const double reach = fpu.fmul(1.05, integer(0x40, budget));
    const double from_now = integer(0x38, now);
    fpu.fcmp(0, from_now, reach);
    if (from_now > reach || nonzero(c, std::int8_t(dear))) {
        fpu.fcmp(0, gain, 1.0);
        if (gain > 1.0) {
            const std::int32_t reputation = club().reputation();
            const std::int32_t floor = reputation >= 1000 ? reputation : 1000;
            const auto offset = std::uint32_t(club().id()) * 12;
            const auto table = Field<std::uint32_t>{c, shortlists_at + 0xdc}.get();
            const std::int32_t target = Field<std::int16_t>{c, table + offset + 6};
            leave(c, {{4, 0x43300000}, {5, offset}, {6, table}});
            stack(0x40, floor);
            stack(0x38, target);
            const double below = fpu.fsub(c.f64(sp + 0x40), k::signed_bias);
            const double ratio = fpu.fdiv(fpu.fsub(c.f64(sp + 0x38), k::signed_bias), below);
            double scale = 1.0;
            fpu.fcmp(0, 1.0, ratio);
            if (!(1.0 > ratio)) {
                scale = 1.2;
                fpu.fcmp(0, 1.2, ratio);
                if (!(1.2 < ratio)) scale = ratio;
            }
            gain = fpu.fmul(gain, scale);
        }
    }
    fpu.fcmp(0, gain, 1.0);
    if (!(gain > 1.0)) return;
    stack(0x48, now);
    factor = fpu.fmul(factor, gain);
    const double raise = fused(at.raise, gain, 0.5, 0.5);
    const double new_value = fpu.fmul(fpu.fsub(c.f64(sp + 0x48), k::signed_bias), raise);
    const double from_worth = natural(0x38, worth);
    const auto value_bits = fpu.fctiwz(0, new_value);
    const double new_worth = fpu.fmul(from_worth, raise);
    c.put64(sp + 0x50, value_bits);
    valuation() = std::int32_t(std::uint32_t(value_bits));
    worth = std::uint32_t(truncated(0x40, new_worth));
}

// 0x103624dc: past the peak age the worth (and a category-5 valuation)
// declines: by 1 / (0.8 + 0.3 * years over) toward a floor (0, or
// 0.00025 * reputation - 1.5 for a reputation over 6000), more for a
// candidate who knows the club's nation (0x100cab10), less for one two years
// further on whose home reputation is below the club's (categories 3, 4).
void Contribution::decline_with_age(Kept<15>& worth, std::int32_t age, std::uint32_t age_limit) {
    const Kept<16> limit{c, std::uint32_t(std::int32_t(std::int8_t(age_limit)))};
    const auto peak = std::int32_t(limit);
    if (age <= peak) return;
    const Kept<13> over{c, std::uint32_t(age - peak)};
    const Player player{c, candidate().player()};
    const std::int32_t reputation = player.world_reputation();
    decline_floor = 0.0;
    decline = fpu.fdiv(1.0, fused(0x10362524, 0.3, integer(0x38, std::int32_t(over)), 0.8));
    if (reputation > 6000) decline_floor = fused(0x1036254c, 0.00025, integer(0x40, reputation), 1.5);
    if (yes(game::knows_nation, 0x10362560, {candidate_at, club().nation(), 1})) {
        decline = fpu.fdiv(decline, fused(0x1036258c, 0.15, integer(0x38, std::int32_t(over)), 0.9));
    } else if (age > peak + 2) {
        const auto category = entry().category();
        if ((category == 4 || category == 3) &&
            Player{c, candidate().player()}.reputation() < club().reputation()) {
            const double years = fpu.fdiv(integer(0x38, std::int32_t(over) - 2), 15.0);
            decline = fpu.fmul(decline, fpu.fsub(1.0, years));
        }
    }
    if (entry().category() == 5) {
        const std::int32_t value = valuation();
        stack(0x38, value);
        const double mix = fused(0x10362638, decline, fpu.fsub(1.0, decline_floor), decline_floor);
        valuation() = std::int32_t(to_unsigned(fpu.fmul(fpu.fsub(c.f64(sp + 0x38), k::signed_bias), mix)));
    }
    stack(0x38, 0);
    c.put32(sp + 0x3c, worth);
    const double mix = fused(0x10362668, decline, fpu.fsub(1.0, decline_floor), decline_floor);
    worth = to_unsigned(fpu.fmul(fpu.fsub(c.f64(sp + 0x38), unsigned_bias), mix));
}

// 0x1036267c: the contribution: the position's ceiling over 1e8 times the
// square of the worth (less its +0x2d decline squared over 6000), within
// the club's, the candidate's, the age's and the squad's caps, adjusted by
// the club's nation and, when capped, at most the entry's limit.
std::int32_t Contribution::contribution(Kept<15>& worth, std::int32_t age) {
    const std::int32_t attribute = Player{c, candidate().player()}.decline();
    const double squared = power(integer(0x38, attribute), 0x103626ac);
    const double lost = fpu.fdiv(squared, 6000.0);
    stack(0x40, 0);
    c.put32(sp + 0x44, worth);
    const double per_unit = fpu.fdiv(ceiling, 1e8);
    // f1 at return: the last value the original computes into f1 (each
    // set_f(1) below marks where it does).
    c.set_f(1, per_unit);
    c.put32(sp + 0x50, 0x43300000);
    const double kept_share = fpu.fsub(1.0, lost);
    const auto n = std::uint32_t(truncated(0x48, fpu.fmul(fpu.fsub(c.f64(sp + 0x40), unsigned_bias), kept_share)));
    double value = fpu.fmul(per_unit, natural(0x50, n * n));
    c.set(4, 0x43300000);

    // Categories 6 and 5 are capped by the club's reputation.
    const auto category = entry().category();
    if (category == 6) {
        const std::int32_t reputation = club().reputation();
        if (reputation > 5000) {
            const double cap_value = fused(0x10362748, 20.0, integer(0x38, reputation - 5000), 50000.0);
            c.set_f(1, cap_value);
            fpu.fcmp(0, value, cap_value);
            // Quirk: the cap then taken is 50000 + (reputation - 100000).
            if (value > cap_value) value = fpu.fadd(50000.0, integer(0x38, reputation - 100000));
        } else {
            const double cap_value = integer(0x38, reputation * 10);
            c.set_f(1, cap_value);
            fpu.fcmp(0, value, cap_value);
            if (value > cap_value) {
                c.set_f(1, 10.0);
                value = fpu.fmul(10.0, integer(0x38, reputation));
            }
        }
    } else if (category == 5) {
        const std::int32_t reputation = club().reputation();
        double cap_value;
        if (reputation > 5000) {
            const double root = fused(0x103627f4, 0.2, integer(0x38, reputation - 5000), 500.0);
            cap_value = fpu.fmul(root, root);
        } else {
            stack(0x40, reputation);
            const double tenth = fpu.fmul(0.1, integer(0x38, reputation));
            cap_value = fpu.fmul(0.1, fpu.fmul(tenth, fpu.fsub(c.f64(sp + 0x40), k::signed_bias)));
        }
        c.set_f(1, cap_value);
        fpu.fcmp(0, value, cap_value);
        if (value > cap_value) value = cap_value;
    }

    // By the candidate's reputation: the ceiling becomes the lesser of the
    // value and a cap growing with it; the factor grows from 8000 on.
    const auto at_most = [&](double cap_value, double current) {
        fpu.fcmp(0, cap_value, current);
        return cap_value < current ? cap_value : current;
    };
    if (const std::int32_t reputation = Player{c, candidate().player()}.world_reputation(); reputation <= 5000) {
        c.set_f(1, 1e7);
        ceiling = at_most(fpu.fmul(1e7, factor), value);
    } else if (reputation <= 8000) {
        stack(0x38, reputation);
        const double slope = fpu.fmul(1666.67, factor);
        c.set_f(1, 1650000.0);
        ceiling = at_most(fused(0x103628ac, slope, fpu.fsub(c.f64(sp + 0x38), k::signed_bias), 1650000.0), value);
    } else if (reputation < 9000) {
        const double scaled = fused(0x103628f0, 7500.0, integer(0x38, reputation), 45e6);
        c.set_f(1, scaled);
        ceiling = at_most(fpu.fmul(factor, scaled), value);
        factor = fpu.fmul(factor, 1.5);
    } else {
        ceiling = 1e8;
        factor = fpu.fmul(factor, 2.5);
    }

    // The jump table at slot::cases_10361490_a, by age (16-40): a cap
    // proportional to the factor; none at 20-27.
    const auto by_age = [](std::int32_t years) -> double {
        switch (years) {
        case 16: return 4500000.0;
        case 17: return 6000000.0;
        case 18: return 8000000.0;
        case 19: return 12000000.0;
        case 28: return 15000000.0;
        case 29: return 12000000.0;
        case 30: return 10000000.0;
        case 31: return 8000000.0;
        case 32: return 5000000.0;
        case 33: return 2500000.0;
        case 34: return 1000000.0;
        case 35: return 750000.0;
        case 36: case 37: case 38: case 39: case 40: return 500000.0;
        default: return 0.0;
        }
    };
    if (const double cap_value = by_age(age); cap_value != 0.0) ceiling = at_most(fpu.fmul(cap_value, factor), ceiling);

    // Over 1.5 million, a candidate of low second potential (under 145)
    // keeps only part of the excess.
    fpu.fcmp(0, ceiling, 1500000.0);
    if (ceiling > 1500000.0) {
        const auto quarter = std::int16_t(valuation() / 4);
        const auto rated = std::int16_t(
            call(game::potential, 0x10362b0c, {club().manager(), candidate_at, club_at, 2, extended(quarter)}));
        const double potential = integer(0x38, rated);
        c.set_f(1, c.f64(sp + 0x38));  // the original loads the stacked integer into f1
        fpu.fcmp(0, potential, 145.0);
        if (potential < 145.0) {
            c.set_f(1, 1500000.0);
            const double share = fused(0x10362b44, 0.025, potential, 2.625);
            fpu.fcmp(0, 0.0, share);
            const double kept = 0.0 > share ? 0.0 : share;
            ceiling = fused(0x10362b60, fpu.fsub(ceiling, 1500000.0), kept, 1500000.0);
        }
    }

    // A squad short of 16 caps it at 670000 per player (at least 1 million,
    // and a reputation's 500ths), more for a candidate over 8000.
    if (const auto squad = club().division().get(); squad != 0) {
        std::int32_t players = Field<std::int16_t>{c, squad + 0x18};
        c.set(5, std::uint32_t(players));
        if (players < 16) {
            const std::int32_t by_reputation = club().reputation() / 500;
            if (by_reputation > players) players = by_reputation;
            c.set(5, std::uint32_t(players));
            const auto per_player = std::int32_t(std::uint32_t(players) * 670000);
            std::int32_t limit = per_player > 1000000 ? per_player : 1000000;
            if (const std::int32_t reputation = Player{c, candidate().player()}.world_reputation(); reputation > 8000)
                limit = std::int32_t(std::uint32_t(limit) + std::uint32_t(reputation - 8000) * 10000);
            c.set(4, std::uint32_t(limit));
            const double cap_value = integer(0x38, limit);
            fpu.fcmp(0, ceiling, cap_value);
            if (ceiling > cap_value) {
                stack(0x38, limit);
                fpu.fcmp(0, 1.0, factor);
                const double from_limit = fpu.fsub(c.f64(sp + 0x38), k::signed_bias);
                const double share = 1.0 < factor ? 1.0 : double(factor);
                c.set_f(1, k::signed_bias);
                ceiling = integer(0x48, truncated(0x40, fpu.fmul(from_limit, share)));
            }
        }
    }

    // Two nations (inferred): one tempers values under 6 million and takes
    // 90% above; the other takes 80%, to a whole number.
    const std::int32_t nation = Field<std::int32_t>{c, club().nation() + 0x68};
    c.set(4, std::uint32_t(nation));
    if (nation == Field<std::int32_t>{c, c.toc(own::favoured_nation)}) {
        fpu.fcmp(0, ceiling, 6000000.0);
        if (!(ceiling < 6000000.0)) {
            ceiling = fpu.fmul(0.9, ceiling);
        } else {
            c.set_f(1, 0.6);
            const double part = fpu.fmul(fpu.fmul(0.6, fpu.fdiv(ceiling, 6000000.0)), ceiling);
            ceiling = fused(0x10362c8c, 0.3, ceiling, part);
        }
    } else if (nation == Field<std::int32_t>{c, c.toc(own::frugal_nation)}) {
        c.set_f(1, k::signed_bias);
        ceiling = integer(0x40, truncated(0x38, fpu.fmul(0.8, ceiling)));
    }

    if (nonzero(c, std::int8_t(capped))) {
        c.set_f(1, k::signed_bias);
        const double limit = integer(0x38, std::int32_t(cap));
        fpu.fcmp(0, limit, ceiling);
        if (limit < ceiling) ceiling = integer(0x40, std::int32_t(cap));
        ceiling = integer(0x50, truncated(0x48, ceiling));
    }
    return truncated(0x38, ceiling);
}

std::uint32_t Contribution::run() {
    draw_manager_staff_record();
    const auto mask = qualifications();
    if (nonzero(c, std::int32_t(entry().loan()))) {
        const bool checked = yes(game::loan_check, 0x103617d4, {candidate_at, 0});
        keep(c, {{22, checked ? 0x10u : 6u}});
    }
    estimate_valuation(mask);
    if (out_of_reach()) return 0;
    apply_premium();
    std::uint32_t age_limit = 0;
    std::int32_t fee = 0;
    Kept<15> worth{c, base_worth(age_limit, fee)};
    if (worth < 1) worth = 1;

    // 0x10361f78: by category.
    const Kept<22> age{c, std::uint32_t(std::int32_t(std::int8_t(candidate().age_halfword())))};
    const auto years = std::int32_t(age);
    const auto prospect = [&](bool second_copy) {
        if (years < 27 && fee > 20000) young_prospect(worth, years, second_copy);
    };
    switch (entry().category()) {
    case 1:
    case 2: prospect(false); break;
    case 3:
        worth = entry().positions() < 0x80 ? to_unsigned(fpu.fdiv(natural(0x38, worth), 1.5))
                                          : to_unsigned(fpu.fdiv(natural(0x40, worth), 1.1));
        prospect(true);
        break;
    case 4: worth = to_unsigned(fpu.fmul(natural(0x38, worth), 0.5)); break;
    case 5:
        worth = to_unsigned(fpu.fdiv(natural(0x38, worth), 3.0));
        age_limit -= 9;
        keep(c, {{23, age_limit}});
        break;
    case 6: worth = to_unsigned(fpu.fdiv(natural(0x38, worth), 5.0)); break;
    default: break;
    }
    decline_with_age(worth, years, age_limit);
    return std::uint32_t(contribution(worth, years));
}
}

std::uint32_t assess_candidate_contribution(Cpu& c, std::uint32_t shortlists, std::uint32_t club, std::uint32_t candidate,
                                            std::uint32_t entry, std::uint32_t valuation, std::uint32_t budget,
                                            std::uint32_t capped) {
    Frame frame(c, entry_point, 0xe0, {.first_gpr = 13, .first_fpr = 28});
    return Contribution{c, frame, shortlists, club, candidate, entry, valuation, budget, capped}.run();
}
}
