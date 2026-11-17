// TRANSFER_MANAGER.CPP, block "text_c": news message 4008's text (0x10407a50): a deal for a person, in ~40 wordings.
#include "transfer_manager_native_text_c.hpp"
#include "transfer_manager_native_text_c_detail.hpp"
#include <array>
#include <utility>

namespace cm::aot::transfer_manager_native {
using namespace text_c;
using guest::Frame;

namespace {
// The routine's frame (offsets from its stack pointer).
constexpr std::uint32_t date_buffer_at = 0x530, template_at = 0x2d8, list_text_at = 0x3a0, dates_at = 0x65c, ordinal_at = 0xe0,
                        amount_cash_at = 0x114, total_cash_at = 0xac, amount_word_at = 0x68, total_word_at = 0x64,
                        object_at_p1 = 0x74, object_at_p3 = 0x6c, object_text_p1 = 0x210, object_text_p3 = 0x148;
constexpr std::uint32_t template_size = 200, list_text_size = 400, date_size = 300, cash_size = 0x32;
constexpr std::uint32_t template_cell = -0x4bd8u, list_text_cell = -0x4bdcu;
constexpr std::uint32_t role_text_fn = 0x100d6300, role_note_fn = 0x100d6af0, known_kind_fn = 0x100c7a00, kind_byte_fn = 0x100c6bf0,
                        object_fn = 0x107156b0, object_text_fn = 0x10716eb0, copy_fn = 0x1077dbf0, ordinal_fn = 0x10746a70,
                        compare_fn = 0x1077dc90, cash_value_fn = 0x10009f20, cash_text_fn = 0x1000b410;
using Stack = std::initializer_list<std::pair<std::uint32_t, std::uint32_t>>;

struct Words {  // the formatter's registers r5... and stack words
    Cpu& c;
    Frame& frame;
    std::uint32_t text, text_base;
    void say_to(std::uint32_t destination, std::uint32_t back, std::uint32_t format, std::initializer_list<std::uint32_t> values,
                Stack stack = {}) const {
        for (const auto& [offset, value] : stack) c.put32(frame.at(std::int32_t(offset)), value);
        format_text(c, back, destination, text_base + format, values);
    }
    void say(std::uint32_t back, std::uint32_t format, std::initializer_list<std::uint32_t> values, Stack stack = {}) const {
        say_to(text, back, format, values, stack);
    }
};
}

void news_text_4008(Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output) {
    Frame frame(c, 0x10407a50, 0xa50, {.first_gpr = 13});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    keep(c, 15, item);
    keep(c, 16, output);
    keep(c, 27, text);
    keep(c, 28, c.toc(toc::clubs));
    keep(c, 29, text_base);
    keep(c, 21, 0xf);
    const Words words{c, frame, text, text_base};
    const auto date = frame.at(date_buffer_at);
    const auto template_text = frame.at(template_at);

    // The text template the message may overwrite (a role's text) starts as a copy of a game string.
    for (std::uint32_t offset = 0; offset < template_size; offset += 4)
        c.put32(template_text + offset, c.u32(c.toc(std::int32_t(template_cell)) + offset));

    const auto date_link = [&](std::int32_t person, std::uint32_t day, std::uint32_t month, std::uint32_t year, std::uint32_t buffer,
                               std::uint32_t back_known, std::uint32_t back_link) {
        const auto record = person_at(c, person);
        if (std::int8_t(invoke(c, date_known_fn, back_known, {record, day, month, year, 0})))
            invoke(c, link_fn, back_link, {output, buffer, link_date, record});
    };

    if (std::int8_t(detail) == 0) {
        // The short message: a person's date and a club, worded by (person + a flag times the club) mod 4.
        const auto person = news_field(c, item, 0, 0x10408d48);
        keep(c, 14, std::uint32_t(person));
        const auto day = news_field(c, item, 1, 0x10408d5c);
        keep(c, 17, std::uint32_t(day));
        const auto month = news_field(c, item, 2, 0x10408d70);
        keep(c, 18, std::uint32_t(month));
        const auto year = news_field(c, item, 3, 0x10408d84);
        keep(c, 19, std::uint32_t(year));
        const auto club = news_field(c, item, 4, 0x10408d98);
        keep(c, 13, std::uint32_t(club));
        const auto flag = std::int32_t(std::int8_t(news_field(c, item, 15, 0x10408da8)));
        keep_signed(c, 15, flag);
        invoke(c, date_text_fn, 0x10408dcc, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date, date_size, 5, std::uint32_t(person)});
        const auto remainder = std::int32_t(std::uint32_t(person) + std::uint32_t(flag) * std::uint32_t(club)) % 4;
        const auto record = club_at(c, club);
        const auto length = name_length(c, record), name = name_of(record);
        if (remainder == 1) words.say(0x10408e7c, 0xbb67, {100, length, length, date, name});
        else if (remainder == 0) words.say(0x10408e40, 0xbb21, {length, length, name, date});
        else if (remainder == 2) words.say(0x10408eb8, 0xbbba, {length, length, 3, name, date});
        else words.say(0x10408ef0, 0xbc08, {length, length, name, date});
        invoke(c, begin_text_fn, 0x10408efc, {output, text});
        invoke(c, link_fn, 0x10408f18, {output, name_of(club_at(c, club)), link_club, club_at(c, club)});
        keep(c, 13, std::uint32_t(person) * person_size);
        date_link(person, std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date, 0x10408f3c, 0x10408f60);
        return;
    }

    // The long message. The list text starts as a copy of another game string.
    const auto list_text = frame.at(list_text_at);
    for (std::uint32_t offset = 0; offset < list_text_size; offset += 4)
        c.put32(list_text + offset, c.u32(c.toc(std::int32_t(list_text_cell)) + offset));
    const auto subject = news_field(c, item, 0, 0x10407ae0);
    const auto day = news_field(c, item, 1, 0x10407af4);
    const auto month = news_field(c, item, 2, 0x10407b08);
    const auto year = news_field(c, item, 3, 0x10407b1c);
    const auto club_a = news_field(c, item, 4, 0x10407b30);
    keep(c, 30, std::uint32_t(club_a));
    const auto club_b = news_field(c, item, 5, 0x10407b40);
    keep(c, 31, std::uint32_t(club_b));
    const auto amount = news_field(c, item, 6, 0x10407b50);
    const auto marker = std::int8_t(news_field(c, item, 9, 0x10407b60));
    const auto object_arg = std::int16_t(std::uint16_t(news_field(c, item, 10, 0x10407b74)));
    const auto object_kind = std::uint16_t(news_field(c, item, 11, 0x10407b88));
    keep(c, 13, object_kind);
    const auto kind_arg = std::int32_t(std::int8_t(news_field(c, item, 12, 0x10407b98)));
    keep_signed(c, 14, kind_arg);
    const auto extra = news_field(c, item, 13, 0x10407ba8);
    const auto count = std::int32_t(std::int8_t(news_field(c, item, 14, 0x10407bb8)));
    keep_signed(c, 18, count);

    // Up to three people, each with a date: the person records and their dates as text.
    std::array<std::uint32_t, 3> records{}, days{}, months{}, years{};
    std::int32_t index = 0xf, listed = 0;
    if (count != 0) {
        keep(c, 26, 0);
        keep(c, 22, frame.at(0x94));
        keep(c, 25, 0);
        keep(c, 23, frame.at(0x88));
        keep(c, 24, frame.at(0x7c));
        keep(c, 20, 0);
        const auto limit = count < 3 ? count : 3;
        for (; std::int32_t(std::int8_t(listed)) < limit; ++listed) {
            const auto at = std::size_t(listed);
            keep(c, 21, std::uint32_t(index + 1));
            const auto person = news_field(c, item, std::uint32_t(index), 0x10407bec);
            keep(c, 17, std::uint32_t(person));
            records[at] = person_at(c, person);
            keep(c, 21, std::uint32_t(index + 2));
            days[at] = std::uint32_t(news_field(c, item, std::uint32_t(index + 1), 0x10407c18));
            keep(c, 21, std::uint32_t(index + 3));
            months[at] = std::uint32_t(news_field(c, item, std::uint32_t(index + 2), 0x10407c2c));
            keep(c, 21, std::uint32_t(index + 4));
            years[at] = std::uint32_t(news_field(c, item, std::uint32_t(index + 3), 0x10407c40));
            index += 4;
            invoke(c, date_text_fn, 0x10407c68, {days[at], months[at], years[at], frame.at(std::int32_t(dates_at + at * date_size)), date_size, 1, std::uint32_t(person)});
            keep(c, 26, std::uint32_t(listed + 1) * 4);
            keep(c, 25, std::uint32_t(listed + 1) * date_size);
            keep(c, 20, std::uint32_t(listed + 1));
        }
        const auto first = frame.at(dates_at), second = frame.at(dates_at + date_size), third = frame.at(dates_at + 2 * date_size);
        if (count == 1) invoke(c, copy_fn, 0x10407ca8, {list_text, first});
        else if (count == 2) words.say_to(list_text, 0x10407cc8, 0x243a, {first, second});
        else if (count == 3) words.say_to(list_text, 0x10407cec, 0x248f, {first, second, third});
        else {
            invoke(c, ordinal_fn, 0x10407d04, {std::uint32_t(count), frame.at(ordinal_at), cash_size, 1});
            words.say_to(list_text, 0x10407d14, 0x250e, {frame.at(ordinal_at)});
        }
    }

    // The wording selector, and optionally a role of the person (rewrites the template text).
    keep(c, 21, std::uint32_t(index + 1));
    const auto selector = std::int32_t(std::int8_t(news_field(c, item, std::uint32_t(index), 0x10407d24)));
    keep_signed(c, 17, selector);
    keep(c, 21, std::uint32_t(index + 2));
    if (news_field(c, item, std::uint32_t(index + 1), 0x10407d38) != 0) {
        keep(c, 21, std::uint32_t(index + 3));
        const auto role_arg = news_field(c, item, std::uint32_t(index + 2), 0x10407d50);
        keep(c, 15, std::uint32_t(role_arg));
        const auto role = std::int32_t(std::int16_t(news_field(c, item, std::uint32_t(index + 3), 0x10407d64)));
        keep_signed(c, 20, role);
        const auto first = invoke(c, role_text_fn, 0x10407d7c, {std::uint32_t(role), template_text, 200, 1});
        keep(c, 19, first);
        const auto second = invoke(c, role_note_fn, 0x10407da4, {std::uint32_t(subject), std::uint32_t(role), std::uint32_t(role_arg), template_text, 200, first, 1});
        keep(c, 19, second);
    }
    invoke(c, date_text_fn, 0x10407dc8, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date, date_size, 1, std::uint32_t(subject)});
    const bool known = std::int8_t(invoke(c, known_kind_fn, 0x10407dd0, {std::uint32_t(kind_arg)})) != 0;

    const auto club_record = [&](std::int32_t club) { return club_at(c, club); };
    const auto length_a = [&] { return name_length(c, club_record(club_a)); };
    const auto name_a = [&] { return name_of(club_record(club_a)); };
    const auto length_b = [&] { return name_length(c, club_record(club_b)); };
    const auto name_b = [&] { return name_of(club_record(club_b)); };
    const auto register_19 = [&] { return std::uint32_t(std::int32_t(std::int8_t(c.r(19)))); };
    const auto at = [&](std::uint32_t offset) { return frame.at(std::int32_t(offset)); };
    const bool has_b = club_b > -1;
    // The wording by selector: 1, 0, or any other value.
    const auto pick = [&](auto&& when_zero, auto&& when_one, auto&& otherwise) {
        if (selector == 1) when_one();
        else if (selector == 0) when_zero();
        else otherwise();
    };
    const auto kind_byte = [&](std::uint32_t where, std::uint32_t back) {
        return invoke(c, kind_byte_fn, back, {std::uint32_t(kind_arg), 0, 0, at(where)});
    };
    const auto fixed_byte = [&](std::uint32_t where) { return std::uint32_t(std::int32_t(c.i8(at(where)))); };
    // The text of a date object, for the wordings that carry one.
    const auto object_text = [&](std::uint32_t object_at, std::uint32_t text_at, std::uint32_t back_object, std::uint32_t back_text) {
        const auto made = invoke(c, object_fn, back_object, {at(object_at), std::uint32_t(std::int32_t(object_arg)), object_kind});
        invoke(c, object_text_fn, back_text, {made, at(text_at), 200, 0xff});
    };
    // Wordings that several branches share (the registers r5-r10 and the stack words the original sets).
    const auto selector_names = [&](std::uint32_t back, std::uint32_t format) {  // 0xaf59, 0xb015
        words.say(back, format, {length_a(), length_a(), register_19(), register_19(), length_b(), length_b()},
                  {{0x38, name_a()}, {0x3c, template_text}, {0x40, date}, {0x44, name_b()}});
    };
    const auto both_clubs = [&](std::uint32_t back, std::uint32_t format) {  // 0xb0df
        words.say(back, format, {length_a(), length_a(), length_b(), length_b(), name_a(), date}, {{0x38, name_b()}});
    };
    const auto alone_selector = [&](std::uint32_t back, std::uint32_t format) {  // 0xb16b, 0xb1fc
        words.say(back, format, {length_a(), length_a(), register_19(), register_19(), name_a(), template_text}, {{0x38, date}});
    };
    const auto alone_other = [&](std::uint32_t back) {  // 0xb291
        words.say(back, 0xb291, {length_a(), length_a(), name_a(), date});
    };
    // Four groups of three wordings, picked by the selector, that depend on the second club and on a total.
    const auto without_amount = [&](std::uint32_t b0, std::uint32_t b1, std::uint32_t b2, std::uint32_t c0, std::uint32_t c1, std::uint32_t c2) {
        if (has_b) pick([&] { selector_names(b0, 0xaf59); }, [&] { selector_names(b1, 0xb015); }, [&] { both_clubs(b2, 0xb0df); });
        else pick([&] { alone_selector(c0, 0xb16b); }, [&] { alone_selector(c1, 0xb1fc); }, [&] { alone_other(c2); });
    };

    if (known && marker != 0) {
        if (object_kind != 0) {
            object_text(object_at_p1, object_text_p1, 0x10407e00, 0x10407e10);
            if (has_b) {
                const auto ret = kind_byte(0x63, 0x10407e2c);
                const auto byte = fixed_byte(0x63);
                keep(c, 13, name_a());
                words.say(0x10407eb0, 0xa73b, {length_a(), length_a(), length_b(), length_b(), byte, byte},
                          {{0x38, length_a()}, {0x3c, length_a()}, {0x40, 100}, {0x44, name_a()}, {0x48, name_b()}, {0x4c, date}, {0x50, ret}, {0x54, name_a()}, {0x58, at(object_text_p1)}});
            } else {
                const auto ret = kind_byte(0x62, 0x10407ec8);
                const auto byte = fixed_byte(0x62);
                words.say(0x10407f1c, 0xa82f, {length_a(), length_a(), byte, byte, 100, name_a()}, {{0x38, date}, {0x3c, ret}, {0x40, at(object_text_p1)}});
            }
        } else if (has_b) {
            const auto ret = kind_byte(0x61, 0x10407f3c);
            const auto byte = fixed_byte(0x61);
            words.say(0x10407fa4, 0xa8e8, {length_a(), length_a(), length_b(), length_b(), byte, byte},
                      {{0x38, name_a()}, {0x3c, name_b()}, {0x40, date}, {0x44, ret}});
        } else {
            const auto ret = kind_byte(0x60, 0x10407fbc);
            const auto byte = fixed_byte(0x60);
            words.say(0x10408000, 0xa97f, {length_a(), length_a(), byte, byte, name_a(), date}, {{0x38, ret}});
        }
    } else if (marker != 0 && amount == 0) {
        if (object_kind != 0) {
            object_text(object_at_p3, object_text_p3, 0x10408038, 0x10408048);
            if (has_b) {
                const auto both = [&](std::uint32_t back, std::uint32_t format, std::uint32_t a_value, std::uint32_t b_value, bool swapped) {
                    keep(c, 13, name_a());
                    if (!swapped)
                        words.say(back, format, {length_a(), length_a(), a_value, a_value, b_value, b_value},
                                  {{0x38, length_a()}, {0x3c, length_a()}, {0x40, 100}, {0x44, name_a()}, {0x48, template_text}, {0x4c, date}, {0x50, name_b()}, {0x54, name_a()}, {0x58, at(object_text_p3)}});
                    else
                        words.say(back, format, {length_a(), length_a(), b_value, b_value, a_value, a_value},
                                  {{0x38, length_a()}, {0x3c, length_a()}, {0x40, 100}, {0x44, name_a()}, {0x48, name_b()}, {0x4c, template_text}, {0x50, date}, {0x54, name_a()}, {0x58, at(object_text_p3)}});
                };
                pick([&] { both(0x104080ec, 0xa9f3, register_19(), length_b(), false); },
                     [&] { both(0x10408174, 0xab0c, register_19(), length_b(), true); },
                     [&] {
                         words.say(0x104081ec, 0xac30, {length_a(), length_a(), length_b(), length_b(), length_a(), length_a()},
                                   {{0x38, 100}, {0x3c, name_a()}, {0x40, date}, {0x44, name_b()}, {0x48, name_a()}, {0x4c, at(object_text_p3)}});
                     });
            } else {
                pick([&] { words.say(0x1040825c, 0xad19, {length_a(), length_a(), register_19(), register_19(), 100, name_a()},
                                     {{0x38, template_text}, {0x3c, date}, {0x40, at(object_text_p3)}}); },
                     [&] { words.say(0x1040829c, 0xadf7, {length_a(), length_a(), 100, date, name_a(), at(object_text_p3)}); },
                     [&] { words.say(0x104082dc, 0xaeaf, {length_a(), length_a(), 100, name_a(), date, at(object_text_p3)}); });
            }
        } else {
            without_amount(0x10408368, 0x104083d4, 0x10408428, 0x10408488, 0x104084d0, 0x10408508);
        }
    } else {
        // A cash amount is written and shown by the wording.
        c.put32(at(amount_word_at), std::uint32_t(amount));
        invoke(c, cash_value_fn, 0x10408520, {at(amount_word_at), 6});
        invoke(c, cash_text_fn, 0x10408538, {at(amount_cash_at), cash_size, c.r(3), c.r(4), 6});
        const auto cash = at(amount_cash_at);
        if (count == 0) {
            if (amount > 0) {
                pick([&] { words.say(0x104085bc, 0xb2f6, {length_a(), length_a(), length_b(), length_b(), name_a(), name_b()}, {{0x38, date}, {0x3c, cash}}); },
                     [&] { words.say(0x10408630, 0xb394, {length_a(), length_a(), length_b(), length_b(), register_19(), register_19()},
                                     {{0x38, name_a()}, {0x3c, name_b()}, {0x40, template_text}, {0x44, date}, {0x48, cash}}); },
                     [&] { words.say(0x1040868c, 0xb470, {length_a(), length_a(), length_b(), length_b(), name_a(), date}, {{0x38, name_b()}, {0x3c, cash}}); });
            } else {
                without_amount(0x10408718, 0x10408784, 0x104087d8, 0x10408838, 0x10408880, 0x104088b8);
            }
        } else if (amount > 0) {
            pick([&] { words.say(0x10408958, 0xb509, {length_a(), length_a(), register_19(), register_19(), length_b(), length_b()},
                                 {{0x38, name_a()}, {0x3c, template_text}, {0x40, date}, {0x44, name_b()}, {0x48, cash}, {0x4c, list_text}}); },
                 [&] { words.say(0x104089d4, 0xb609, {length_a(), length_a(), length_b(), length_b(), register_19(), register_19()},
                                 {{0x38, name_a()}, {0x3c, name_b()}, {0x40, template_text}, {0x44, date}, {0x48, cash}, {0x4c, list_text}}); },
                 [&] { words.say(0x10408a38, 0xb72a, {length_a(), length_a(), length_b(), length_b(), name_a(), date},
                                 {{0x38, name_b()}, {0x3c, cash}, {0x40, list_text}}); });
        } else {
            pick([&] { words.say(0x10408ac4, 0xb7fe, {length_a(), length_a(), length_b(), length_b(), register_19(), register_19()},
                                 {{0x38, name_a()}, {0x3c, name_b()}, {0x40, template_text}, {0x44, date}, {0x48, list_text}}); },
                 [&] { words.say(0x10408b20, 0xb8e5, {length_a(), length_a(), length_b(), length_b(), name_a(), date}, {{0x38, name_b()}, {0x3c, list_text}}); },
                 [&] { words.say(0x10408b7c, 0xb9b2, {length_a(), length_a(), length_b(), length_b(), name_a(), date}, {{0x38, name_b()}, {0x3c, list_text}}); });
        }
    }

    invoke(c, begin_text_fn, 0x10408b88, {output, text});
    invoke(c, link_fn, 0x10408ba4, {output, name_a(), link_club, club_record(club_a)});
    if (club_b != -1) invoke(c, link_fn, 0x10408bc8, {output, name_b(), link_club, club_record(club_b)});
    if (extra != 0) {
        c.put32(at(total_word_at), std::uint32_t(extra) + std::uint32_t(amount));
        invoke(c, cash_value_fn, 0x10408bf0, {at(total_word_at), 6});
        invoke(c, cash_text_fn, 0x10408c08, {at(total_cash_at), cash_size, c.r(3), c.r(4), 6});
        if (std::int32_t(invoke(c, compare_fn, 0x10408c14, {at(amount_cash_at), at(total_cash_at)})) != 0) {
            if (amount != 0) words.say(0x10408c3c, 0xba7b, {at(total_cash_at)});
            else words.say(0x10408c54, 0xbad0, {at(total_cash_at)});
            invoke(c, paragraph_fn, 0x10408c60, {output, text});
        }
    }
    keep(c, 13, std::uint32_t(subject) * person_size);
    date_link(subject, std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date, 0x10408c88, 0x10408cac);

    // The dates of the listed people, linked.
    if (count != 0 && count <= 3) {
        keep(c, 21, 0);
        keep(c, 17, at(0xa0));
        keep(c, 19, 0);
        keep(c, 15, at(0x94));
        keep(c, 14, at(0x88));
        keep(c, 13, at(0x7c));
        for (std::int32_t i = 0; std::int32_t(std::int8_t(i)) < count; ++i) {
            const auto row = std::size_t(i);
            keep(c, 20, records[row]);
            if (std::int8_t(invoke(c, date_known_fn, 0x10408cf4, {records[row], days[row], months[row], years[row], 0})))
                invoke(c, link_fn, 0x10408d14, {output, at(dates_at + row * date_size), link_date, records[row]});
            keep(c, 13, at(0x7c + 4 * std::uint32_t(i + 1)));
            keep(c, 14, at(0x88 + 4 * std::uint32_t(i + 1)));
            keep(c, 15, at(0x94 + 4 * std::uint32_t(i + 1)));
            keep(c, 17, at(0xa0 + 4 * std::uint32_t(i + 1)));
            keep(c, 19, std::uint32_t(i + 1) * date_size);
            keep(c, 21, std::uint32_t(i + 1));
        }
    }
}
}
