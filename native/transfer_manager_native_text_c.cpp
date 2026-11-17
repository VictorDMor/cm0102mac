// TRANSFER_MANAGER.CPP, block "text_c": the texts of news messages 4012, 4014, 4015, 4007 and 4006 (4008 is in _4008.cpp).
#include "transfer_manager_native_text_c.hpp"
#include "transfer_manager_native_text_c_detail.hpp"

namespace cm::aot::transfer_manager_native {
using namespace text_c;
using guest::Frame;

namespace {
constexpr std::uint32_t buffer_at = 0x40, stack_argument = 0x38;
constexpr std::uint32_t cash_value_fn = 0x10009f20, cash_text_fn = 0x1000b410;
constexpr std::uint32_t refund_check_fn = 0x10416d60, news_note_fn = 0x102cd0c0;
constexpr std::int32_t manager_cell = -0x7b40;
}

// 4012: a person moves between two clubs; the text names both clubs. A short first part may also tell the news desk about a refund.
void news_text_4012(Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output) {
    Frame frame(c, 0x10402e70, 0x1b0, {.first_gpr = 18});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    const auto session = c.toc(session_cell);
    const auto buffer = frame.at(buffer_at);
    keep(c, 20, item);
    keep(c, 26, session);
    keep(c, 22, text);
    keep(c, 18, detail);
    keep(c, 23, c.toc(toc::people));
    keep(c, 27, output);
    keep(c, 24, c.toc(toc::clubs));
    keep(c, 25, text_base);

    const auto person = news_field(c, item, 0, 0x10402eac);
    keep(c, 31, std::uint32_t(person));
    std::int8_t kind = std::int8_t(news_field(c, item, 9, 0x10402ec0));
    keep_signed(c, 19, kind);
    if (kind == 0) {
        const auto index = news_field(c, item, 7, 0x10402ed4);
        keep(c, 21, std::uint32_t(index));
        const auto expected = news_field(c, item, 8, 0x10402ee4);
        // The record list of the manager: an index past its end reads the first entry (the original's quirk).
        const auto list = c.u32(c.toc(manager_cell));
        const auto slots = c.u32(list);
        const auto record = c.u32(index < c.i32(list + 4) ? slots + std::uint32_t(index) * 4 : slots);
        if (record != 0 && c.i32(record + 0x2c) == expected) {
            const auto answer = invoke(c, refund_check_fn, 0x10402f30, {record, std::uint32_t(person)});
            keep(c, 19, answer);
            kind = std::int8_t(answer);
            if (std::int8_t(answer) != 0) {
                const auto slot = session + std::uint32_t(std::int32_t(c.i16(session + 0x2c58)) * 0x2c0);
                invoke(c, news_note_fn, 0x10402f68,
                       {c.u32(c.toc(toc::news)), c.u32(slot + 4), c.u32(item + 0xdc), 9, std::uint32_t(std::int32_t(std::int8_t(answer))), 0});
            }
        }
    }

    const auto person_record = [&] { return person_at(c, person); };
    if (std::int8_t(detail) != 0) {
        const auto day = news_field(c, item, 1, 0x10402f7c);
        keep(c, 30, std::uint32_t(day));
        const auto month = news_field(c, item, 2, 0x10402f90);
        keep(c, 29, std::uint32_t(month));
        const auto year = news_field(c, item, 3, 0x10402fa4);
        keep(c, 28, std::uint32_t(year));
        const auto from_club = news_field(c, item, 4, 0x10402fb8);
        keep(c, 26, std::uint32_t(from_club));
        const auto to_club = news_field(c, item, 5, 0x10402fc8);
        keep(c, 21, std::uint32_t(to_club));
        const auto flag = std::int8_t(news_field(c, item, 10, 0x10402fd8));
        keep_signed(c, 18, flag);
        invoke(c, date_text_fn, 0x10402ffc, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), buffer, date_buffer_size, 1, std::uint32_t(person)});
        const auto to_offset = std::uint32_t(to_club) * club_size, from_offset = std::uint32_t(from_club) * club_size;
        keep(c, 21, to_offset);
        keep(c, 20, from_offset);
        const auto clubs = c.u32(c.toc(toc::clubs));
        const auto from_record = clubs + from_offset, to_record = clubs + to_offset;
        c.put32(frame.at(stack_argument), buffer);
        const auto from_length = name_length(c, from_record), to_length = name_length(c, to_record);
        format_text(c, 0x10403048, text, text_base + 0x791a, {from_length, from_length, to_length, to_length, name_of(from_record), name_of(to_record)});
        invoke(c, begin_text_fn, 0x10403054, {output, text});
        if (kind == 1) {
            format_text(c, 0x10403088, text, text_base + 0x79b8, {to_length, to_length, name_of(to_record)});
        } else if (kind == 4) {
            format_text(c, flag == 0 ? 0x104030f0 : 0x104030c4, text, text_base + (flag == 0 ? 0x7a3f : 0x79f5), {to_length, to_length, name_of(to_record)});
        } else {
            c.put8(text, 0);
        }
        invoke(c, paragraph_fn, 0x10403108, {output, text});
        if (from_club > -1) invoke(c, link_fn, 0x10403128, {output, name_of(from_record), link_club, from_record});
        invoke(c, link_fn, 0x10403140, {output, name_of(to_record), link_club, to_record});
        keep(c, 18, std::uint32_t(person) * person_size);
        if (std::int8_t(invoke(c, date_known_fn, 0x10403160, {person_record(), std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), 0})))
            invoke(c, link_fn, 0x10403180, {output, buffer, link_date, person_record()});
        return;
    }

    const auto day = news_field(c, item, 1, 0x10403190);
    keep(c, 19, std::uint32_t(day));
    const auto month = news_field(c, item, 2, 0x104031a4);
    keep(c, 20, std::uint32_t(month));
    const auto year = news_field(c, item, 3, 0x104031b8);
    keep(c, 21, std::uint32_t(year));
    invoke(c, date_text_fn, 0x104031e0, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), buffer, date_buffer_size, 5, std::uint32_t(person)});
    format_text(c, 0x104031f0, text, text_base + 0x7a8e, {buffer});
    invoke(c, begin_text_fn, 0x104031fc, {output, text});
    keep(c, 18, std::uint32_t(person) * person_size);
    if (std::int8_t(invoke(c, date_known_fn, 0x1040321c, {person_record(), std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), 0})))
        invoke(c, link_fn, 0x1040323c, {output, buffer, link_date, person_record()});
}

// 4014: a person is offered to a club; the text names the clubs and, when the offer was refused, the reasons
// 0x10410e50 gives.
void news_text_4014(Cpu& c, std::uint32_t owner, std::uint32_t item, std::uint32_t detail, std::uint32_t output) {
    Frame frame(c, 0x10403460, 0x340, {.first_gpr = 16});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    const auto date_buffer = frame.at(0x1d0), reasons = frame.at(buffer_at);
    keep(c, 23, item);
    keep(c, 20, owner);
    keep(c, 24, text);
    keep(c, 16, detail);
    keep(c, 21, c.toc(toc::people));
    keep(c, 27, output);
    keep(c, 26, c.toc(toc::clubs));
    keep(c, 22, text_base);

    const auto person = news_field(c, item, 0, 0x1040349c);
    keep(c, 31, std::uint32_t(person));
    const auto day = news_field(c, item, 1, 0x104034b0);
    keep(c, 30, std::uint32_t(day));
    const auto month = news_field(c, item, 2, 0x104034c4);
    keep(c, 29, std::uint32_t(month));
    const auto year = news_field(c, item, 3, 0x104034d8);
    keep(c, 28, std::uint32_t(year));
    const auto to_club = news_field(c, item, 5, 0x104034ec);
    keep(c, 25, std::uint32_t(to_club));
    const auto marker = std::int8_t(news_field(c, item, 6, 0x10403500));
    keep_signed(c, 19, marker);
    const auto value_7 = news_field(c, item, 7, 0x10403510);
    keep(c, 18, std::uint32_t(value_7));
    const auto value_8 = news_field(c, item, 8, 0x10403524);
    keep(c, 17, std::uint32_t(value_8));
    const auto value_9 = news_field(c, item, 9, 0x10403538);
    keep(c, 16, std::uint32_t(value_9));
    const auto club_record = [&](std::int32_t club) { return club_at(c, club); };
    const auto date_link = [&](std::uint32_t back_known, std::uint32_t back_link) {
        const auto record = person_at(c, person);
        if (std::int8_t(invoke(c, date_known_fn, back_known, {record, std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), 0})))
            invoke(c, link_fn, back_link, {output, date_buffer, link_date, record});
    };

    if (std::int8_t(detail) != 0) {
        const auto from_club = news_field(c, item, 4, 0x10403550);
        keep(c, 23, std::uint32_t(from_club));
        invoke(c, date_text_fn, 0x10403578, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date_buffer, date_buffer_size, 1, std::uint32_t(person)});
        c.put8(reasons, 0);
        c.set_f(1, c.f64(c.toc(-0x4ba0) + 0x110));
        c.put32(frame.at(stack_argument), reasons);
        const auto refused = std::int8_t(invoke_native(c, 0x10410e50, 0x104035ac,
                                                       {owner, std::uint32_t(from_club), std::uint32_t(to_club), std::uint32_t(value_8), std::uint32_t(value_9), std::uint32_t(value_7)}));
        const auto to = club_record(to_club);
        if (from_club == -1) {
            const auto length = name_length(c, to);
            if (refused != 0) format_text(c, 0x104036f8, text, text_base + 0x7c81, {length, length, date_buffer, name_of(to), reasons});
            else format_text(c, 0x1040372c, text, text_base + 0x7cfe, {length, length, date_buffer, name_of(to)});
        } else {
            const auto from = club_record(from_club);
            const auto from_length = name_length(c, from), to_length = name_length(c, to);
            if (marker == 1) {
                c.put32(frame.at(stack_argument), name_of(to));
                format_text(c, 0x10403608, text, text_base + 0x7ac7, {from_length, from_length, to_length, to_length, name_of(from), date_buffer});
            } else if (refused == 0) {
                c.put32(frame.at(stack_argument), name_of(to));
                format_text(c, 0x104036b8, text, text_base + 0x7bf9, {from_length, from_length, to_length, to_length, name_of(from), date_buffer});
            } else {
                c.put32(frame.at(stack_argument), name_of(to));
                c.put32(frame.at(stack_argument + 4), reasons);
                format_text(c, 0x10403668, text, text_base + 0x7b57, {from_length, from_length, to_length, to_length, name_of(from), date_buffer});
            }
        }
        invoke(c, begin_text_fn, 0x10403738, {output, text});
        if (from_club > -1) invoke(c, link_fn, 0x1040375c, {output, name_of(club_record(from_club)), link_club, club_record(from_club)});
        invoke(c, link_fn, 0x10403778, {output, name_of(club_record(to_club)), link_club, club_record(to_club)});
        keep(c, 16, std::uint32_t(person) * person_size);
        date_link(0x10403798, 0x104037b8);
        return;
    }

    invoke(c, date_text_fn, 0x104037dc, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date_buffer, date_buffer_size, 5, std::uint32_t(person)});
    keep(c, 16, std::uint32_t(to_club) * club_size);
    const auto to = club_record(to_club);
    const auto length = name_length(c, to);
    format_text(c, 0x1040380c, text, text_base + 0x7d61, {length, length, date_buffer, name_of(to)});
    invoke(c, begin_text_fn, 0x10403818, {output, text});
    keep(c, 17, std::uint32_t(person) * person_size);
    date_link(0x10403838, 0x10403858);
    invoke(c, link_fn, 0x10403870, {output, name_of(club_record(to_club)), link_club, club_record(to_club)});
}

// 4015: a loan or a deal between two clubs on two dates; when the second figure is the larger, its amount
// is written as cash text and handed to the formatter.
void news_text_4015(Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output) {
    Frame frame(c, 0x10403a90, 0x350, {.first_gpr = 15});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    const auto first_date = frame.at(0xb0), second_date = frame.at(0x1dc), cash_word = frame.at(0x48), cash_buffer = frame.at(0x4c);
    keep(c, 16, item);
    keep(c, 19, text);
    keep(c, 30, c.toc(toc::people));
    keep(c, 15, detail);
    keep(c, 18, c.toc(toc::clubs));
    keep(c, 31, output);
    keep(c, 17, text_base);

    const auto second_person = news_field(c, item, 6, 0x10403ac8);
    keep(c, 29, std::uint32_t(second_person));
    const auto second_day = news_field(c, item, 7, 0x10403adc);
    keep(c, 28, std::uint32_t(second_day));
    const auto second_month = news_field(c, item, 8, 0x10403af0);
    keep(c, 27, std::uint32_t(second_month));
    const auto second_year = news_field(c, item, 9, 0x10403b04);
    keep(c, 26, std::uint32_t(second_year));
    const auto date_link = [&](std::int32_t person, std::int32_t day, std::int32_t month, std::int32_t year, std::uint32_t buffer,
                               std::uint32_t back_known, std::uint32_t back_link) {
        const auto record = person_at(c, person);
        if (std::int8_t(invoke(c, date_known_fn, back_known, {record, std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), 0})))
            invoke(c, link_fn, back_link, {output, buffer, link_date, record});
    };

    if (std::int8_t(detail) == 0) {
        invoke(c, date_text_fn, 0x10403dac, {std::uint32_t(second_day), std::uint32_t(second_month), std::uint32_t(second_year), second_date, date_buffer_size, 5, std::uint32_t(second_person)});
        format_text(c, 0x10403dbc, text, text_base + 0x7fab, {second_date});
        invoke(c, begin_text_fn, 0x10403dc8, {output, text});
        keep(c, 15, std::uint32_t(second_person) * person_size);
        date_link(second_person, second_day, second_month, second_year, second_date, 0x10403de8, 0x10403e08);
        return;
    }

    const auto from_club = news_field(c, item, 0, 0x10403b1c);
    keep(c, 20, std::uint32_t(from_club));
    const auto to_club = news_field(c, item, 1, 0x10403b2c);
    keep(c, 21, std::uint32_t(to_club));
    const auto first_person = news_field(c, item, 2, 0x10403b3c);
    keep(c, 25, std::uint32_t(first_person));
    const auto first_day = news_field(c, item, 3, 0x10403b50);
    keep(c, 24, std::uint32_t(first_day));
    const auto first_month = news_field(c, item, 4, 0x10403b64);
    keep(c, 23, std::uint32_t(first_month));
    const auto first_year = news_field(c, item, 5, 0x10403b78);
    keep(c, 22, std::uint32_t(first_year));
    const auto amount = news_field(c, item, 10, 0x10403b8c);
    keep(c, 15, std::uint32_t(amount));
    const auto threshold = news_field(c, item, 11, 0x10403b9c);
    keep(c, 16, std::uint32_t(threshold));
    invoke(c, date_text_fn, 0x10403bc0, {std::uint32_t(first_day), std::uint32_t(first_month), std::uint32_t(first_year), first_date, date_buffer_size, 1, std::uint32_t(first_person)});
    invoke(c, date_text_fn, 0x10403be0, {std::uint32_t(second_day), std::uint32_t(second_month), std::uint32_t(second_year), second_date, date_buffer_size, 1, std::uint32_t(second_person)});

    const auto from = club_at(c, from_club), to = club_at(c, to_club);
    const auto to_length = name_length(c, to), from_length = name_length(c, from);
    if (amount > threshold) {
        c.put32(cash_word, std::uint32_t(amount));
        invoke(c, cash_value_fn, 0x10403bf8, {cash_word, 6});
        invoke(c, cash_text_fn, 0x10403c10, {cash_buffer, 100, c.r(3), c.r(4), 6});
        c.put32(frame.at(stack_argument), name_of(from));
        c.put32(frame.at(stack_argument + 4), first_date);
        c.put32(frame.at(stack_argument + 8), cash_buffer);
        format_text(c, 0x10403c6c, text, text_base + 0x7dba, {to_length, to_length, from_length, from_length, name_of(to), second_date});
    } else {
        c.put32(frame.at(stack_argument), name_of(from));
        c.put32(frame.at(stack_argument + 4), first_date);
        format_text(c, 0x10403cc4, text, text_base + 0x7ed8, {to_length, to_length, from_length, from_length, name_of(to), second_date});
    }
    invoke(c, begin_text_fn, 0x10403cd0, {output, text});
    invoke(c, link_fn, 0x10403cec, {output, name_of(club_at(c, from_club)), link_club, club_at(c, from_club)});
    invoke(c, link_fn, 0x10403d08, {output, name_of(club_at(c, to_club)), link_club, club_at(c, to_club)});
    keep(c, 15, std::uint32_t(first_person) * person_size);
    date_link(first_person, first_day, first_month, first_year, first_date, 0x10403d28, 0x10403d48);
    keep(c, 15, std::uint32_t(second_person) * person_size);
    date_link(second_person, second_day, second_month, second_year, second_date, 0x10403d68, 0x10403d88);
}

// 4007: three clubs and an amount (written as cash text); the unflagged variant names one club and the amount's sign.
void news_text_4007(Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output) {
    Frame frame(c, 0x10406af0, 0x260, {.first_gpr = 18});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    const auto date_buffer = frame.at(0x90), cash_word = frame.at(0x58), cash_buffer = frame.at(0x5c);
    constexpr std::uint32_t date_size = 400;
    keep(c, 23, item);
    keep(c, 24, text);
    keep(c, 25, c.toc(toc::people));
    keep(c, 18, detail);
    keep(c, 26, c.toc(toc::clubs));
    keep(c, 27, output);
    keep(c, 19, text_base);

    const auto person = news_field(c, item, 0, 0x10406b28);
    keep(c, 31, std::uint32_t(person));
    const auto day = news_field(c, item, 1, 0x10406b3c);
    keep(c, 30, std::uint32_t(day));
    const auto month = news_field(c, item, 2, 0x10406b50);
    keep(c, 29, std::uint32_t(month));
    const auto year = news_field(c, item, 3, 0x10406b64);
    keep(c, 28, std::uint32_t(year));
    const auto sign = std::int32_t(std::int8_t(news_field(c, item, 4, 0x10406b78)));
    keep_signed(c, 22, sign);
    const auto club_a = news_field(c, item, 5, 0x10406b88);
    keep(c, 20, std::uint32_t(club_a));
    const auto date_link = [&](std::uint32_t back_known, std::uint32_t back_link) {
        const auto record = person_at(c, person);
        if (std::int8_t(invoke(c, date_known_fn, back_known, {record, std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), 0})))
            invoke(c, link_fn, back_link, {output, date_buffer, link_date, record});
    };

    if (std::int8_t(detail) == 0) {
        invoke(c, date_text_fn, 0x10406d48, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date_buffer, date_size, 5, std::uint32_t(person)});
        keep(c, 18, std::uint32_t(club_a) * club_size);
        const auto club = club_at(c, club_a);
        const auto length = name_length(c, club);
        format_text(c, 0x10406d84, text, text_base + 0xa1b6, {length, length, 100, name_of(club), std::uint32_t(sign), date_buffer});
        invoke(c, begin_text_fn, 0x10406d90, {output, text});
        invoke(c, link_fn, 0x10406da8, {output, name_of(club_at(c, club_a)), link_club, club_at(c, club_a)});
        keep(c, 18, std::uint32_t(person) * person_size);
        date_link(0x10406dc8, 0x10406de8);
        return;
    }

    const auto club_b = news_field(c, item, 6, 0x10406ba0);
    keep(c, 21, std::uint32_t(club_b));
    const auto amount = news_field(c, item, 7, 0x10406bb0);
    keep(c, 18, std::uint32_t(amount));
    const auto club_c = news_field(c, item, 9, 0x10406bc0);
    keep(c, 23, std::uint32_t(club_c));
    c.put32(cash_word, std::uint32_t(amount));
    invoke(c, cash_value_fn, 0x10406bd4, {cash_word, 6});
    invoke(c, cash_text_fn, 0x10406bec, {cash_buffer, 0x32, c.r(3), c.r(4), 6});
    invoke(c, date_text_fn, 0x10406c0c, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date_buffer, date_size, 1, std::uint32_t(person)});
    keep(c, 23, std::uint32_t(club_c) * club_size);
    keep(c, 19, std::uint32_t(club_b) * club_size);
    keep(c, 20, std::uint32_t(club_a) * club_size);
    const auto a = club_at(c, club_a), b = club_at(c, club_b), third = club_at(c, club_c);
    const auto a_length = name_length(c, a), b_length = name_length(c, b), third_length = name_length(c, third);
    c.put32(frame.at(stack_argument), third_length);
    c.put32(frame.at(stack_argument + 0x4), name_of(a));
    c.put32(frame.at(stack_argument + 0x8), cash_buffer);
    c.put32(frame.at(stack_argument + 0xc), std::uint32_t(sign));
    c.put32(frame.at(stack_argument + 0x10), date_buffer);
    c.put32(frame.at(stack_argument + 0x14), name_of(b));
    c.put32(frame.at(stack_argument + 0x18), name_of(third));
    format_text(c, 0x10406c90, text, text_base + 0xa095, {a_length, a_length, b_length, b_length, 100, third_length});
    invoke(c, begin_text_fn, 0x10406c9c, {output, text});
    invoke(c, link_fn, 0x10406cb4, {output, name_of(club_at(c, club_a)), link_club, club_at(c, club_a)});
    invoke(c, link_fn, 0x10406ccc, {output, name_of(club_at(c, club_b)), link_club, club_at(c, club_b)});
    invoke(c, link_fn, 0x10406ce4, {output, name_of(club_at(c, club_c)), link_club, club_at(c, club_c)});
    keep(c, 18, std::uint32_t(person) * person_size);
    date_link(0x10406d04, 0x10406d24);
}

// 4006: a person, a club and an amount (cash text); the flagged variant picks one of four texts by two
// values (item fields 5 and 7) and names two clubs.
void news_text_4006(Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output) {
    Frame frame(c, 0x10406fd0, 0x260, {.first_gpr = 18});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    const auto date_buffer = frame.at(0x94), cash_buffer = frame.at(0x60);
    constexpr std::uint32_t date_size = 400;
    keep(c, 24, item);
    keep(c, 23, text);
    keep(c, 22, c.toc(toc::people));
    keep(c, 18, detail);
    keep(c, 25, c.toc(toc::clubs));
    keep(c, 31, output);
    keep(c, 20, text_base);

    const auto person = news_field(c, item, 0, 0x10407008);
    keep(c, 26, std::uint32_t(person));
    const auto day = news_field(c, item, 1, 0x1040701c);
    keep(c, 28, std::uint32_t(day));
    const auto month = news_field(c, item, 2, 0x10407030);
    keep(c, 29, std::uint32_t(month));
    const auto year = news_field(c, item, 3, 0x10407044);
    keep(c, 30, std::uint32_t(year));
    const auto amount = news_field(c, item, 4, 0x10407058);
    keep(c, 19, std::uint32_t(amount));
    const auto club_a = news_field(c, item, 6, 0x10407068);
    keep(c, 27, std::uint32_t(club_a));
    const auto date_link = [&](std::uint32_t back_known, std::uint32_t back_link) {
        const auto record = person_at(c, person);
        if (std::int8_t(invoke(c, date_known_fn, back_known, {record, std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), 0})))
            invoke(c, link_fn, back_link, {output, date_buffer, link_date, record});
    };

    if (std::int8_t(detail) == 0) {
        invoke(c, date_text_fn, 0x10407380, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date_buffer, date_size, 5, std::uint32_t(person)});
        const auto cash_word = frame.at(0x58);
        c.put32(cash_word, std::uint32_t(amount));
        invoke(c, cash_value_fn, 0x10407390, {cash_word, 6});
        invoke(c, cash_text_fn, 0x104073a8, {cash_buffer, 0x32, c.r(3), c.r(4), 6});
        keep(c, 19, std::uint32_t(club_a) * club_size);
        const auto club = club_at(c, club_a);
        const auto length = name_length(c, club);
        format_text(c, 0x104073e4, text, text_base + 0xa6c6, {length, length, 100, name_of(club), cash_buffer, date_buffer});
        invoke(c, begin_text_fn, 0x104073f0, {output, text});
        invoke(c, link_fn, 0x10407408, {output, name_of(club_at(c, club_a)), link_club, club_at(c, club_a)});
        keep(c, 19, std::uint32_t(person) * person_size);
        date_link(0x10407428, 0x10407448);
        return;
    }

    const auto selector = news_field(c, item, 5, 0x10407080);
    keep(c, 21, std::uint32_t(selector));
    const auto kind = std::int32_t(std::int8_t(news_field(c, item, 7, 0x10407090)));
    keep_signed(c, 18, kind);
    const auto club_b = news_field(c, item, 8, 0x104070a0);
    keep(c, 24, std::uint32_t(club_b));
    invoke(c, date_text_fn, 0x104070c4, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date_buffer, date_size, 1, std::uint32_t(person)});
    const auto cash_word = frame.at(0x5c);
    c.put32(cash_word, std::uint32_t(amount));
    invoke(c, cash_value_fn, 0x104070d4, {cash_word, 6});
    invoke(c, cash_text_fn, 0x104070ec, {cash_buffer, 0x32, c.r(3), c.r(4), 6});
    const auto b = club_at(c, club_b), a = club_at(c, club_a);
    const auto b_length = name_length(c, b), a_length = name_length(c, a);
    // The four texts differ in the format and in whether the selector itself is passed on (the other three stack words repeat).
    const bool plain = selector == 1;
    std::uint32_t format, back;
    if (kind == 2) {
        format = plain ? 0xa22a : 0xa340;
        back = plain ? 0x1040716c : 0x104071e4;
    } else {
        format = plain ? 0xa463 : 0xa58e;
        back = plain ? 0x10407260 : 0x104072d8;
    }
    c.put32(frame.at(stack_argument), b_length);
    c.put32(frame.at(stack_argument + 0x4), name_of(b));
    c.put32(frame.at(stack_argument + 0x8), name_of(a));
    c.put32(frame.at(stack_argument + 0xc), cash_buffer);
    c.put32(frame.at(stack_argument + 0x10), date_buffer);
    if (plain) {
        c.put32(frame.at(stack_argument + 0x14), name_of(b));
    } else {
        c.put32(frame.at(stack_argument + 0x14), std::uint32_t(selector));
        c.put32(frame.at(stack_argument + 0x18), name_of(b));
    }
    format_text(c, back, text, text_base + format, {b_length, b_length, a_length, a_length, 100, b_length});
    invoke(c, begin_text_fn, 0x104072e4, {output, text});
    invoke(c, link_fn, 0x10407300, {output, name_of(club_at(c, club_b)), link_club, club_at(c, club_b)});
    invoke(c, link_fn, 0x1040731c, {output, name_of(club_at(c, club_a)), link_club, club_at(c, club_a)});
    keep(c, 19, std::uint32_t(person) * person_size);
    date_link(0x1040733c, 0x1040735c);
}
}
