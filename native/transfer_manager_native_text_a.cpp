// TRANSFER_MANAGER.CPP, block "text_a": the texts of three transfer news messages (kinds 3020, 4032, 4035).
#include "transfer_manager_native_text_a.hpp"
#include "transfer_manager_native_text_a_detail.hpp"

namespace cm::aot::transfer_manager_native {
using namespace text_a;

// Kind 3020 (NEWS.CPP 0x102c6168): a person and the club named in the item; `full` selects the longer text.
void news_text_3020(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x104106d0, 0x190, {.first_gpr = 22});
    const auto text = c.toc(text_cell), buffer = c.toc(buffer_cell);
    const auto people_cell = c.toc(toc::people);
    const Clubs clubs{c, c.toc(toc::clubs)};
    keep(c, 23, buffer);
    keep(c, 30, people_cell);
    keep(c, 24, text);
    keep(c, 25, full);
    keep(c, 26, output);
    keep(c, 27, item);
    keep(c, 31, clubs.cell);
    const auto person = field(c, item, 0, 0x10410704);
    keep(c, 22, person);
    const auto day = field(c, item, 1, 0x10410718);
    keep(c, 29, day);
    const auto month = field(c, item, 2, 0x1041072c);
    keep(c, 28, month);
    const auto year = field(c, item, 3, 0x10410740);
    keep(c, 27, year);
    const auto club = field(c, item, 4, 0x10410754);
    keep(c, 25, club);
    constexpr std::uint32_t description = 0x38;
    const auto record = [&] { return c.u32(people_cell) + person * person_size; };
    if (std::int8_t(full) != 0) {
        describe(c, 0x10410780, day, month, year, frame.at(description), 300, 1, person);
        const auto club_offset = club * club_size;
        keep(c, 25, club_offset);
        say(c, frame, 0x104107b8, buffer, text + text_page + 0x8dc,
            {100, clubs.long_len(club), clubs.long_len(club), frame.at(description), clubs.long_name(club)});
        begin_text(c, 0x104107c4, output, buffer);
        const auto person_offset = person * person_size;
        keep(c, 23, person_offset);
        if (known(c, 0x104107e4, record(), day, month, year)) link(c, 0x10410804, output, frame.at(description), link_person, record());
        link(c, 0x1041081c, output, clubs.long_name(club), link_club, clubs.at(club));
    } else {
        describe(c, 0x10410840, day, month, year, frame.at(description), 300, 5, person);
        say(c, frame, 0x10410858, buffer, text + text_page + 0x9c1, {100, frame.at(description)});
        begin_text(c, 0x10410864, output, buffer);
        const auto person_offset = person * person_size;
        keep(c, 23, person_offset);
        if (known(c, 0x10410884, record(), day, month, year)) link(c, 0x104108a4, output, frame.at(description), link_person, record());
    }
}

// Kind 4032 (0x102c669c): a person and two clubs; when the item's offer record is found, its status byte
// is written back into the item (word 10, through NEWS.CPP) and a further sentence follows the longer text.
void news_text_4032(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x10410a60, 0x150, {.first_gpr = 15});
    const auto table = c.toc(-0x7bb0), news = c.toc(-0x7bf0);
    const auto text = c.toc(text_cell), buffer = c.toc(buffer_cell);
    const auto people_cell = c.toc(toc::people);
    const Clubs clubs{c, c.toc(toc::clubs)};
    keep(c, 31, buffer);
    keep(c, 16, item);
    keep(c, 18, table);
    keep(c, 17, full);
    keep(c, 19, news);
    keep(c, 25, output);
    keep(c, 20, people_cell);
    keep(c, 21, clubs.cell);
    keep(c, 22, text);
    constexpr std::uint32_t description = 0x40;
    const auto person = field(c, item, 0, 0x10410a9c);
    keep(c, 29, person);
    const auto day = field(c, item, 1, 0x10410ab0);
    keep(c, 28, day);
    const auto month = field(c, item, 2, 0x10410ac4);
    keep(c, 27, month);
    const auto year = field(c, item, 3, 0x10410ad8);
    keep(c, 26, year);
    const auto buyer = field(c, item, 4, 0x10410aec);
    keep(c, 23, buyer);
    const auto seller = field(c, item, 5, 0x10410afc);
    keep(c, 24, seller);
    std::int32_t status = std::int8_t(field(c, item, 10, 0x10410b0c));
    keep_signed(c, 30, status);
    // The slot of the current human club in the news table is read again at each of the three updates.
    const auto news_slot = [&] { return c.u32(table + std::uint32_t(std::int32_t(c.i16(table + 0x2c58))) * 0x2c0 + 4); };
    if (status == 1) {
        const auto first = field(c, item, 6, 0x10410b24);
        keep(c, 15, first);
        const auto second = field(c, item, 7, 0x10410b34);
        const auto offers = c.u32(c.toc(-0x7b40));
        const auto slot = std::int32_t(first) < c.i32(offers + 4) ? c.u32(offers) + first * 4 : c.u32(offers);
        const auto offer = c.u32(slot);
        if (offer && c.i32(offer + 0x2c) == std::int32_t(second)) {
            const auto stored = c.u8(offer + 0x34);
            if (std::int8_t(stored) != 1) {
                keep(c, 30, stored);
                invoke(c, put_news_word, 0x10410bb0, {c.u32(news), news_slot(), c.u32(item + 0xdc), 0xb, std::uint32_t(std::int32_t(std::int8_t(stored))), 0});
            }
        } else if (offer && person == c.u32(offer + 0xc) && buyer == c.u32(offer + 8)) {
            keep(c, 30, 0);
            invoke(c, put_news_word, 0x10410c00, {c.u32(news), news_slot(), c.u32(item + 0xdc), 10, 0, 0});
        } else {
            keep(c, 30, 4);
            invoke(c, put_news_word, 0x10410c30, {c.u32(news), news_slot(), c.u32(item + 0xdc), 10, 4, 0});
        }
        status = std::int32_t(c.r(30));
    }
    const auto record = [&] { return c.u32(people_cell) + person * person_size; };
    if (std::int8_t(full) != 0) {
        describe(c, 0x10410c54, day, month, year, frame.at(description), 200, 1, person);
        const auto buyer_offset = buyer * club_size, seller_offset = seller * club_size;
        keep(c, 15, buyer_offset);
        keep(c, 16, seller_offset);
        say(c, frame, 0x10410ca4, buffer, text + text_page + 0x9fc,
            {clubs.short_len(seller), clubs.short_len(seller), clubs.short_len(buyer), clubs.short_len(buyer),
             clubs.short_name(seller), clubs.short_name(buyer), frame.at(description)});
        begin_text(c, 0x10410cb0, output, buffer);
        link(c, 0x10410cc8, output, clubs.short_name(buyer), link_club, clubs.at(buyer));
        link(c, 0x10410ce0, output, clubs.short_name(seller), link_club, clubs.at(seller));
        const auto person_offset = person * person_size;
        keep(c, 16, person_offset);
        if (known(c, 0x10410d00, record(), day, month, year)) link(c, 0x10410d20, output, frame.at(description), link_person, record());
        const auto suffix = std::int8_t(status);
        if (suffix == 3) {
            say(c, frame, 0x10410d54, buffer, text + 0x7460, {clubs.long_len(buyer), clubs.long_len(buyer), clubs.long_name(buyer)});
        } else if (suffix == 0) {
            say(c, frame, 0x10410d8c, buffer, text + text_page + 0xa92, {clubs.long_len(buyer), clubs.long_len(buyer), clubs.long_name(buyer)});
        } else {
            c.put8(buffer, 0);
        }
        if (std::int8_t(c.u8(buffer)) != 0) extend_text(c, 0x10410db0, output, buffer);
    } else {
        describe(c, 0x10410dd4, day, month, year, frame.at(description), 200, 5, person);
        say(c, frame, 0x10410de8, buffer, text + text_page + 0xacd, {frame.at(description)});
        begin_text(c, 0x10410df4, output, buffer);
        const auto person_offset = person * person_size;
        keep(c, 15, person_offset);
        if (known(c, 0x10410e14, record(), day, month, year)) link(c, 0x10410e34, output, frame.at(description), link_person, record());
    }
}

// Kind 4035 (0x102c670c): a person, a fee and up to three clubs.
void news_text_4035(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x104116e0, 0x250, {.first_gpr = 18});
    const auto text = c.toc(text_cell), buffer = c.toc(buffer_cell);
    const auto people_cell = c.toc(toc::people);
    const Clubs clubs{c, c.toc(toc::clubs)};
    keep(c, 20, item);
    keep(c, 22, buffer);
    keep(c, 31, people_cell);
    keep(c, 21, full);
    keep(c, 23, clubs.cell);
    keep(c, 24, text);
    keep(c, 26, output);
    constexpr std::uint32_t description = 0x88, money = 0x54, money_slot = 0x50;
    const auto person = field(c, item, 0, 0x10411714);
    keep(c, 30, person);
    const auto day = field(c, item, 1, 0x10411728);
    keep(c, 29, day);
    const auto month = field(c, item, 2, 0x1041173c);
    keep(c, 28, month);
    const auto year = field(c, item, 3, 0x10411750);
    keep(c, 27, year);
    const auto third = field(c, item, 4, 0x10411764);
    keep(c, 18, third);
    const auto fee = field(c, item, 5, 0x10411774);
    keep(c, 19, fee);
    const auto first = field(c, item, 6, 0x10411784);
    keep(c, 25, first);
    const auto second = field(c, item, 7, 0x10411794);
    keep(c, 20, second);
    const auto record = [&] { return c.u32(people_cell) + person * person_size; };
    if (std::int8_t(full) == 0) {
        describe(c, 0x104117c0, day, month, year, frame.at(description), 400, 5, person);
        const auto first_offset = first * club_size;
        keep(c, 18, first_offset);
        say(c, frame, 0x104117f8, buffer, text + text_page + 0xf34,
            {clubs.long_len(first), clubs.long_len(first), 100, clubs.long_name(first), frame.at(description)});
        begin_text(c, 0x10411804, output, buffer);
        link(c, 0x1041181c, output, clubs.long_name(first), link_club, clubs.at(first));
        const auto person_offset = person * person_size;
        keep(c, 18, person_offset);
        if (known(c, 0x1041183c, record(), day, month, year)) link(c, 0x1041185c, output, frame.at(description), link_person, record());
        return;
    }
    c.put32(frame.at(money_slot), fee);
    const auto amount = invoke(c, 0x10009f20, 0x10411874, {frame.at(money_slot), 6});
    invoke(c, 0x1000b410, 0x1041188c, {frame.at(money), 0x32, amount, c.r(4), 6});
    describe(c, 0x104118a8, day, month, year, frame.at(description), 400, 1, person);
    const auto third_offset = third * club_size;
    keep(c, 21, third_offset);
    const auto second_offset = second * club_size;
    keep(c, 24, second_offset);
    const auto first_offset = first * club_size;
    keep(c, 25, first_offset);
    say(c, frame, 0x10411928, buffer, text + text_page + 0xf90,
        {clubs.long_len(first), clubs.long_len(first), 100, clubs.long_len(second), clubs.long_len(second),
         clubs.long_len(third), clubs.long_len(third), clubs.long_name(first), frame.at(money), frame.at(description),
         clubs.long_name(second), clubs.long_name(third)});
    begin_text(c, 0x10411934, output, buffer);
    link(c, 0x1041194c, output, clubs.long_name(first), link_club, clubs.at(first));
    link(c, 0x10411964, output, clubs.long_name(second), link_club, clubs.at(second));
    link(c, 0x1041197c, output, clubs.long_name(third), link_club, clubs.at(third));
    const auto person_offset = person * person_size;
    keep(c, 18, person_offset);
    if (known(c, 0x1041199c, record(), day, month, year)) link(c, 0x104119bc, output, frame.at(description), link_person, record());
}
}
