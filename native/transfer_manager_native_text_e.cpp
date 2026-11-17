// TRANSFER_MANAGER.CPP, block "text_e": the texts of news messages 3011, 4011, 4013 and 4017 (4016 is in _4016.cpp).
#include "transfer_manager_native_text_e.hpp"
#include "transfer_manager_native_text_e_detail.hpp"

namespace cm::aot::transfer_manager_native {
using namespace text_e;
using guest::Frame;

namespace {
constexpr std::int32_t staff_cell = -0x7b98;
constexpr std::uint32_t staff_size = 0x128, staff_name = 0xae, staff_name_length = 0x115;
constexpr std::uint32_t staff_link_fn = 0x100c76c0;
}

// 3011: a person on a date; the long form also names a member of the staff table (field 5) and links him.
void news_text_3011(Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output) {
    Frame frame(c, 0x1040fda0, 0x1a0, {.first_gpr = 20});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    const auto staff_table = c.toc(staff_cell);
    const auto date_buffer = frame.at(0x38);
    keep(c, 20, item);
    keep(c, 29, staff_table);
    keep(c, 30, text);
    keep(c, 23, detail);
    keep(c, 31, c.toc(toc::people));
    keep(c, 24, output);
    keep(c, 22, text_base);

    const auto person = news_field(c, item, 0, 0x1040fdd8);
    keep(c, 28, std::uint32_t(person));
    const auto day = news_field(c, item, 1, 0x1040fdec);
    keep(c, 27, std::uint32_t(day));
    const auto month = news_field(c, item, 2, 0x1040fe00);
    keep(c, 26, std::uint32_t(month));
    const auto year = news_field(c, item, 3, 0x1040fe14);
    keep(c, 25, std::uint32_t(year));
    const std::int8_t flag = std::int8_t(news_field(c, item, 4, 0x1040fe28));
    keep_signed(c, 21, flag);
    const auto member = news_field(c, item, 5, 0x1040fe38);
    keep(c, 23, std::uint32_t(member));
    const auto staff_at = [&](std::int32_t index) { return c.u32(c.toc(staff_cell)) + std::uint32_t(index) * staff_size; };
    const auto date_link = [&](std::uint32_t back_known, std::uint32_t back_link) {
        keep(c, 22, std::uint32_t(person) * person_size);
        const auto record = person_at(c, person);
        if (std::int8_t(invoke(c, date_known_fn, back_known, {record, std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), 0})))
            invoke(c, link_fn, back_link, {output, date_buffer, link_date, record});
    };

    if (std::int8_t(detail) != 0) {
        invoke(c, date_text_fn, 0x1040fe64, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date_buffer, date_buffer_size, 1, std::uint32_t(person)});
        if (flag != 0) {
            say(c, frame, 0x1040fe84, text, text_base + 0x1054b, {100, date_buffer});
        } else {
            const auto record = staff_at(member);
            const auto length = std::uint32_t(std::int32_t(c.i8(record + staff_name_length)));
            say(c, frame, 0x1040fec0, text, text_base + 0x1059e, {100, length, length, date_buffer, record + staff_name});
        }
        invoke(c, begin_text_fn, 0x1040fecc, {output, text});
        date_link(0x1040feec, 0x1040ff0c);
        keep(c, 22, std::uint32_t(member) * staff_size);
        const auto record = staff_at(member);
        const auto linked = invoke(c, staff_link_fn, 0x1040ff1c, {record});
        invoke(c, link_fn, 0x1040ff38, {output, staff_at(member) + staff_name, link_club, linked});
        return;
    }

    invoke(c, date_text_fn, 0x1040ff5c, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date_buffer, date_buffer_size, 5, std::uint32_t(person)});
    say(c, frame, flag != 0 ? 0x1040ff7c : 0x1040ff98, text, text_base + (flag != 0 ? 0x1063f : 0x10676), {100, date_buffer});
    invoke(c, begin_text_fn, 0x1040ffa4, {output, text});
    date_link(0x1040ffc4, 0x1040ffe4);
}

// 4011: the person (field 2) on a date, and the two clubs (fields 0 and 1). The marker (field 6) 0x11 swaps the
// clubs' roles; the flags byte (field 10) picks the wording, and no text is built for flags the tests do not name
// (the shared text buffer then still holds the previous message's text).
void news_text_4011(Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output) {
    Frame frame(c, 0x10409410, 0x1b0, {.first_gpr = 17});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    const auto date_buffer = frame.at(0x40);
    keep(c, 17, item);
    keep(c, 28, text);
    keep(c, 18, c.toc(toc::people));
    keep(c, 19, detail);
    keep(c, 26, c.toc(toc::clubs));
    keep(c, 29, output);
    keep(c, 27, text_base);

    const auto person = news_field(c, item, 2, 0x10409448);
    keep(c, 22, std::uint32_t(person));
    const auto day = news_field(c, item, 3, 0x1040945c);
    keep(c, 23, std::uint32_t(day));
    const auto month = news_field(c, item, 4, 0x10409470);
    keep(c, 24, std::uint32_t(month));
    const auto year = news_field(c, item, 5, 0x10409484);
    keep(c, 25, std::uint32_t(year));
    const auto date_link = [&](std::uint32_t back_known, std::uint32_t back_link) {
        keep(c, 17, std::uint32_t(person) * person_size);
        const auto record = person_at(c, person);
        if (std::int8_t(invoke(c, date_known_fn, back_known, {record, std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), 0})))
            invoke(c, link_fn, back_link, {output, date_buffer, link_date, record});
    };

    if (std::int8_t(detail) == 0) {
        invoke(c, date_text_fn, 0x10409970, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date_buffer, date_buffer_size, 5, std::uint32_t(person)});
        say(c, frame, 0x10409988, text, text_base + 0xc335, {100, date_buffer});
        invoke(c, begin_text_fn, 0x10409994, {output, text});
        date_link(0x104099b4, 0x104099d4);
        return;
    }

    const auto first_club = news_field(c, item, 0, 0x1040949c);
    keep(c, 30, std::uint32_t(first_club));
    const auto second_club = news_field(c, item, 1, 0x104094ac);
    keep(c, 31, std::uint32_t(second_club));
    const auto marker = std::int32_t(std::int8_t(news_field(c, item, 6, 0x104094bc)));
    keep_signed(c, 20, marker);
    const auto second_flag = std::int32_t(std::int8_t(news_field(c, item, 7, 0x104094cc)));
    keep_signed(c, 19, second_flag);
    const auto third_flag = std::int32_t(std::int8_t(news_field(c, item, 8, 0x104094dc)));
    keep_signed(c, 21, third_flag);
    const auto flags = std::int32_t(std::int8_t(news_field(c, item, 10, 0x104094ec)));
    keep_signed(c, 17, flags);
    invoke(c, date_text_fn, 0x10409510, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date_buffer, date_buffer_size, 1, std::uint32_t(person)});

    const auto a = club_at(c, first_club), b = club_at(c, second_club);
    // One wording: "lead" is the club whose length goes first and whose name is passed in the formatter's registers,
    // "other" is the one whose name goes to the stack.
    const auto wording = [&](std::uint32_t lead, std::uint32_t other, std::uint32_t format, std::uint32_t back) {
        const auto lead_length = name_length(c, lead), other_length = name_length(c, other);
        c.put32(frame.at(stack_argument), date_buffer);
        c.put32(frame.at(stack_argument + 4), name_of(other));
        say(c, frame, back, text, text_base + format, {lead_length, lead_length, 100, other_length, other_length, name_of(lead)});
    };
    if (marker == 0x11) {
        if (flags == 0) wording(a, b, 0xbd9b, 0x10409578);
        else if ((flags & 5) != 0) wording(a, b, 0xbe36, 0x104095dc);
        else if ((flags & 0x23) != 0) wording(a, b, 0xbf20, 0x10409640);
    } else if (flags == 0) {
        wording(b, a, 0xbfea, 0x104096a4);
    } else if (flags == 3) {
        wording(b, a, 0xc087, 0x10409708);
    } else if (flags == 4) {
        wording(b, a, 0xc155, 0x1040976c);
    } else if (flags == 0x22) {
        wording(b, a, 0xc22c, 0x104097d0);
    }
    invoke(c, begin_text_fn, 0x104097dc, {output, text});
    if (second_flag != 0) {
        const auto subject = marker == 0x11 ? b : a;
        const auto length = name_length(c, subject);
        if (marker == 0x11) {
            say(c, frame, third_flag == 0 ? 0x10409858 : 0x10409824, text, text_base + (third_flag == 0 ? 0x9ee4 : 0xc2f8), {length, length, name_of(subject)});
        } else {
            say(c, frame, third_flag == 0 ? 0x104098c8 : 0x10409894, text, text_base + (third_flag == 0 ? 0x9ee4 : 0xc2f8), {length, length, name_of(subject)});
        }
        invoke(c, paragraph_fn, 0x104098d4, {output, text});
    }
    date_link(0x104098f4, 0x10409914);
    invoke(c, link_fn, 0x10409930, {output, name_of(club_at(c, second_club)), link_club, club_at(c, second_club)});
    invoke(c, link_fn, 0x1040994c, {output, name_of(club_at(c, first_club)), link_club, club_at(c, first_club)});
}

// 4013: two clubs (fields 0 and 1) and two people on two dates; when the amount (field 10) is above its threshold
// (field 11) it is written as cash text and handed to the formatter. Kind 0x16 (field 12) picks between two sets of wordings.
void news_text_4013(Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output) {
    Frame frame(c, 0x10409bd0, 0x350, {.first_gpr = 14});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    const auto first_date = frame.at(0xb0), second_date = frame.at(0x1dc), cash_word = frame.at(0x48), cash_buffer = frame.at(0x4c);
    keep(c, 16, item);
    keep(c, 30, text);
    keep(c, 14, c.toc(toc::people));
    keep(c, 18, detail);
    keep(c, 15, c.toc(toc::clubs));
    keep(c, 31, output);
    keep(c, 17, text_base);

    const auto second_person = news_field(c, item, 6, 0x10409c08);
    keep(c, 29, std::uint32_t(second_person));
    const auto second_day = news_field(c, item, 7, 0x10409c1c);
    keep(c, 28, std::uint32_t(second_day));
    const auto second_month = news_field(c, item, 8, 0x10409c30);
    keep(c, 27, std::uint32_t(second_month));
    const auto second_year = news_field(c, item, 9, 0x10409c44);
    keep(c, 26, std::uint32_t(second_year));
    const auto date_link = [&](std::int32_t person, std::int32_t day, std::int32_t month, std::int32_t year, std::uint32_t buffer,
                               std::uint32_t back_known, std::uint32_t back_link) {
        keep(c, 15, std::uint32_t(person) * person_size);
        const auto record = person_at(c, person);
        if (std::int8_t(invoke(c, date_known_fn, back_known, {record, std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), 0})))
            invoke(c, link_fn, back_link, {output, buffer, link_date, record});
    };

    if (std::int8_t(detail) == 0) {
        invoke(c, date_text_fn, 0x10409fdc, {std::uint32_t(second_day), std::uint32_t(second_month), std::uint32_t(second_year), second_date, date_buffer_size, 5, std::uint32_t(second_person)});
        say(c, frame, 0x10409fec, text, text_base + 0x7fab, {second_date});
        invoke(c, begin_text_fn, 0x10409ff8, {output, text});
        date_link(second_person, second_day, second_month, second_year, second_date, 0x1040a018, 0x1040a038);
        return;
    }

    const auto first_club = news_field(c, item, 0, 0x10409c5c);
    keep(c, 24, std::uint32_t(first_club));
    const auto second_club = news_field(c, item, 1, 0x10409c6c);
    keep(c, 25, std::uint32_t(second_club));
    const auto first_person = news_field(c, item, 2, 0x10409c7c);
    keep(c, 20, std::uint32_t(first_person));
    const auto first_day = news_field(c, item, 3, 0x10409c90);
    keep(c, 21, std::uint32_t(first_day));
    const auto first_month = news_field(c, item, 4, 0x10409ca4);
    keep(c, 22, std::uint32_t(first_month));
    const auto first_year = news_field(c, item, 5, 0x10409cb8);
    keep(c, 23, std::uint32_t(first_year));
    const auto amount = news_field(c, item, 10, 0x10409ccc);
    keep(c, 18, std::uint32_t(amount));
    const auto threshold = news_field(c, item, 11, 0x10409cdc);
    keep(c, 19, std::uint32_t(threshold));
    const auto kind = std::int32_t(std::int16_t(news_field(c, item, 12, 0x10409cec)));
    keep_signed(c, 16, kind);
    invoke(c, date_text_fn, 0x10409d10, {std::uint32_t(first_day), std::uint32_t(first_month), std::uint32_t(first_year), first_date, date_buffer_size, 1, std::uint32_t(first_person)});
    invoke(c, date_text_fn, 0x10409d30, {std::uint32_t(second_day), std::uint32_t(second_month), std::uint32_t(second_year), second_date, date_buffer_size, 1, std::uint32_t(second_person)});

    const bool larger = amount > threshold;
    if (larger) cash_amount_text(c, cash_word, cash_buffer, 100, std::uint32_t(amount), 0x10409d48, 0x10409d60);
    const auto a = club_at(c, first_club), b = club_at(c, second_club);
    const auto a_length = name_length(c, a), b_length = name_length(c, b);
    c.put32(frame.at(stack_argument), name_of(b));
    c.put32(frame.at(stack_argument + 4), first_date);
    if (larger) c.put32(frame.at(stack_argument + 8), cash_buffer);
    std::uint32_t format, back;
    if (kind == 0x16) {
        format = larger ? 0xc368 : 0xc492;
        back = larger ? 0x10409dd0 : 0x10409e2c;
    } else {
        format = larger ? 0xc571 : 0xc684;
        back = larger ? 0x10409e98 : 0x10409ef4;
    }
    say(c, frame, back, text, text_base + format, {a_length, a_length, b_length, b_length, name_of(a), second_date});
    invoke(c, begin_text_fn, 0x10409f00, {output, text});
    invoke(c, link_fn, 0x10409f1c, {output, name_of(club_at(c, first_club)), link_club, club_at(c, first_club)});
    invoke(c, link_fn, 0x10409f38, {output, name_of(club_at(c, second_club)), link_club, club_at(c, second_club)});
    date_link(first_person, first_day, first_month, first_year, first_date, 0x10409f58, 0x10409f78);
    date_link(second_person, second_day, second_month, second_year, second_date, 0x10409f98, 0x10409fb8);
}

// 4017: a person (field 0) on a date and the clubs. The kind (field 12) picks the form: 0 a move between two clubs,
// 1 two people and an amount, anything else one club. The people's name texts come from DATABASE.CPP (0x100caeb0).
void news_text_4017(Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output) {
    Frame frame(c, 0x1040bc10, 0x4c0, {.first_gpr = 15});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    constexpr std::uint32_t name_size = 500;
    const auto first_name = frame.at(0x27c), second_name = frame.at(0x88), cash_word = frame.at(0x50), cash_buffer = frame.at(0x54);
    keep(c, 19, item);
    keep(c, 28, text);
    keep(c, 29, c.toc(toc::people));
    keep(c, 15, detail);
    keep(c, 30, c.toc(toc::clubs));
    keep(c, 31, output);
    keep(c, 18, text_base);

    const auto person = news_field(c, item, 0, 0x1040bc48);
    keep(c, 20, std::uint32_t(person));
    const auto day = news_field(c, item, 1, 0x1040bc5c);
    keep(c, 27, std::uint32_t(day));
    const auto month = news_field(c, item, 2, 0x1040bc70);
    keep(c, 26, std::uint32_t(month));
    const auto year = news_field(c, item, 3, 0x1040bc84);
    keep(c, 25, std::uint32_t(year));
    const auto link_person = [&](std::uint32_t record, std::uint32_t buffer, std::int32_t d, std::int32_t m, std::int32_t y,
                                 std::uint32_t back_known, std::uint32_t back_link) {
        if (std::int8_t(invoke(c, date_known_fn, back_known, {record, std::uint32_t(d), std::uint32_t(m), std::uint32_t(y), 0})))
            invoke(c, link_fn, back_link, {output, buffer, link_date, record});
    };

    if (std::int8_t(detail) == 0) {
        invoke(c, date_text_fn, 0x1040c12c, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), first_name, name_size, 5, std::uint32_t(person)});
        say(c, frame, 0x1040c140, text, text_base + 0xe939, {first_name});
        invoke(c, begin_text_fn, 0x1040c14c, {output, text});
        keep(c, 15, std::uint32_t(person) * person_size);
        link_person(person_at(c, person), first_name, day, month, year, 0x1040c16c, 0x1040c18c);
        return;
    }

    const auto kind = std::int8_t(news_field(c, item, 12, 0x1040bc9c));
    if (kind == 0) {
        const auto from_club = news_field(c, item, 4, 0x1040bcb0);
        keep(c, 15, std::uint32_t(from_club));
        const auto to_club = news_field(c, item, 5, 0x1040bcc0);
        keep(c, 17, std::uint32_t(person) * person_size);
        keep(c, 16, std::uint32_t(to_club));
        invoke(c, person_name_fn, 0x1040bce0, {person_at(c, person), first_name, name_size, 1});
        keep(c, 15, std::uint32_t(from_club) * club_size);
        keep(c, 16, std::uint32_t(to_club) * club_size);
        const auto from = club_at(c, from_club), to = club_at(c, to_club);
        const auto from_length = name_length(c, from), to_length = name_length(c, to);
        c.put32(frame.at(stack_argument), name_of(from));
        say(c, frame, 0x1040bd30, text, text_base + 0xe577, {to_length, to_length, from_length, from_length, first_name, name_of(to)});
        invoke(c, begin_text_fn, 0x1040bd3c, {output, text});
        link_person(person_at(c, person), first_name, day, month, year, 0x1040bd58, 0x1040bd78);
        invoke(c, link_fn, 0x1040bd90, {output, name_of(club_at(c, to_club)), link_club, club_at(c, to_club)});
        invoke(c, link_fn, 0x1040bda8, {output, name_of(club_at(c, from_club)), link_club, club_at(c, from_club)});
        return;
    }

    if (kind == 1) {
        const auto from_club = news_field(c, item, 4, 0x1040bdc0);
        keep(c, 23, std::uint32_t(from_club));
        const auto to_club = news_field(c, item, 5, 0x1040bdd0);
        keep(c, 24, std::uint32_t(to_club));
        const auto first_figure = news_field(c, item, 6, 0x1040bde0);
        keep(c, 15, std::uint32_t(first_figure));
        const auto second_figure = news_field(c, item, 7, 0x1040bdf0);
        keep(c, 16, std::uint32_t(second_figure));
        const auto second_person = news_field(c, item, 8, 0x1040be00);
        keep(c, 21, std::uint32_t(second_person));
        const auto second_day = news_field(c, item, 9, 0x1040be10);
        keep(c, 17, std::uint32_t(second_day));
        const auto second_month = news_field(c, item, 10, 0x1040be24);
        keep(c, 19, std::uint32_t(second_month));
        const auto second_year = news_field(c, item, 11, 0x1040be38);
        keep(c, 22, std::uint32_t(person) * person_size);
        keep(c, 20, std::uint32_t(second_year));
        invoke(c, person_name_fn, 0x1040be58, {person_at(c, person), first_name, name_size, 1});
        keep(c, 21, std::uint32_t(second_person) * person_size);
        invoke(c, person_name_fn, 0x1040be74, {person_at(c, second_person), second_name, name_size, 1});
        const auto from = club_at(c, from_club), to = club_at(c, to_club);
        const auto from_length = name_length(c, from), to_length = name_length(c, to);
        c.put32(frame.at(stack_argument), first_name);
        c.put32(frame.at(stack_argument + 0x4), name_of(to));
        c.put32(frame.at(stack_argument + 0x8), name_of(to));
        c.put32(frame.at(stack_argument + 0xc), name_of(from));
        c.put32(frame.at(stack_argument + 0x10), second_name);
        std::uint32_t format, back;
        if (second_figure > first_figure) {
            cash_amount_text(c, cash_word, cash_buffer, 0x32, std::uint32_t(second_figure), 0x1040be8c, 0x1040bea4);
            c.put32(frame.at(stack_argument + 0x14), cash_buffer);
            format = 0xe641;
            back = 0x1040bf18;
        } else {
            format = 0xe7aa;
            back = 0x1040bf88;
        }
        say(c, frame, back, text, text_base + format, {to_length, to_length, to_length, to_length, from_length, from_length});
        invoke(c, begin_text_fn, 0x1040bf94, {output, text});
        link_person(person_at(c, person), first_name, day, month, year, 0x1040bfb0, 0x1040bfd0);
        link_person(person_at(c, second_person), second_name, second_day, second_month, second_year, 0x1040bfec, 0x1040c00c);
        invoke(c, link_fn, 0x1040c028, {output, name_of(club_at(c, to_club)), link_club, club_at(c, to_club)});
        invoke(c, link_fn, 0x1040c044, {output, name_of(club_at(c, from_club)), link_club, club_at(c, from_club)});
        return;
    }

    const auto club = news_field(c, item, 5, 0x1040c054);
    keep(c, 16, std::uint32_t(person) * person_size);
    keep(c, 15, std::uint32_t(club));
    invoke(c, person_name_fn, 0x1040c074, {person_at(c, person), first_name, name_size, 1});
    keep(c, 15, std::uint32_t(club) * club_size);
    const auto record = club_at(c, club);
    const auto length = name_length(c, record);
    say(c, frame, 0x1040c0a8, text, text_base + 0xe8c8, {length, length, first_name, name_of(record)});
    invoke(c, begin_text_fn, 0x1040c0b4, {output, text});
    link_person(person_at(c, person), first_name, day, month, year, 0x1040c0d0, 0x1040c0f0);
    invoke(c, link_fn, 0x1040c108, {output, name_of(club_at(c, club)), link_club, club_at(c, club)});
}
}
