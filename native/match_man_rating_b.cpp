// MATCH_MAN.CPP rating_b block (match_man_rating_b.hpp): 0x10216830.
#include "match_man_rating_b.hpp"
#include "match_man_adapter.hpp"
#include "match_man_rating_b_parts.hpp"
#include "match_man_rating_b_fit.hpp"
#include <optional>

namespace cm::aot::match_man {
namespace {
using namespace appraisal;
using guest::compare;
using guest::compare_logical;
using guest::Kept;
using guest::nonzero_logical;

// The stack slots of 0x10216830 (offsets from its stack pointer).
namespace slot_of {
constexpr u32 convert_a = 0x80, convert_b = 0x88, convert_c = 0x90, convert_d = 0x98, result_a = 0xa0;
constexpr u32 ceiling = 0xb0;   // 16-bit: 100, or the game's value / 100
constexpr u32 limit = 0xb2;     // 16-bit: 10000, or the game's value
constexpr u32 estimate = 0xb4;  // 16-bit: the judge's estimate
constexpr u32 judged = 0xb6;    // 16-bit: -1, or the judged current ability
constexpr u32 flag_4 = 0xb8;    // flags & 4
constexpr u32 flag_800 = 0xbc;  // flags & 0x800
constexpr u32 player_flags = 0xc0;  // 0x10117490's result
}
constexpr i32 rejected = -0x2710;
constexpr u32 pool_conversion = 0x90;  // 2^52 + 2^31

// What the routine keeps in r13-r31 while it works, and its phases. The original
// reuses r13, r14, r16, r19-r22 as temporaries in places; they are assigned there.
struct Appraisal {
    Cpu& c;
    Fpu& fpu;
    Frame& frame;
    Kept<13> r13;        // the table of values by rating (TOC -0x5290); later flags & 0x100, and temporaries
    Kept<14> judge;      // the judging subject (the argument, or the club's default judge)
    Kept<15> subject;
    Kept<16> role;       // the positions asked for
    Kept<17> flags;      // the components asked for
    Kept<18> club;
    Kept<19> floor;      // the minimum for the running average
    Kept<20> judge_arg;
    Kept<21> age;        // the subject's age as a signed byte
    Kept<22> has_floor;  // 1 when a floor was given
    Kept<23> total;      // sum of the judged abilities
    Kept<24> count;      // how many were summed
    Kept<25> staff;      // the judge's staff data
    Kept<26> position;   // the value of the position (and of the subject as a player)
    Kept<27> modifier;   // the adjustments
    Kept<28> player_data;
    Kept<29> singles;    // pool of single constants (TOC -0x527c)
    Kept<30> doubles;    // pool of double constants (TOC -0x5278)
    Kept<31> relation;   // 0x10082f90's record of subject and club, or 0
    Kept<14>& adjustment = judge;  // r14 again: the staff and relation blocks use it as a running adjustment
    Kept<16>& share = role;        // r16 again: a percentage the relation lookups leave
    double f31_start;

    Appraisal(Cpu& cpu, Fpu& f, Frame& fr, u32 person_arg, u32 role_arg, u32 flags_arg, u32 club_arg, i32 floor_arg, u32 judge_in)
        : c(cpu), fpu(f), frame(fr),
          r13(cpu, cpu.toc(slot::value_table)), judge(cpu, cpu.r(14)), subject(cpu, person_arg), role(cpu, role_arg),
          flags(cpu, flags_arg), club(cpu, club_arg), floor(cpu, reg(floor_arg)), judge_arg(cpu, judge_in),
          age(cpu, cpu.r(21)), has_floor(cpu, 0), total(cpu, 0), count(cpu, 0), staff(cpu, cpu.r(25)),
          position(cpu, 0), modifier(cpu, 0), player_data(cpu, cpu.r(28)), singles(cpu, cpu.toc(-0x527c)),
          doubles(cpu, cpu.toc(-0x5278)), relation(cpu, cpu.r(31)), f31_start(cpu.f(31)) {}

    // Constants: the pool at r30 (doubles) and r29 (singles, lfs).
    double D(u32 offset) const { return c.f64(doubles + offset); }
    double F(u32 offset) const { return double(c.f32(singles + offset)); }
    double conversion() const { return D(pool_conversion); }
    u32 at(u32 offset) const { return frame.at(offset); }
    // Signed bytes of the player data and of the judge's staff data.
    i32 stat(u32 offset) const { return s8(c.u8(player_data + offset)); }
    u32 stat_raw(u32 offset) const { return c.u8(player_data + offset); }
    i32 staff_stat(u32 offset) const { return s8(c.u8(staff + offset)); }
    // rlwinm. r0,r17,0,mb,mb: tests a component bit and sets cr0.
    bool wants(u32 mask) const { return compare(c, i32(u32(flags) & mask), 0) != 0; }
    // rlwinm. r0,r16,... (the role register as the original reads it).
    i32 divw(i32 a, i32 b) const { return i32(u32(divide_signed<4>(reg(a), reg(b)))); }
    // The subject is a record of the people table (index below the count): `r4 = [r15]; cmpw r4,[count]`.
    bool in_people_table() const {
        if (!nonzero_logical(c, subject)) return true;
        const u32 count_at = c.toc(slot::people_count);
        const i32 index = c.i32(subject), people = c.i32(count_at);
        c.set({{3, count_at}, {4, reg(index)}, {0, reg(people)}});
        compare(c, index, people);
        return index < people;
    }
    // The call to the game's object (TOC slot -0x7ba0) with the subject.
    u32 game() const { return c.toc(slot::game); }
    u32 world() const { return c.toc(slot::world); }
    // "extsb. r0,r22; mulli r3,r23,0xc8; extsb r0,r24; divw r0,r3,r0; cmpw r0,r19; bge": the
    // running average (200 times the sum over the count) below the floor ends the routine.
    bool below_floor(i32 divisor) {
        compare(c, s8(has_floor), 0);
        if (!s8(has_floor)) return false;
        const i32 scaled = i32(reg(i32(total)) * 200);
        c.set(3, reg(scaled));
        const i32 average = divw(scaled, divisor);
        compare(c, average, i32(floor));
        return average < i32(floor);
    }
    bool below_floor_by_count() { return below_floor(s8(count)); }
    // The number of the judge's estimates so far: extsb r24.
    i32 counted() const { return s8(count); }

    std::optional<i32> begin();
    void load_context();
    void position_value();
    std::optional<i32> averages();
    void trends();
    void form();
    void overall();
    bool inside_window(const DateSlots& slots);
    double narrow(u32 site, double noise, i32 days);
    void add_scaled(u32 site, double a, double trend, double b, bool single, bool extend, bool mixed_in_f1);
    i32 age_days_leaving(u32 person_record, bool in_form);
    void add_bounded_trunc(double mixed);
    void development();
    void nation_bias();
    void scale_modifier(u32 pool_offset, bool pair_in_r3_r0 = true);
    void club_factor();
    void scale_by_age();
    void scale_single(double factor);
    std::optional<i32> market();
    void staff_fit();
    void fit_groups();
    void after_groups();
    void foreign_penalty();
    void relation_block();
    void club_relation_block();
    std::optional<i32> relation_share(u32 first_back, u32 second_back, u32 third_back);
    void ending();
    i32 finish();
};

std::optional<i32> Appraisal::begin() {
    compare(c, i32(subject), 0);  // or. r15,r3,r3
    c.put16(at(slot_of::judged), 0xffff);
    c.put16(at(slot_of::limit), 10000);
    c.put16(at(slot_of::ceiling), 100);
    if (!subject) return guest::result(c, reg(rejected));
    player_data = c.u32(subject + appraisal::person::player);
    if (!nonzero_logical(c, player_data)) return guest::result(c, reg(rejected));
    return std::nullopt;
}

void Appraisal::load_context() {
    const u32 anchor_flag = u32(flags) & component::anchor;
    compare(c, i32(anchor_flag), 0);
    c.put32(at(slot_of::flag_800), anchor_flag);
    // The game's own limits (0x101878d0) are asked unless the anchor flag alone is set.
    bool ask = !anchor_flag;
    if (!ask) {
        ask = compare(c, i32(u32(flags) & bit(0xa)), 0) != 0;
        if (!ask) ask = compare(c, i32(u32(flags) & bit(0xf)), 0) != 0;
    }
    if (ask) {
        c.set({{3, c.u32(game())}, {4, subject}, {5, 0}});
        c.call(0x101878d0, 0x102168dc);
        const i32 limit = s16(c.r(3));
        c.put16(at(slot_of::limit), reg(limit));
        c.put16(at(slot_of::ceiling), reg(magic_div(s16(c.u16(at(slot_of::limit))), 0x51eb851f, 5)));  // / 100
    }
    age = reg(s8(u32(c.i16(subject + appraisal::person::age))));
    const u32 lookups = c.u32(c.toc(slot::lookups));
    if (nonzero_logical(c, lookups)) {
        c.set({{3, lookups}, {4, subject}, {5, club}});
        c.call(0x10082f90, 0x1021692c);
        relation = c.r(3);
    } else {
        relation = 0;
    }
    if (compare(c, i32(floor), -1) > 0) has_floor = 1;
    // The judge: the argument, else the club's default judge.
    if (nonzero_logical(c, judge_arg)) {
        staff = c.u32(judge_arg + appraisal::person::staff);
        judge = judge_arg;
    } else if (nonzero_logical(c, club)) {
        const u32 chosen = c.u32(club + club::default_judge);
        if (nonzero_logical(c, chosen)) {
            staff = c.u32(chosen + appraisal::person::staff);
            judge = chosen;
        } else {
            staff = 0;
            judge = 0;
        }
    } else {
        staff = 0;
        judge = 0;
    }
    c.set({{3, subject}, {4, judge}, {5, 0}});
    c.call(callee::estimate, 0x10216990);
    c.put16(at(slot_of::estimate), c.r(3));
    c.set(3, player_data);
    c.call(callee::rating_flags, 0x102169a0);
    c.put32(at(slot_of::player_flags), c.r(3));
    if (compare(c, s16(c.r(3)), 0) == 0) position = reg(-100);
}

// The value of the positions asked for (the role mask) and, for components the
// original tests next, how it is adjusted. Each position reads a different rating
// byte of the player data; a position the player is bad at is taken from a
// neighbouring one, and the table at TOC -0x5290 turns a rating into a value.
void Appraisal::position_value() {
    const auto value_at = [&](i32 index) { return value_of_rating(c, r13, index); };
    const auto add_value = [&](i32 index) { position = u32(position) + value_at(index); };
    const u32 positions = reg(s16(role));
    if (!wants(bit(0x15))) {
        const i32 flags_word = i32(c.u32(at(slot_of::player_flags)));
        c.set(3, positions & 1);
        if (positions & 1) {
            // Goalkeepers: the square of the goalkeeper rating, less 300.
            const i32 keeping = stat(0xf);
            position = reg(keeping * keeping) + u32(position) - 0x12c;
        } else if (c.set(3, u32(flags_word) & 1), flags_word & 1) {
            position = u32(position) - 200;
        } else if (c.set(3, positions & 2), positions & 2) {
            add_value(stat(0x10));
        } else if (c.set(3, positions & 4), positions & 4) {
            add_value(stat(0x11));
        } else if (c.set(4, positions), s16(role) == 8) {
            // The best of four neighbouring ratings; r3 holds the best before the last compare,
            // r4 the last rating, r5 its raw byte.
            u32 best = stat_raw(0x16);
            u32 raw = 0;
            for (const u32 offset : {0x11u, 0x12u, 0x13u}) {
                raw = stat_raw(offset);
                c.set({{3, reg(s8(best))}, {4, reg(s8(raw))}, {5, raw}});
                if (s8(raw) > s8(best)) best = raw;
            }
            add_value(s8(best));
        } else if (c.set(3, positions & 8), positions & 8) {
            c.set(3, positions & bit(0x14));
            add_value(positions & (bit(0x14) | bit(0x18)) ? stat(0x16) : stat(0x12));
        } else if (c.set(3, positions & 0x10), positions & 0x10) {
            c.set(3, positions & bit(0x14));
            if (positions & (bit(0x14) | bit(0x18))) {
                const u32 first = stat_raw(0x13), second = stat_raw(0x14);
                u32 chosen = first;
                c.set({{5, first}, {4, second}, {3, reg(s8(second))}});
                if (s8(second) > s8(first)) {
                    chosen = second;
                    c.set(5, second);
                }
                add_value(s8(chosen));
            } else if (s16(role) == 0x10) {
                const u32 low = stat_raw(0x12), high = stat_raw(0x14);
                u32 best = low;  // r4
                c.set({{6, low}, {5, high}, {4, low}, {3, reg(s8(high))}});
                if (s8(high) > s8(low)) best = high;
                if (s8(high) > s8(low)) c.set(4, high);
                const u32 third = stat_raw(0x13);  // r7
                c.set({{7, third}, {3, reg(s8(best))}});
                u32 result;
                if (s8(best) > s8(third)) {
                    c.set(3, reg(s8(high)));
                    result = s8(high) > s8(low) ? high : low;
                } else {
                    result = third;
                }
                c.set(6, result);
                add_value(s8(result));
            } else {
                // The wing-back case: the last two ratings count five less. extsb of
                // the sum can wrap for a rating of -128 or below: kept.
                const i32 a = stat(0x14) - 5, b = stat(0x13), d = stat(0x12) - 5;
                const u32 first_raw = stat_raw(0x14);
                c.set({{3, reg(s8(first_raw))}, {5, reg(a)}, {6, reg(b)}, {4, reg(b)}});
                i32 best = b;
                if (a > b) {
                    best = a;
                    c.set(4, reg(a));
                }
                c.set(3, reg(s8(stat_raw(0x12))));
                i32 result;
                if (best > d) result = a > b ? a : b;
                else result = d;
                c.set(6, reg(result));
                add_value(s8(reg(result)));
            }
        } else if (c.set(3, positions & 0x20), positions & 0x20) {
            c.set(3, positions & bit(0x14));
            if (positions & (bit(0x14) | bit(0x18))) {
                add_value(stat(0x14));
            } else {
                // The average-type value: a ratio of two ratings from the pool.
                const i32 first = stat(0x14), sum = first + stat(0x1a);
                c.set({{4, reg(first) ^ 0x80000000u}, {3, 0x43300000}});
                const double sum_value = fpu.fsub(store_pair(c, at(0xa0) + 0, sum), conversion());
                const double scaled = fpu.fmul(D(0x210), sum_value);
                const double first_value = fpu.fsub(store_pair(c, at(0xa8), first), conversion());
                double chosen = scaled;
                c.set_f(1, scaled);
                if (fcompare(fpu, first_value, scaled).greater) {
                    store_pair(c, at(0x98), first);
                    chosen = fpu.fsub(c.f64(at(0x98)), conversion());
                    c.set_f(1, chosen);
                }
                add_value(i32(truncate_at(c, fpu, at(0x90), chosen)));
            }
        } else if (positions & 0x40) {
            add_value(stat(0x15));
        } else {
            position = u32(position) + 100;
        }
    } else {
        position = u32(position) + 100;
    }
    // The second adjustment, by the role's other bits: ratings above ten, in tens.
    if (wants(bit(0x16))) {
        position = u32(position) + 100;
        return;
    }
    c.set(3, positions);
    const auto above_ten = [&](i32 rating) { return (rating - 10) * 10; };
    if (positions & bit(0x13)) {
        c.set(3, reg(stat(0x1a)));
        position = u32(position) + reg(above_ten(stat(0x1a)));
    } else if (positions & bit(0x18)) {
        c.set(3, reg(stat(0x18)));
        position = u32(position) + reg(above_ten(stat(0x18)));
    } else if (positions & bit(0x17)) {
        const i32 better = std::max(half(stat(0x18)), stat(0x19));
        c.set({{4, reg(better)}, {3, reg(stat(0x30))}});
        position = reg(above_ten(better) + stat(0x30)) + u32(position) - 20;
    } else if (positions & bit(0x16)) {
        c.set(3, reg(stat(0x19)));
        position = u32(position) + reg(above_ten(stat(0x19)));
    } else if (positions & bit(0x15)) {
        const i32 better = std::max(half(stat(0x17)), stat(0x19));
        c.set({{4, reg(better)}, {3, reg(stat(0x3b))}});
        position = reg(above_ten(better) + stat(0x3b)) + u32(position) - 20;
    } else if (positions & bit(0x14)) {
        c.set(3, reg(stat(0x17)));
        position = u32(position) + reg(above_ten(stat(0x17)));
    } else {
        position = u32(position) + 100;
    }
}

// The running estimate of the player's ability: each component asked for adds a
// judged value to `total` and one to `count`. With a floor given, the routine
// gives up (-10000) the moment the average falls below it.
std::optional<i32> Appraisal::averages() {
    constexpr u32 rejected_result = reg(rejected);
    // The player's own estimate of the judged ability (the first component).
    if (wants(component::dated_current)) {
        count = 1;
        const i32 own = c.i16(player_data + 0xa);
        total = reg(magic_div(own, 0x51eb851f, 4));  // / 50
        c.set({{4, reg(own)}, {3, own < 0 ? 1u : 0u}});
        const u32 person_club = c.u32(subject + appraisal::person::club);
        compare_logical(c, club, person_club);
        if (club == person_club && nonzero_logical(c, judge_arg) &&
            judge_date_window(c, frame, subject, judge_arg, {0x60, 0x78, 0x58, 0x10216ebc})) {
            // A person young enough: the estimate shrinks with age (in days).
            const i32 days = age_in_days(c, subject);
            const u32 today = c.toc(slot::date);
            c.set({{3, reg(days)}, {4, 0x43300000}, {5, u32(total) ^ 0x80000000u},
                   {6, reg(c.i16(subject + appraisal::person::birth_day))},
                   {7, c.u16(today + date_year) - c.u16(subject + appraisal::person::birth_year)}});
            const double age_value = fpu.fsubs(store_pair(c, at(0x98), days), conversion());
            const double total_before = store_pair(c, at(0x90), i32(u32(total)));
            const double factor = fpu.multiply_add(0x10216f6c, D(0x2f0), age_value, D(0x2e8));
            const double rounded = fpu.frsp(factor);
            const double total_value = fpu.fsubs(total_before, conversion());
            c.set_f(1, total_value);
            total = truncate_at(c, fpu, at(0xa0), fpu.fmuls(total_value, rounded));
        }
        if (below_floor(1)) return guest::result(c, rejected_result);
    }
    if (wants(bit(0x1b))) {
        const u32 nation = c.u32(subject + appraisal::person::nation);
        c.set({{3, nation}, {4, 0x43300000}});
        compare(c, s8(has_floor), 0);
        count = u32(count) + 1;
        const i32 rating = c.i16(player_data + 0x8);
        c.set({{5, reg(rating)}, {3, reg(rating) ^ 0x80000000u}});
        const double reputation = fpu.fsub(store_pair(c, at(0x98), c.i16(nation + 0xac)), conversion());
        const double ratio = fpu.fdiv(reputation, D(0x140));
        const double rating_value = fpu.fsub(store_pair(c, at(0x90), rating), conversion());
        const double divisor = fpu.fmul(D(0x1b8), fpu.fsub(D(0x438), ratio));
        c.set_f(1, rating_value);
        total = u32(total) + truncate_at(c, fpu, at(0xa0), fpu.fdiv(rating_value, divisor));
        if (below_floor_by_count()) return guest::result(c, rejected_result);
    }
    if (wants(bit(0x1a))) {
        compare(c, s8(has_floor), 0);
        count = u32(count) + 1;
        const i32 rating = c.i16(player_data + 0xc);
        c.set({{4, reg(rating)}, {3, rating < 0 ? 1u : 0u}});
        total = u32(total) + reg(magic_div(rating, 0x51eb851f, 4));  // / 50
        if (below_floor_by_count()) return guest::result(c, rejected_result);
    }
    const u32 flag_4 = u32(flags) & bit(0x1d);
    compare(c, i32(flag_4), 0);
    c.set(0, flag_4);
    c.put32(at(slot_of::flag_4), flag_4);
    if (flag_4 && in_people_table()) {
        c.set_f(1, F(0x8));
        c.set({{3, c.u32(world())}, {4, player_data}, {5, 5}, {6, 0x12}});
        c.call(0x10303560, 0x102170dc);
        const double product = fpu.fmul(D(0xc0), c.f(1));
        const i32 value = i32(truncate_at(c, fpu, at(slot_of::convert_c), product));
        c.set(0, reg(value));
        if (compare(c, value, 0) != 0) {
            total = u32(total) + reg(value);
            count = u32(count) + 1;
        }
    }
    if (wants(bit(0x18))) {
        if (compare(c, counted(), 0) != 0) {
            c.set(3, reg(i32(reg(i32(total)) * 50)));
            c.put16(at(slot_of::estimate), reg(divw(i32(reg(i32(total)) * 50), counted())));
        }
        c.set({{7, reg(c.i16(at(slot_of::estimate)))}, {3, judge}, {4, subject}, {5, club}, {6, 2}});
        match_man::call(c, callee::judged_ability, 0x1021713c);
        c.put16(at(slot_of::judged), c.r(3));
        compare(c, s8(has_floor), 0);
        count = u32(count) + 1;
        total = u32(total) + reg(c.i16(at(slot_of::judged)));
        if (below_floor_by_count()) return guest::result(c, rejected_result);
    }
    r13 = u32(flags) & bit(0x17);
    compare(c, i32(r13), 0);
    if (r13) {
        if (compare(c, counted(), 0) != 0) {
            c.set({{3, c.u32(c.toc(slot::lookups))}, {4, subject}});
            c.call(0x10082f20, 0x10217190);
            const u32 found = c.r(3);
            compare_logical(c, found, 0);
            c.set(4, c.toc(slot::last_relation));
            c.put32(c.r(4), found);
            u32 found_club = 0;
            if (found) {
                const u32 clubs = c.u32(c.toc(slot::clubs));
                c.set(3, clubs);
                found_club = clubs + c.u32(found + 4) * 0x248;
            }
            compare_logical(c, found_club, club);
            if (found_club == club) {
                const i32 quotient = divw(i32(reg(i32(total)) * 25), counted());
                const u32 data = c.u32(subject + appraisal::person::player);
                c.set({{4, reg(quotient)}, {3, data}});
                const i32 current = c.i16(data + 6);
                c.put16(at(slot_of::estimate), reg(quotient) + reg(current * 25));
            } else {
                c.set(3, reg(i32(reg(i32(total)) * 50)));
                c.put16(at(slot_of::estimate), reg(divw(i32(reg(i32(total)) * 50), counted())));
            }
        }
        c.set({{7, reg(c.i16(at(slot_of::estimate)))}, {3, judge}, {4, subject}, {5, club}, {6, 1}});
        match_man::call(c, callee::judged_ability, 0x10217218);
        i32 judged = s16(c.r(3));
        const i32 earlier = c.i16(at(slot_of::judged));
        c.set(4, reg(earlier));
        if (compare(c, earlier, -1) > 0 && compare(c, judged, earlier) < 0) {
            judged = earlier;
            c.set(3, reg(earlier));
        }
        compare(c, s8(has_floor), 0);
        total = u32(total) + reg(judged);
        count = u32(count) + 1;
        if (below_floor_by_count()) return guest::result(c, rejected_result);
    }
    return std::nullopt;
}

// total += trunc(count * (a * trend - b)): the count's conversion words at 0x90, the result at
// 0x98. `single` for the fmsubs/fsubs/fmuls form, `extend` for the extsb of the result.
// The person's age in days with the registers the trends site (r3 days, r4 the exponent word, r5 the
// person's day, r6 the years' difference) or the form site (r3..r9, below) leave behind.
i32 Appraisal::age_days_leaving(u32 person_record, bool in_form) {
    const i32 days = age_in_days(c, person_record);
    const u32 today = c.toc(slot::date);
    const u32 years = c.u16(today + date_year) - c.u16(person_record + appraisal::person::birth_year);
    const u32 person_day = reg(c.i16(person_record + appraisal::person::birth_day));
    if (!in_form) {
        c.set({{3, reg(days)}, {4, 0x43300000}, {5, person_day}, {6, years}});
    } else {
        c.set({{3, reg(magic_div(stat(0xf), 0x66666667, 2))}, {4, reg(days) ^ 0x80000000u}, {5, player_data}, {6, 0x43300000},
               {7, reg(c.i16(today + date_day))}, {8, person_day}, {9, reg(days)}});
    }
    return days;
}

void Appraisal::add_scaled(u32 site, double a, double trend, double b, bool single, bool extend, bool mixed_in_f1) {
    c.set(3, reg(counted()) ^ 0x80000000u);
    const double pair = store_pair(c, at(0x90), counted());
    const double mixed = fpu.multiply_add(site, a, trend, b);
    const double count_value = single ? fpu.fsubs(pair, conversion()) : fpu.fsub(pair, conversion());
    const double product = single ? fpu.fmuls(mixed, count_value) : fpu.fmul(mixed, count_value);
    c.set_f(1, mixed_in_f1 ? mixed : conversion());
    const u32 low = truncate_at(c, fpu, at(slot_of::convert_d), product);
    total = u32(total) + (extend ? reg(s8(low)) : low);
}

// The club is the subject's own and the judge's date lies within the window (the original's
// inlined date checks): the subject is still developing.
bool Appraisal::inside_window(const DateSlots& slots) {
    return u32(club) == c.u32(subject + appraisal::person::club) && nonzero_logical(c, judge_arg) &&
           judge_date_window(c, frame, subject, judge_arg, slots);
}

// The noise value mixed with the subject's age in days: (0x2f8 * (0x300 - age) + noise * age) / 0x300.
double Appraisal::narrow(u32 site, double noise, i32 days) {
    const double first = fpu.fsubs(store_pair(c, at(0x90), days), conversion());
    const double second = fpu.fsubs(store_pair(c, at(0x98), days), conversion());
    const double flipped = fpu.fsub(D(0x300), first);
    c.set_f(1, flipped);
    const double mixed = fpu.fmuls(noise, second);
    return fpu.frsp(fpu.fdiv(fpu.multiply_add(site, D(0x2f8), flipped, mixed), D(0x300)));
}

// The world's measure of the player's development (0x10303090): added in proportion to the count.
void Appraisal::trends() {
    if (wants(bit(0xd)) && in_people_table()) {
        c.set({{3, c.u32(world())}, {4, player_data}});
        c.call(0x10303090, 0x10217298);
        const i32 trend = s8(c.r(3));
        const double trend_value = fpu.fsub(store_pair(c, at(0x90), trend), conversion());
        c.set_f(1, trend_value);
        double f31 = fpu.frsp(fpu.fdiv(trend_value, D(0x100)));
        c.set_f(31, f31);
        compare_logical(c, club, c.u32(subject + appraisal::person::club));
        if (inside_window({0x50, 0x70, 0x48, 0x10217344})) f31 = narrow(0x10217400, f31, age_days_leaving(subject, false));
        c.set_f(31, f31);
        if (fcompare(fpu, f31, D(0x308)).greater)
            add_scaled(0x1021743c, D(0x310), f31, D(0x318), false, true, false);
        else if (fcompare(fpu, f31, D(0x178)).greater)
            add_scaled(0x10217490, D(0x320), f31, D(0x328), false, true, false);
    }
}

// The judge's feel for the player's form (0x10303560, in three measures), scaling the total by
// the count; the home club's judge also corrects it by the subject's age.
void Appraisal::form() {
    if (!wants(bit(0x1e)) || !in_people_table()) return;
    compare(c, counted(), 0);
    c.set_f(1, F(0x20));
    // The judge's own estimate over 150, as a single: 0x20 plus the excess over 0x24, capped by 0x330.
    const auto bounded = [&](i32 excess) {
        const double scaled = fpu.fdivs(fpu.fsubs(store_pair(c, at(0x90), excess), conversion()), F(0x24));
        const double sum = fpu.fadds(F(0x20), scaled);
        double capped = D(0x330);
        if (!fcompare(fpu, capped, sum).less) capped = sum;
        c.set_f(1, fpu.frsp(capped));
    };
    bool measured = false;
    if (counted() != 0) {
        const i32 average = divw(i32(total), counted());
        compare(c, average, 0x96);
        if (average > 0x96) {
            bounded(average - 0x96);
            measured = true;
        }
    }
    if (!measured) {
        const i32 estimate = c.i16(at(slot_of::estimate));
        compare(c, estimate, 0x96);
        if (estimate > 0x96) bounded(estimate - 0x96);
    }
    c.set({{3, c.u32(world())}, {4, player_data}, {5, 6}, {6, 0x12}});
    c.call(0x10303560, 0x102175a4);
    double f31 = c.f(1);
    c.set_f(31, f31);
    compare_logical(c, club, c.u32(subject + appraisal::person::club));
    if (inside_window({0x40, 0x68, 0x38, 0x10217628})) {
        f31 = narrow(0x10217710, f31, age_days_leaving(subject, true));
        c.set_f(31, f31);
        // Goalkeepers peak later: the threshold age is 34 plus a tenth of the goalkeeper rating.
        const i32 threshold = magic_div(stat(0xf), 0x66666667, 2) + 0x22;  // rating / 10
        compare(c, s8(age), threshold);
        if (s8(age) > threshold) {
            const u32 object = c.u32(world());
            const double length = c.f64(object + 0x18);
            const auto ratio = [&] {
                return fcompare(fpu, length, D(0x178)).equal ? D(0x308) : fpu.fdiv(c.f64(object + 0x10), length);
            };
            c.set_f(1, length);
            if (fcompare(fpu, f31, ratio()).greater) {
                const double single_ratio = fpu.frsp(ratio());
                c.set_f(1, single_ratio);
                f31 = fpu.multiply_add(0x1021777c, f31, F(0x28), fpu.fmuls(single_ratio, F(0x28)));
                c.set_f(31, f31);
            }
        }
    }
    if (fcompare(fpu, f31, D(0x340)).greater)
        add_scaled(0x102177b0, F(0x2c), f31, F(0x30), true, true, false);
    else if (fcompare(fpu, f31, D(0x308)).greater)
        add_scaled(0x10217804, D(0x348), f31, D(0x248), false, true, false);
    else if (fcompare(fpu, f31, D(0x178)).greater)
        add_scaled(0x10217858, F(0x2c), f31, F(0x30), true, true, false);
    // The club's own player (or a countryman of its club's) is judged again with the club's bias.
    if (!nonzero_logical(c, club)) return;
    const u32 person_club = c.u32(subject + appraisal::person::club);
    compare_logical(c, club, person_club);
    if (club != person_club) {
        if (!nonzero_logical(c, person_club)) return;
        const u32 nation_a = c.u32(club + club::nation), nation_b = c.u32(person_club + club::nation);
        compare_logical(c, nation_a, nation_b);
        if (nation_a != nation_b) return;
    }
    f31 = fpu.frsp(fpu.fmul(D(0x198), f31));
    c.set_f(31, f31);
    c.set_f(1, f31);
    c.set({{3, c.u32(world())}, {4, player_data}, {5, 1}, {6, 0x12}});
    c.call(0x10303560, 0x102178cc);
    const double measure = c.f(1);
    if (fcompare(fpu, measure, D(0x308)).greater) {
        add_scaled(0x102178fc, D(0x320), measure, D(0x350), false, true, true);
    } else if (fcompare(fpu, measure, f31).less) {
        c.set(3, reg(counted()) ^ 0x80000000u);
        const double count_pair = store_pair(c, at(0x90), counted());
        const double scaled = fpu.fdivs(fpu.fmuls(F(0x34), measure), f31);
        const double count_value = fpu.fsubs(count_pair, conversion());
        const double excess = fpu.fsubs(scaled, F(0x34));
        c.set_f(1, count_value);
        total = u32(total) + reg(s8(truncate_at(c, fpu, at(slot_of::convert_d), fpu.fmuls(count_value, excess))));
    }
    c.set_f(1, F(0x50));
    c.set({{3, c.u32(world())}, {4, player_data}, {5, 6}, {6, 0x14}});
    c.call(0x10303560, 0x1021798c);
    const double mixed = fpu.multiply_add(0x102179a4, D(0xe0), c.f(1), D(0x358));
    c.set(3, reg(counted()) ^ 0x80000000u);
    const double count_value = fpu.fsubs(store_pair(c, at(0x90), counted()), conversion());
    c.set_f(1, conversion());
    const double narrowed = fpu.frsp(mixed);
    total = u32(total) + truncate_at(c, fpu, at(slot_of::convert_d), fpu.fmuls(narrowed, count_value));
}

// The average of the judged abilities becomes the modifier: beyond 150 each point counts 0x20 more.
void Appraisal::overall() {
    compare(c, counted(), 0);
    if (counted() != 0) {
        const i32 average = divw(i32(total), counted());
        compare(c, average, 0x96);
        modifier = reg(average);
        if (average > 0x96) {
            const double excess = fpu.fsub(store_pair(c, at(0x90), average - 0x96), conversion());
            c.set_f(1, excess);
            modifier = truncate_at(c, fpu, at(slot_of::convert_d), fpu.multiply_add(0x10217a14, D(0x20), excess, D(0x360)));
        }
    }
    if (wants(bit(0x1c)) && in_people_table()) {
        const double rating = fpu.fsub(store_pair(c, at(0x90), stat(0x45)), conversion());
        c.set_f(1, rating);
        modifier = u32(modifier) + truncate_at(c, fpu, at(slot_of::convert_d), fpu.multiply_add(0x10217a74, D(0x20), rating, D(0x8)));
    }
}

// modifier = trunc(pool[offset] * modifier): the conversion words at 0x80, the result at 0x88.
void Appraisal::scale_modifier(u32 pool_offset, bool pair_in_r3_r0) {
    if (pair_in_r3_r0) c.set({{3, u32(modifier) ^ 0x80000000u}, {0, 0x43300000}});
    const double value = fpu.fsub(store_pair(c, at(slot_of::convert_a), i32(u32(modifier))), conversion());
    c.set_f(1, conversion());
    modifier = truncate_at(c, fpu, at(slot_of::convert_b), fpu.fmul(D(pool_offset), value));
}

// What a younger or older, taller or smaller player is worth to the club: the modifier by the
// age, the club's reputation, the player's own ratings and the round-trip of his nation.
void Appraisal::development() {
    if (wants(bit(0x11))) {
        const i32 gap = magic_div(7000 - c.i16(club + club::reputation), 0x51eb851f, 5);  // / 100
        const i32 reputation_limit = s8(reg(gap >= 10 ? gap : 10));
        const u32 data = c.u32(subject + appraisal::person::player);
        c.set(5, data);
        i32 threshold;
        if (c.u8(data + 0xf) == 20) threshold = 0x24;
        else if (c.u8(data + 0x15) == 20 || c.u8(data + 0x14) == 20) threshold = 0x22;
        else threshold = 0x23;
        threshold = s8(reg(threshold));
        compare(c, s8(age), threshold);
        if (s8(age) > threshold) {
            const i32 excess = s8(age) - threshold;
            c.set(5, 0x43300000);
            const double excess_value = fpu.fsub(store_pair(c, at(slot_of::convert_d), excess), conversion());
            const double limit_value = fpu.fsub(store_pair(c, at(slot_of::result_a), reputation_limit), conversion());
            const double modifier_value = fpu.fsub(store_pair(c, at(slot_of::convert_c), i32(u32(modifier))), conversion());
            const i32 less_five = i32(u32(modifier)) - 5;
            const double less_value = fpu.fsub(store_pair(c, at(0xa8), less_five), conversion());
            const double ratio = fpu.fdiv(excess_value, limit_value);
            double chosen = fpu.fmul(modifier_value, fpu.fsub(D(0x0), ratio));
            c.set_f(1, chosen);
            if (!fcompare(fpu, chosen, less_value).less) {
                store_pair(c, at(slot_of::convert_b), less_five);
                chosen = fpu.fsub(c.f64(at(slot_of::convert_b)), conversion());
                c.set_f(1, chosen);
            }
            modifier = truncate_at(c, fpu, at(slot_of::convert_a), chosen);
        }
        c.set({{3, c.u32(game())}, {4, subject}});
        c.call(0x1018c340, 0x10217bb4);
        if (nonzero_logical(c, c.r(3))) modifier = u32(modifier) - reg(c.i16(c.r(3) + 0x34));
        if (r13 != 0) {
            const u32 flags_word = c.u32(at(slot_of::player_flags));
            const i32 limit = s8(compare(c, i32(flags_word & 1), 0) != 0 ? 0x1d : 0x19);
            c.set({{5, reg(limit)}, {3, 0x43300000}, {4, reg(counted()) ^ 0x80000000u}});
            const double count_value = fpu.fsub(store_pair(c, at(0x80), counted()), conversion());
            const double decay = fpu.multiply_add(0x10217c10, D(0x388), count_value, D(0x368));
            c.set_f(1, decay);
            compare(c, s8(age), limit);
            if (s8(age) > limit && fcompare(fpu, decay, D(0x178)).greater) {
                const double years = fpu.fsub(store_pair(c, at(0x80), s8(age) - limit), conversion());
                modifier = u32(modifier) - truncate_at(c, fpu, at(slot_of::convert_b), fpu.fmul(decay, years));
            }
        }
        if (!(u32(role) & 1)) {
            // The sum of half of each of the ten rating bytes 0x10..0x19 above 35 counts the excess
            // over 20, squared, in 250ths. The halves go through the stack words 0xc4..0xd0 (kept).
            i32 halves[10];
            const u32 order[10] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x16, 0x15, 0x18, 0x17, 0x19};
            i32 sum = 0;
            for (int n = 0; n < 10; ++n) { halves[n] = half(stat(order[n])); sum += halves[n]; }
            c.put32(at(0xc4), u32(sign_bit(stat(0x14))));
            c.put32(at(0xc8), u32(sign_bit(stat(0x16))));
            c.put32(at(0xcc), u32(sign_bit(stat(0x15))));
            c.put32(at(0xd0), u32(sign_bit(stat(0x18))));
            floor = reg(stat(0x11));
            has_floor = reg(halves[0]);
            total = u32(sign_bit(stat(0x19)));
            count = u32(sign_bit(stat(0x13)));
            c.set({{3, reg(halves[8])}, {4, reg(halves[7])}, {5, reg(stat(0x18) + sign_bit(stat(0x18)))},
                   {6, reg(halves[6])}, {7, reg(stat(0x15) + sign_bit(stat(0x15)))}, {8, reg(sum)},
                   {9, reg(halves[5])}, {10, reg(halves[4])}, {11, reg(halves[1])},
                   {12, reg(stat(0x14) + sign_bit(stat(0x14)))}});
            if (sum > 0x23) {
                const i32 over = sum - 20;
                modifier = u32(modifier) + reg(magic_div(i32(reg(over) * reg(over)), 0x10624dd3, 4));  // / 250
                c.set(3, 0x10624dd3);
            }
        }
        nation_bias();
    }
}

// The factors by which the modifier is scaled for a nation of the position jump table's case
// (constants of the pool): the first for a nation of region 14, the other for the rest.
namespace {
struct NationFactor { u32 region_14, other; };
constexpr NationFactor nation_factors[10] = {{0x3f8, 0x3f0}, {0x3f0, 0x3e8}, {0x3e8, 0x3e0}, {0x3e0, 0x3d8}, {0x3d8, 0x3d0},
                                             {0x3d0, 0x3c8}, {0x3c8, 0x3c0}, {0x3c0, 0x3b8}, {0x3b8, 0x3a8}, {0x3a8, 0x3b0}};
constexpr u32 region_offset = 0x118;
}

// A foreign player is worth less (or more) to the club: the player's own rating byte 0x2f, then, when
// the club's nation lies in another region, 0x103092f0's opinion, the subject's own caps (+0x5e)
// and a case of the position jump table (indexed by that opinion) scale the modifier.
void Appraisal::nation_bias() {
    const double adaptability = fpu.fsub(store_pair(c, at(slot_of::convert_a), stat(0x2f) - 10), conversion());
    const u32 club_nation = c.u32(club + club::nation);
    c.set({{3, reg(stat(0x2f))}, {4, club_nation}});
    compare_logical(c, club_nation, 0);
    c.set_f(1, adaptability);
    modifier = u32(modifier) + truncate_at(c, fpu, at(slot_of::convert_b), fpu.fdiv(adaptability, D(0x20)));
    if (!club_nation) return;
    const u32 nation = c.u32(subject + appraisal::person::nation);
    const u32 region_raw = c.u8(club_nation + 0x119);
    const i32 club_region = s8(region_raw), person_region = s8(c.u8(nation + 0x119));
    c.set({{3, reg(club_region)}, {6, region_raw}, {0, reg(person_region)}});
    compare(c, club_region, person_region);
    if (club_region == person_region) return;
    c.set({{3, c.u32(world())}, {4, subject}, {5, club}, {6, region_raw}, {7, 1}});
    c.call(0x103092f0, 0x10217e10);
    compare(c, i32(r13), 0);
    i32 opinion = s8(c.r(3));
    if (r13 != 0) {
        const i32 base = half(s8(c.u8(subject + appraisal::person::byte_61)));
        const i32 offset = s8(reg(i32(age) - base + 5));
        const double opinion_value = fpu.fsub(store_pair(c, at(0x80), opinion), conversion());
        const double offset_value = fpu.fsub(store_pair(c, at(0x88), offset), conversion());
        double chosen = fpu.multiply_add(0x10217e74, D(0x398), offset_value, D(0x390));
        if (fcompare(fpu, opinion_value, chosen).greater) {
            store_pair(c, at(0x90), opinion);
            chosen = fpu.fsub(c.f64(at(0x90)), conversion());
        }
        c.set_f(1, offset_value);
        opinion = i32(truncate_at(c, fpu, at(slot_of::convert_d), chosen));
        // r3 = age less the half, r0 = the offset as a conversion word, r4 = the exponent word.
        c.set({{0, reg(offset) ^ 0x80000000u}, {3, reg(i32(age) - base)}, {4, 0x43300000}});
    } else {
        c.set({{0, reg(opinion) ^ 0x80000000u}, {3, 0x43300000}});
        const double modifier_value = fpu.fsub(store_pair(c, at(0x88), i32(u32(modifier))), conversion());
        c.set_f(1, modifier_value);
        const double bound = fpu.multiply_add(0x10217ed4, D(0x3a0), modifier_value, D(0x100));
        c.set_f(1, bound);
        const double opinion_value = fpu.fsub(store_pair(c, at(0x80), opinion), conversion());
        if (fcompare(fpu, opinion_value, bound).less) opinion = i32(truncate_at(c, fpu, at(0x80), bound));
    }
    // r5 holds the opinion from here: the caps clamp and the jump read it.
    c.set(5, reg(opinion));
    const u32 caps = c.u8(subject + appraisal::person::byte_5e);
    c.set(0, caps);
    if (compare_logical(c, caps, 0) != 0) {
        const u32 nation_record = c.u32(subject + appraisal::person::nation);
        const i32 reputation = c.i16(nation_record + 0xac);
        const i32 known = reputation + i32(caps * caps);
        c.set({{0, reg(known)}, {3, reg(reputation)}});
        if (compare(c, known, 0x1388) > 0) {
            const i32 clipped = s8(reg(opinion));
            c.set({{0, reg(clipped)}, {3, 5}});
            compare(c, clipped, 5);
            const i32 clamped = s8(reg(clipped > 5 ? clipped : 5));
            c.set({{3, reg(clipped > 5 ? clipped : 5)}, {5, reg(clamped)}});
            opinion = clamped;
        }
    }
    const u32 chosen_case = reg(s8(reg(opinion)));
    c.set(0, chosen_case);
    if (compare_logical(c, chosen_case, 9) > 0) return;
    const u32 target = c.u32(c.toc(slot::position_jump) + (chosen_case << 2));
    c.set({{0, chosen_case << 2}, {3, target}});
    c.s.wr<4>(guest::reg::ctr, target);
    c.set(3, c.u32(subject + appraisal::person::nation));
    const u32 region = c.u8(c.r(3) + region_offset);
    c.set(0, region);
    compare(c, i32(region), 14);
    const auto& factor = nation_factors[chosen_case];
    scale_modifier(region == 14 ? factor.region_14 : factor.other, true);
}

// modifier = trunc(extended factor * modifier) with single-precision steps: the conversion
// words at 0x80, fsubs, fmuls, result at 0x88.
void Appraisal::scale_single(double factor) {
    c.set({{3, u32(modifier) ^ 0x80000000u}, {0, 0x43300000}});
    const double value = fpu.fsubs(store_pair(c, at(slot_of::convert_a), i32(u32(modifier))), conversion());
    c.set_f(1, conversion());
    modifier = truncate_at(c, fpu, at(slot_of::convert_b), fpu.fmuls(value, factor));
}

// By the subject's age and his caps: the modifier is scaled down for the old, the unused and the
// unfit, and set to 5 for those who have played too much.
void Appraisal::scale_by_age() {
    const i32 years = s8(age);
    const auto played = [&] { return u32(c.u8(subject + appraisal::person::byte_5e)); };
    if (compare(c, years, 0x22) > 0) {
        const u32 caps = played();
        if (compare_logical(c, caps, 0) == 0) scale_modifier(0x1c0);
        else if (caps < 8) scale_modifier(0x210);
        else if (years > 0x23 && caps < 0x19) scale_modifier(0x380);
        else if (caps < 0xf) scale_modifier(0x190);
        else if (caps < 0x23) scale_modifier(0x370);
    } else if (compare(c, years, 0x1b) > 0) {
        const u32 caps = played();
        if (compare_logical(c, caps, 0) == 0) scale_modifier(0x210);
        else if (caps < 8) scale_modifier(0x380);
        else if (caps < 0xf) scale_modifier(0x290);
    } else if (compare(c, years, 0x18) > 0) {
        const u32 caps = played();
        if (compare_logical(c, caps, 0) == 0) scale_modifier(0x380);
        else if (caps < 8) scale_modifier(0x190);
        else if (caps < 0xf) scale_modifier(0x370);
        else if (caps > 0x14) modifier = 5;
    } else if (compare(c, years, 0x15) > 0) {
        const u32 caps = played();
        if (compare_logical(c, caps, 0) == 0) scale_modifier(0x370);
        else if (caps < 6) scale_modifier(0x198);
        else if (caps < 0xa) scale_modifier(0x378);
        else if (caps > 0x14) modifier = 5;
    } else if (compare_logical(c, played(), 0xa) > 0) {
        modifier = 5;
    }
}

// How the club that holds the subject (or his lack of one) and the club's relation to him change the
// modifier: the subject's clubs' reputations, his age and the relation's rating.
void Appraisal::club_factor() {
    if (!nonzero_logical(c, relation) || !nonzero_logical(c, club) || !wants(bit(6)) || !in_people_table()) return;
    const u32 person_club = c.u32(subject + appraisal::person::club);
    if (!nonzero_logical(c, person_club)) {
        c.set({{3, c.toc(slot::date)}, {4, c.u32(subject + appraisal::person::birth_day)}, {5, c.u32(subject + appraisal::person::birth_day + 4)}});
        c.call(callee::date_days, 0x10218448);
        if (compare(c, i32(c.r(3)), 0x3c) > 0) scale_modifier(0x190);
    } else {
        const i32 person_reputation = c.i16(person_club + club::reputation);
        const i32 club_reputation = c.i16(club + club::reputation);
        const i32 floor_reputation = person_reputation - 0x4e2;
        c.set({{3, reg(person_reputation)}, {5, reg(club_reputation)}, {6, reg(floor_reputation)}});
        bool skip_to_end = false;
        if (compare(c, club_reputation, floor_reputation) > 0) {
            const i32 relation_rating = s16(reg(s8(c.u8(relation + 0x44)) + 100));
            c.set(7, reg(relation_rating));
            compare(c, relation_rating, 0x7d);
            if (relation_rating >= 0x7d) {
                skip_to_end = true;
            } else {
                double f2 = F(0x3c);
                compare(c, s16(reg(club_reputation)), 0);
                if (s16(reg(club_reputation)) != 0) {
                    f2 = F(0x40);
                    c.set({{4, 0x43300000}, {3, reg(club_reputation) ^ 0x80000000u}});
                    const double floor_value = fpu.fsubs(store_pair(c, at(0x80), floor_reputation), conversion());
                    const double reputation_value = fpu.fsubs(store_pair(c, at(0x88), club_reputation), conversion());
                    const double quotient = fpu.fdivs(floor_value, reputation_value);
                    const double rating_value = fpu.fsubs(store_pair(c, at(0x90), relation_rating), conversion());
                    const double product = fpu.fmuls(quotient, rating_value);
                    c.set_f(1, rating_value);
                    f2 = fpu.fmuls(f2, product);
                    c.set_f(1, product);
                    if (fcompare(fpu, f2, D(0x0)).greater) f2 = F(0x44);
                    else if (fcompare(fpu, f2, D(0x190)).less) f2 = F(0x3c);
                }
                const auto narrowed = [&](u32 offset) { f2 = fpu.frsp(fpu.fmul(D(offset), f2)); };
                if (compare(c, s8(age), 0x22) > 0) narrowed(0x190);
                else if (compare(c, s8(age), 0x1e) > 0) narrowed(0x370);
                else if (compare(c, s8(age), 0x1b) > 0) narrowed(0x378);
                if (compare_logical(c, c.u8(player_data + 0xf), 0x14) == 0) {
                    if (compare(c, s8(age), 0x12) < 0) narrowed(0x190);
                    else if (compare(c, s8(age), 0x14) < 0) narrowed(0x370);
                    else narrowed(0x378);
                }
                scale_single(f2);
            }
        } else {
            const i32 relation_rating = s8(c.u8(relation + 0x44));
            compare(c, relation_rating, 0x19);
            if (relation_rating >= 0x19 || compare(c, s8(age), 0x1e) <= 0) {
                skip_to_end = true;
            } else {
                double f2;
                if (compare(c, s8(age), 0x25) > 0) f2 = F(0x28);
                else if (compare(c, s8(age), 0x22) > 0) f2 = F(0x3c);
                else f2 = F(0x48);
                // Never true: the branch already requires an age above 30 (the original's dead code, kept).
                if (compare(c, club_reputation, 0x1482) > 0 && compare(c, s8(age), 0x14) < 0) {
                    if (compare(c, relation_rating, -0x4b) < 0) f2 = F(0x28);
                    else if (compare(c, relation_rating, -0x19) < 0) f2 = F(0x4c);
                    else if (compare(c, s16(reg(relation_rating)), 0) < 0) f2 = F(0x48);
                }
                const auto narrowed = [&](u32 offset) { f2 = fpu.frsp(fpu.fmul(D(offset), f2)); };
                if (compare_logical(c, c.u8(player_data + 0xf), 0x14) == 0) {
                    if (compare(c, s8(age), 0x12) < 0) narrowed(0x190);
                    else if (compare(c, s8(age), 0x15) < 0) narrowed(0x370);
                    else narrowed(0x378);
                }
                scale_single(f2);
            }
        }
        (void)skip_to_end;
        c.set({{3, subject}, {4, 0}});
        c.call(0x100d3040, 0x102186e8);
        if (compare(c, s8(c.r(3)), 0) != 0) scale_modifier(0x378);
    }
    scale_by_age();
}

// The market's view of the subject: the game's own opinion of how much he is worth
// (0x10187650) against a base of 10000 and the player's reputation.
std::optional<i32> Appraisal::market() {
    const u32 anchor_word = c.u32(at(slot_of::flag_800));
    if (compare(c, i32(anchor_word), 0) == 0) {
        const bool unasked = wants(bit(0xa));
        r13 = reg(c.i16(at(slot_of::limit)));
        if (!unasked) {
            c.set({{3, c.u32(game())}, {4, c.u32(subject)}});
            c.call(0x10187650, 0x10218af8);
            if (compare(c, s16(c.r(3)), 0x3e8) > 0) {
                c.set({{3, c.u32(game())}, {4, c.u32(subject)}});
                c.call(0x10187650, 0x10218b14);
                r13 = u32(r13) - reg(s16(c.r(3)) - 0x1f4);
            }
        }
        const i32 worth = i32(u32(r13));
        compare(c, worth, 0x1388);
        // A cheap player ends the routine here: his value is the position's, in hundreds, less 0x30000+0xd40.
        if (worth < 0x1388) return guest::result(c, reg(i32(reg(i32(u32(position))) * 100) - 0x30000 + worth - 0xd40));
        compare(c, worth, 0x2328);
        if (worth < 0x2328) {
            const i32 scaled = magic_div(i32(reg(worth) * 3), 0x51eb851f, 5);  // / 100
            modifier = u32(modifier) - reg(0x118 - scaled);
        } else {
            modifier = u32(modifier) - reg(magic_div(10000 - worth, 0x51eb851f, 5));  // / 100
        }
        c.set({{3, c.u32(game())}, {4, subject}});
        c.call(0x1018c340, 0x10218b9c);
        if (nonzero_logical(c, c.r(3))) {
            const i32 injury = c.i16(c.r(3) + 0x30);
            compare(c, injury, 10);
            modifier = u32(modifier) - reg(injury <= 10 ? injury * 2 : injury + 0x14);
        }
    }
    if (wants(bit(0xa))) {
        r13 = reg(c.i16(at(slot_of::ceiling)));
        const i32 ceiling = i32(u32(r13));
        compare(c, ceiling, 0x63);
        bool sold = false;
        if (ceiling >= 0x63) {
            const i32 first = s8(c.u8(player_data + 0x3c));
            compare(c, first, 0xe);
            if (first > 0xe) {
                const i32 second = s8(c.u8(player_data + 0x2d));
                compare(c, second, 8);
                if (second < 8) {
                    c.set({{3, c.u32(game())}, {4, c.u32(subject)}});
                    c.call(0x10187650, 0x10218c08);
                    compare(c, s16(c.r(3)), 0x1f4);
                    if (s16(c.r(3)) < 0x1f4) {
                        modifier = u32(modifier) - 200;
                        sold = true;
                    }
                }
            }
        }
        if (!sold) modifier = u32(modifier) + reg(100 - ceiling);
    }
    return std::nullopt;
}

// The player's fit with the judge's own staff (flag 0x1000, a position of at least 100): a few
// small bonuses by the judge's coaching bytes, 0x100da050 / 0x100da0b0 scaling the modifier, then
// 0x100d9580 / 0x100d9520 and the position groups of fit_groups.
void Appraisal::staff_fit() {
    if (!wants(bit(0x13)) || !nonzero_logical(c, staff) || !nonzero_logical(c, club) || compare(c, i32(position), 0x64) < 0) return;
    if (compare(c, s8(age), 0x17) <= 0) {
        const double rating = fpu.fsub(store_pair(c, at(0x80), staff_stat(0x42) - 10), conversion());
        c.set_f(1, rating);
        modifier = u32(modifier) + truncate_at(c, fpu, at(slot_of::convert_b), fpu.fdiv(rating, D(0x20)));
    }
    c.set({{3, subject}, {4, club}});
    c.call(0x100da050, 0x10218c98);
    if (compare(c, s8(c.r(3)), 0) != 0) {
        // The modifier is held below 0x190 times itself or ten under (the smaller wins).
        const i32 value = i32(u32(modifier));
        const double first = fpu.fsub(store_pair(c, at(0x88), value), conversion());
        store_pair(c, at(0x80), value - 10);
        double chosen = fpu.fmul(D(0x190), first);
        const double shifted = fpu.fsub(c.f64(at(0x80)), conversion());
        c.set_f(1, chosen);
        if (fcompare(fpu, shifted, chosen).less) {
            store_pair(c, at(0x90), value - 10);
            chosen = fpu.fsub(c.f64(at(0x90)), conversion());
            c.set_f(1, chosen);
        }
        modifier = truncate_at(c, fpu, at(slot_of::convert_d), chosen);
    } else {
        c.set({{3, subject}, {4, club}});
        c.call(0x100da0b0, 0x10218d10);
        if (compare(c, s8(c.r(3)), 0) != 0) {
            const i32 value = i32(u32(modifier));
            const double first = fpu.fsub(store_pair(c, at(0x88), value), conversion());
            store_pair(c, at(0x80), value + 10);
            double chosen = fpu.fmul(D(0x40), first);
            const double shifted = fpu.fsub(c.f64(at(0x80)), conversion());
            c.set_f(1, chosen);
            if (fcompare(fpu, shifted, chosen).greater) {
                store_pair(c, at(0x90), value + 10);
                chosen = fpu.fsub(c.f64(at(0x90)), conversion());
                c.set_f(1, chosen);
            }
            modifier = truncate_at(c, fpu, at(slot_of::convert_d), chosen);
        }
    }
    c.set({{3, subject}, {4, judge_arg}});
    c.call(0x100d9580, 0x10218d84);
    if (compare(c, s8(c.r(3)), 0) != 0) {
        modifier = u32(modifier) - 0x32;
    } else {
        c.set({{3, subject}, {4, judge_arg}});
        c.call(0x100d9520, 0x10218da0);
        if (compare(c, s8(c.r(3)), 0) != 0) {
            // 0x180 times the modifier, or twenty above it (the larger wins).
            const i32 value = i32(u32(modifier));
            const double first = fpu.fsub(store_pair(c, at(0x80), value), conversion());
            const double raised_pair = store_pair(c, at(0x88), value + 0x14);
            double chosen = fpu.fmul(D(0x180), first);
            const double raised = fpu.fsub(raised_pair, conversion());
            c.set_f(1, chosen);
            if (!fcompare(fpu, chosen, raised).greater) {
                store_pair(c, at(0x90), value + 0x14);
                chosen = fpu.fsub(c.f64(at(0x90)), conversion());
                c.set_f(1, chosen);
            }
            modifier = truncate_at(c, fpu, at(slot_of::convert_d), chosen);
        }
    }
    fit_groups();
    after_groups();
}

// The judge's coaching bytes against the player's ratings, by position group (the role's bits:
// goalkeepers 1, then 2 and 4, 8, 0x10, 0x20, 0x40), in match_man_rating_b_fit.cpp. The running sum
// is left in r4 and the role in r0, which after_groups reads.
void Appraisal::fit_groups() {
    staff_fit_sum(c, {staff, player_data, subject, judge, role});
}

// After the groups: a goalkeeper-trait bonus, and the sum (r4) scaled by the modifier into +-0x50.
void Appraisal::after_groups() {
    const u32 player_flags = c.u32(at(slot_of::player_flags));
    c.set(3, player_flags & bit(0x19));
    if (compare(c, i32(player_flags & bit(0x19)), 0) != 0) {
        const u32 positions = c.r(0) & bit(0x19);
        c.set(0, positions);
        if (compare(c, i32(positions), 0) != 0) goalkeeper_trait_bonus(c, staff, player_data);
    }
    compare(c, i32(c.r(4)), 0);
    if (c.r(4) != 0) {
        // fnmsub: 0 - 0x400 * modifier, times the sum, truncated, then clamped to -80..80.
        const double modifier_value = fpu.fsub(store_pair(c, at(0x88), i32(u32(modifier))), conversion());
        const double sum_pair = store_pair(c, at(0x80), i32(c.r(4)));
        const double scaled = fpu.multiply_add(0x10219670, D(0x400), modifier_value, D(0x0));
        c.set_f(1, modifier_value);
        const double sum_value = fpu.fsub(sum_pair, conversion());
        c.set_f(1, sum_value);
        const i32 limit = i32(truncate_at(c, fpu, at(0x90), fpu.fmul(sum_value, scaled)));
        c.set(3, reg(limit));
        compare(c, limit, -0x50);
        i32 clamped = -0x50;
        if (limit >= -0x50) {
            compare(c, limit, 0x50);
            clamped = limit > 0x50 ? 0x50 : limit;
        }
        c.set(0, reg(clamped));
        modifier = u32(modifier) + reg(clamped);
    }
}

// Flag 0x1000000: a foreigner to the club's nation (neither nation matches) is worth less.
void Appraisal::foreign_penalty() {
    if (!wants(bit(7)) || !nonzero_logical(c, club)) return;
    const u32 first = c.u32(subject + appraisal::person::nation), second = c.u32(subject + appraisal::person::second_nation);
    const u32 club_nation = c.u32(club + club::nation);
    c.set({{0, first}, {3, club_nation}});
    if (compare_logical(c, first, club_nation) == 0) return;
    c.set(0, second);
    if (compare_logical(c, second, club_nation) == 0) return;
    modifier = u32(modifier) - 0x32;
}

// The three calls that find the person's relation to the club and the share (a signed byte, in
// percent) of two counts in that record: the value returned is 50 less half the share, left to the
// caller (nullopt when a call or the share ends the lookup).
std::optional<i32> Appraisal::relation_share(u32 first_back, u32 second_back, u32 third_back) {
    const u32 object = c.u32(c.toc(slot::extra));
    c.set({{3, object}, {4, subject}});
    c.call(0x103ecb00, first_back);
    r13 = c.r(3);
    if (compare(c, i32(u32(r13)), 0) == 0) return std::nullopt;
    c.set({{3, c.u32(c.toc(slot::lookups))}, {4, subject}});
    c.call(0x10082f20, second_back);
    const u32 found = c.r(3);
    c.set(4, c.toc(slot::last_relation));
    compare_logical(c, found, 0);
    c.put32(c.r(4), found);
    u32 found_club = 0;
    if (found) {
        const u32 index = c.u32(found + 4);
        c.set(0, index * 0x248);
        found_club = c.u32(c.toc(slot::clubs)) + index * 0x248;
    }
    c.set(5, found_club);
    c.set({{3, c.u32(c.toc(slot::extra))}, {4, subject}, {6, 1}});
    c.call(0x103ecef0, third_back);
    if (compare(c, s8(c.r(3)), 0) == 0) return std::nullopt;
    const u32 second = c.u16(r13 + 6), first = c.u16(r13 + 4);
    c.set({{4, second}, {5, first}});
    if (compare(c, i32(second + first), 0) != 0) {
        c.set(3, first * 100);
        const i32 quotient = divw(i32(first * 100), i32(first + second));
        c.set(0, reg(quotient));
        share = reg(s8(reg(quotient)));
    }
    const i32 percent = s8(share);
    c.set(3, reg(percent));
    if (compare(c, percent, 0x46) >= 0) return std::nullopt;
    const i32 result = 0x32 - half(percent);
    c.set(0, reg(result));
    return result;
}

// Flag 0x10000: the club relation the person has with the club (the relation record's kind byte
// at +0x60), adding to the modifier by the kind (affection and the share of the two counts).
void Appraisal::relation_block() {
    adjustment = 0;
    share = 0;
    if (!nonzero_logical(c, relation)) return;
    const u32 relation_flags = c.u32(relation + 0x50);
    c.set(3, relation_flags);
    const u32 masked = relation_flags & 0x2808;
    c.set(0, masked);
    // The mood: a signed byte over a pool divisor, capped by another pool value.
    const auto mood = [&](u32 divisor, u32 cap) {
        c.set(3, c.u8(relation + 0x4c));
        const i32 value = s8(c.u8(relation + 0x4c));
        const double mood_value = fpu.fsub(store_pair(c, at(0x80), value), conversion());
        double chosen = D(cap);
        const double scaled = fpu.fdiv(mood_value, D(divisor));
        c.set_f(1, mood_value);
        if (!fcompare(fpu, chosen, scaled).less) chosen = scaled;
        adjustment = truncate_at(c, fpu, at(0x88), chosen);
    };
    if (compare(c, i32(masked), 0) != 0) mood(0x20, 0x408);
    else if (compare(c, i32(relation_flags), 0) != 0) mood(0x80, 0x410);
    const u32 kind = c.u8(relation + 0x60) & 0xf;
    c.set(0, kind);
    compare(c, i32(kind), 3);
    if (kind == 3) {
        if (wants(bit(8))) {
            adjustment = u32(adjustment) + 10;
        } else {
            const i32 diff = (s8(c.u8(relation + 0x45)) - s8(c.u8(relation + 0x46))) * 5;
            const i32 ceiling = c.i16(at(slot_of::ceiling));
            const double ceiling_value = fpu.fsub(store_pair(c, at(0x80), ceiling), conversion());
            const double diff_pair = store_pair(c, at(0x88), diff);
            const double mixed = fpu.multiply_add(0x102199b8, D(0x80), ceiling_value, D(0x418));
            const double diff_value = fpu.fsub(diff_pair, conversion());
            c.set_f(1, mixed);
            adjustment = u32(adjustment) + truncate_at(c, fpu, at(0x90), fpu.fadd(mixed, diff_value));
            if (const auto value = relation_share(0x102199e0, 0x102199f8, 0x10219a38)) adjustment = u32(adjustment) + reg(*value);
        }
    } else if (kind == 1 || kind == 2) {
        adjustment = u32(adjustment) + (kind == 1 ? 0x19 : 0xf);
        if (!wants(bit(8))) {
            const auto value = kind == 1 ? relation_share(0x102197f0, 0x10219808, 0x10219848)
                                         : relation_share(0x102198b0, 0x102198c8, 0x10219908);
            if (value) adjustment = u32(adjustment) + reg(*value);
        }
    } else if (kind == 7) {
        adjustment = u32(adjustment) - 0x14;
    }
    modifier = u32(modifier) + u32(adjustment);
}

// Flag 0x20000: the same relation looked up from the club (0x10082f90): the kind byte decides
// the adjustment (+25, 0, or -25 with the share), then the person's own market value is looked at.
void Appraisal::club_relation_block() {
    c.set({{3, c.u32(c.toc(slot::lookups))}, {4, subject}, {5, club}});
    c.call(0x10082f90, 0x10219aac);
    const u32 found = c.r(3);
    compare_logical(c, found, 0);
    share = 0;
    if (found) {
        const i32 kind = i32(c.u8(found + 0x60) & 0xf);
        c.set(0, reg(kind));
        const auto kind_is = [&](i32 limit) { return compare(c, kind, limit); };
        if (kind_is(6) >= 0) {
            if (kind_is(8) == 0) adjustment = 0x19;
            else adjustment = 0;
        } else if (kind_is(4) >= 0) {
            adjustment = 0x19;
        } else if (kind_is(1) >= 0) {
            adjustment = reg(-0x19);
            if (!wants(bit(8)))
                if (const auto value = relation_share(0x10219b04, 0x10219b1c, 0x10219b5c)) adjustment = reg(-0x19 - *value);
        } else {
            adjustment = 0;
        }
        modifier = u32(modifier) + u32(adjustment);
    }
    if (c.u32(at(slot_of::flag_800)) != 0) return;
    // The game's opinion of the person (0x10187650): a very low one lowers the modifier by the
    // pool's scale of it, a high one (over 500) by another.
    const auto ask = [&](u32 back) {
        c.set({{3, c.u32(game())}, {4, c.u32(subject)}});
        c.call(0x10187650, back);
        return s16(c.r(3));
    };
    const auto lower_by = [&](i32 opinion, u32 site, double scale, double base, bool added) {
        const double value = fpu.fsub(store_pair(c, at(0x80), opinion), conversion());
        c.set_f(1, scale);
        const double mixed = fpu.multiply_add(site, scale, value, base);
        (void)added;
        modifier = u32(modifier) - truncate_at(c, fpu, at(0x88), mixed);
    };
    const i32 first = ask(0x10219bd4);
    compare(c, first, -0x3e8);
    if (first < -0x3e8) {
        lower_by(ask(0x10219bf0), 0x10219c18, D(0x428), D(0x348), true);
    } else if (compare(c, ask(0x10219c40), 0x1f4) > 0) {
        lower_by(ask(0x10219c5c), 0x10219c84, D(0x158), D(0x28), false);
    }
}

// The last adjustments: caps (flag 0x100000), or the unrated player of a club (flag 4: a person of
// the table with no club under 5000, or his club's other record scaled), the people table's
// bound, and the club's own player raised.
void Appraisal::ending() {
    if (wants(bit(0xb))) {
        const i32 capped = i32(c.u8(subject + appraisal::person::byte_5e)) * 5;
        c.set({{0, reg(capped)}, {3, 0x32}});
        compare(c, capped, 0x32);
        const i32 taken = capped >= 0x32 ? 0x32 : capped;
        c.set(3, reg(taken));
        modifier = u32(modifier) - reg(taken);
    } else if (compare(c, i32(c.u32(at(slot_of::flag_4))), 0) != 0 && in_people_table()) {
        const u32 person_club = c.u32(subject + appraisal::person::club);
        c.set(3, person_club);
        bool done = false;
        if (!nonzero_logical(c, person_club)) {
            const i32 rating = c.i16(player_data + 8);
            c.set(0, reg(rating));
            if (compare(c, rating, 0x1388) < 0) {
                modifier = u32(modifier) - 0x1e;
                done = true;
            }
        }
        if (!done && nonzero_logical(c, person_club)) {
            const u32 record = c.u32(person_club + 0x30);
            c.set(3, record);
            if (nonzero_logical(c, record)) {
                const i32 reputation = c.i16(record + 0x18);
                c.set(3, reg(reputation));
                const double value = fpu.fsub(store_pair(c, at(0x80), reputation - 0x10), conversion());
                c.set_f(1, conversion());
                modifier = u32(modifier) + truncate_at(c, fpu, at(0x88), fpu.fmul(D(0x100), value));
            }
        }
    }
    if (!in_people_table()) modifier = u32(modifier) - 0x1e;
    const u32 person_club = c.u32(subject + appraisal::person::club);
    c.set(0, person_club);
    if (compare_logical(c, person_club, club) == 0) {
        // The club's own player: the modifier is raised to 0x2d8 times itself or five more.
        const i32 value = i32(u32(modifier));
        const double pair = store_pair(c, at(0x88), value);
        const double raised_pair = store_pair(c, at(0x80), value + 5);
        const double current = fpu.fsub(pair, conversion());
        double chosen = fpu.fmul(D(0x2d8), current);
        const double raised = fpu.fsub(raised_pair, conversion());
        c.set_f(1, chosen);
        if (fcompare(fpu, raised, chosen).greater) {
            store_pair(c, at(0x90), value + 5);
            chosen = fpu.fsub(c.f64(at(0x90)), conversion());
            c.set_f(1, chosen);
        }
        modifier = truncate_at(c, fpu, at(slot_of::convert_d), chosen);
    }
}

// The result: the position's value and the modifier: unmixed for the anchor flag, otherwise the
// two multiplied, added or scaled by what is left of 500 (a negative position).
i32 Appraisal::finish() {
    if (compare(c, i32(u32(flags)), 0x800) == 0) return guest::result(c, u32(position));
    if (compare(c, i32(u32(position)), 0) < 0) return guest::result(c, reg((500 - i32(u32(modifier))) * i32(u32(position))));
    if (compare(c, i32(u32(modifier)), 1) < 0) return guest::result(c, u32(position) + u32(modifier));
    return guest::result(c, u32(position) * u32(modifier));
}
}  // namespace

std::int32_t rating_b(Cpu& c, std::uint32_t person, std::uint32_t role, std::uint32_t flags, std::uint32_t club,
                      std::int32_t floor, std::uint32_t judge) {
    constexpr std::uint32_t entry = 0x10216830;
    c.set(0, c.lr());
    Frame frame(c, entry, 0x140, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 31});
    Fpu fpu(c);
    Appraisal a(c, fpu, frame, person, role, flags, club, floor, judge);
    if (const auto early = a.begin()) return *early;
    a.load_context();a.position_value();if (const auto early = a.averages()) return *early;
    a.trends();a.form();a.overall();a.development();a.club_factor();if (const auto early = a.market()) return *early;
    a.staff_fit();a.foreign_penalty();if (a.wants(bit(0xf))) a.relation_block();
    else if (a.wants(bit(0xe))) a.club_relation_block();
    a.ending();return a.finish();
}
}
