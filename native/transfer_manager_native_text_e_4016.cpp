// TRANSFER_MANAGER.CPP, block "text_e": the text of news message 4016 (0x1040a280), a club, a person and a second club
// in about forty wordings picked by the kind (field 7).
#include "transfer_manager_native_text_e.hpp"
#include "transfer_manager_native_text_e_detail.hpp"

namespace cm::aot::transfer_manager_native {
using namespace text_e;
using guest::Frame;

namespace {
constexpr std::uint32_t index_text_buffer = 0x58, index_text_fn = 0x10168a00, club_index_word = 0x2c;
constexpr std::int32_t no_club = -1;

// The wordings of the long form, by kind and by whether a second club (field 0) is named. The kind lists in the
// original's test order; the tests are on disjoint values, so only the order of the fall-through to the last wording matters:
// kinds 3, 5 and 0x23 have a wording only with a second club.
struct Story {
    Cpu& c;
    const Frame& frame;
    std::uint32_t text, text_base;
    std::uint32_t date;
    std::int32_t from, club, third;  // fields 0, 1 and 6

    std::uint32_t len(std::int32_t index) const { return name_length(c, club_at(c, index)); }
    std::uint32_t name(std::int32_t index) const { return name_of(club_at(c, index)); }
    void say_at(std::uint32_t back, std::uint32_t format, std::initializer_list<std::uint32_t> values) const {
        say(c, frame, back, text, text_base + format, values);
    }
    // "from, club, third": the three clubs' lengths and names, the date text between them.
    void with_from(std::uint32_t back, std::uint32_t format) const {
        say_at(back, format, {100, len(from), len(from), len(club), len(club), len(third), len(third), date, name(from), name(club), name(third)});
    }
    void without_from(std::uint32_t back, std::uint32_t format) const {
        say_at(back, format, {100, len(club), len(club), len(third), len(third), date, name(club), name(third)});
    }
    // The wordings that name the third club first.
    void third_first_with_from(std::uint32_t back, std::uint32_t format, std::int32_t other) const {
        say_at(back, format, {len(third), len(third), 100, len(other), len(other), name(third), date, name(other)});
    }
    void third_first_alone(std::uint32_t back, std::uint32_t format) const {
        say_at(back, format, {len(third), len(third), 100, name(third), date});
    }
    void from_and_club(std::uint32_t back, std::uint32_t format) const {
        say_at(back, format, {100, len(from), len(from), len(club), len(club), date, name(from), name(club)});
    }
    void club_alone(std::uint32_t back, std::uint32_t format) const {
        say_at(back, format, {100, len(club), len(club), date, name(club)});
    }
    void club_three_times(std::uint32_t back, std::uint32_t format) const {
        say_at(back, format, {100, len(from), len(from), len(club), len(club), len(club), len(club), date, name(from), name(club), name(club)});
    }
    void club_four_times(std::uint32_t back, std::uint32_t format) const {
        say_at(back, format, {100, len(club), len(club), len(club), len(club), date, name(club), name(club)});
    }
    void from_twice(std::uint32_t back, std::uint32_t format) const {
        say_at(back, format, {100, len(from), len(from), len(club), len(club), len(from), len(from), date, name(from), name(club), name(from)});
    }
};
}

void news_text_4016(Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output) {
    Frame frame(c, 0x1040a280, 0x220, {.first_gpr = 18});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    const auto date_buffer = frame.at(0xbc);
    keep(c, 20, item);
    keep(c, 29, text);
    keep(c, 22, c.toc(toc::people));
    keep(c, 18, detail);
    keep(c, 28, c.toc(toc::clubs));
    keep(c, 26, output);
    keep(c, 21, text_base);

    const auto club = news_field(c, item, 1, 0x1040a2b8);
    keep(c, 31, std::uint32_t(club));
    const auto person = news_field(c, item, 2, 0x1040a2c8);
    keep(c, 27, std::uint32_t(person));
    const auto day = news_field(c, item, 3, 0x1040a2dc);
    keep(c, 23, std::uint32_t(day));
    const auto month = news_field(c, item, 4, 0x1040a2f0);
    keep(c, 24, std::uint32_t(month));
    const auto year = news_field(c, item, 5, 0x1040a304);
    keep(c, 25, std::uint32_t(year));
    const std::int8_t flag = std::int8_t(news_field(c, item, 8, 0x1040a318));
    keep_signed(c, 18, flag);
    const auto date_link = [&](std::uint32_t back_known, std::uint32_t back_link) {
        keep(c, 19, std::uint32_t(person) * person_size);
        const auto record = person_at(c, person);
        if (std::int8_t(invoke(c, date_known_fn, back_known, {record, std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), 0})))
            invoke(c, link_fn, back_link, {output, date_buffer, link_date, record});
    };

    if (std::int8_t(detail) == 0) {
        invoke(c, date_text_fn, 0x1040b2bc, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date_buffer, date_buffer_size, 5, std::uint32_t(person)});
        // Which of three wordings a plain message takes comes from two remainders of the club and the person:
        // the club's remainder by 3 against the person's by 4, then the club's by 4 against the person's by 2, then the
        // club's by 3 against the person's by 2 (0x1040b35c).
        const auto club_by_three = club % 3, club_by_four = club % 4, person_by_two = person % 2, person_by_four = person % 4;
        if (flag == 0) {
            if (club_by_three == person_by_four) {
                say(c, frame, 0x1040b314, text, text_base + 0xe43a, {100, date_buffer});
            } else if (club_by_four == person_by_two) {
                say(c, frame, 0x1040b358, text, text_base + 0xe468, {100, date_buffer});
            } else if (club_by_three == person_by_two) {
                say(c, frame, 0x1040b378, text, text_base + 0xe49d, {date_buffer, std::uint32_t(club_by_three)});
            } else {
                const auto record = club_at(c, club);
                const auto length = name_length(c, record);
                say(c, frame, 0x1040b3b4, text, text_base + 0xe4cb, {100, length, length, date_buffer, name_of(record)});
            }
        } else {
            const auto record = club_at(c, club);
            const auto length = name_length(c, record);
            say(c, frame, 0x1040b3f0, text, text_base + 0xe521, {100, length, length, date_buffer, name_of(record)});
        }
        invoke(c, begin_text_fn, 0x1040b3fc, {output, text});
        date_link(0x1040b41c, 0x1040b43c);
        invoke(c, link_fn, 0x1040b458, {output, name_of(club_at(c, club)), link_club, club_at(c, club)});
        return;
    }

    const auto from = news_field(c, item, 0, 0x1040a330);
    keep(c, 30, std::uint32_t(from));
    const auto third = news_field(c, item, 6, 0x1040a340);
    keep(c, 19, std::uint32_t(third));
    const std::int32_t kind = std::int16_t(news_field(c, item, 7, 0x1040a350));
    keep_signed(c, 20, kind);
    invoke(c, date_text_fn, 0x1040a374, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), date_buffer, date_buffer_size, 1, std::uint32_t(person)});

    const Story story{c, frame, text, text_base, date_buffer, from, club, third};
    const bool has_from = from != no_club;
    switch (kind) {
    case 0xb:
        if (flag == 0) {
            if (has_from) story.with_from(0x1040a404, 0xc74c); else story.without_from(0x1040a460, 0xc829);
        } else {
            if (has_from) story.with_from(0x1040a4e4, 0xc8db); else story.without_from(0x1040a540, 0xc9b4);
        }
        break;
    case 0x24:
        if (has_from) story.third_first_with_from(0x1040a5ac, 0xca62, from); else story.third_first_alone(0x1040a5e8, 0xcb21);
        break;
    case 0x17:
        if (third == club) {
            if (has_from) story.third_first_with_from(0x1040a65c, 0xcbb2, from); else story.third_first_alone(0x1040a698, 0xcc7b);
        } else {
            story.third_first_with_from(0x1040a6f4, 0xcd1c, club);
        }
        break;
    case 0x18:
        if (third == club) {
            if (has_from) story.third_first_with_from(0x1040a768, 0xcde3, from); else story.third_first_alone(0x1040a7a4, 0xceb1);
        } else {
            story.third_first_with_from(0x1040a800, 0xcf57, club);
        }
        break;
    case 0x13:
        if (has_from) story.from_and_club(0x1040a86c, 0xd023); else story.club_alone(0x1040a8a8, 0xd0e7);
        break;
    case 0xe:
        if (has_from) story.with_from(0x1040a934, 0xd180); else story.without_from(0x1040a990, 0xd286);
        break;
    case 0x14:
        if (has_from) story.with_from(0x1040aa1c, 0xd360); else story.without_from(0x1040aa78, 0xd456);
        break;
    case 0x15:
        if (has_from) story.with_from(0x1040ab04, 0xd521); else story.without_from(0x1040ab60, 0xd616);
        break;
    case 0xc:
        story.with_from(0x1040abe4, 0xd6e0);
        break;
    case 0xd: {
        // The text of the club's word at +0x2c (INDEX.CPP) goes to a 100-byte buffer the wording takes last.
        const auto club_offset = std::uint32_t(club) * club_size;
        keep(c, 20, club_offset);
        const auto buffer = frame.at(index_text_buffer);
        invoke(c, index_text_fn, 0x1040ac10, {c.u32(c.u32(c.toc(toc::clubs)) + club_offset + club_index_word), buffer, 100, 2});
        story.say_at(0x1040ac98, 0xd7e7, {100, story.len(from), story.len(from), story.len(club), story.len(club), story.len(third),
                                          story.len(third), 100, 100, date_buffer, story.name(from), story.name(club), story.name(third), buffer});
        break;
    }
    case 0xf:
        story.without_from(0x1040acfc, 0xd909);
        break;
    case 0x10:
        story.with_from(0x1040ad80, 0xd9eb);
        break;
    case 0x11:
        if (has_from) story.with_from(0x1040ae0c, 0xdae6); else story.without_from(0x1040ae68, 0xdbd4);
        break;
    case 4:
        if (has_from) story.club_three_times(0x1040af60, 0xdd7d); else story.club_four_times(0x1040afac, 0xde8b);
        break;
    case 0x22:
        if (has_from) story.club_three_times(0x1040b028, 0xdf6e); else story.club_four_times(0x1040b074, 0xe05e);
        break;
    default:
        if (has_from && kind == 3) {
            story.club_three_times(0x1040aee4, 0xdc97);
        } else if (has_from && (kind == 5 || kind == 0x23)) {
            story.from_twice(kind == 5 ? 0x1040b0f0 : 0x1040b16c, kind == 5 ? 0xe123 : 0xe233);
        } else if (has_from) {
            story.from_and_club(0x1040b1d0, 0xe325);
        } else {
            story.club_alone(0x1040b20c, 0xe3c5);
        }
        break;
    }
    invoke(c, begin_text_fn, 0x1040b218, {output, text});
    date_link(0x1040b238, 0x1040b258);
    invoke(c, link_fn, 0x1040b274, {output, name_of(club_at(c, club)), link_club, club_at(c, club)});
    if (from != no_club) invoke(c, link_fn, 0x1040b298, {output, name_of(club_at(c, from)), link_club, club_at(c, from)});
}
}
