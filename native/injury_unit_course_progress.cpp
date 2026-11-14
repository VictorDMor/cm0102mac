// INJURY.CPP, block "course": one injury's recovery or relapse with its notices (injury_unit_course.hpp).
#include "injury_unit_course.hpp"
#include "injury_unit_course_detail.hpp"
#include <stdexcept>

namespace cm::aot::injury_unit {
using guest::Cpu;
using guest::Frame;
using namespace course;

namespace {
constexpr std::int32_t news_cell = -0x7bf0,        // the cell holding the news object
    world_cell = -0x7b24,                         // a cell holding the world object some outcomes hand on
    lookup_cell = -0x7b7c,                        // what 0x10082f20 is asked with
    flag_table_cell = -0x7b9c;                    // the cell holding the per-person flag table (100 bytes a person)
constexpr std::uint32_t news_check = 0x102cdbf0, news_begin = 0x102bdf90, news_put = 0x102cb4b0, news_send = 0x102cc030,
                        find_deal = 0x10082f20, set_deal_flag = 0x1037f7d0, notify_world = 0x10006900,
                        staff_a_check = 0x100d9580, staff_a_act = 0x100d9e90, staff_a_report = 0x100d9960,
                        staff_b_check = 0x100d9520, staff_b_act = 0x100d9e10, staff_b_report = 0x100d9b80;
// The injury type ids the course knows by kind (the type table's rows, 0xc bytes, class byte at +1).
constexpr std::int32_t type_m = 0x4d, type_n = 0x4e, type_o = 0x4f, type_q = 0x51;
constexpr std::uint32_t class_with_course = 0xc;
// Record bytes: a club's staff pointer, a club's news and its person's source (club +0x2c) records.
constexpr std::uint32_t club_staff = 0x90, club_news_source = 0x2c, source_id = 0x74, deal_flags = 0x50,
                        deal_rating = 0x4c, profile_form = 0x44, profile_age = 0xa;
constexpr std::uint32_t status_stage = 0x14, status_progress = 0x15;

std::uint32_t sign_extend(std::int32_t value) { return std::uint32_t(value); }

// The game's inline "random below": no draw for a bound of 0, else one sample reduced by the bound (the
// remainder keeps the sample's sign, then the bound's: negative bounds negate it). The original also has
// a scaled floating-point path for bounds past 16 bits. Every bound here is a signed byte, or the sum of
// two of them less five (-261..254), so that path (0x1018df0c..0x1018df90 and its copies) cannot run.
// The bound is left in r17 and the result in r18, as the original leaves them.
std::int32_t random_below(Cpu& c, std::int32_t bound, std::uint32_t back) {
    c.set(17, sign_extend(bound));
    std::int32_t result = 0;
    if (bound != 0) {
        const auto sample = std::int32_t(draw(c, back));
        if (bound > 0xffff || bound < -0xffff) throw std::logic_error("INJURY.CPP 0x1018dcb0: a bound past 16 bits is unreachable");
        // divw, mullw, subf, neg, all wrapping (the division in 64 bits: defined for any word in the base cell).
        const auto remainder = std::uint32_t(sample) - std::uint32_t(std::int64_t(sample) / bound) * std::uint32_t(bound);
        result = std::int32_t(bound > 0 ? remainder : 0u - remainder);
    }
    c.set(18, sign_extend(result));
    return result;
}
}

void injury_course(Cpu& c, std::uint32_t manager_object, std::uint32_t person_record, std::uint32_t stage_word,
                   std::uint32_t type_word, std::uint32_t queued_word, std::uint32_t days_word) {
    Frame frame(c, 0x1018dcb0, 0x180, {.first_gpr = 17});
    const auto news_object_cell = c.toc(news_cell), world_object_cell = c.toc(world_cell);
    c.set({{31, person_record}, {20, news_object_cell}, {21, world_object_cell}, {17, manager_object},
           {27, c.toc(toc::floats)}, {18, stage_word}, {29, c.toc(sample_table::base)}, {19, type_word},
           {25, c.toc(sample_table::first)}, {22, queued_word}, {24, c.toc(sample_table::last)}, {23, days_word},
           {30, c.toc(sample_table::cursor)}});
    const auto person = person_record;
    if (person == 0) return;
    if (std::int8_t(queued_word) == 0) {
        // Called by the daily pass for a person with a club; with a due row (queued) any person's course runs.
        if (c.u32(person + person::club) == 0) return;
        if (std::int8_t(invoke(c, news_check, 0x1018dd14, {c.u32(news_object_cell), c.u32(person + person::club)})) == 0)
            return;
    }

    // The news item under construction (item 0x7d1) is a buffer in this frame.
    const auto item = frame.at(0x38);
    const auto put = [&](std::uint32_t back, std::uint32_t field, std::uint32_t value) {
        invoke(c, news_put, back, {item, field, value});
    };
    invoke(c, news_begin, 0x1018dd30, {item, 0x7d1, 0});
    const auto linked_id = [&](std::uint32_t link) { return c.u32(c.u32(person + link) + 0x20); };
    put(0x1018dd40, 0, sign_extend(std::int8_t(stage_word)));
    const auto type = std::int32_t(std::int8_t(type_word));
    c.set(26, sign_extend(type));
    put(0x1018dd54, 1, sign_extend(type));
    put(0x1018dd64, 2, c.u32(person));
    put(0x1018dd78, 3, linked_id(person::link_a));
    put(0x1018dd8c, 4, linked_id(person::link_b));
    put(0x1018dda0, 5, linked_id(person::link_c));
    put(0x1018ddb0, 6, sign_extend(c.i16(person + person::days_a)));
    put(0x1018ddc0, 7, c.u16(person + person::days_b));
    put(0x1018ddd0, 8, sign_extend(std::int16_t(days_word)));

    const auto club = c.u32(person + person::club);
    // The outcome code (r26: -1 none, 0..3) and the notice (r23: 1 good, -1 bad, 0 none) the news item ends with.
    std::int32_t outcome = -1, notice = 0;
    const auto set_outcome = [&](std::int32_t value) { outcome = value; c.set(26, sign_extend(value)); };
    const auto set_notice = [&](std::int32_t value) { notice = value; c.set(23, sign_extend(value)); };
    c.set({{28, club}, {26, sign_extend(-1)}, {23, 0}});

    const auto staff = [&] { return c.u32(club + club_staff); };
    const auto has_staff = [&] { return club != 0 && staff() != 0; };
    const auto status_of = [&] { return c.u32(manager_object + manager::status_rows) + c.u32(person) * status::size; };
    const auto finish = [&] {
        put(0x1018f2d8, 9, sign_extend(std::int8_t(outcome)));
        put(0x1018f2e8, 10, sign_extend(std::int8_t(notice)));
        const auto source = club != 0 ? c.u32(club + club_news_source) : c.u32(person + person::home);
        const auto source_value = source != 0 ? c.u32(source + source_id) : 0;
        invoke(c, news_send, 0x1018f328, {c.u32(news_object_cell), item, club, 0, source, source_value, queued_word});
    };
    // The first of the staff routines' pairs: a check, then an act when it holds.
    const auto staff_pair = [&](std::uint32_t check, std::uint32_t check_back, std::uint32_t act, std::uint32_t act_back) {
        if (has_staff() && std::int8_t(invoke(c, check, check_back, {person, staff()})) != 0)
            invoke(c, act, act_back, {person, staff()});
    };

    if (c.u8(c.toc(toc::injury_types) + std::uint32_t(type) * injury_type::size + 1) != class_with_course) {
        finish();
        return;
    }
    const auto sample = [&](std::uint32_t back) { return std::int32_t(draw(c, back)); };
    const auto roll = std::int16_t(sample(0x1018de14) % 32000);

    if (type == type_q) {
        const auto deal = invoke(c, find_deal, 0x1018de70, {c.u32(c.toc(lookup_cell)), person});
        c.set(26, deal);
        if (deal == 0 || c.u32(person + person::profile) == 0 || club == 0) {
            set_notice(1);
            set_outcome(0);
            finish();
            return;
        }
        // Each of three deal flags that is set can be cleared by a draw against the person's attribute C.
        struct Flag { std::uint32_t bit, bound_back, roll_back, call_back; };
        for (const Flag flag : {Flag{0x20000, 0x1018decc, 0x1018e014, 0x1018e064}, Flag{0x20, 0x1018e0a8, 0x1018e1f0, 0x1018e23c},
                                Flag{0x40, 0x1018e280, 0x1018e3c8, 0x1018e414}}) {
            if ((c.u32(deal + deal_flags) & flag.bit) == 0) continue;
            const auto bound = random_below(c, std::int8_t(c.u8(person + person::attribute_c)), flag.bound_back);
            if (bound > sample(flag.roll_back) % 25) {
                invoke(c, set_deal_flag, flag.call_back, {deal, flag.bit});
                set_notice(1);
            }
        }
        bool rating_moves = notice != 0;
        if (!rating_moves) rating_moves = random_below(c, std::int8_t(c.u8(person + person::attribute_c)), 0x1018e454) != 0;
        if (rating_moves) {
            const auto moved = std::int32_t(std::int8_t(c.u8(deal + deal_rating))) + 0xf;
            if (moved > 100) c.put8(deal + deal_rating, 100);
            else if (moved < -100) c.put8(deal + deal_rating, 0x9c);
            else c.put8(deal + deal_rating, c.u8(deal + deal_rating) + 0xf);
        }
        if (std::int8_t(notice) != 0) {
            set_outcome(2);
            staff_pair(staff_a_check, 0x1018e5e8, staff_a_act, 0x1018e5fc);
            if (has_staff() && random_below(c, std::int8_t(c.u8(person + person::attribute_b)), 0x1018e644) > 5)
                invoke(c, staff_a_report, 0x1018e784, {person, staff(), 1});
        } else {
            if (c.u32(world_object_cell) != 0 && std::int8_t(c.u8(person + person::attribute_b)) < 12) {
                const auto bound = random_below(c, std::int8_t(c.u8(person + person::attribute_b)), 0x1018e7d4);
                if (bound < sample(0x1018e91c) % 5) {
                    invoke(c, notify_world, 0x1018e970, {c.u32(world_object_cell), person, club, 4});
                    return;
                }
            }
            set_outcome(0);
        }
        // Whatever happened, the person has had a bad time: one more draw decides what it costs.
        set_notice(1);
        std::int32_t sum = std::int32_t(std::int8_t(c.u8(person + person::attribute_c))) +
                           std::int32_t(std::int8_t(c.u8(person + person::attribute_b)));
        if (c.i16(c.u32(person + person::profile) + profile_age) > 0x1c52) sum -= 5;
        const auto bound = random_below(c, std::int16_t(sum), 0x1018e9d8);
        if (bound >= sample(0x1018eb20) % 20) {
            finish();
            return;
        }
        const auto which = sample(0x1018eb7c) % 4;
        const auto lower = [&](std::uint32_t at, std::uint32_t floor_value = 1) {
            if (std::int8_t(c.u8(at)) > std::int32_t(floor_value)) c.put8(at, c.u8(at) - 1);
        };
        const auto raise_a = [&] { if (std::int8_t(c.u8(person + person::attribute_a)) < 0x14) c.put8(person + person::attribute_a, c.u8(person + person::attribute_a) + 1); };
        if (which == 1) {
            raise_a();
            lower(person + person::attribute_b);
        } else if (which >= 0 && which < 1) {
            raise_a();
            lower(person + person::attribute_d);
            lower(c.u32(person + person::profile) + profile_form);
            lower(person + person::attribute_b);
        } else {
            lower(person + person::attribute_d);
            lower(c.u32(person + person::profile) + profile_form);
        }
        finish();
        return;
    }

    if (std::int8_t(stage_word) == 0) {
        if (type == type_m) {
            if (roll == 0) {
                set_outcome(2);
                c.put8(status_of() + status_stage, 0xff);
                c.put8(status_of() + status_progress, 0);
                if (std::int8_t(c.u8(person + person::attribute_b)) > 10) set_notice(1);
            } else if (roll < 5) {
                set_outcome(1);
                c.put8(status_of() + status_progress, 0);
                if (std::int8_t(c.u8(person + person::attribute_b)) > 15 && std::int8_t(c.u8(person + person::attribute_a)) > 10)
                    set_notice(1);
            } else {
                set_outcome(0);
                if (std::int8_t(c.u8(person + person::attribute_d)) < 5) set_notice(-1);
            }
        }
    } else {
        if (type == type_o) {
            if (roll < 30000) {
                set_outcome(2);
                c.put8(status_of() + status_stage, 0xff);
                c.put8(status_of() + status_progress, 0);
            } else if (sample(0x1018edd8) % 5 != 0) {
                set_outcome(1);
                c.put8(status_of() + status_progress, 0);
            } else {
                set_outcome(3);
                const auto added = std::int8_t(sample(0x1018ee54) % 4 + 1);
                c.put8(status_of() + status_progress, c.u8(status_of() + status_progress) + std::uint8_t(added));
            }
        } else if (type == type_n) {
            if (roll < 0x733c) {
                set_outcome(2);
                c.put8(status_of() + status_stage, 0xff);
                c.put8(status_of() + status_progress, 0);
            } else if (roll < 30000) {
                set_outcome(1);
                c.put8(status_of() + status_progress, 0);
            } else {
                set_outcome(0);
            }
        }
        // The person's flag byte in the 100-byte rows loses its top bit.
        const auto flag_byte = c.u32(c.toc(flag_table_cell)) + c.u32(person) * 100 + 0xd;
        c.put8(flag_byte, c.u8(flag_byte) & 0x7f);
    }
    if (std::int8_t(notice) == 1) {
        staff_pair(staff_a_check, 0x1018ef68, staff_a_act, 0x1018ef7c);
        if (has_staff() && random_below(c, std::int8_t(c.u8(person + person::attribute_b)), 0x1018efc4) > 5)
            invoke(c, staff_a_report, 0x1018f104, {person, staff(), 1});
    } else if (std::int8_t(notice) == -1) {
        staff_pair(staff_b_check, 0x1018f12c, staff_b_act, 0x1018f140);
        if (has_staff() && random_below(c, std::int8_t(c.u8(person + person::attribute_d)), 0x1018f188) == 0)
            invoke(c, staff_b_report, 0x1018f2c8, {person, staff(), 1});
    }
    finish();
}
}
