// TRANSFER_MANAGER.CPP, block "text_b": news message 4002's text (0x10401c90).
#include "transfer_manager_native_text_b.hpp"
#include "transfer_manager_native_text_b_detail.hpp"
#include <array>

namespace cm::aot::transfer_manager_native {
using namespace text_b;
using guest::Cpu;
using guest::Frame;

namespace {
constexpr std::uint32_t update_news = 0x102cd0c0;       // (news manager, player slot, item id, 0xb, state, 0)
constexpr std::uint32_t copy_text = 0x1077dbf0;         // (destination, source): strcpy
constexpr std::uint32_t ordinal_text = 0x10746a70;      // (n, buffer, size, 1)
constexpr std::uint32_t cash_value = 0x10009f20, cash_text = 0x1000b410;  // (word address, 6) -> 64-bit; (buffer, 0x32, hi, lo, 6)
constexpr std::uint32_t offer_person = 0xc, offer_club = 8, offer_id = 0x2c, offer_state = 0x34, item_id = 0xdc;
constexpr std::uint32_t listed_at = 0x58, second_at = 0x4c, third_at = 0x40;  // the three name-id arrays (3 words each)
constexpr std::uint32_t joined_at = 0x3c8, dates_at = 0x29c, extra_at = 0x10c, ordinal_at = 0xd8, wage_at = 0x70, fee_at = 0xa4;
constexpr std::uint32_t cash_word_wage = 0x38, cash_word_fee = 0x3c, text_size = 0x32, extra_size = 300;
constexpr unsigned first_extra_slot = 0xe;
// Format texts, from the text base.
constexpr std::uint32_t text_two = 0x243a, text_three = 0x248f, text_ordinal = 0x250e;
constexpr std::uint32_t text_fee_wage = 0x7131, text_fee_wage_extra = 0x71b4, text_fee_only = 0x7267, text_none = 0x72d2,
                        text_wage_extra = 0x7329, text_extra = 0x73cf, text_state_3 = 0x7460, text_state_0 = 0x749d,
                        text_plain = 0x74d6;
}

void news_text_4002(Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output) {
    Frame frame(c, 0x10401c90, 0x7b0, {.first_gpr = 13});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    const auto session = c.toc(session_cell);
    const auto news_manager_cell = c.toc(toc::news);
    keep(c, 15, item);
    keep(c, 17, session);
    keep(c, 27, text);
    keep(c, 18, detail);
    keep(c, 13, news_manager_cell);
    keep(c, 16, output);
    keep(c, 28, c.toc(toc::clubs));
    keep(c, 29, text_base);
    keep(c, 21, first_extra_slot);

    const auto say_to = [&](std::uint32_t destination, std::uint32_t back, std::uint32_t format, std::initializer_list<std::uint32_t> values) {
        c.set(3, destination);
        c.set(4, text_base + format);
        unsigned n = 5;
        for (const auto value : values) c.set(n++, value);
        c.call(format_fn, back);
    };
    const auto say = [&](std::uint32_t back, std::uint32_t format, std::initializer_list<std::uint32_t> values) {
        say_to(text, back, format, values);
    };
    const auto name_of = [&](std::int32_t club) { return club_at(c, club) + club_name; };
    const auto say_club = [&](std::uint32_t back, std::uint32_t format, std::int32_t club) {
        const auto length = std::uint32_t(club_name_len(c, club_at(c, club)));
        say(back, format, {length, length, name_of(club)});
    };
    const auto session_slot = [&] { return session + std::uint32_t(std::int32_t(c.i16(session + 0x2c58)) * 0x2c0); };
    // Tells the news that the offer's state changed (an offer record's state byte, or 4 when it is not this player's).
    const auto update_state = [&](std::uint32_t back, std::uint32_t state) {
        invoke(c, update_news, back, {c.u32(news_manager_cell), c.u32(session_slot() + 4), c.u32(item + item_id), 0xb, state, 0});
    };

    const auto person = news_field(c, item, 0, 0x10401cd0);
    keep(c, 19, std::uint32_t(person));
    const auto from_club = news_field(c, item, 4, 0x10401ce4);
    keep(c, 30, std::uint32_t(from_club));
    auto kind = std::uint32_t(std::int32_t(std::int8_t(news_field(c, item, 0xb, 0x10401cf4))));
    keep(c, 14, kind);
    if (std::int8_t(kind) == 1) {
        const auto offer_index = news_field(c, item, 6, 0x10401d0c);
        keep(c, 20, std::uint32_t(offer_index));
        const auto wanted_id = news_field(c, item, 7, 0x10401d1c);
        // The manager's offers (array at +0, count at +4); an index past the end reads the first one.
        const auto offers = c.u32(c.toc(toc::manager));
        const auto count = c.i32(offers + 4);
        const auto array = c.u32(offers);
        const auto offer = c.u32(offer_index < count ? array + std::uint32_t(offer_index) * 4 : array);
        if (offer == 0 || c.i32(offer + offer_id) != wanted_id) {
            if (offer == 0 || person != c.i32(offer + offer_person) || from_club != c.i32(offer + offer_club)) {
                kind = 4;
                keep(c, 14, kind);
                update_state(0x10401e14, 4);
            } else {
                kind = 0;
                keep(c, 14, kind);
                update_state(0x10401de4, 0);
            }
        } else {
            const auto state = c.u8(offer + offer_state);
            if (std::int8_t(state) != 1) {
                kind = state;
                keep(c, 14, kind);
                update_state(0x10401d94, std::uint32_t(std::int32_t(std::int8_t(state))));
            }
        }
    }

    if (std::int8_t(detail) == 0) {
        const auto surname = news_field(c, item, 1, 0x104023c4);
        keep(c, 13, std::uint32_t(surname));
        const auto given = news_field(c, item, 2, 0x104023d8);
        keep(c, 14, std::uint32_t(given));
        const auto nationality = news_field(c, item, 3, 0x104023ec);
        keep(c, 15, std::uint32_t(nationality));
        const auto to_club = news_field(c, item, 5, 0x10402400);
        keep(c, 17, std::uint32_t(to_club));
        invoke(c, date_text_fn, 0x10402424, {std::uint32_t(surname), std::uint32_t(given), std::uint32_t(nationality), frame.at(dates_at),
                                             date_buffer_size, 5, std::uint32_t(person)});
        const auto length = std::uint32_t(club_name_len(c, club_at(c, to_club)));
        say(0x10402454, text_plain, {length, length, name_of(to_club), frame.at(dates_at)});
        invoke(c, begin_text_fn, 0x10402460, {output, text});
        invoke(c, link_fn, 0x1040247c, {output, name_of(from_club), link_club, club_at(c, from_club)});
        keep(c, 17, std::uint32_t(person) * person_size);
        if (std::int8_t(invoke(c, date_known_fn, 0x104024a0, {person_at(c, person), std::uint32_t(surname), std::uint32_t(given), std::uint32_t(nationality), 0})))
            invoke(c, link_fn, 0x104024c4, {output, frame.at(dates_at), link_date, person_at(c, person)});
        return;
    }

    // The original copies a 400-byte template text (a TOC cell's address) to sp+0x10c here, word by word.
    {
        const auto source = c.toc(-0x4bd4);
        for (std::uint32_t word = 0; word < 100; ++word) c.put32(frame.at(extra_at) + word * 4, c.u32(source + word * 4));
    }
    const auto surname = news_field(c, item, 1, 0x10401e50);
    const auto given = news_field(c, item, 2, 0x10401e64);
    const auto nationality = news_field(c, item, 3, 0x10401e78);
    const auto to_club = news_field(c, item, 5, 0x10401e8c);
    keep(c, 31, std::uint32_t(to_club));
    const auto wage = news_field(c, item, 0xa, 0x10401e9c);
    keep(c, 13, std::uint32_t(wage));
    const auto fee = news_field(c, item, 0xc, 0x10401eac);
    const auto extras = std::int32_t(std::int8_t(news_field(c, item, 0xd, 0x10401ebc)));
    keep_signed(c, 18, extras);
    std::array<std::uint32_t, 3> records{}, first_id{}, second_id{}, third_id{};
    if (extras != 0) {
        keep(c, 26, 0);
        keep(c, 22, frame.at(listed_at));
        keep(c, 25, 0);
        keep(c, 23, frame.at(second_at));
        keep(c, 24, frame.at(third_at));
        keep(c, 20, 0);
        const std::int32_t shown = extras < 3 ? extras : 3;
        std::uint32_t cursor = first_extra_slot;
        for (std::int32_t i = 0; std::int8_t(i) < std::int8_t(shown); ++i) {
            keep(c, 21, cursor + 1);
            const auto extra_person = news_field(c, item, cursor++, 0x10401ef4);
            keep(c, 17, std::uint32_t(extra_person));
            records[i] = person_at(c, extra_person);
            keep(c, 21, cursor + 1);
            first_id[i] = std::uint32_t(news_field(c, item, cursor++, 0x10401f20));
            keep(c, 21, cursor + 1);
            second_id[i] = std::uint32_t(news_field(c, item, cursor++, 0x10401f34));
            keep(c, 21, cursor + 1);
            third_id[i] = std::uint32_t(news_field(c, item, cursor++, 0x10401f48));
            invoke(c, date_text_fn, 0x10401f70, {first_id[i], second_id[i], third_id[i], frame.at(joined_at + std::uint32_t(i) * extra_size),
                                                 extra_size, 1, std::uint32_t(extra_person)});
            keep(c, 26, std::uint32_t(i + 1) * 4);
            keep(c, 25, std::uint32_t(i + 1) * extra_size);
            keep(c, 20, std::uint32_t(i + 1));
        }
        const auto joined = frame.at(joined_at);
        if (extras == 1) {
            invoke(c, copy_text, 0x10401fb0, {frame.at(extra_at), joined});
        } else if (extras == 2) {
            say_to(frame.at(extra_at), 0x10401fd0, text_two, {joined, joined + extra_size});
        } else if (extras == 3) {
            say_to(frame.at(extra_at), 0x10401ff4, text_three, {joined, joined + extra_size, joined + 2 * extra_size});
        } else {
            invoke(c, ordinal_text, 0x1040200c, {std::uint32_t(extras), frame.at(ordinal_at), text_size, 1});
            say_to(frame.at(extra_at), 0x1040201c, text_ordinal, {frame.at(ordinal_at)});
        }
    }

    const auto dates = frame.at(dates_at);
    invoke(c, date_text_fn, 0x1040203c, {std::uint32_t(surname), std::uint32_t(given), std::uint32_t(nationality), dates, date_buffer_size, 1, std::uint32_t(person)});
    const auto joined_length = [&] { return std::uint32_t(club_name_len(c, club_at(c, to_club))); };
    // The cash text of a word (the amount, passed by address) into a buffer.
    const auto cash = [&](std::uint32_t word_at, std::uint32_t amount, std::uint32_t buffer, std::uint32_t back_value, std::uint32_t back_text) {
        c.put32(frame.at(word_at), amount);
        invoke(c, cash_value, back_value, {frame.at(word_at), 6});
        const auto high = c.r(3), low = c.r(4);
        invoke(c, cash_text, back_text, {buffer, text_size, high, low, 6});
    };
    const auto joined_name = [&] { return name_of(to_club); };
    if (fee > wage) {
        cash(cash_word_fee, std::uint32_t(fee), frame.at(fee_at), 0x10402058, 0x10402070);
        const auto length = joined_length();
        if (std::int8_t(extras) == 0) say(0x104020ac, text_fee_wage, {length, length, joined_name(), frame.at(fee_at), dates});
        else say(0x104020e8, text_fee_wage_extra, {length, length, joined_name(), frame.at(fee_at), frame.at(extra_at), dates});
    } else {
        cash(cash_word_wage, std::uint32_t(wage), frame.at(wage_at), 0x104020fc, 0x10402114);
        const auto length = joined_length();
        if (std::int8_t(extras) == 0) {
            if (wage > 0) say(0x10402158, text_fee_only, {length, length, joined_name(), frame.at(wage_at), dates});
            else say(0x1040218c, text_none, {length, length, joined_name(), dates});
        } else if (wage > 0) {
            say(0x104021d0, text_wage_extra, {length, length, joined_name(), frame.at(wage_at), frame.at(extra_at), dates});
        } else {
            say(0x10402208, text_extra, {length, length, joined_name(), frame.at(extra_at), dates});
        }
    }
    invoke(c, begin_text_fn, 0x10402214, {output, text});
    if (std::int8_t(kind) == 3) say_club(0x1040224c, text_state_3, from_club);
    else if (std::int8_t(kind) == 0) say_club(0x10402284, text_state_0, from_club);
    else c.put8(text, 0);
    if (std::int8_t(c.u8(text)) != 0) invoke(c, paragraph_fn, 0x104022a8, {output, text});
    invoke(c, link_fn, 0x104022c4, {output, name_of(from_club), link_club, club_at(c, from_club)});
    invoke(c, link_fn, 0x104022e0, {output, joined_name(), link_club, club_at(c, to_club)});
    keep(c, 13, std::uint32_t(person) * person_size);
    if (std::int8_t(invoke(c, date_known_fn, 0x10402304, {person_at(c, person), std::uint32_t(surname), std::uint32_t(given), std::uint32_t(nationality), 0})))
        invoke(c, link_fn, 0x10402328, {output, dates, link_date, person_at(c, person)});

    // The extra people (up to three): each one's date, when the record knows it, links to the text written for it.
    if (extras != 0 && extras <= 3) {
        keep(c, 21, 0);
        keep(c, 17, frame.at(0x64));
        keep(c, 19, 0);
        keep(c, 15, frame.at(listed_at));
        keep(c, 14, frame.at(second_at));
        keep(c, 13, frame.at(third_at));
        for (std::int32_t i = 0; i < extras; ++i) {
            keep(c, 20, records[i]);
            if (std::int8_t(invoke(c, date_known_fn, 0x10402370, {records[i], first_id[i], second_id[i], third_id[i], 0})))
                invoke(c, link_fn, 0x10402390, {output, frame.at(joined_at + std::uint32_t(i) * extra_size), link_date, records[i]});
            keep(c, 13, frame.at(third_at + 4 * std::uint32_t(i + 1)));
            keep(c, 14, frame.at(second_at + 4 * std::uint32_t(i + 1)));
            keep(c, 15, frame.at(listed_at + 4 * std::uint32_t(i + 1)));
            keep(c, 17, frame.at(0x64 + 4 * std::uint32_t(i + 1)));
            keep(c, 19, std::uint32_t(i + 1) * extra_size);
            keep(c, 21, std::uint32_t(i + 1));
        }
    }
}
}
