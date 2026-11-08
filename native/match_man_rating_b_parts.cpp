// The parts shared by 0x10216830 and 0x10211e70 (match_man_rating_b_parts.hpp).
#include "match_man_rating_b_parts.hpp"

namespace cm::aot::match_man::appraisal {
bool date_before_person(Cpu& c, u32 date, u32 copy_at, u32 person, const DateForm& form) {
    // lwz, lwz, stw, stw: the 8 bytes to the stack.
    const u32 first = c.u32(date), second = c.u32(date + 4);
    c.put32(copy_at, first);
    c.put32(copy_at + 4, second);
    c.set({{form.word_b, second}, {form.word_a, first}});
    const u32 year = c.u16(copy_at + 2), person_year = c.u16(person + person::birth_year);
    c.set({{form.person_year, person_year}, {form.other_year, year}});
    c.compare_logical(0, year, person_year);  // cmplw
    i32 earlier;
    if (year < person_year) {
        earlier = 1;
        c.set(form.result, 1);
    } else if (year > person_year) {
        earlier = 0;
        c.set(form.result, 0);
    } else {
        // xor, srawi, and, subf, rlwinm 1,31,31: the original's signed compare of the days.
        const i32 day = c.i16(copy_at), person_day = c.i16(person + person::birth_day);
        const i32 difference = day ^ person_day;
        earlier = i32((reg(difference >> 1) - reg(difference & person_day)) >> 31) & 1;
        c.set({{form.person_day, reg(person_day)}, {form.half, reg(difference >> 1)}, {form.result, reg(earlier)}});
    }
    guest::compare(c, s8(reg(earlier)), 0);  // extsb.
    return earlier != 0;
}

bool judge_date_window(Cpu& c, const Frame& frame, u32 person, u32 judge, const DateSlots& slots) {
    if (!date_before_person(c, judge + person::birth_day, frame.at(slots.judge_date), person, slots.first)) return false;
    c.set({{4, c.toc(slot::date)}, {3, frame.at(slots.buffer)}, {5, days_in_year}});
    c.call(callee::add_days, slots.return_after_call);
    return date_before_person(c, frame.at(slots.buffer), frame.at(slots.later_date), person, slots.second);
}

i32 age_in_days(Cpu& c, u32 person) {
    const u32 today = c.toc(slot::date);
    const i32 years = i32(c.u16(today + date_year)) - i32(c.u16(person + person::birth_year));
    const i32 days = i32(c.i16(today + date_day)) - i32(c.i16(person + person::birth_day));
    return i32(reg(years) * days_in_year + reg(days));
}

u32 value_of_rating(Cpu& c, u32 table, i32 index) { return c.u32(table + (reg(index) << 2)); }
}
