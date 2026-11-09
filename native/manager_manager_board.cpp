// MANAGER_MANAGER.CPP board reaction block (manager_manager_board.hpp): 0x101a05c0, the two sets of flags
// the board draws from a job record, and 0x101a2eb0, the reaction that moves the clubs' rows by them.
//
// Both keep the original's stack image: judge's prologue and its int-to-double words (0x43300000 and
// value ^ 0x80000000) and `fctiwz` result, react's prologue and the two flag halfwords judge writes into its
// frame (read back as the stack holds them when judge ends at once), and the registers the original holds at
// each call and leaves at its end (r3-r10, cr0, f1).
#include "manager_manager_board.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "manager_manager_board_detail.hpp"
#include "manager_manager_detail.hpp"
#include <array>
#include <span>

namespace cm::aot::manager_manager {
using guest::Cpu;
using guest::Fpu;
using guest::Frame;
using guest::Kept;
using guest::compare;
using guest::compare_logical;
using guest::nonzero;
using guest::nonzero_logical;
using namespace board_detail;
namespace {

std::int32_t sbyte(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }

// One step of a flag ladder: the pool double the ratio is compared with (fcmpo, `bge` past it) and the
// flag bit set when the ratio is below it. The last rung of a ladder has no comparison.
struct Rung {
    std::uint32_t limit;
    std::uint32_t bit;
};
// How a set of flags reads the standing against the club's reputation: with the standing not under the
// reputation's square only `weak` is set; above it a high valuation (over 250000) and a low one each have a
// ladder of ratios (the standing over the valuation), ending in a bit set unconditionally.
struct Scale {
    std::uint32_t weak;
    std::array<Rung, 4> high;
    std::uint32_t high_last;
    std::array<Rung, 2> low;
    std::uint32_t low_last;
};
// The club's set (first halfword) and the employer's set (second halfword) differ in their bits.
constexpr Scale club_scale{.weak = 0x4,
                           .high = {{{0x60, 0x10}, {0x20, 0x8}, {0x98, 0x4}, {0x100, 0x2}}},
                           .high_last = 0x1,
                           .low = {{{0x108, 0x8}, {0x110, 0x4}}}, .low_last = 0x2};
constexpr Scale employer_scale{.weak = 0x4,
                               .high = {{{0x0, 0x1}, {0x20, 0x2}, {0x98, 0x4}, {0x118, 0x8}}},
                               .high_last = 0x10,
                               .low = {{{0x108, 0x2}, {0x110, 0x4}}}, .low_last = 0x8};

// 0x101a05c0. The original keeps, in registers: the flag pointers r22 and r29, the valuation r23, the club
// (later the person's employer) r24, the context cell r25, the record (later the standing) r26, the floats
// pool r27, the object r28, the person r30 and the pool r31.
class Judge {
public:
    Judge(Cpu& c, Frame& frame, std::uint32_t object, std::uint32_t record, std::uint32_t flags_a,
          std::uint32_t flags_b)
        : c(c), frame(frame), fpu(c), record(record), flags_a(flags_a), flags_b(flags_b),
          valuation(c, c.r(23)), club(c, c.r(24)), standing(c, record) {
        guest::keep(c, {{25, c.toc(toc::context)}, {27, c.toc(toc::pool_floats)}, {28, object}, {22, flags_a},
                        {29, flags_b}, {31, c.toc(toc::pool)}});
        pool = c.r(31);
    }

    void run() {
        const auto closed = c.u8(record + job::closed);
        compare(c, std::int32_t(closed), 1);
        if (closed == 1) return;
        c.put16(flags_b, 0);
        c.put16(flags_a, 0);
        const auto index = c.u32(record + job::person);
        const auto people = c.u32(c.toc(toc::people));
        c.set(3, people);
        person = people + index * person::size;
        guest::keep(c, {{30, person}});
        const auto career = c.u32(person + person::career);
        compare_logical(c, career);
        if (career == 0) return;
        const auto club_index = c.u32(record + job::club);
        club = c.u32(c.toc(toc::clubs)) + club_index * club::size;
        c.set({{3, club}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
        c.call(callee::club_state, 0x101a0650);
        const auto listed = nonzero(c, sbyte(c.r(3)));  // extsb. r0,r3
        valuation = value_of(club, listed, {0x101a067c, 0x101a06a0, 0x101a06cc, 0x101a06f0});
        const auto answered = std::int32_t(std::uint32_t(valuation));
        compare(c, answered);
        if (answered == 0) valuation = 1;  // the club's valuation is never 0 below
        c.set({{3, std::uint32_t(standing)}, {4, 0}, {5, 0}, {6, 0}, {7, 1}});
        c.call(callee::standing, 0x101a0718);
        standing = c.r(3);
        compare_logical(c, club);
        if (club != 0) club_flags();
        employer_flags();
    }

private:
    // The valuation 0x10084be0 of the person against `other` (the person's employer when it has one, else
    // the fallback club): `listed` is the club state's answer, which with the employer picks the same flag.
    // The calls differ only by the address they return to: `back` is indexed by (no listing, no employer).
    std::uint32_t value_of(std::uint32_t fallback, bool listed, const std::array<std::uint32_t, 4>& back) {
        const auto employer = c.u32(person + person::employer);
        c.set(5, employer);
        const bool by_employer = nonzero_logical(c, employer);
        c.set({{3, c.u32(c.r(25))}, {4, person}, {6, 0}, {7, 0xffffffffu}, {8, (listed || by_employer) ? 1u : 0u}});
        if (!by_employer) c.set(5, fallback);
        c.call(callee::valuation, back[(listed ? 0 : 2) + (by_employer ? 0 : 1)]);
        return c.r(3);
    }
    std::uint32_t row_of(std::uint32_t id) { return c.u32(c.r(28) + object::table) + id * row::size; }
    void add_flags(std::uint32_t pointer, std::uint32_t bit) { c.put16(pointer, c.u16(pointer) | bit); }
    double constant(std::uint32_t at) { return c.f64(pool + at); }
    // Whether the ratio is below the pool double at `at` (fcmpo, then `bge` past the flag).
    bool below(double ratio, std::uint32_t at) {
        fpu.fcmp(0, ratio, constant(at));
        return (c.cr(0) & 8) != 0;
    }
    // The flag the ratio and the sizes pick: `weak` when the standing does not exceed the limit (the
    // reputation's square), else the high or the low valuation's ladder.
    void set_flags(std::uint32_t pointer, const Scale& scale, double ratio, std::int32_t limit,
                   std::int32_t valuation_value) {
        compare(c, std::int32_t(std::uint32_t(standing)), limit);
        if (std::int32_t(std::uint32_t(standing)) <= limit) return add_flags(pointer, scale.weak);
        compare(c, valuation_value, high_valuation);
        if (valuation_value > high_valuation) {
            for (const auto& rung : scale.high)
            if (below(ratio, rung.limit)) return add_flags(pointer, rung.bit);
            return add_flags(pointer, scale.high_last);
        }
        for (const auto& rung : scale.low)
            if (below(ratio, rung.limit)) return add_flags(pointer, rung.bit);
        add_flags(pointer, scale.low_last);
    }
    // The person's queries about the club: 0x100d9480/0x100d94d0, then the same about the club's second
    // person (0x100d9520/0x100d9580, club +0x80). Each pair adds `first` when its first query answers,
    // else `second` when its second does; true when either answered. The club's set goes on to the
    // second person's pair whatever came out; the employer's set ends at the first answer.
    bool pair_flags(std::uint32_t pointer, std::uint32_t about, std::uint32_t first, std::uint32_t second,
                    const std::array<std::uint32_t, 4>& back) {
        c.set({{3, person}, {4, about}});
        c.call(callee::pair_a, back[0]);
        if (nonzero(c, sbyte(c.r(3)))) {
            add_flags(pointer, first);
            return true;
        }
        c.set({{3, person}, {4, about}});
        c.call(callee::pair_b, back[1]);
        if (!nonzero(c, sbyte(c.r(3)))) return false;
        add_flags(pointer, second);
        return true;
    }
    void assistant_flags(std::uint32_t pointer, std::uint32_t about, std::uint32_t first, std::uint32_t second,
                         const std::array<std::uint32_t, 2>& back) {
        c.set({{4, c.u32(about + club::assistant)}, {3, person}});
        c.call(callee::assistant_a, back[0]);
        if (nonzero(c, sbyte(c.r(3)))) return add_flags(pointer, first);
        c.set({{4, c.u32(about + club::assistant)}, {3, person}});
        c.call(callee::assistant_b, back[1]);
        if (nonzero(c, sbyte(c.r(3)))) add_flags(pointer, second);
    }

    // The compiler's int-to-double: the high word and the flipped value go to the frame, the double is loaded.
    void store_pair(std::uint32_t at, std::uint32_t value) {
        c.put32(frame.at(at), conversion_high);
        c.put32(frame.at(at + 4), value ^ 0x80000000u);
    }
    double load_pair(std::uint32_t at) { return c.f64(frame.at(at)); }
    // The reputation (a halfword) times the scale, squared, truncated through the frame's `truncated` slot.
    std::uint32_t reputation_limit(std::uint32_t club_record, std::uint32_t pair, double bias,
                                   double scale) {
        const auto reputation = c.i16(club_record + club::reputation);
        c.put32(frame.at(pair + 4), std::uint32_t(std::int32_t(reputation)) ^ 0x80000000u);
        auto f0 = load_pair(pair);
        f0 = fpu.fsub(f0, bias);
        f0 = fpu.fmul(scale, f0);
        f0 = fpu.fmul(f0, f0);
        c.put64(frame.at(slot::truncated), fpu.fctiwz(0, f0));
        return c.u32(frame.at(slot::truncated + 4));
    }

    // 0x101a0724..0x101a0938: the club's market side. The valuation is added to the row's double (+0x38).
    void club_flags() {
        const auto value = std::uint32_t(valuation);
        const auto id = c.u32(club + club::id);
        compare(c, std::int32_t(value));  // cmpwi r23,0: never 0 (replaced by 1 above), so the ratio is computed
        c.put32(frame.at(slot::fourth + 4), value ^ 0x80000000u);
        const auto row = row_of(id);
        c.put32(frame.at(slot::fourth), conversion_high);
        const double bias = constant(pool::bias);
        c.set_f(2, bias);
        auto f0 = load_pair(slot::fourth);
        const auto f3 = c.f64(row + row::drift);
        f0 = fpu.fsub(f0, bias);
        c.put32(frame.at(slot::third), conversion_high);
        const auto scale = constant(pool::scale);
        f0 = fpu.fadd(f3, f0);
        c.put_f64(row + row::drift, f0);
        const auto limit = reputation_limit(club, slot::third, bias, scale);
        c.set(5, limit);
        store_pair(slot::second, std::uint32_t(standing));
        auto f1 = load_pair(slot::second);
        c.put32(frame.at(slot::first + 4), value ^ 0x80000000u);
        f1 = fpu.fsubs(f1, bias);
        c.put32(frame.at(slot::first), conversion_high);
        f0 = load_pair(slot::first);
        f0 = fpu.fsubs(f0, bias);
        f1 = fpu.fdivs(f1, f0);
        c.set_f(1, f1);
        set_flags(flags_a, club_scale, f1, std::int32_t(limit), std::int32_t(value));
        pair_flags(flags_a, club, 0x40, 0x100, {0x101a08bc, 0x101a08e0});
        assistant_flags(flags_a, club, 0x40, 0x100, {0x101a0900, 0x101a0924});
    }

    // 0x101a0938..0x101a0c08: the same for the person's employer, subtracted from its row.
    void employer_flags() {
        club = c.u32(person + person::employer);
        compare_logical(c, club);
        if (club == 0) return;
        c.set({{3, club}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
        c.call(callee::club_state, 0x101a095c);
        const auto listed = nonzero(c, sbyte(c.r(3)));
        const auto value = value_of(club, listed, {0x101a0988, 0x101a09a8, 0x101a09d0, 0x101a09f0});
        const auto id = c.u32(club + club::id);
        compare(c, std::int32_t(value));  // cmpwi r3,0
        c.set(5, value ^ 0x80000000u);
        c.put32(frame.at(slot::first + 4), value ^ 0x80000000u);
        const auto row = row_of(id);
        c.put32(frame.at(slot::first), conversion_high);
        const double bias = constant(pool::bias);
        c.set_f(2, bias);
        auto f0 = load_pair(slot::first);
        const auto f3 = c.f64(row + row::drift);
        f0 = fpu.fsub(f0, bias);
        c.put32(frame.at(slot::second), conversion_high);
        const auto scale = constant(pool::scale);
        f0 = fpu.fsub(f3, f0);
        c.put_f64(row + row::drift, f0);
        const auto limit = reputation_limit(club, slot::second, bias, scale);
        c.set(6, limit);
        double ratio;
        if (value != 0) {
            store_pair(slot::third, std::uint32_t(standing));
            auto f1 = load_pair(slot::third);
            c.put32(frame.at(slot::fourth + 4), value ^ 0x80000000u);
            f1 = fpu.fsubs(f1, bias);
            c.put32(frame.at(slot::fourth), conversion_high);
            f0 = load_pair(slot::fourth);
            f0 = fpu.fsubs(f0, bias);
            ratio = fpu.fdivs(f1, f0);
        } else {
            ratio = c.f32(c.r(27) + empty_ratio);  // lfs
        }
        c.set_f(1, ratio);
        set_flags(flags_b, employer_scale, ratio, std::int32_t(limit), std::int32_t(value));
        // Each query that answers ends the judging.
        if (pair_flags(flags_b, club, 0x20, 0x80, {0x101a0b88, 0x101a0bac})) return;
        assistant_flags(flags_b, club, 0x20, 0x80, {0x101a0bd0, 0x101a0bf4});
    }

    Cpu& c;
    Frame& frame;
    Fpu fpu;
    std::uint32_t record, flags_a, flags_b, pool = 0, person = 0;
    Kept<23> valuation;
    Kept<24> club;
    Kept<26> standing;
};
}  // namespace

void judge(Cpu& c, std::uint32_t object, std::uint32_t record, std::uint32_t flags_a, std::uint32_t flags_b) {
    c.set(0, c.lr());
    Frame frame(c, 0x101a05c0, 0x90, {.first_gpr = 22});
    Judge(c, frame, object, record, flags_a, flags_b).run();
    lmw_leaves(c, frame.caller());
}
namespace {
// A step of the board's reaction: a halfword of the row moved by `delta` (wrapping to 16 bits).
struct Step {
    std::uint32_t field;
    std::int32_t delta;
};
// A flag bit and the steps it makes (two at most), in the original's order; `offset_in_r5` says whether the
// original leaves the row's byte offset in r5 there (mulli r5), as the callee that follows may see.
struct Rule {
    std::uint32_t mask;
    std::array<Step, 2> steps;
    unsigned count;
    bool offset_in_r5;
};
// The flag that moves the value and the confidence (both sets use these): the lowest rule wins; bit 0x4
// (the third) is a rule of its own that moves nothing.
constexpr std::array<Rule, 5> standing_rules{{
    {0x1, {{{row::value, -0x7d}, {row::confidence, -0x1f4}}}, 2, true},
    {0x2, {{{row::value, -0x19}, {row::confidence, -0xfa}}}, 2, true},
    {0x4, {{}}, 0, false},
    {0x8, {{{row::value, 0x19}, {row::confidence, 0x19}}}, 2, true},
    {0x10, {{{row::value, 0x4b}, {row::confidence, 0x4b}}}, 2, true},
}};
// The flags that move the trust: the club's set (first halfword) and the employer's set differ.
constexpr std::array<Rule, 4> club_trust_rules{{
    {0x20, {{{row::confidence, -0x64}, {row::trust, -0x64}}}, 2, true},
    {0x40, {{{row::trust, 0x32}, {}}}, 1, false},
    {0x80, {{{row::trust, 0xa}, {}}}, 1, false},
    {0x100, {{{row::trust, -0x64}, {row::confidence, -0x32}}}, 2, true},
}};
constexpr std::array<Rule, 4> employer_trust_rules{{
    {0x20, {{{row::trust, -0x64}, {row::confidence, -0x19}}}, 2, true},
    {0x40, {{{row::trust, 0x32}, {row::confidence, 0x19}}}, 2, true},
    {0x80, {{{row::trust, 0xa}, {}}}, 1, false},
    {0x100, {{{row::trust, -0x64}, {}}}, 1, false},
}};

class Reaction {
public:
    Reaction(Cpu& c, Frame& frame, std::uint32_t object, std::uint32_t record)
        : c(c), frame(frame), object(object), record(record), r31(c, record), r30(c, c.r(30)) {
        guest::keep(c, {{29, object}});
    }

    void run() {
        const auto people = c.u32(c.toc(toc::people));
        const auto index = c.u32(record + job::person);
        person = people + index * person::size;
        r30 = person;
        const auto flags_a = frame.at(flags_a_slot), flags_b = frame.at(flags_b_slot);
        c.set(7, people);
        guest::enter(c, 0x101a2ef0, {object, record, flags_a, flags_b});
        judge(c, c.r(3), c.r(4), c.r(5), c.r(6));
        // The row of the record's club by the first set.
        auto club = c.u32(record + job::club);
        move_row(club, flags_a, standing_rules, club_trust_rules);
        const auto employer = c.u32(person + person::employer);
        c.set(3, employer);
        compare_logical(c, employer);
        if (employer == 0) return;
        r31 = c.u32(employer);  // the employer's id
        c.call(callee::club_flag, 0x101a3100);
        if (nonzero(c, sbyte(c.r(3)))) return;
        c.set({{4, c.u32(person + person::employer)}, {3, c.u32(c.toc(toc::employers))}});
        c.call(callee::employer_record, 0x101a3118);
        const auto found = c.r(3);
        if (nonzero_logical(c, found)) {
            const auto barred = c.u8(found + employer_record::barred);
            compare(c, std::int32_t(barred), 1);
            if (barred == 1) return;
        }
        move_row(r31, frame.at(flags_b_slot), standing_rules, employer_trust_rules);
        const auto again = c.u32(person + person::employer);
        c.set({{3, c.u32(again + club::assistant)}, {4, person}});
        c.call(callee::notify, 0x101a3334);
    }

private:
    std::uint32_t row_at(std::uint32_t offset) { return c.u32(object + object::table) + offset; }
    void step(std::uint32_t offset, const Step& s) {
        const auto at = row_at(offset) + s.field;
        c.put16(at, c.u16(at) + std::uint32_t(s.delta));
    }
    // Both blocks: the standing rules, then the trust rules, then the clamps (the value to 1..10000,
    // the trust to at least -10000). `flags` is the frame's halfword, read again for each part.
    void move_row(std::uint32_t id, std::uint32_t flags, std::span<const Rule> standing, std::span<const Rule> trust) {
        const auto offset = id * row::size;
        apply(offset, flags, standing);
        apply(offset, flags, trust);
        c.set(4, offset);
        auto value = std::int32_t(c.i16(row_at(offset) + row::value));
        compare(c, value, 1);
        if (value < 1) c.put16(row_at(offset) + row::value, 1);
        value = c.i16(row_at(offset) + row::value);
        compare(c, value, 0x2710);
        if (value > 0x2710) c.put16(row_at(offset) + row::value, 0x2710);
        const auto held = std::int32_t(c.i16(row_at(offset) + row::trust));
        compare(c, held, -0x2710);
        if (held <= -0x2710) c.put16(row_at(offset) + row::trust, std::uint32_t(-0x2710));
    }
    void apply(std::uint32_t offset, std::uint32_t flags_at, std::span<const Rule> rules) {
        const auto flags = std::uint32_t(c.i16(flags_at));
        for (const auto& rule : rules) {
            const auto masked = flags & rule.mask;
            compare(c, std::int32_t(masked));  // rlwinm.
            if (masked == 0) continue;
            if (rule.offset_in_r5) c.set(5, offset);
            for (unsigned k = 0; k < rule.count; ++k) step(offset, rule.steps[k]);
            return;
        }
    }

    Cpu& c;
    Frame& frame;
    std::uint32_t object, record, person = 0;
    Kept<31> r31;
    Kept<30> r30;
};
}  // namespace

void react(Cpu& c, std::uint32_t object, std::uint32_t record) {
    c.set(0, c.lr());
    Frame frame(c, 0x101a2eb0, 0x50, {.first_gpr = 29});
    Reaction(c, frame, object, record).run();
}
}  // namespace cm::aot::manager_manager
