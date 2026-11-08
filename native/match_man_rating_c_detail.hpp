#pragma once
// The rating_c block's record offsets, stack slots and the phases of 0x10211e70 (match_man_rating_c*.cpp).
// 0x10211e70 rates a person for one place of a side: the same appraisal as 0x10216830 (match_man_rating_b)
// with the place's own roles, the match's form and the human manager's tail added. Its compiler register
// assignment differs from 0x10216830's, so the phases are written again here (helpers shared with rating_b
// come from match_man_rating_b_parts.hpp / _detail.hpp, which stay as they are).
// Meanings marked "inferred" come from how the values are used; the offsets are the original's.
#include "match_man_rating_b_parts.hpp"
#include <optional>

namespace cm::aot::match_man::placement {
using guest::Cpu;
using guest::Fpu;
using guest::Frame;
using guest::Kept;
using guest::compare;
using guest::compare_logical;
using guest::nonzero_logical;
using appraisal::i32;
using appraisal::u32;
using appraisal::bit;
using appraisal::s8;
using appraisal::s16;
using appraisal::reg;
using appraisal::half;
using appraisal::sign_bit;
using appraisal::magic_div;
using appraisal::store_pair;
using appraisal::truncate_at;
using appraisal::fcompare;
using appraisal::DateSlots;
namespace component = appraisal::component;

constexpr u32 entry = 0x10211e70;
constexpr i32 rejected = -0x2710;
// Record offsets (a person's own are in appraisal::person).
namespace rec {
// the people table's count, TOC -0x7be4
constexpr u32 data_current_estimate = 0xa;  // 16-bit, in the player data
}
// The stack frame (offsets from the new r1).
namespace at {
constexpr u32 date_a = 0x40, date_b = 0x48, date_c = 0x50, date_d = 0x58, date_e = 0x60, buffer_e = 0x78, buffer_d = 0x70, buffer_b = 0x68;
constexpr u32 convert_slot = 0x90;  // the int-to-double words, result words: the phases give their own
constexpr u32 ceiling = 0xb0, limit = 0xb2, has_floor = 0xb4, estimate = 0xb6, judged = 0xb8, judge = 0xbc;
constexpr u32 flag_4 = 0xc0, flag_800 = 0xc4, side_record = 0xc8, side_offset = 0xcc, side_index = 0xd0, player_flags = 0xd4;
constexpr u32 role = 0x186, side = 0x190;
// the caller's stack arguments: the floor and the place's words
constexpr u32 floor = 0x198, place_first = 0x19c, place_second = 0x1a0, place_role = 0x1a4, extra = 0x1a8, place_rating = 0x1ac, human = 0x1b0;
}

// The routine: what it keeps in r13-r31 and its phases (the original reuses several of them as temporaries
// in later phases; they are assigned where it assigns them).
struct Placement {
    Cpu& c;
    Fpu& fpu;
    Frame& frame;
    Kept<13> relation;    // 0x10082f90's record of the person and the club, or 0; later the role and other records
    Kept<14> adjust;      // the place's first word (the high word of its mask); later the role, the adjustments, the result
    Kept<15> person;
    Kept<16> data;        // the player data (the argument); the human manager's loop counter at the end
    Kept<17> record;      // the person's record on the side (the argument), or 0
    Kept<18> flags;       // the components asked for
    Kept<19> club;
    Kept<20> judge_arg;   // the judge given; later working values
    Kept<21> second_word; // the place's second word (low word of its mask); later working values
    Kept<22> alt_role;    // the place's rating halfword (an alternative role mask); later working values
    Kept<23> age;         // the person's age as a signed byte; later the staff fit's sum
    Kept<24> total;       // the sum of the judged abilities
    Kept<25> count;       // how many were summed
    Kept<26> staff;       // the judge's staff data
    Kept<27> second;      // the alternative role's value; later working values
    Kept<28> position;    // the value of the role
    Kept<29> worth;       // the market's worth of the person
    Kept<30> modifier;
    Kept<31> doubles;     // the pool at TOC -0x5278
    Placement(Cpu& cpu, Fpu& f, Frame& fr);

    double D(u32 offset) const { return c.f64(doubles + offset); }
    double F(u32 offset) const { return double(c.f32(c.toc(-0x527c) + offset)); }
    double conversion() const { return D(0x90); }
    u32 at(u32 offset) const { return frame.at(offset); }
    i32 stat(u32 offset) const { return s8(c.u8(data + offset)); }
    u32 stat_raw(u32 offset) const { return c.u8(data + offset); }
    i32 staff_stat(u32 offset) const { return s8(c.u8(staff + offset)); }
    bool wants(u32 mask) const { return compare(c, i32(u32(flags) & mask), 0) != 0; }
    i32 counted() const { return s8(count); }
    u32 game() const { return c.toc(-0x7ba0); }
    u32 world() const { return c.toc(-0x7b3c); }
    i32 divw(i32 a, i32 b) const;
    bool in_people_table() const;
    // `lbz 0xb4; extsb.` (cr0 of the floor byte) and the running average against the floor.
    bool has_floor() const { return compare(c, s8(c.u8(at(at::has_floor))), 0) != 0; }
    bool floor_fails(bool by_count);
    bool inside_window(const DateSlots& slots);

    // The phases, in the original's order.
    std::optional<i32> begin(u32 role, u32 side);
    void load_context();
    void position_values();
    std::optional<i32> averages();
    void trends();
    void form();
    void overall();
    void place_bonuses();
    void club_factor();
    std::optional<i32> development();
    std::optional<i32> market();
    void staff_fit();
    void side_and_nation();
    void relations();
    void ending();
    std::int32_t finish();

    // Parts.
    void position_value();
    void position_second();
    i32 role_bonus(i32 role);
    i32 wing_back() const;
    i32 average_type(u32 sum_at, u32 first_at, u32 again_at, u32 result_at);
    bool alt_applies();
    void form_of_match();
    bool human_manager_on_side(u32 match);
    void flag_4_measure();
    std::optional<i32> judged_estimates();
    void add_scaled(u32 site, double a, double x, double b, bool single, bool mixed_in_f1);
    double narrow(u32 site, double noise, i32 days);
    enum class Test { first_small, second_small, first_group, second_group, third_group_second, first_high, first_high_r4, second_high };
    bool place_has(u32 mask, Test test);
    bool extra_has(u32 mask, bool into_r4);
    bool role_byte_has(u32 mask);
    bool role_byte_bit(u32 mask);
    u32 scaled_term(u32 pair_at, i32 value, u32 divisor_offset, u32 result_at, double* quotient = nullptr, bool into_f1 = true);
    u32 three_attributes(u32 divisor_offset, const u32 (&pairs)[3], const u32 (&results)[3]);
    void scale_modifier(u32 pool_offset);
    void scale_single(double factor);
    void scale_by_age();
    void halves_of_ratings();
    void nation_bias();
    i32 ask_opinion(u32 back);
    void fit_groups();
    void after_groups();
    std::optional<i32> relation_share(u32 first_back, u32 second_back, u32 third_back);
    void scale_by_clock();
};
}
