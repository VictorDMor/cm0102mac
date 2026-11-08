#pragma once
// The parts of 0x10216830 that 0x10211e70 runs too, with the stack slots and
// return addresses of the caller passed in (match_man_rating_b_parts.cpp).
#include "match_man_rating_b_detail.hpp"

namespace cm::aot::match_man::appraisal {
// Where one inlined "born before / within a year" date check keeps its three
// stack copies and what follows its `bl 0x10717b20`.
// Which registers the compiler used for one inlined date check, so its scratch values are left
// where the original leaves them: the first word read goes to `word_a`, the second to `word_b`,
// then the two years to `person_year` and `other_year`, the result to `result`; when the years
// are equal the days go through `half` (the xor shifted) and `person_day`.
struct DateForm {
    unsigned word_a, word_b, person_year, other_year, result, half, person_day;
};
// 0x10216830's two checks: the judge's date (r3, r4, result r0) and the date a year ahead (r0, r4, result r3).
inline constexpr DateForm judge_form{3, 0, 3, 4, 0, 3, 4};
inline constexpr DateForm ahead_form{0, 3, 0, 4, 3, 4, 5};
struct DateSlots {
    u32 judge_date;        // frame offset: the copy of the judge's 8-byte date at +0x4c
    u32 buffer;            // frame offset: the 8 bytes 0x10717b20 writes (r3 = sp + buffer)
    u32 later_date;        // frame offset: the copy of that buffer, compared the second time
    u32 return_after_call; // the address after the `bl 0x10717b20`
    DateForm first = judge_form, second = ahead_form;
};
// The inlined 8-byte date compare: true when the date at `date` (a copy at
// `copy_at`) is earlier than the person's date at +0x4c (years as unsigned halves, days
// by the original's bit-trick signed compare). Leaves cr0 as `extsb.` of the result.
bool date_before_person(Cpu& c, u32 date, u32 copy_at, u32 person, const DateForm& form);
// The two-step check: the judge's date (at judge+0x4c) is before the person's,
// then so is the date 365 days after today (0x10717b20 into `buffer`). True when both hold.
bool judge_date_window(Cpu& c, const Frame& frame, u32 person, u32 judge, const DateSlots& slots);
// The person's age in days: (today's year - birth year) * 365 + today's day - birth day.
i32 age_in_days(Cpu& c, u32 person);
// The index into the table at TOC -0x5290 (positions of ratings 0..): the word at table + (index << 2).
u32 value_of_rating(Cpu& c, u32 table, i32 index);
}
