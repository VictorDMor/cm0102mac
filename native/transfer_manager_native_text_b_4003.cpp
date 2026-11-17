// TRANSFER_MANAGER.CPP, block "text_b": news message 4003's text (0x104026b0).
#include "transfer_manager_native_text_b.hpp"
#include "transfer_manager_native_text_b_detail.hpp"

namespace cm::aot::transfer_manager_native {
using namespace text_b;
using guest::Cpu;
using guest::Frame;

namespace {
constexpr std::uint32_t buffer_at = 0x40, stack_argument = 0x38;
constexpr std::int32_t session_pending_club = 0x24;
// Format texts, from the text base.
constexpr std::uint32_t text_dated_unknown = 0x785b, text_dated_first = 0x78a0, text_dated_other = 0x78db;
constexpr std::uint32_t text_to_club = 0x752f, text_pending_first = 0x7597, text_first = 0x7625, text_pending_other = 0x76b1,
                        text_other = 0x773a, text_flagged = 0x77c2;

// The date text and, when the person's date is known, the link to it (the last two steps of both variants).
void link_person_date(Cpu& c, Frame& frame, std::uint32_t output, std::int32_t person, std::uint32_t day, std::uint32_t month,
                      std::uint32_t year, std::uint32_t back_known, std::uint32_t back_link) {
    const auto record = person_at(c, person);
    if (std::int8_t(invoke(c, date_known_fn, back_known, {record, day, month, year, 0}))) {
        invoke(c, link_fn, back_link, {output, frame.at(buffer_at), link_date, person_at(c, person)});
    }
}
}

void news_text_4003(Cpu& c, std::uint32_t item, std::int8_t detail, std::uint32_t output) {
    Frame frame(c, 0x104026b0, 0x1b0, {.first_gpr = 18});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    const auto buffer = frame.at(buffer_at);
    keep(c, 19, item);
    keep(c, 29, text);
    keep(c, 31, output);
    keep(c, 21, c.toc(toc::people));
    keep(c, 30, c.toc(toc::clubs));
    keep(c, 24, text_base);
    const auto say = [&](std::uint32_t back, std::uint32_t format, std::initializer_list<std::uint32_t> values) {
        c.set(3, text);
        c.set(4, text_base + format);
        unsigned n = 5;
        for (const auto value : values) c.set(n++, value);
        c.call(format_fn, back);
    };

    if (detail == 0) {
        const auto person = news_field(c, item, 0, 0x10402a5c);
        keep(c, 18, std::uint32_t(person));
        const auto day = news_field(c, item, 1, 0x10402a70);
        keep(c, 22, std::uint32_t(day));
        const auto month = news_field(c, item, 2, 0x10402a84);
        keep(c, 23, std::uint32_t(month));
        const auto year = news_field(c, item, 3, 0x10402a98);
        keep(c, 25, std::uint32_t(year));
        const auto date_kind = news_field(c, item, 4, 0x10402aac);
        keep(c, 20, std::uint32_t(date_kind));
        const auto marker = std::int8_t(news_field(c, item, 9, 0x10402abc));
        keep_signed(c, 19, marker);
        invoke(c, date_text_fn, 0x10402ae0, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), buffer, date_buffer_size, 5, std::uint32_t(person)});
        if (date_kind == -1) say(0x10402af8, text_dated_unknown, {buffer});
        else if (marker == 1) say(0x10402b14, text_dated_first, {buffer});
        else say(0x10402b28, text_dated_other, {buffer});
        invoke(c, begin_text_fn, 0x10402b34, {output, text});
        keep(c, 19, std::uint32_t(person) * person_size);
        link_person_date(c, frame, output, person, std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), 0x10402b54, 0x10402b74);
        return;
    }

    // The player the news is about may be the one the manager is waiting on (his pending club is the item's second club).
    std::uint32_t pending_club = 0;
    {
        const auto session = c.toc(session_cell);
        const auto slot = session + std::uint32_t(std::int32_t(c.i16(session + 0x2c58)) * 0x2c0);
        if (const auto current = c.u32(slot + 4)) pending_club = c.u32(current + session_pending_club);
    }
    keep(c, 20, pending_club);
    const auto person = news_field(c, item, 0, 0x10402710);
    keep(c, 22, std::uint32_t(person));
    const auto day = news_field(c, item, 1, 0x10402724);
    keep(c, 23, std::uint32_t(day));
    const auto month = news_field(c, item, 2, 0x10402738);
    keep(c, 28, std::uint32_t(month));
    const auto year = news_field(c, item, 3, 0x1040274c);
    keep(c, 27, std::uint32_t(year));
    const auto from_club = news_field(c, item, 4, 0x10402760);
    keep(c, 25, std::uint32_t(from_club));
    const auto to_club = news_field(c, item, 5, 0x10402770);
    keep(c, 26, std::uint32_t(to_club));
    // Field 6 only matters as zero or not (the original's neg/or/sign-bit test).
    const auto flagged = news_field(c, item, 6, 0x10402780) != 0;
    keep(c, 19, flagged ? 1 : 0);
    const auto marker = std::int8_t(news_field(c, item, 9, 0x1040279c));
    keep_signed(c, 18, marker);
    invoke(c, date_text_fn, 0x104027c0, {std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), buffer, date_buffer_size, 1, std::uint32_t(person)});

    // Club names: the formatter takes each club's name length twice and its name; the text buffer address goes on the stack.
    const auto name_of = [&](std::int32_t club) { return club_at(c, club) + club_name; };
    const auto two_clubs = [&](std::uint32_t back, std::uint32_t format) {
        c.put32(frame.at(stack_argument), buffer);
        const auto to_length = std::uint32_t(club_name_len(c, club_at(c, to_club)));
        const auto from_length = std::uint32_t(club_name_len(c, club_at(c, from_club)));
        say(back, format, {from_length, from_length, to_length, to_length, name_of(from_club), name_of(to_club)});
    };
    if (flagged) {
        two_clubs(0x104029c0, text_flagged);
    } else if (from_club == -1) {
        const auto length = std::uint32_t(club_name_len(c, club_at(c, to_club)));
        say(0x10402800, text_to_club, {length, length, buffer, name_of(to_club)});
    } else {
        const bool pending = pending_club != 0 && std::int32_t(c.u32(pending_club)) == to_club;
        if (marker == 1) two_clubs(pending ? 0x1040286c : 0x104028bc, pending ? text_pending_first : text_first);
        else two_clubs(pending ? 0x10402920 : 0x10402970, pending ? text_pending_other : text_other);
    }
    invoke(c, begin_text_fn, 0x104029cc, {output, text});
    if (from_club > -1) invoke(c, link_fn, 0x104029f0, {output, name_of(from_club), link_club, club_at(c, from_club)});
    invoke(c, link_fn, 0x10402a0c, {output, name_of(to_club), link_club, club_at(c, to_club)});
    keep(c, 19, std::uint32_t(person) * person_size);
    link_person_date(c, frame, output, person, std::uint32_t(day), std::uint32_t(month), std::uint32_t(year), 0x10402a2c, 0x10402a4c);
}
}
