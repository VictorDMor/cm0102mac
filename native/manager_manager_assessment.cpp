// MANAGER_MANAGER.CPP assessment block (manager_manager_assessment.hpp): 0x101ad080, a staff
// member's score for a job at a club. The score is an integer that a series of rules scales,
// each through the compiler's int-to-double idiom (two stack words, a pool constant, an fctiwz
// into a stack double and a reload of its low word); those stack words are part of the image
// later callees read, so each rule stores them where the original does.
#include "manager_manager_assessment.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "manager_manager_assessment_detail.hpp"
#include "manager_manager_detail.hpp"
#include <array>
#include <cmath>
#include <span>

namespace cm::aot::manager_manager {
using guest::Cpu;
using guest::Fpu;
using guest::compare;
using guest::compare_logical;
using namespace assessment;
namespace {
constexpr std::int32_t rejected = -0x2710;
namespace pool {  // offsets in the pool (TOC[-0x536c]) of doubles
constexpr unsigned zero = 0x0, bias = 0x38, one = 0x50;
}
namespace callee {
constexpr std::uint32_t gap = 0x100cdef0, pair_a = 0x100d9480, pair_b = 0x100d94d0, other_a = 0x100d9520,
                        other_b = 0x100d9580, in_region = 0x100cab10, club_flag = 0x100c7d00,
                        date_object = 0x10715880, calendar = 0x107156f0, statistic = 0x103092f0, power = 0x10780fc0;
}
namespace toc {
constexpr std::int32_t pool = -0x536c, date = -0x7b80, people_count = -0x7be4, singles = -0x5370, stat_owner = -0x7b3c;
}

// A similarity rule: the score is scaled by 1 - |second - person| / divisor over one staff
// attribute. The slots are the original's, one set per rule.
struct Similarity {
    std::uint32_t attribute;
    unsigned divisor;  // pool offset
    unsigned difference, in, out;
};
// A weight rule: the score is scaled by 1 + value / divisor, the value one staff attribute or
// a personality byte less 10.
struct Weight {
    bool personality;
    std::uint32_t attribute;
    unsigned divisor, value, in, out;
};
constexpr bool staff_byte = false, personality = true;

// The rule chains of each job family (slots as the original has them).
constexpr Similarity similar_a[] = {
    {0x2e, 0x128, 0x1a0, 0x198, 0x190}, {0x32, 0x240, 0x188, 0x180, 0x178}, {0x33, 0x128, 0x170, 0x168, 0x160},
    {0x34, 0xc0, 0x158, 0x150, 0x148},  {0x35, 0xc0, 0x140, 0x138, 0x130},  {0x3a, 0x248, 0x128, 0x120, 0x118},
    {0x3c, 0x248, 0x110, 0x108, 0x100}, {0x3f, 0x250, 0xf8, 0xf0, 0xe8},    {0x42, 0xc0, 0xe0, 0xd8, 0xd0}};
constexpr Weight weights_a_first[] = {
    {staff_byte, 0x31, 0x258, 0xd8, 0xd0, 0xe0},   {staff_byte, 0x30, 0x258, 0xf0, 0xe8, 0xf8},
    {staff_byte, 0x37, 0x30, 0x108, 0x100, 0x110}, {staff_byte, 0x38, 0x30, 0x120, 0x118, 0x128},
    {staff_byte, 0x39, 0x30, 0x138, 0x130, 0x140}, {staff_byte, 0x3b, 0x30, 0x150, 0x148, 0x158}};
constexpr Weight weight_a_last = {staff_byte, 0x41, 0, 0xd8, 0xd0, 0xe0};  // divisor by the club's reputation
constexpr Weight weights_a_personality[] = {
    {personality, 0x65, 0x250, 0xd8, 0xd0, 0xe0},    {personality, 0x64, 0x250, 0xf0, 0xe8, 0xf8},
    {personality, 0x66, 0x250, 0x108, 0x100, 0x110}, {personality, 0x67, 0x250, 0x120, 0x118, 0x128}};

constexpr Similarity similar_b[] = {
    {0x2e, 0xc0, 0xe0, 0xe8, 0xf0},     {0x32, 0xc0, 0xf8, 0x100, 0x108},   {0x33, 0x128, 0x110, 0x118, 0x120},
    {0x34, 0x268, 0x128, 0x130, 0x138}, {0x35, 0xc0, 0x140, 0x148, 0x150},  {0x3a, 0x128, 0x158, 0x160, 0x168},
    {0x3c, 0x128, 0x170, 0x178, 0x180}, {0x3f, 0xc0, 0x188, 0x190, 0x198},  {0x42, 0x240, 0x1a0, 0x1a8, 0x1b0}};
constexpr Weight weights_b[] = {
    {staff_byte, 0x31, 0x30, 0xd8, 0xd0, 0xe0},     {staff_byte, 0x30, 0x30, 0xf0, 0xe8, 0xf8},
    {staff_byte, 0x37, 0x270, 0x108, 0x100, 0x110}, {staff_byte, 0x38, 0x270, 0x120, 0x118, 0x128},
    {staff_byte, 0x39, 0x278, 0x138, 0x130, 0x140}, {staff_byte, 0x3b, 0x270, 0x150, 0x148, 0x158},
    {staff_byte, 0x41, 0x30, 0x168, 0x160, 0x170},  {personality, 0x65, 0x250, 0x180, 0x178, 0x188},
    {personality, 0x64, 0x250, 0x198, 0x190, 0x1a0}, {personality, 0x66, 0x250, 0x1b0, 0x1a8, 0x1b8},
    {personality, 0x67, 0x250, 0xc0, 0xc8, 0xb8}};

constexpr Similarity similar_c[] = {
    {0x2e, 0xc0, 0xc8, 0xd0, 0xd8},     {0x32, 0x128, 0xe0, 0xe8, 0xf0},    {0x33, 0x128, 0xf8, 0x100, 0x108},
    {0x34, 0x268, 0x110, 0x118, 0x120}, {0x35, 0xc0, 0x128, 0x130, 0x138},  {0x3a, 0x128, 0x140, 0x148, 0x150},
    {0x3c, 0x128, 0x158, 0x160, 0x168}, {0x3f, 0xc0, 0x170, 0x178, 0x180},  {0x42, 0x240, 0x188, 0x190, 0x198}};
constexpr Weight weights_c[] = {
    {staff_byte, 0x31, 0x250, 0xc0, 0xb8, 0xc8},    {staff_byte, 0x30, 0x250, 0xd8, 0xd0, 0xe0},
    {staff_byte, 0x37, 0x270, 0xf0, 0xe8, 0xf8},    {staff_byte, 0x38, 0x270, 0x108, 0x100, 0x110},
    {staff_byte, 0x39, 0x270, 0x120, 0x118, 0x128}, {staff_byte, 0x3b, 0x258, 0x138, 0x130, 0x140},
    {staff_byte, 0x41, 0x240, 0x150, 0x148, 0x158}, {personality, 0x65, 0xb8, 0x168, 0x160, 0x170},
    {personality, 0x64, 0xb8, 0x180, 0x178, 0x188}, {personality, 0x66, 0xb8, 0x198, 0x190, 0x1a0},
    {personality, 0x67, 0xb8, 0x1b0, 0x1a8, 0x1b8}};

constexpr Similarity similar_d[] = {
    {0x2e, 0x30, 0xc8, 0xd0, 0xd8},     {0x32, 0x30, 0xe0, 0xe8, 0xf0},    {0x33, 0x30, 0xf8, 0x100, 0x108},
    {0x34, 0x30, 0x110, 0x118, 0x120},  {0x35, 0x30, 0x128, 0x130, 0x138}, {0x3a, 0x30, 0x140, 0x148, 0x150},
    {0x3c, 0x30, 0x158, 0x160, 0x168},  {0x3f, 0x30, 0x170, 0x178, 0x180}, {0x42, 0x30, 0x188, 0x190, 0x198}};
constexpr Weight weights_d[] = {
    {staff_byte, 0x37, 0x250, 0xc0, 0xb8, 0xc8},   {staff_byte, 0x38, 0x250, 0xd8, 0xd0, 0xe0},
    {personality, 0x65, 0x30, 0xf0, 0xe8, 0xf8},   {personality, 0x64, 0x30, 0x108, 0x100, 0x110},
    {personality, 0x66, 0x30, 0x120, 0x118, 0x128}, {personality, 0x67, 0x30, 0x138, 0x130, 0x140}};

constexpr Weight weights_e[] = {
    {staff_byte, 0x3e, 0x128, 0xc0, 0xb8, 0xc8}, {personality, 0x65, 0xb8, 0xd8, 0xd0, 0xe0},
    {personality, 0x64, 0xb8, 0xf0, 0xe8, 0xf8}, {personality, 0x66, 0xb8, 0x108, 0x100, 0x110},
    {personality, 0x67, 0xb8, 0x120, 0x118, 0x128}};

class Assessment {
public:
    Assessment(Cpu& cpu, guest::Frame& f, std::uint32_t p, std::uint32_t club_record, std::uint32_t second_person,
               std::uint32_t job_code)
        : c(cpu), fpu(cpu), frame(f), person(p), club(club_record), job(job_code),
          second(cpu, second_person), role(cpu, cpu.r(27)), score(cpu, cpu.r(28)), shared_region(cpu, cpu.r(26)) {
        pool_base = c.toc(toc::pool);
        count_ptr = c.toc(toc::people_count);
        date_ptr = c.toc(toc::date);
        guest::keep(c, {{22, person}, {23, club}, {25, job}, {29, pool_base}, {30, date_ptr}, {31, count_ptr}});
    }
    std::uint32_t run();

private:
    Cpu& c;
    Fpu fpu;
    guest::Frame& frame;
    std::uint32_t person, club, job;
    guest::Kept<24> second;  // the second person, or none
    guest::Kept<27> role;    // the person's role byte (not sign-extended)
    guest::Kept<28> score;
    guest::Kept<26> shared_region;  // r26: the caller's value until the tail sets it
    std::uint32_t pool_base = 0, count_ptr = 0, date_ptr = 0;
    std::int32_t age = 0;  // r21
    // What the rule chains leave in r3..r10 comes from their operands: every term's, kept here.
    struct Terms {
        std::int32_t difference[12], mine[12], value[12], out[12], start = 0;
    };
    Terms terms;
    double last_x = 0, last_weight = 0;  // the last weight rule's operands (f1 at the end of a chain)

    std::int32_t value() const { return std::int32_t(std::uint32_t(score)); }
    double constant(unsigned offset) const { return c.f64(pool_base + offset); }
    std::uint32_t at(unsigned offset) const { return frame.at(std::int32_t(offset)); }
    // The int-to-double idiom: the words 0x43300000 and v^0x80000000 at `slot`, then the double
    // there less the bias.
    void store_conversion(unsigned slot, std::int32_t v) {
        c.put32(at(slot), conversion_high);
        c.put32(at(slot + 4), flipped(v));
    }
    double widen(unsigned slot, std::int32_t v) {
        store_conversion(slot, v);
        return fpu.fsub(c.f64(at(slot)), constant(pool::bias));
    }
    // fctiwz into the stack double at `slot` and the reload of its low word.
    std::int32_t narrow(unsigned slot, double x) {
        c.put64(at(slot), fpu.fctiwz(0, x));
        return c.i32(at(slot + 4));
    }
    // score = int(score * constant(k)).
    void scale(unsigned k, unsigned in, unsigned out) {
        const double x = widen(in, value());
        score = std::uint32_t(narrow(out, fpu.fmul(constant(k), x)));
        c.set_f(1, constant(pool::bias));  // lfd f1,bias; lfd f2,k
        c.set_f(2, constant(k));
    }
    // score = int(score * (constant(base) + n / constant(divisor))).
    void scale_by_ratio(unsigned base, std::int32_t n, unsigned divisor, unsigned n_slot, unsigned in, unsigned out) {
        const double numerator = widen(n_slot, n);
        const double weight = fpu.fadd(constant(base), fpu.fdiv(numerator, constant(divisor)));
        const double x = widen(in, value());
        score = std::uint32_t(narrow(out, fpu.fmul(x, weight)));
        c.set_f(1, x);
        c.set_f(2, constant(base));
    }
    // The tail's ratio rule over the staff record's level (r3 = person staff).
    void level_ratio(unsigned base, unsigned divisor) {
        const auto staff = c.u32(person + person::staff);
        const auto before = value();
        c.set(3, staff);
        scale_by_ratio(base, c.i16(staff + staff::level), divisor, 0xc0, 0xb8, 0xc8);
        c.set({{4, conversion_high}, {5, flipped(before)}});
    }
    std::uint32_t call(std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        unsigned n = 3;
        for (const auto v : arguments) c.set(n++, v);
        c.call(entry, back);
        return c.r(3);
    }
    bool nonzero_byte(std::uint32_t result) { return compare(c, std::int32_t(std::int8_t(result))) != 0; }  // extsb.
    bool in_region(std::uint32_t back, std::uint32_t region_record) {
        return nonzero_byte(call(callee::in_region, back, {person, region_record, 1}));
    }
    std::int32_t role_byte() const { return std::int32_t(std::int8_t(std::uint32_t(role))); }
    bool manager_role() {  // the role is 12 or 5 (two compares in a row)
        const auto r0 = role_byte();
        c.set(0, std::uint32_t(r0));
        return compare(c, r0, 0xc) == 0 || compare(c, r0, 5) == 0;
    }
    std::int32_t reputation(std::uint32_t record) const { return c.i16(record + club::reputation); }

    std::uint32_t exit_with(std::int32_t result) {
        lmw_leaves(c, frame.caller());
        c.set(3, std::uint32_t(result));
        return std::uint32_t(result);
    }
    std::uint32_t reject() { return exit_with(rejected); }
    // The person among the people table's last sixteen (id >= count - 16): r3 the count, r4 the
    // id, r0 the limit, cr0 the signed compare, as the original's three copies of this test leave them.
    bool among_last_sixteen() {
        c.set(3, c.u32(count_ptr));
        c.set(4, c.u32(person + person::id));
        const auto limit = std::int32_t(c.r(3)) - 0x10;
        c.set(0, std::uint32_t(limit));
        return compare(c, std::int32_t(c.r(4)), limit) >= 0;
    }

    static std::uint32_t flipped(std::int32_t v) { return std::uint32_t(v) ^ 0x80000000u; }  // xoris r,r,0x8000
    void similarity(std::span<const Similarity> rules, std::uint32_t mine, std::uint32_t theirs, const double* factor) {
        unsigned k = 0;
        for (const auto& rule : rules) {
            const std::int32_t difference =
                std::int32_t(c.i8(theirs + rule.attribute)) - std::int32_t(c.i8(mine + rule.attribute));
            terms.difference[k] = difference;
            terms.mine[k] = c.i8(mine + rule.attribute);
            const double d = widen(rule.difference, difference);
            double divisor = constant(rule.divisor);
            if (factor) divisor = fpu.fmul(divisor, *factor);
            const double weight = fpu.fsub(constant(pool::one), fpu.fdiv(std::fabs(d), divisor));
            const double x = widen(rule.in, value());
            score = std::uint32_t(narrow(rule.out, fpu.fmul(x, weight)));
            terms.out[k++] = value();
        }
    }
    // The chain's last result; the caller decides when r28 takes it.
    std::int32_t weights(std::span<const Weight> rules, std::uint32_t staff_record, std::int32_t start) {
        std::int32_t current = start;
        unsigned k = 0;
        terms.start = start;
        for (const auto& rule : rules) {
            const std::int32_t v = rule.personality ? std::int32_t(c.i8(person + rule.attribute)) - 10
                                                    : std::int32_t(c.i8(staff_record + rule.attribute));
            terms.value[k] = v;
            const double q = fpu.fdiv(widen(rule.value, v), constant(rule.divisor));
            const double weight = fpu.fadd(constant(pool::one), q);
            const double x = widen(rule.in, current);
            current = narrow(rule.out, fpu.fmul(x, weight));
            terms.out[k++] = current;
            last_x = x;
            last_weight = weight;
        }
        return current;
    }
    // The person's club or previous club is this one (the two compares).
    bool at_club() {
        auto r0 = c.u32(person + person::club_link);
        compare_logical(c, r0, club);
        if (r0 == club) return true;
        r0 = c.u32(person + person::previous_link);
        compare_logical(c, r0, club);
        return r0 == club;
    }
    // max(constant(0x1f8) * score, constant(0x200) + score), through two stack pairs.
    void floor_rule(unsigned first, unsigned second_slot, unsigned out) {
        const double scaled = fpu.fmul(constant(0x1f8), widen(first, value()));
        const double floor = fpu.fadd(constant(0x200), widen(second_slot, value()));
        fpu.fcmp(0, scaled, floor);
        score = std::uint32_t(narrow(out, scaled > floor ? scaled : floor));
        c.set_f(1, constant(0x200));
    }

    bool manager_rules();  // jobs 5 and 12; false: rejected
    bool role_rules();
    bool role_by_club(unsigned own, unsigned other, std::int32_t limit);
    bool role_limited(unsigned k, std::int32_t limit);
    void age_rules();
    void young_rules();
    void gap_rule();
    void second_person_manager();
    void attribute_rules();
    void similarity_block(std::span<const Similarity> rules, unsigned in, unsigned out);
    bool tail();  // false: rejected
    bool region_gap_rule();
    void club_flag_rules();
    bool unflagged_rules();
    void reputation_rules();
    void calendar_rules();
    void closing_rules();
    void leave_weights_bc() {  // r3..r8 as the weight chains of jobs 6, 7, 8, 13, 14, 15 leave them
        c.set({{3, conversion_high}, {4, std::uint32_t(c.i8(person + person::personality_d))}, {5, flipped(terms.out[9])},
               {6, flipped(terms.value[7])}, {7, flipped(terms.value[6])}, {8, flipped(terms.start)}});
    }
};

// Roles 6, 7, 8, 11, 13 and 15: one constant at the person's club, another elsewhere, and
// elsewhere a reputation limit.
bool Assessment::role_by_club(unsigned own, unsigned other, std::int32_t limit) {
    if (at_club()) {
        scale(own, 0x1a8, 0x1b0);
        return true;
    }
    scale(other, 0x1a8, 0x1b0);
    const auto r0 = reputation(club);
    compare(c, r0, limit);
    return r0 <= limit;
}
bool Assessment::role_limited(unsigned k, std::int32_t limit) {
    scale(k, 0x1a8, 0x1b0);
    const auto r0 = reputation(club);
    compare(c, r0, limit);
    return r0 <= limit;
}

bool Assessment::role_rules() {
    const auto r0 = std::uint32_t(role_byte()) - 5;
    compare_logical(c, r0, 0xb);
    if (r0 > 0xb) return role_limited(0x228, 0x1676);  // roles below 5 and above 16
    switch (r0) {
    case 0: return true;                                       // 5
    case 1: case 8: return role_by_club(0x10, 0x208, 0x1676);  // 6, 13
    case 2: return role_by_club(0x20, 0x210, 0x1482);          // 7
    case 3: return role_by_club(0x208, 0x220, 0x1482);         // 8
    case 4: return role_limited(0x60, 0xcb2);                  // 9
    case 6: return role_by_club(0x1f0, 0x10, 0x1482);          // 11
    case 7: scale(0x1f0, 0x1a8, 0x1b0); return true;           // 12
    case 10: return role_by_club(0x208, 0x218, 0x1676);        // 15
    case 11: {                                                 // 16: scaled first, then by club
        scale(0x10, 0x1a8, 0x1b0);
        if (at_club()) return true;
        const auto limit = reputation(club);
        compare(c, limit, 0x1676);
        return limit <= 0x1676;
    }
    default: return role_limited(0x228, 0x1676);  // 10, 14
    }
}

// A person under 38 with a staff level under 4250: the score scaled by a factor linear in the age.
void Assessment::young_rules() {
    const auto staff = c.u32(person + person::staff);
    c.set(3, staff);
    const auto level = std::int32_t(c.i16(staff + staff::level));
    c.set(0, std::uint32_t(level));
    compare(c, level, 0x109a);
    if (level >= 0x109a) return;
    const double a = widen(0x1b0, age);
    const double factor = fpu.multiply_add(0x101ad980, constant(0x70), a, constant(0x1f0));
    const double x = widen(0x1a8, value());
    score = std::uint32_t(narrow(0x1b8, fpu.fmul(x, factor)));
    c.set_f(1, x);
}

void Assessment::age_rules() {
    compare(c, age, 0x3b);
    if (age > 0x3b) {
        if (nonzero_byte(call(callee::club_flag, 0x101ad878, {club}))) {
            const auto r0 = reputation(club);
            compare(c, r0, 0x1b58);
            if (r0 > 0x1b58) {
                scale(0x230, 0x1a8, 0x1b0);
                return;
            }
        }
        const auto r0 = reputation(club);
        compare(c, r0, 0x1c52);
        scale(r0 > 0x1c52 ? 0x1d0 : 0x1f0, 0x1a8, 0x1b0);
        return;
    }
    compare(c, age, 0x26);
    if (age < 0x26) young_rules();
}

// Under 45 at a club of modest reputation: the span of the staff record's low..high values
// (at most 50) through the import's power function, scaled by the second person's rating.
void Assessment::gap_rule() {
    const auto staff = c.u32(person + person::staff);
    std::int32_t span = 0x32;
    const auto low = std::int32_t(c.i16(staff + staff::low));
    const auto high = std::int32_t(c.i16(staff + staff::high));
    const auto gap = high - low;
    c.set(5, staff);
    c.set(0, std::uint32_t(gap));
    compare(c, gap, 0x32);
    if (gap <= 0x32) span = gap;
    const double numerator = widen(0x1a8, span);
    c.set_f(1, numerator);
    c.set_f(2, constant(0x98));
    c.set({{3, flipped(span)}, {4, std::uint32_t(span)}});
    c.call(callee::power, 0x101ad9f0);
    c.set(2, c.u32(frame.at(0x14)));
    compare_logical(c, std::uint32_t(second));
    const auto rating = second ? std::int32_t(c.i8(c.u32(std::uint32_t(second) + person::staff) + 0x3d)) : 0xa;
    c.set(0, std::uint32_t(rating));
    const double other = widen(0x1b0, rating);
    const double product = fpu.fmul(other, c.f(1));
    c.set_f(1, product);
    const auto extra = narrow(0x1b8, fpu.fdiv(product, constant(0x180)));
    c.set(0, std::uint32_t(extra));
    score = std::uint32_t(value()) + std::uint32_t(extra);
}

void Assessment::second_person_manager() {
    const auto theirs = c.u32(std::uint32_t(second) + person::staff);
    const auto mine = c.u32(person + person::staff);
    c.set(5, theirs);
    c.set(3, mine);
    // The second person's attribute 0x36 widens every divisor: 0x90 - 0x238 * it.
    const double b36 = widen(0x1b8, c.i8(theirs + 0x36));
    const double factor = fpu.multiply_add(0x101adac4, constant(0x238), b36, constant(0x90));
    scale(0x28, 0x1a8, 0x1b0);
    similarity(similar_a, mine, theirs, &factor);
    c.set({{3, flipped(terms.difference[8])}, {4, std::uint32_t(terms.mine[8])}, {5, flipped(terms.difference[7])},
           {6, std::uint32_t(terms.difference[7])}, {7, std::uint32_t(terms.mine[7])}, {8, flipped(terms.out[5])},
           {9, flipped(terms.difference[6])}});
}

void Assessment::attribute_rules() {
    const auto staff = c.u32(person + person::staff);
    c.set(3, staff);
    auto current = weights(weights_a_first, staff, value());
    const auto r6 = reputation(club);
    compare(c, r6, 0x1c52);
    unsigned divisor = 0x260;
    if (r6 <= 0x1c52) {
        compare(c, r6, 0x1676);
        divisor = r6 > 0x1676 ? 0x240 : 0x30;
    }
    Weight last = weight_a_last;
    last.divisor = divisor;
    const auto entry_score = terms.start;
    current = weights(std::span(&last, 1), staff, current);
    score = std::uint32_t(weights(weights_a_personality, staff, current));
    c.set_f(1, last_x);
    c.set({{3, std::uint32_t(c.i8(person + person::personality_d))}, {4, flipped(terms.value[2])}, {5, flipped(terms.value[1])},
           {6, conversion_high}, {7, flipped(current)}, {8, flipped(entry_score)}});
}

bool Assessment::manager_rules() {
    if (among_last_sixteen()) return true;
    if (!role_rules()) return false;
    age_rules();
    compare(c, age, 0x2d);
    if (age < 0x2d) {
        const auto r0 = reputation(club);
        compare(c, r0, 0x1b58);
        if (r0 < 0x1b58) gap_rule();
    }
    compare_logical(c, std::uint32_t(second));
    if (second) second_person_manager();
    attribute_rules();
    return true;
}

void Assessment::similarity_block(std::span<const Similarity> rules, unsigned in, unsigned out) {
    compare_logical(c, std::uint32_t(second));
    if (!second) return;
    const auto theirs = c.u32(std::uint32_t(second) + person::staff);
    const auto mine = c.u32(person + person::staff);
    scale(0x28, in, out);
    similarity(rules, mine, theirs, nullptr);
    c.set({{3, std::uint32_t(terms.mine[8])}, {4, c.u8(mine + 0x42)}, {5, conversion_high}, {6, flipped(terms.difference[7])},
           {7, std::uint32_t(terms.mine[7])}, {8, flipped(terms.out[5])}, {9, std::uint32_t(terms.mine[5])},
           {10, flipped(terms.difference[4])}});
}

// The person's region record against the club's: a tier gap scales the score.
bool Assessment::region_gap_rule() {
    const auto club_reputation = reputation(club);
    c.set(0, std::uint32_t(club_reputation));
    compare(c, club_reputation, 0x128e);
    if (club_reputation <= 0x128e) return true;
    auto club_region = c.u32(club + club::region);
    c.set(4, club_region);
    if (compare_logical(c, club_region) == 0) return true;
    auto person_reference = c.u32(person + person::club_ref);
    c.set(0, person_reference);
    if (compare_logical(c, person_reference) == 0) return true;
    if (!in_region(0x101af6d0, club_region)) return true;
    const auto person_club = c.u32(person + person::club_link);
    c.set(5, person_club);
    if (compare_logical(c, person_club) != 0) {
        const auto person_club_region = c.u32(person_club + club::region);
        const auto this_region = c.u32(club + club::region);
        c.set({{3, person_club_region}, {0, this_region}});
        if (compare_logical(c, person_club_region, this_region) == 0) return true;
    }
    club_region = c.u32(club + club::region);
    c.set(4, club_region);
    compare_logical(c, person_club);
    person_reference = c.u32(person + person::club_ref);
    c.set(3, person_reference);
    const auto club_tier = c.u8(club_region + region::tier);
    auto reference_tier = c.u8(person_reference + region::tier);
    c.set({{6, club_tier}, {7, reference_tier}});
    if (person_club != 0) {
        const auto own_region = c.u32(person_club + club::region);
        c.set(3, own_region);
        if (compare_logical(c, own_region) != 0) {
            const auto own_tier = c.u8(own_region + region::tier);
            c.set(4, own_tier);
            const auto reference_signed = std::int32_t(std::int8_t(reference_tier));
            const auto own_signed = std::int32_t(std::int8_t(own_tier));
            c.set({{0, std::uint32_t(reference_signed)}, {3, std::uint32_t(own_signed)}});
            if (compare(c, own_signed, reference_signed) > 0) {
                reference_tier = own_tier;
                c.set(7, reference_tier);
            }
        }
    }
    if (compare_logical(c, person_club) != 0) {
        const auto person_club_reputation = reputation(person_club);
        c.set(0, std::uint32_t(person_club_reputation));
        compare(c, person_club_reputation, 0x1a5e);
        if (person_club_reputation >= 0x1a5e) return true;
    }
    const auto high_tier = std::int32_t(std::int8_t(reference_tier)), low_tier = std::int32_t(std::int8_t(club_tier));
    c.set({{3, std::uint32_t(high_tier)}, {0, std::uint32_t(low_tier)}});
    if (compare(c, high_tier, low_tier) >= 0) return true;
    const auto difference = high_tier - low_tier;
    c.set(3, flipped(difference));
    c.set(0, conversion_high);
    const double gap = widen(0xb8, difference);
    double factor = fpu.frsp(fpu.multiply_add(0x101af780, constant(0x238), gap, constant(pool::one)));
    fpu.fcmp(0, factor, constant(0x10));
    if (factor < constant(0x10)) {
        const auto singles = c.toc(toc::singles);
        c.set(3, singles);
        factor = double(c.f32(singles + 0x44));
    }
    c.set_f(1, constant(pool::one));  // lfd f1,0x50 before the fused multiply-add
    if (!manager_role()) return false;
    // This rule's conversion rounds to single (fsubs at 0x101af7cc) before the fmuls.
    store_conversion(0xb8, value());
    const double x = fpu.fsubs(c.f64(at(0xb8)), constant(pool::bias));
    c.set_f(1, constant(pool::bias));
    const double scaled = fpu.fmuls(x, factor);
    score = std::uint32_t(narrow(0xc0, scaled));
    return true;
}

// Reputation over 6250: the club's first (0x30) record, the person's club level and the
// staff level add or reject.
bool Assessment::unflagged_rules() {
    const auto club_reputation = reputation(club);
    c.set(5, std::uint32_t(club_reputation));
    compare(c, club_reputation, 0x1c52);
    bool special = false;
    if (club_reputation > 0x1c52) {
        const auto league = c.u32(club + club::league);
        c.set(0, league);
        if (compare_logical(c, league) != 0) {
            const auto club_region = c.u32(club + club::region);
            c.set(4, club_region);
            if (compare_logical(c, club_region) != 0) {
                const auto tier = std::int32_t(c.i8(club_region + region::tier));
                c.set(0, std::uint32_t(tier));
                compare(c, tier, 0xa);
                special = tier > 0xa;
            }
        }
    }
    if (!special) {
        const auto club_region = c.u32(club + club::region);
        c.set(4, club_region);
        if (in_region(0x101afde8, club_region)) {
            const auto job_code = std::int32_t(std::int8_t(job));
            c.set(0, std::uint32_t(job_code));
            compare(c, job_code, 9);
            if (job_code != 9) level_ratio(0x20, 0x298);
        }
        return true;
    }
    std::int32_t diff = 0;
    const auto person_club = c.u32(person + person::club_link);
    c.set(3, person_club);
    if (compare_logical(c, person_club) != 0) {
        diff = reputation(person_club) - club_reputation;
    } else {
        const auto staff_record = c.u32(person + person::staff);
        c.set(3, staff_record);
        if (compare_logical(c, staff_record) == 0) return false;
        diff = std::int32_t(c.i16(staff_record + staff::level)) - club_reputation;
    }
    c.set({{3, flipped(diff)}, {0, conversion_high}});
    compare(c, club_reputation, 0x203a);
    const double d = widen(0xb8, diff);
    const auto bonus = narrow(0xc0, fpu.fmul(constant(0x2a0), d));
    c.set_f(1, constant(pool::bias));
    c.set_f(2, constant(0x2a0));
    const auto bonus16 = std::int32_t(std::int16_t(bonus));  // extsh r0,r0 at 0x101afc10
    c.set(0, std::uint32_t(bonus16));
    score = std::uint32_t(value()) + std::uint32_t(bonus16);
    if (club_reputation > 0x203a) {
        const auto staff_record = c.u32(person + person::staff);
        c.set(3, staff_record);
        if (compare_logical(c, staff_record) != 0) {
            const auto level = std::int32_t(c.i16(staff_record + staff::level));
            c.set(0, std::uint32_t(level));
            compare(c, level, 0x109a);
            if (level < 0x109a) {
                if (in_region(0x101afc40, c.r(4))) return false;
                const auto current_club = c.u32(person + person::club_link);
                c.set(3, current_club);
                if (compare_logical(c, current_club) == 0) return false;
                const auto person_league = c.u32(current_club + club::league);
                c.set(4, person_league);
                if (compare_logical(c, person_league) == 0) return false;
                const auto club_league = c.u32(club + club::league);
                c.set(3, club_league);
                const auto person_league_rank = std::int32_t(c.i16(person_league + 0x18));
                const auto club_league_rank = std::int32_t(c.i16(club_league + 0x18));
                c.set({{4, std::uint32_t(person_league_rank)}, {0, std::uint32_t(club_league_rank)}});
                if (compare(c, person_league_rank, club_league_rank) < 0) return false;
            }
        }
    }
    const auto job_code = std::int32_t(std::int8_t(job));
    c.set(0, std::uint32_t(job_code));
    compare(c, job_code, 9);
    if (job_code == 9) return true;
    if (manager_role()) {
        const auto club_region = c.u32(club + club::region);
        c.set(4, club_region);
        if (in_region(0x101afd18, club_region)) level_ratio(0x20, 0x298);
        else level_ratio(0x10, 0x288);
    } else {
        level_ratio(0x20, 0x288);
    }
    return true;
}

// A club flagged by 0x100c7d00: the same person region shapes the score with additions.
void Assessment::club_flag_rules() {
    const auto reference = c.u32(person + person::club_ref);
    const auto region = c.u32(club + club::region);
    c.set({{0, reference}, {4, region}});
    compare_logical(c, reference, region);
    if (reference == region) {
        const auto v = value();
        c.set(3, conversion_high);
        const double a = widen(0xb8, v);
        const double scaled = fpu.fmul(constant(0x280), a);
        const double added = widen(0xc0, v + 0x7d0);
        fpu.fcmp(0, scaled, added);
        double result = scaled;
        if (!(scaled > added)) result = widen(0xc8, v + 0x7d0);
        c.set_f(1, result);
        score = std::uint32_t(narrow(0xd0, result));
    } else if (!in_region(0x101af874, region)) {
        const auto rep = reputation(club);
        const double n = widen(0xc0, rep);
        const double weight = fpu.fsub(constant(0x10), fpu.fdiv(n, constant(0x288)));
        const auto before = value();
        const double x = widen(0xb8, before);
        score = std::uint32_t(narrow(0xc8, fpu.fmul(x, weight)));
        c.set_f(1, x);
        c.set(4, flipped(before));
    } else {
        scale(0x290, 0xb8, 0xc0);
    }
}

void Assessment::reputation_rules() {
    const auto club_reputation = reputation(club);
    c.set(0, std::uint32_t(club_reputation));
    compare(c, club_reputation, 0x186a);
    if (club_reputation <= 0x186a) return;
    if (!manager_role()) {
        level_ratio(0x20, 0x288);
        return;
    }
    const auto region = c.u32(club + club::region);
    c.set(4, region);
    if (in_region(0x101af990, region)) level_ratio(0x20, 0x298);
    else level_ratio(0x10, 0x288);
}

// A person among the table's last sixteen is skipped; otherwise the age gap to the second
// person zeroes the score below 4250 and scales it by 10 up to 6750; and below 7250, a calendar
// date later than the global one scales it again.
void Assessment::calendar_rules() {
    if (!among_last_sixteen()) return;
    const auto age_gap = std::int32_t(std::int16_t(call(callee::gap, 0x101afa70, {person, std::uint32_t(second), 1})));
    c.set(0, std::uint32_t(age_gap));
    if (compare(c, age_gap, 0x109a) < 0) {
        score = 0;
    } else {
        compare(c, age_gap, 0x1a5e);
        if (age_gap < 0x1a5e) {
            const auto before = value();
            scale(0x10, 0xb8, 0xc0);
            c.set(4, flipped(before));  // this site's idiom holds the word in r4
        }
    }
    c.set(0, std::uint32_t(age_gap));
    if (compare(c, age_gap, 0x1c52) >= 0) return;
    const auto record = call(callee::calendar, 0x101afae0, {frame.at(0x40), 0x17, 6, 0x7d3, std::uint32_t(-1)});
    const auto w0 = c.u32(record), w1 = c.u32(record + 4);
    c.set({{4, w0}, {0, w1}});
    c.put32(frame.at(0x38), w0);
    const auto date_low = c.u16(date_ptr + 2);
    const auto stored_low = c.u16(frame.at(0x3a));
    c.set({{3, date_low}, {4, stored_low}});
    c.put32(frame.at(0x3c), w1);
    compare_logical(c, stored_low, date_low);
    std::uint32_t later;
    if (stored_low > date_low) {
        later = 1;
    } else if (stored_low < date_low) {
        later = 0;
    } else {
        const auto a = std::int32_t(c.i16(frame.at(0x38))), b = std::int32_t(c.i16(date_ptr));
        c.set(4, std::uint32_t(a));
        const auto x = a ^ b;
        const auto shifted = x >> 1;
        c.set(3, std::uint32_t(shifted));
        const auto difference = shifted - (x & a);
        later = std::uint32_t(difference) >> 31;
    }
    c.set(0, later);
    if (compare(c, std::int32_t(std::int8_t(later))) == 0) return;
    scale(0x10, 0xb8, 0xc0);
}

// The last rules: the statistic callee's byte, then the region category of r26.
void Assessment::closing_rules() {
    const auto region = c.u32(club + club::region);
    c.set(5, region);
    if (compare_logical(c, region) != 0) {
        const auto owner = c.toc(toc::stat_owner);
        c.set(3, owner);
        const auto category = c.u8(region + region::stats_id);
        c.set({{4, person}, {6, category}, {5, club}});
        c.set(3, c.u32(owner));
        c.set(7, 1);
        c.call(callee::statistic, 0x101afe7c);
        const auto byte = std::int32_t(std::int8_t(c.r(3)));
        c.set({{3, conversion_high}, {4, flipped(byte)}, {0, flipped(value())}});
        const double a = widen(0xb8, byte);
        const double factor = fpu.multiply_add(0x101afeb4, constant(0x70), a, constant(0x20));
        c.set_f(1, factor);
        const double x = widen(0xc0, value());
        score = std::uint32_t(narrow(0xc8, fpu.fmul(x, factor)));
    }
    if (compare_logical(c, shared_region) == 0) {
        scale(pool::zero, 0xb8, 0xc0);
        return;
    }
    const auto category = std::int32_t(c.i8(std::uint32_t(shared_region) + region::category));
    c.set(0, std::uint32_t(category));
    const auto by_club_region = [&](auto&& accept, unsigned k) {
        const auto record = c.u32(club + club::region);
        c.set(3, record);
        if (compare_logical(c, record) == 0) return;
        if (accept(record)) scale(k, 0xb8, 0xc0);
    };
    compare(c, category, 2);
    if (category == 2) {
        by_club_region([&](std::uint32_t record) {
            const auto record_category = c.u8(record + region::category);
            c.set(0, record_category);
            return compare(c, std::int32_t(record_category), 1) == 0;
        }, 0x10);
    } else if (category > 2) {
        compare(c, category, 4);
        if (category >= 4) {
            scale(0x70, 0xb8, 0xc0);
        } else {
            by_club_region([&](std::uint32_t record) {
                const auto record_category = std::int32_t(c.i8(record + region::category));
                c.set(0, std::uint32_t(record_category));
                return compare(c, record_category, 1) == 0 || compare(c, record_category, 2) == 0;
            }, pool::zero);
        }
    } else {
        compare(c, category, 1);
        if (category < 1) scale(0x70, 0xb8, 0xc0);
    }
}

bool Assessment::tail() {
    // The region shared with the person's club or previous club, else the person's reference.
    {
        bool found = false;
        for (const auto link : {person::club_link, person::previous_link}) {
            const auto linked_club = c.u32(person + link);
            c.set(3, linked_club);
            if (compare_logical(c, linked_club) == 0) continue;
            const auto linked_region = c.u32(linked_club + club::region);
            const auto club_region = c.u32(club + club::region);
            c.set({{3, linked_region}, {0, club_region}});
            if (compare_logical(c, linked_region, club_region) == 0) {
                shared_region = linked_region;
                found = true;
                break;
            }
        }
        if (!found) {
            const auto reference = c.u32(person + person::club_ref);
            c.set(0, reference);
            if (compare_logical(c, reference) != 0) shared_region = reference;
        }
    }
    if (!region_gap_rule()) return false;
    if (nonzero_byte(call(callee::club_flag, 0x101af7e8, {club}))) {
        club_flag_rules();
        reputation_rules();
        calendar_rules();
    } else if (!unflagged_rules()) {
        return false;
    }
    closing_rules();
    return true;
}

std::uint32_t Assessment::run() {
    const auto staff = c.u32(person + person::staff);
    compare_logical(c, staff);
    if (!staff) return reject();
    // The second person falls back to the club's (club+0x80), and to none without a staff record.
    bool usable = false;
    if (compare_logical(c, std::uint32_t(second)) != 0) {
        const auto second_staff = c.u32(std::uint32_t(second) + person::staff);
        usable = compare_logical(c, second_staff) != 0;
    }
    if (!usable) {
        second = c.u32(club + club::second_person);
        if (compare_logical(c, std::uint32_t(second)) != 0) {
            const auto second_staff = c.u32(std::uint32_t(second) + person::staff);
            if (compare_logical(c, second_staff) == 0) second = 0;
        } else {
            second = 0;
        }
    }
    // The role: the primary byte, else the secondary, else 11; r21 the rating's low byte.
    const auto primary = c.u8(person + person::role_primary);
    const auto rating = c.i16(person + person::rating);
    c.set({{4, primary}, {3, guest::extended(rating)}});
    c.set(0, std::uint32_t(std::int32_t(std::int8_t(primary))));
    age = std::int32_t(std::int8_t(guest::extended(rating)));
    c.set(21, std::uint32_t(age));
    if (compare(c, std::int32_t(std::int8_t(primary))) != 0) {
        role = primary;
    } else {
        const auto secondary = c.u8(person + person::role_secondary);
        c.set(3, secondary);
        c.set(0, std::uint32_t(std::int32_t(std::int8_t(secondary))));
        role = compare(c, std::int32_t(std::int8_t(secondary))) != 0 ? secondary : 0xb;
    }
    // The age gap between the person and the second person (or an empty request).
    if (compare_logical(c, std::uint32_t(second)) != 0) {  // cmplwi r24,0 (cr0 reaches the calls)
        score = std::uint32_t(std::int32_t(std::int16_t(call(callee::gap, 0x101ad13c, {person, std::uint32_t(second), 1}))));
    } else {
        struct Date { std::uint32_t slot, back; };
        for (const auto date : {Date{0x7c, 0x101ad14c}, Date{0x84, 0x101ad154}, Date{0x8c, 0x101ad15c},
                                Date{0x94, 0x101ad164}, Date{0x9c, 0x101ad16c}})
            call(callee::date_object, date.back, {at(date.slot)});
        const auto region = c.u32(club + club::region);
        c.set({{6, region}, {0, 0}});
        c.put32(at(0x58), 0);
        c.put32(at(0x54), region);
        c.put32(at(0x64), 0);
        c.put32(at(0x5c), 0);
        c.put32(at(0x58), 0);
        c.put32(at(0x6c), club);
        c.put32(at(0x48), 0);
        score = std::uint32_t(std::int32_t(std::int16_t(call(callee::gap, 0x101ad1a0, {person, at(0x48), 1}))));
    }
    // A manager-like role at a big club adds for the staff record's values.
    const auto staff_record = c.u32(person + person::staff);
    c.set(4, staff_record);
    if (compare_logical(c, staff_record) != 0 && manager_role()) {
        const auto r0 = reputation(club);
        c.set(0, std::uint32_t(r0));
        compare(c, r0, 0x109a);
        if (r0 > 0x109a) {
            c.set(3, std::uint32_t(std::int32_t(c.i16(staff_record + staff::low))));
            const auto level = std::int32_t(c.i16(staff_record + staff::level));
            c.set(0, std::uint32_t(level));
            c.set(3, c.r(3) * 0x19);
            compare(c, level, 0xcb2);
            score = std::uint32_t(value()) + c.r(3);
            if (level > 0xcb2) {
                const auto bonus = std::uint32_t(std::int32_t(c.i16(staff_record + staff::high))) * 5;
                c.set(0, bonus);
                score = std::uint32_t(value()) + bonus;
            }
        }
    }
    // At the person's own club: a squared personality byte, or a flat bonus.
    {
        const auto r0 = c.u32(person + person::club_link);
        compare_logical(c, r0, club);
        if (r0 == club) {
            if (!among_last_sixteen()) {
                const auto byte = std::int32_t(c.i8(person + person::personality_b));
                c.set(0, std::uint32_t(byte * byte));
                score = std::uint32_t(value()) + std::uint32_t(byte * byte);
            } else {
                score = std::uint32_t(value()) + 0x64;
            }
        }
    }
    // The pair callees: a floor under the score, or a flat factor.
    if (nonzero_byte(call(callee::pair_a, 0x101ad238, {person, club}))) floor_rule(0x1b8, 0x1b0, 0x1a8);
    else if (nonzero_byte(call(callee::pair_b, 0x101ad2a8, {person, club}))) scale(pool::zero, 0x1a8, 0x1b0);
    if (nonzero_byte(call(callee::other_a, 0x101ad2ec, {person, std::uint32_t(second)}))) floor_rule(0x1a8, 0x1b0, 0x1b8);
    else if (nonzero_byte(call(callee::other_b, 0x101ad35c, {person, std::uint32_t(second)}))) scale(pool::zero, 0x1a8, 0x1b0);
    // The job: 5 and 12 weigh the person's role, 6..15 weigh the second person's attributes.
    const auto code = std::int32_t(std::int8_t(job));
    const auto r0 = std::uint32_t(code) - 5;
    c.set({{3, std::uint32_t(code)}, {0, r0}});
    compare_logical(c, r0, 0xa);
    if (r0 > 0xa) return reject();
    switch (r0) {
    case 0: case 7:  // 5, 12
        if (!manager_rules()) return reject();
        break;
    case 1: case 2: case 8: case 9:  // 6, 7, 13, 14
        similarity_block(similar_b, 0xd0, 0xd8);
        score = std::uint32_t(weights(weights_b, c.u32(person + person::staff), value()));
        c.set_f(1, constant(pool::one));
        leave_weights_bc();
        break;
    case 3: case 10:  // 8, 15
        similarity_block(similar_c, 0xb8, 0xc0);
        score = std::uint32_t(weights(weights_c, c.u32(person + person::staff), value()));
        c.set_f(1, last_x);
        leave_weights_bc();
        break;
    case 4:  // 9
        similarity_block(similar_d, 0xb8, 0xc0);
        score = std::uint32_t(weights(weights_d, c.u32(person + person::staff), value()));
        c.set_f(1, last_x);
        c.set({{3, flipped(terms.value[5])}, {4, flipped(terms.out[4])}, {5, flipped(terms.value[3])},
               {6, flipped(terms.value[2])}, {7, flipped(terms.value[1])}});
        break;
    case 5:  // 10
        score = std::uint32_t(weights(weights_e, c.u32(person + person::staff), value()));
        c.set_f(1, last_weight);
        c.set({{3, flipped(terms.start)}, {4, flipped(terms.value[4])}, {5, flipped(terms.value[3])},
               {6, flipped(terms.value[2])}, {7, flipped(terms.value[1])}});
        break;
    default: return reject();  // 11
    }
    if (!tail()) return reject();
    c.set(3, std::uint32_t(value()));
    return exit_with(value());
}
}  // namespace

std::uint32_t assess(Cpu& c, std::uint32_t person, std::uint32_t club, std::uint32_t second, std::uint32_t job) {
    c.set(0, c.lr());
    guest::Frame frame(c, 0x101ad080, 0x1f0, {.first_gpr = 21});
    Assessment assessment(c, frame, person, club, second, job);
    return assessment.run();
}
}  // namespace cm::aot::manager_manager
