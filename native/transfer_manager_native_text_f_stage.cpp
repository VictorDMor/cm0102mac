// TRANSFER_MANAGER.CPP, block "text_f": the texts of the transfer news messages 0x1040d0d0 and 0x1040d820, which
// first bring a word of the stored news item up to the stage of the offer they name.
#include "transfer_manager_native_text_f.hpp"
#include "transfer_manager_native_text_a_detail.hpp"

namespace cm::aot::transfer_manager_native {
using namespace text_a;

namespace {
constexpr std::uint32_t news_slot_size = 0x2c0, news_slot_club = 4, offer_serial = 0x2c, offer_stage = 0x34, club_stored = 0x90;

// The offer list's entry for `slot` (a slot past the list's size reads the list's first word instead).
std::uint32_t offer_of(Cpu& c, std::uint32_t slot) {
    const auto manager = c.u32(c.toc(-0x7b40));
    const auto size = c.u32(manager + 4);
    const auto at = std::int32_t(slot) < std::int32_t(size) ? c.u32(manager) + slot * 4 : c.u32(manager);
    return c.u32(at);
}
// The current club's news slot (NEWS.CPP's table at the session cell).
std::uint32_t news_slot_word(Cpu& c, std::uint32_t table) {
    return c.u32(table + std::uint32_t(std::int32_t(c.i16(table + 0x2c58))) * news_slot_size + news_slot_club);
}
// NEWS.CPP 0x102cd0c0: sets word `index` of the stored item in the current club's slot.
void store_news_word(Cpu& c, std::uint32_t table, std::uint32_t item, std::uint32_t site, std::uint32_t index, std::uint32_t value,
                     std::uint32_t last) {
    const auto news = c.u32(c.toc(-0x7bf0));
    const auto slot = news_slot_word(c, table);
    invoke(c, put_news_word, after(site), {news, slot, c.u32(item + 0xdc), index, value, last});
}
}

// 0x1040d0d0: a person, a club (field 4) and a second club (field 5, -1 for none); field 10 is a stage code that,
// when 1, is replaced by the stage of the offer named by fields 11 and 12 (and stored in word 10 of the item).
void news_text_1040d0d0(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1040d0d0, 0x290, {.first_gpr = 15});
    constexpr std::uint32_t description = 0x118, word_a = 0xb4, word_b = 0x50, stage_word = 10;
    const auto table = c.toc(-0x7bb0), words = c.toc(-0x7b98), buffer = c.toc(buffer_cell), news_cell = c.toc(-0x7bf0);
    const auto people_cell = c.toc(toc::people), text = c.toc(text_cell);
    const Clubs clubs{c, c.toc(toc::clubs)};
    keep(c, 21, item);
    keep(c, 17, table);
    keep(c, 18, words);
    keep(c, 16, full);
    keep(c, 27, buffer);
    keep(c, 31, output);
    keep(c, 20, news_cell);
    keep(c, 28, people_cell);
    keep(c, 29, clubs.cell);
    keep(c, 19, text);
    const auto person = field(c, item, 0, 0x1040d110);
    keep(c, 26, person);
    const auto day = field(c, item, 1, 0x1040d124);
    keep(c, 25, day);
    const auto month = field(c, item, 2, 0x1040d138);
    keep(c, 24, month);
    const auto year = field(c, item, 3, 0x1040d14c);
    keep(c, 23, year);
    const auto first = field(c, item, 4, 0x1040d160);
    keep(c, 30, first);
    auto stage = std::int8_t(field(c, item, 10, 0x1040d170));
    keep_signed(c, 22, stage);
    if (stage == 1) {
        const auto slot = field(c, item, 11, 0x1040d188);
        keep(c, 15, slot);
        const auto serial = field(c, item, 12, 0x1040d198);
        const auto offer = offer_of(c, slot);
        if (offer != 0 && c.u32(offer + offer_serial) == serial) {
            const auto stored = c.u8(offer + offer_stage);
            if (std::int8_t(stored) != 1) {
                keep(c, 22, stored);
                store_news_word(c, table, item, 0x1040d210, stage_word, std::uint32_t(std::int32_t(std::int8_t(stored))), 0);
                stage = std::int8_t(stored);
            }
        } else if (offer != 0 && person == c.u32(offer + 0xc) && first == c.u32(offer + 8)) {
            keep(c, 22, 0);
            store_news_word(c, table, item, 0x1040d260, stage_word, 0, 0);
            stage = 0;
        } else {
            keep(c, 22, 4);
            store_news_word(c, table, item, 0x1040d290, stage_word, 4, 0);
            stage = 4;
        }
    }
    const auto desc = frame.at(description);
    // The closing sentence of the long text for stage 0 and 3 (the formatter is told the club's name length twice).
    const auto closing = [&](std::uint32_t site) {
        say(c, frame, site, buffer, text + text_page - 0xf07, {clubs.long_len(first), clubs.long_len(first), clubs.long_name(first)});
    };
    if (std::int8_t(full) != 0) {
        const auto second = field(c, item, 5, 0x1040d2a4);
        keep(c, 20, second);
        const auto word = field(c, item, 9, 0x1040d2b4);
        keep(c, 15, word);
        describe(c, 0x1040d2d8, day, month, year, desc, 300, 1, person);
        if (second != 0xffffffffu) {
            invoke(c, 0x10168a00, after(0x1040d2fc), {c.u32(words) + word * 0x128, frame.at(word_a), 100, 10});
            say(c, frame, 0x1040d36c, buffer, text + text_page - 0x10be,
                {100, clubs.long_len(second), clubs.long_len(second), clubs.long_len(first), clubs.long_len(first), 100, 100, desc,
                 clubs.long_name(second), clubs.long_name(first), frame.at(word_a)});
        } else {
            invoke(c, 0x10168a00, after(0x1040d38c), {c.u32(words) + word * 0x128, frame.at(word_b), 100, 10});
            say(c, frame, 0x1040d3d8, buffer, text + text_page - 0xfcd,
                {100, clubs.long_len(first), clubs.long_len(first), 100, 100, desc, clubs.long_name(first), frame.at(word_b)});
        }
        begin_text(c, 0x1040d3e4, output, buffer);
        if (stage == 0) closing(0x1040d41c);
        else if (stage == 3) closing(0x1040d45c);
        const auto person_offset = person * person_size;
        keep(c, 15, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040d47c, record, day, month, year)) link(c, 0x1040d49c, output, desc, link_person, record);
        link(c, 0x1040d4b8, output, clubs.long_name(first), link_club, clubs.at(first));
        if (second != 0xffffffffu) link(c, 0x1040d4dc, output, clubs.long_name(second), link_club, clubs.at(second));
    } else {
        describe(c, 0x1040d500, day, month, year, desc, 300, 5, person);
        const auto first_offset = first * club_size;
        keep(c, 15, first_offset);
        say(c, frame, 0x1040d538, buffer, text + text_page - 0xecb, {100, clubs.long_len(first), clubs.long_len(first), desc, clubs.long_name(first)});
        begin_text(c, 0x1040d544, output, buffer);
        const auto person_offset = person * person_size;
        keep(c, 16, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040d564, record, day, month, year)) link(c, 0x1040d584, output, desc, link_person, record);
        link(c, 0x1040d59c, output, clubs.long_name(first), link_club, clubs.at(first));
    }
}

// 0x1040d820: a person, two clubs (fields 4 and 5), a count (field 8), a month and year (fields 10 and 11) and a
// contract-style date object; the stage word 9 is first brought up to the stage of the offer named by fields 6 and 7.
void news_text_1040d820(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1040d820, 0x360, {.first_gpr = 13});
    constexpr std::uint32_t date_object = 0x48, contract_date = 0x50, contract_count = 0x5c, contract_text = 0xb4, date_text = 0x118,
                            description = 0x1e0, stage_word = 9;
    const auto table = c.toc(-0x7bb0), buffer = c.toc(buffer_cell), people_cell = c.toc(toc::people), text = c.toc(text_cell);
    const Clubs clubs{c, c.toc(toc::clubs)};
    keep(c, 14, item);
    keep(c, 20, table);
    keep(c, 23, buffer);
    keep(c, 15, full);
    keep(c, 13, people_cell);
    keep(c, 27, output);
    keep(c, 24, clubs.cell);
    keep(c, 25, text);
    auto stage = std::int8_t(field(c, item, 9, 0x1040d858));
    keep_signed(c, 21, stage);
    if (stage == 0) {
        const auto slot = field(c, item, 6, 0x1040d86c);
        keep(c, 16, slot);
        const auto serial = field(c, item, 7, 0x1040d87c);
        const auto offer = offer_of(c, slot);
        if (offer != 0 && c.u32(offer + offer_serial) == serial) {
            const auto stored = c.u8(offer + offer_stage);
            if (std::int8_t(stored) != 0) {
                keep(c, 21, stored);
                store_news_word(c, table, item, 0x1040d8f8, stage_word, std::uint32_t(std::int32_t(std::int8_t(stored))), item);
                stage = std::int8_t(stored);
            }
        }
    }
    const auto desc = frame.at(description), date_words = frame.at(date_text);
    if (std::int8_t(full) != 0) {
        const auto person = field(c, item, 0, 0x1040d90c);
        keep(c, 31, person);
        const auto day = field(c, item, 1, 0x1040d920);
        keep(c, 30, day);
        const auto month = field(c, item, 2, 0x1040d934);
        keep(c, 29, month);
        const auto year = field(c, item, 3, 0x1040d948);
        keep(c, 28, year);
        const auto first = field(c, item, 4, 0x1040d95c);
        keep(c, 26, first);
        const auto second = field(c, item, 5, 0x1040d96c);
        keep(c, 18, second);
        const auto count = field(c, item, 8, 0x1040d97c);
        keep(c, 17, count);
        const auto date_month = field(c, item, 10, 0x1040d98c) & 0xffff;
        keep(c, 16, date_month);
        const auto date_year = field(c, item, 11, 0x1040d99c) & 0xffff;
        keep(c, 15, date_year);
        const auto expected = field(c, item, 12, 0x1040d9ac);
        const auto second_offset = second * club_size;
        keep(c, 22, second_offset);
        keep(c, 18, expected);
        const auto stored = c.u32(c.u32(clubs.cell) + second_offset + club_stored);
        keep(c, 19, stored);
        const auto current = news_slot_word(c, table);
        keep(c, 20, current);
        const bool other = stored == 0 || stored != current;
        keep(c, 14, other);
        describe(c, 0x1040da0c, day, month, year, desc, 300, 1, person);
        invoke(c, 0x103721f0, after(0x1040da14), {frame.at(contract_date)});
        c.put32(frame.at(contract_count), count);
        invoke(c, 0x10372590, after(0x1040da2c), {frame.at(contract_date), frame.at(contract_text), 2, 2});
        const auto date = invoke(c, 0x107156b0, after(0x1040da3c), {frame.at(date_object), std::uint32_t(std::int32_t(std::int16_t(date_month))), date_year});
        invoke(c, 0x10716eb0, after(0x1040da4c), {date, date_words, 200, 0xff});
        if (other) {
            say(c, frame, 0x1040dab0, buffer, text + text_page - 0xe6f,
                {clubs.long_len(first), clubs.long_len(first), clubs.long_len(second), clubs.long_len(second), 100, clubs.long_name(first),
                 clubs.long_name(second), desc, date_words});
        } else {
            say(c, frame, 0x1040daf0, buffer, text + text_page - 0xdbd,
                {clubs.long_len(first), clubs.long_len(first), 100, clubs.long_name(first), desc, date_words});
        }
        begin_text(c, 0x1040dafc, output, buffer);
        if (stored == current && std::int32_t(count) > 0) {
            if (count == expected) say(c, frame, 0x1040db24, buffer, text + text_page - 0xd30, {});
            else say(c, frame, 0x1040db3c, buffer, text + text_page - 0xcf5, {frame.at(contract_text)});
            extend_text(c, 0x1040db48, output, buffer);
        }
        switch (stage) {
        case 0xf:
        case 8: say(c, frame, 0x1040db84 + 0, buffer, text + 0x49db, {clubs.long_len(second), clubs.long_len(second), clubs.long_name(second)}); break;
        case 2: say(c, frame, 0x1040dbb8, buffer, text + 0x4a18, {clubs.long_len(second), clubs.long_len(second), clubs.long_name(second)}); break;
        case 1: say(c, frame, 0x1040dbec, buffer, text + 0x4992, {clubs.short_len(second), clubs.short_len(second), clubs.short_name(second)}); break;
        default: c.put8(buffer, 0); break;
        }
        if (std::int8_t(c.u8(buffer)) != 0) extend_text(c, 0x1040dc10, output, buffer);
        link(c, 0x1040dc2c, output, clubs.long_name(first), link_club, clubs.at(first));
        link(c, 0x1040dc44, output, clubs.long_name(second), link_club, clubs.at(second));
        const auto person_offset = person * person_size;
        keep(c, 14, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040dc64, record, day, month, year)) link(c, 0x1040dc84, output, desc, link_person, record);
    } else {
        const auto person = field(c, item, 0, 0x1040dc94);
        keep(c, 15, person);
        const auto day = field(c, item, 1, 0x1040dca8);
        keep(c, 16, day);
        const auto month = field(c, item, 2, 0x1040dcbc);
        keep(c, 17, month);
        const auto year = field(c, item, 3, 0x1040dcd0);
        keep(c, 18, year);
        const auto first = field(c, item, 4, 0x1040dce4);
        keep(c, 14, first);
        describe(c, 0x1040dd08, day, month, year, desc, 300, 5, person);
        const auto first_offset = first * club_size;
        keep(c, 14, first_offset);
        say(c, frame, 0x1040dd3c, buffer, text + text_page - 0xc9d, {clubs.long_len(first), clubs.long_len(first), clubs.long_name(first), desc});
        begin_text(c, 0x1040dd48, output, buffer);
        link(c, 0x1040dd60, output, clubs.long_name(first), link_club, clubs.at(first));
        const auto person_offset = person * person_size;
        keep(c, 14, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040dd80, record, day, month, year)) link(c, 0x1040dda0, output, desc, link_person, record);
    }
}
}
