// TRANSFER_MANAGER.CPP, block "text_f": the texts of transfer news messages 0x1040df70, 0x1040e490 and 0x1040eda0.
// Each takes a person (field 0) with a date (fields 1-3) and one or two clubs; the short text names only the person.
#include "transfer_manager_native_text_f.hpp"
#include "transfer_manager_native_text_a_detail.hpp"

namespace cm::aot::transfer_manager_native {
using namespace text_a;

namespace {
constexpr std::uint32_t competition_size = 0x6c, competition_name = 0x1a, competition_len = 0x6a;

// Whether the club's stored word (+0x90) is empty or differs from the current news slot's club.
bool other_club_word(Cpu& c, std::uint32_t table, const Clubs& clubs, std::uint32_t club_offset) {
    const auto slot = std::uint32_t(std::int32_t(c.i16(table + 0x2c58))) * 0x2c0;
    const auto stored = c.u32(c.u32(clubs.cell) + club_offset + 0x90);
    const auto current = c.u32(table + slot + 4);
    return stored == 0 || stored != current;
}
}

// 0x1040df70: a person, two clubs (fields 4 and 5) and a month/year (fields 8 and 9) in the long text.
void news_text_1040df70(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1040df70, 0x280, {.first_gpr = 18});
    constexpr std::uint32_t date_object = 0x48, date_text = 0x50, description = 0x118;
    const auto table = c.toc(-0x7bb0), buffer = c.toc(buffer_cell), people_cell = c.toc(toc::people), text = c.toc(text_cell);
    const Clubs clubs{c, c.toc(toc::clubs)};
    keep(c, 25, table);
    keep(c, 26, buffer);
    keep(c, 27, people_cell);
    keep(c, 28, clubs.cell);
    keep(c, 29, text);
    keep(c, 18, item);
    keep(c, 31, output);
    if (std::int8_t(full) != 0) {
        const auto person = field(c, item, 0, 0x1040dfac);
        keep(c, 23, person);
        const auto day = field(c, item, 1, 0x1040dfc0);
        keep(c, 22, day);
        const auto month = field(c, item, 2, 0x1040dfd4);
        keep(c, 21, month);
        const auto year = field(c, item, 3, 0x1040dfe8);
        keep(c, 20, year);
        const auto first = field(c, item, 4, 0x1040dffc);
        keep(c, 30, first);
        const auto second = field(c, item, 5, 0x1040e00c);
        keep(c, 24, second);
        const auto date_month = field(c, item, 8, 0x1040e01c) & 0xffff;
        keep(c, 19, date_month);
        const auto date_year = field(c, item, 9, 0x1040e02c) & 0xffff;
        const auto second_offset = second * club_size;
        keep(c, 24, second_offset);
        keep(c, 18, date_year);
        const auto other = other_club_word(c, table, clubs, second_offset);
        keep(c, 25, other);
        describe(c, 0x1040e08c, day, month, year, frame.at(description), 300, 1, person);
        const auto date = invoke(c, 0x107156b0, after(0x1040e09c), {frame.at(date_object), std::uint32_t(std::int32_t(std::int16_t(date_month))), date_year});
        invoke(c, 0x10716eb0, after(0x1040e0ac), {date, frame.at(date_text), 200, 0xff});
        if (other) {
            say(c, frame, 0x1040e118, buffer, text + text_page - 0xc46,
                {clubs.long_len(second), clubs.long_len(second), 100, clubs.long_len(first), clubs.long_len(first), 100,
                 clubs.long_name(second), frame.at(description), clubs.long_name(first), frame.at(date_text)});
        } else {
            say(c, frame, 0x1040e160, buffer, text + text_page - 0xb8e,
                {100, clubs.long_len(first), clubs.long_len(first), 100, frame.at(description), clubs.long_name(first), frame.at(date_text)});
        }
        begin_text(c, 0x1040e16c, output, buffer);
        link(c, 0x1040e188, output, clubs.long_name(first), link_club, clubs.at(first));
        link(c, 0x1040e1a0, output, clubs.long_name(second), link_club, clubs.at(second));
        const auto person_offset = person * person_size;
        keep(c, 18, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040e1c0, record, day, month, year)) link(c, 0x1040e1e0, output, frame.at(description), link_person, record);
    } else {
        const auto person = field(c, item, 0, 0x1040e1f0);
        keep(c, 19, person);
        const auto day = field(c, item, 1, 0x1040e204);
        keep(c, 20, day);
        const auto month = field(c, item, 2, 0x1040e218);
        keep(c, 21, month);
        const auto year = field(c, item, 3, 0x1040e22c);
        keep(c, 22, year);
        describe(c, 0x1040e254, day, month, year, frame.at(description), 300, 5, person);
        say(c, frame, 0x1040e26c, buffer, text + text_page - 0xafb, {100, frame.at(description)});
        begin_text(c, 0x1040e278, output, buffer);
        const auto person_offset = person * person_size;
        keep(c, 18, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040e298, record, day, month, year)) link(c, 0x1040e2b8, output, frame.at(description), link_person, record);
    }
}

// 0x1040e490: a person, two clubs (fields 4 and 5), a code (field 6) choosing the sentence and a competition index (field 7).
void news_text_1040e490(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1040e490, 0x1c0, {.first_gpr = 18});
    constexpr std::uint32_t description = 0x50;
    const auto table = c.toc(-0x7bb0), buffer = c.toc(buffer_cell), people_cell = c.toc(toc::people), text = c.toc(text_cell);
    const Clubs clubs{c, c.toc(toc::clubs)};
    keep(c, 19, table);
    keep(c, 21, item);
    keep(c, 27, buffer);
    keep(c, 30, output);
    keep(c, 22, people_cell);
    keep(c, 28, clubs.cell);
    keep(c, 20, text);
    if (std::int8_t(full) != 0) {
        const auto person = field(c, item, 0, 0x1040e4cc);
        keep(c, 23, person);
        const auto day = field(c, item, 1, 0x1040e4e0);
        keep(c, 24, day);
        const auto month = field(c, item, 2, 0x1040e4f4);
        keep(c, 25, month);
        const auto year = field(c, item, 3, 0x1040e508);
        keep(c, 26, year);
        const auto first = field(c, item, 4, 0x1040e51c);
        keep(c, 29, first);
        const auto second = field(c, item, 5, 0x1040e52c);
        keep(c, 31, second);
        const auto code = std::int8_t(field(c, item, 6, 0x1040e53c));
        keep_signed(c, 18, code);
        const auto competition_index = field(c, item, 7, 0x1040e54c);
        const auto second_offset = second * club_size;
        keep(c, 31, second_offset);
        keep(c, 21, competition_index);
        const auto other = other_club_word(c, table, clubs, second_offset);
        keep(c, 19, other);
        describe(c, 0x1040e5ac, day, month, year, frame.at(description), 300, 1, person);
        const auto desc = frame.at(description);
        const auto len1 = clubs.long_len(first), len2 = clubs.long_len(second);
        const auto short1 = clubs.short_name(first), short2 = clubs.short_name(second);
        const auto long1 = clubs.long_name(first);
        const auto competitions = c.toc(-0x7bbc);
        switch (code) {
        case 4:
            if (other) say(c, frame, 0x1040e620, buffer, text + text_page - 0xabc, {len2, len2, 100, len1, len1, len1, len1, short2, desc, long1, long1});
            else say(c, frame, 0x1040e66c, buffer, text + text_page - 0x9d7, {100, len1, len1, len1, len1, desc, short1, short1});
            break;
        case 9:
            if (other) say(c, frame, 0x1040e6d4, buffer, text + text_page - 0x917, {len2, len2, 100, len1, len1, short2, desc, long1});
            else say(c, frame, 0x1040e710, buffer, text + text_page - 0x85a, {100, len1, len1, desc, short1});
            break;
        case 0x26: say(c, frame, 0x1040e764, buffer, text + text_page - 0x7c2, {100, len1, len1, len1, len1, desc, short1, short1}); break;
        case 0x1e: say(c, frame, 0x1040e7b8, buffer, text + text_page - 0x6f9, {100, len1, len1, len1, len1, desc, short1, short1}); break;
        case 0x25: say(c, frame, 0x1040e80c, buffer, text + text_page - 0x631, {100, len1, len1, len1, len1, desc, short1, short1}); break;
        case 0x1d: {
            const auto base = c.u32(competitions);
            const auto competition = base + competition_index * competition_size;
            say(c, frame, 0x1040e874, buffer, text + text_page - 0x573,
                {100, len1, len1, std::uint32_t(std::int32_t(c.i8(competition + competition_len))), desc, short1, competition + competition_name});
            break;
        }
        case 0x1f: say(c, frame, 0x1040e8d4, buffer, text + text_page - 0x4a4, {100, len1, len1, len2, len2, desc, short1, short2}); break;
        default: break;
        }
        begin_text(c, 0x1040e8e0, output, buffer);
        link(c, 0x1040e8fc, output, short1, link_club, clubs.at(first));
        link(c, 0x1040e914, output, short2, link_club, clubs.at(second));
        const auto person_offset = person * person_size;
        keep(c, 18, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040e934, record, day, month, year)) link(c, 0x1040e954, output, desc, link_person, record);
    } else {
        const auto person = field(c, item, 0, 0x1040e964);
        keep(c, 18, person);
        const auto day = field(c, item, 1, 0x1040e978);
        keep(c, 19, day);
        const auto month = field(c, item, 2, 0x1040e98c);
        keep(c, 21, month);
        const auto year = field(c, item, 3, 0x1040e9a0);
        keep(c, 23, year);
        describe(c, 0x1040e9c8, day, month, year, frame.at(description), 300, 5, person);
        say(c, frame, 0x1040e9dc, buffer, text + text_page - 0x3e5, {frame.at(description)});
        begin_text(c, 0x1040e9e8, output, buffer);
        const auto person_offset = person * person_size;
        keep(c, 18, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040ea08, record, day, month, year)) link(c, 0x1040ea28, output, frame.at(description), link_person, record);
    }
}
// 0x1040eda0: a person and a club (field 4) with a second club (field 5, -1 for none); a kind (field 7) that adds a
// second person (fields 8-11) and a code (field 6) choosing the sentence.
void news_text_1040eda0(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1040eda0, 0x2f0, {.first_gpr = 14});
    constexpr std::uint32_t first_description = 0x174, second_description = 0x48;
    const auto buffer = c.toc(buffer_cell), people_cell = c.toc(toc::people), text = c.toc(text_cell);
    const Clubs clubs{c, c.toc(toc::clubs)};
    keep(c, 29, buffer);
    keep(c, 16, item);
    keep(c, 17, people_cell);
    keep(c, 31, output);
    keep(c, 30, clubs.cell);
    keep(c, 15, text);
    const auto desc = frame.at(first_description);
    if (std::int8_t(full) != 0) {
        const auto person = field(c, item, 0, 0x1040edd8);
        keep(c, 18, person);
        const auto day = field(c, item, 1, 0x1040edec);
        keep(c, 19, day);
        const auto month = field(c, item, 2, 0x1040ee00);
        keep(c, 20, month);
        const auto year = field(c, item, 3, 0x1040ee14);
        keep(c, 21, year);
        const auto first = field(c, item, 4, 0x1040ee28);
        keep(c, 22, first);
        const auto second = field(c, item, 5, 0x1040ee38);
        keep(c, 23, second);
        const auto code = std::int8_t(field(c, item, 6, 0x1040ee48));
        keep_signed(c, 14, code);
        const auto kind = std::int8_t(field(c, item, 7, 0x1040ee58));
        keep_signed(c, 28, kind);
        describe(c, 0x1040ee7c, day, month, year, desc, 300, 1, person);
        std::uint32_t other = 0, other_day = 0, other_month = 0, other_year = 0;
        if (kind != 0) {
            other = field(c, item, 8, 0x1040ee90);
            keep(c, 27, other);
            other_day = field(c, item, 9, 0x1040eea4);
            keep(c, 26, other_day);
            other_month = field(c, item, 10, 0x1040eeb8);
            keep(c, 25, other_month);
            other_year = field(c, item, 11, 0x1040eecc);
            keep(c, 24, other_year);
            describe(c, 0x1040eef4, other_day, other_month, other_year, frame.at(second_description), 300, 1, other);
        }
        const bool second_none = second == 0xffffffffu;
        const auto first_len = clubs.long_len(first), first_name = clubs.long_name(first);
        if (kind == 0) {
            if (!second_none)
                say(c, frame, 0x1040ef5c, buffer, text + text_page - 0x3af,
                    {100, clubs.long_len(second), clubs.long_len(second), first_len, first_len, desc, clubs.long_name(second), first_name});
            else say(c, frame, 0x1040ef98, buffer, text + text_page - 0x2d1, {100, first_len, first_len, desc, first_name});
        } else if (kind == 1 && code == 0) {
            say(c, frame, 0x1040f00c, buffer, text + text_page - 0x222,
                {100, clubs.long_len(second), clubs.long_len(second), first_len, first_len, desc, clubs.long_name(second), first_name,
                 frame.at(second_description)});
        } else if (!second_none) {
            say(c, frame, 0x1040f070, buffer, text + text_page - 0x11a,
                {100, clubs.long_len(second), clubs.long_len(second), first_len, first_len, desc, clubs.long_name(second), first_name});
        } else {
            say(c, frame, 0x1040f0ac, buffer, text + text_page - 0x19, {100, first_len, first_len, desc, first_name});
        }
        begin_text(c, 0x1040f0b8, output, buffer);
        link(c, 0x1040f0d4, output, first_name, link_club, clubs.at(first));
        if (!second_none) link(c, 0x1040f0f8, output, clubs.long_name(second), link_club, clubs.at(second));
        auto person_offset = person * person_size;
        keep(c, 14, person_offset);
        auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040f118, record, day, month, year)) link(c, 0x1040f138, output, desc, link_person, record);
        if (kind != 0) {
            person_offset = other * person_size;
            keep(c, 14, person_offset);
            record = c.u32(people_cell) + person_offset;
            if (known(c, 0x1040f160, record, other_day, other_month, other_year))
                link(c, 0x1040f180, output, frame.at(second_description), link_person, record);
        }
    } else {
        const auto person = field(c, item, 0, 0x1040f190);
        keep(c, 14, person);
        const auto day = field(c, item, 1, 0x1040f1a4);
        keep(c, 18, day);
        const auto month = field(c, item, 2, 0x1040f1b8);
        keep(c, 16, month);
        const auto year = field(c, item, 3, 0x1040f1cc);
        keep(c, 19, year);
        describe(c, 0x1040f1f4, day, month, year, desc, 300, 5, person);
        say(c, frame, 0x1040f20c, buffer, text + text_page + 0xb9, {100, desc});
        begin_text(c, 0x1040f218, output, buffer);
        const auto person_offset = person * person_size;
        keep(c, 14, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040f238, record, day, month, year)) link(c, 0x1040f258, output, desc, link_person, record);
    }
}
// 0x1040f450: a person and a club (field 4) with a second club (field 5, -1 for none); the text quotes the first
// club's name word (0x10168a00) in a 100-byte buffer of the frame.
void news_text_1040f450(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1040f450, 0x270, {.first_gpr = 20});
    constexpr std::uint32_t description = 0x110, word_a = 0xac, word_b = 0x48, club_word = 0x2c;
    const auto buffer = c.toc(buffer_cell), people_cell = c.toc(toc::people), text = c.toc(text_cell);
    const Clubs clubs{c, c.toc(toc::clubs)};
    keep(c, 26, buffer);
    keep(c, 31, item);
    keep(c, 27, people_cell);
    keep(c, 21, output);
    keep(c, 28, clubs.cell);
    keep(c, 29, text);
    const auto desc = frame.at(description);
    if (std::int8_t(full) != 0) {
        const auto person = field(c, item, 0, 0x1040f488);
        keep(c, 25, person);
        const auto day = field(c, item, 1, 0x1040f49c);
        keep(c, 24, day);
        const auto month = field(c, item, 2, 0x1040f4b0);
        keep(c, 23, month);
        const auto year = field(c, item, 3, 0x1040f4c4);
        keep(c, 22, year);
        const auto first = field(c, item, 4, 0x1040f4d8);
        keep(c, 30, first);
        const auto second = field(c, item, 5, 0x1040f4e8);
        keep(c, 31, second);
        describe(c, 0x1040f50c, day, month, year, desc, 300, 1, person);
        const auto first_offset = first * club_size;
        keep(c, 20, first_offset);
        const auto first_len = clubs.long_len(first);
        if (second != 0xffffffffu) {
            invoke(c, 0x10168a00, after(0x1040f534), {c.u32(clubs.at(first) + club_word), frame.at(word_a), 100, 8});
            say(c, frame, 0x1040f598, buffer, text + text_page + 0xff,
                {clubs.long_len(second), clubs.long_len(second), 100, 100, first_len, first_len, clubs.long_name(second), desc,
                 frame.at(word_a), clubs.long_name(first)});
        } else {
            invoke(c, 0x10168a00, after(0x1040f5bc), {c.u32(clubs.at(first) + club_word), frame.at(word_b), 100, 8});
            say(c, frame, 0x1040f5fc, buffer, text + text_page + 0x1cd, {100, 100, first_len, first_len, desc, frame.at(word_b), clubs.long_name(first)});
        }
        begin_text(c, 0x1040f608, output, buffer);
        link(c, 0x1040f624, output, clubs.long_name(first), link_club, clubs.at(first));
        if (second != 0xffffffffu) link(c, 0x1040f648, output, clubs.long_name(second), link_club, clubs.at(second));
        const auto person_offset = person * person_size;
        keep(c, 20, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040f668, record, day, month, year)) link(c, 0x1040f688, output, desc, link_person, record);
    } else {
        const auto person = field(c, item, 0, 0x1040f698);
        keep(c, 20, person);
        const auto day = field(c, item, 1, 0x1040f6ac);
        keep(c, 22, day);
        const auto month = field(c, item, 2, 0x1040f6c0);
        keep(c, 23, month);
        const auto year = field(c, item, 3, 0x1040f6d4);
        keep(c, 24, year);
        describe(c, 0x1040f6fc, day, month, year, desc, 300, 5, person);
        say(c, frame, 0x1040f714, buffer, text + text_page + 0x272, {100, desc});
        begin_text(c, 0x1040f720, output, buffer);
        const auto person_offset = person * person_size;
        keep(c, 20, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040f740, record, day, month, year)) link(c, 0x1040f760, output, desc, link_person, record);
    }
}

// 0x1040f990: a person (field 2) and a club pair (fields 0 and 1) with a date (fields 3-5), and a second person
// (field 6, date fields 7-9); the long text adds a fee when field 10 is above field 11.
void news_text_1040f990(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1040f990, 0x420, {.first_gpr = 15});
    constexpr std::uint32_t first_description = 0x180, second_description = 0x2ac, fee_slot = 0x50, fee_text = 0x11c;
    constexpr std::uint32_t word_a = 0xb8, word_b = 0x54, club_word = 0x2c;
    const auto buffer = c.toc(buffer_cell), people_cell = c.toc(toc::people), text = c.toc(text_cell);
    const Clubs clubs{c, c.toc(toc::clubs)};
    keep(c, 16, item);
    keep(c, 18, buffer);
    keep(c, 19, people_cell);
    keep(c, 15, full);
    keep(c, 30, clubs.cell);
    keep(c, 31, output);
    keep(c, 17, text);
    const auto other = field(c, item, 6, 0x1040f9c4);
    keep(c, 29, other);
    const auto other_day = field(c, item, 7, 0x1040f9d8);
    keep(c, 28, other_day);
    const auto other_month = field(c, item, 8, 0x1040f9ec);
    keep(c, 27, other_month);
    const auto other_year = field(c, item, 9, 0x1040fa00);
    keep(c, 26, other_year);
    const auto desc = frame.at(first_description), other_desc = frame.at(second_description);
    if (std::int8_t(full) != 0) {
        const auto first = field(c, item, 0, 0x1040fa18);
        keep(c, 20, first);
        const auto second = field(c, item, 1, 0x1040fa28);
        keep(c, 21, second);
        const auto person = field(c, item, 2, 0x1040fa38);
        keep(c, 25, person);
        const auto day = field(c, item, 3, 0x1040fa4c);
        keep(c, 24, day);
        const auto month = field(c, item, 4, 0x1040fa60);
        keep(c, 23, month);
        const auto year = field(c, item, 5, 0x1040fa74);
        keep(c, 22, year);
        const auto fee = field(c, item, 10, 0x1040fa88);
        keep(c, 15, fee);
        const auto fee_floor = field(c, item, 11, 0x1040fa98);
        keep(c, 16, fee_floor);
        describe(c, 0x1040fabc, day, month, year, desc, 300, 1, person);
        describe(c, 0x1040fadc, other_day, other_month, other_year, other_desc, 300, 1, other);
        const auto first_len = [&] { return clubs.long_len(first); };
        const auto second_len = [&] { return clubs.long_len(second); };
        if (std::int32_t(fee) > std::int32_t(fee_floor)) {
            c.put32(frame.at(fee_slot), fee);
            const auto amount = invoke(c, 0x10009f20, after(0x1040faf4), {frame.at(fee_slot), 6});
            invoke(c, 0x1000b410, after(0x1040fb0c), {frame.at(fee_text), 100, amount, c.r(4), 6});
            const auto first_offset = first * club_size;
            keep(c, 16, first_offset);
            invoke(c, 0x10168a00, after(0x1040fb2c), {c.u32(clubs.at(first) + club_word), frame.at(word_a), 100, 8});
            say(c, frame, 0x1040fba0, buffer, text + text_page + 0x2a8,
                {second_len(), second_len(), first_len(), first_len(), 100, 100, clubs.long_name(second), other_desc, clubs.long_name(first),
                 desc, frame.at(word_a), frame.at(fee_text)});
        } else {
            const auto first_offset = first * club_size;
            keep(c, 16, first_offset);
            invoke(c, 0x10168a00, after(0x1040fbc4), {c.u32(clubs.at(first) + club_word), frame.at(word_b), 100, 8});
            say(c, frame, 0x1040fc30, buffer, text + text_page + 0x402,
                {second_len(), second_len(), first_len(), first_len(), 100, 100, clubs.long_name(second), other_desc, clubs.long_name(first),
                 desc, frame.at(word_b)});
        }
        begin_text(c, 0x1040fc3c, output, buffer);
        link(c, 0x1040fc58, output, clubs.long_name(first), link_club, clubs.at(first));
        link(c, 0x1040fc74, output, clubs.long_name(second), link_club, clubs.at(second));
        auto person_offset = person * person_size;
        keep(c, 15, person_offset);
        auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040fc94, record, day, month, year)) link(c, 0x1040fcb4, output, desc, link_person, record);
        person_offset = other * person_size;
        keep(c, 15, person_offset);
        record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040fcd4, record, other_day, other_month, other_year)) link(c, 0x1040fcf4, output, other_desc, link_person, record);
    } else {
        describe(c, 0x1040fd18, other_day, other_month, other_year, other_desc, 300, 5, other);
        say(c, frame, 0x1040fd30, buffer, text + text_page + 0x510, {100, other_desc});
        begin_text(c, 0x1040fd3c, output, buffer);
        const auto person_offset = other * person_size;
        keep(c, 15, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040fd5c, record, other_day, other_month, other_year)) link(c, 0x1040fd7c, output, other_desc, link_person, record);
    }
}
}
