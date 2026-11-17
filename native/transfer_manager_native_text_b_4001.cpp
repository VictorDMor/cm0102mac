// TRANSFER_MANAGER.CPP, block "text_b": news message 4001's text (0x104001e0).
#include "transfer_manager_native_text_b.hpp"
#include "transfer_manager_native_text_b_4001.hpp"

namespace cm::aot::transfer_manager_native {
using namespace text_b;
using guest::Cpu;
using guest::Frame;

namespace {
constexpr std::uint32_t cash_word_at = 0x48, cash_size = 0xc8;
constexpr std::uint32_t competition_cell = -0x7bbc, competition_size = 0x6c, competition_name = 0x4d, link_competition = 0x7d0;
// Format texts of the short form (no detail), from the text base.
constexpr std::uint32_t text_signed_other = 0x6fb6, text_signed_first = 0x6fef, text_free = 0x702c, text_loan_first = 0x7068, text_loan_other = 0x70a2,
                        text_loan_club = 0x70e0;
}

void news_text_4001(Cpu& c, std::uint32_t item, std::int8_t detail, std::uint32_t output) {
    Frame frame(c, 0x104001e0, 0x620, {.first_gpr = 13});
    const auto text = c.toc(text_buffer_cell);
    const auto text_base = c.toc(text_base_cell);
    keep(c, 25, text);
    keep(c, 17, item);
    keep(c, 13, c.toc(toc::people));
    keep(c, 15, output);
    keep(c, 26, c.toc(toc::clubs));
    keep(c, 27, text_base);
    const auto date = frame.at(Signing::date_at);

    if (detail == 0) {
        const auto person = news_field(c, item, 0, 0x104017e0);
        keep(c, 16, std::uint32_t(person));
        const auto n1 = news_field(c, item, 1, 0x104017f4);
        keep(c, 18, std::uint32_t(n1));
        const auto n2 = news_field(c, item, 2, 0x10401808);
        keep(c, 19, std::uint32_t(n2));
        const auto n3 = news_field(c, item, 3, 0x1040181c);
        keep(c, 20, std::uint32_t(n3));
        const auto club = news_field(c, item, 5, 0x10401830);
        keep(c, 14, std::uint32_t(club));
        const auto kind = std::int32_t(std::int8_t(news_field(c, item, 7, 0x10401840)));
        keep_signed(c, 21, kind);
        const auto reason = std::int32_t(std::int16_t(news_field(c, item, 8, 0x10401850)));
        keep_signed(c, 22, reason);
        const auto marker = std::int32_t(std::int8_t(news_field(c, item, 9, 0x10401860)));
        keep_signed(c, 17, marker);
        invoke(c, date_text_fn, 0x10401884, {std::uint32_t(n1), std::uint32_t(n2), std::uint32_t(n3), date, date_buffer_size, 5, std::uint32_t(person)});
        const auto say = [&](std::uint32_t back, std::uint32_t format, std::initializer_list<std::uint32_t> values) {
            c.set(3, text);
            c.set(4, text_base + format);
            unsigned n = 5;
            for (const auto value : values) c.set(n++, value);
            c.call(format_fn, back);
        };
        if (kind == 2) {
            if (reason == 6) say(0x104018d8, text_free, {100, date});
            else if (marker == 1) say(0x104018ac, text_signed_other, {date});
            else say(0x104018c0, text_signed_first, {date});
        } else if (kind == 3) {
            if (reason == 0) {
                if (marker == 1) say(0x10401904, text_loan_first, {date});
                else say(0x10401918, text_loan_other, {date});
            } else if (reason & 2) {
                const auto record = club_at(c, club);
                const auto length = std::uint32_t(club_name_len(c, record));
                say(0x10401954, text_loan_club, {length, length, record + club_name, date});
            } else {
                say(0x1040196c, text_free, {100, date});
            }
        }
        invoke(c, begin_text_fn, 0x10401978, {output, text});
        keep(c, 14, std::uint32_t(person) * person_size);
        if (std::int8_t(invoke(c, date_known_fn, 0x10401998, {person_at(c, person), std::uint32_t(n1), std::uint32_t(n2), std::uint32_t(n3), 0})))
            invoke(c, link_fn, 0x104019b8, {output, date, link_date, person_at(c, person)});
        return;
    }

    Signing s{.c = c, .frame = frame, .text = text, .text_base = text_base, .output = output};
    s.person = news_field(c, item, 0, 0x1040021c);
    keep(c, 23, std::uint32_t(s.person));
    const auto n1 = news_field(c, item, 1, 0x10400230);
    keep(c, 22, std::uint32_t(n1));
    const auto n2 = news_field(c, item, 2, 0x10400244);
    keep(c, 21, std::uint32_t(n2));
    const auto n3 = news_field(c, item, 3, 0x10400258);
    keep(c, 20, std::uint32_t(n3));
    s.from = news_field(c, item, 4, 0x1040026c);
    keep(c, 28, std::uint32_t(s.from));
    s.to = news_field(c, item, 5, 0x1040027c);
    keep(c, 29, std::uint32_t(s.to));
    // Field 6 only matters as zero or not.
    s.flagged = news_field(c, item, 6, 0x1040028c) != 0;
    keep(c, 19, s.flagged ? 1 : 0);
    s.kind = std::int32_t(std::int8_t(news_field(c, item, 7, 0x104002a8)));
    keep_signed(c, 16, s.kind);
    s.reason = std::int32_t(std::int16_t(news_field(c, item, 8, 0x104002b8)));
    keep_signed(c, 18, s.reason);
    s.marker = std::int32_t(std::int8_t(news_field(c, item, 9, 0x104002c8)));
    keep_signed(c, 14, s.marker);
    s.amount = news_field(c, item, 0xa, 0x104002d8);
    keep_signed(c, 30, s.amount);
    s.competition = news_field(c, item, 0xb, 0x104002e8);
    keep_signed(c, 31, s.competition);
    s.extra = std::int32_t(std::int8_t(news_field(c, item, 0xc, 0x104002f8)));
    keep_signed(c, 17, s.extra);
    invoke(c, date_text_fn, 0x1040031c, {std::uint32_t(n1), std::uint32_t(n2), std::uint32_t(n3), date, date_buffer_size, 1, std::uint32_t(s.person)});
    if (s.amount != 0) {
        c.put32(frame.at(cash_word_at), std::uint32_t(s.amount));
        invoke(c, 0x10009f20, 0x10400334, {frame.at(cash_word_at), 6});
        const auto high = c.r(3), low = c.r(4);
        invoke(c, 0x1000b410, 0x1040034c, {s.cash(), cash_size, high, low, 6});
    }
    write_signing(s);

    invoke(c, begin_text_fn, 0x10401728, {output, text});
    if (s.from != -1) invoke(c, link_fn, 0x1040174c, {output, s.name(s.from), link_club, club_at(c, s.from)});
    invoke(c, link_fn, 0x10401768, {output, s.name(s.to), link_club, club_at(c, s.to)});
    keep(c, 14, std::uint32_t(s.person) * person_size);
    if (std::int8_t(invoke(c, date_known_fn, 0x10401788, {person_at(c, s.person), std::uint32_t(n1), std::uint32_t(n2), std::uint32_t(n3), 0})))
        invoke(c, link_fn, 0x104017a8, {output, date, link_date, person_at(c, s.person)});
    if (s.competition != -1) {
        const auto record = c.u32(c.toc(competition_cell)) + std::uint32_t(s.competition) * competition_size;
        invoke(c, link_fn, 0x104017d0, {output, record + competition_name, link_competition, record});
    }
}
}
