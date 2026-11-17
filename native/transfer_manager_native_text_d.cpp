// TRANSFER_MANAGER.CPP, block "text_d": the texts of the transfer news messages 4009 and 4031.
#include "transfer_manager_native_text_d.hpp"
#include "transfer_manager_native_text_a_detail.hpp"

namespace cm::aot::transfer_manager_native {
using namespace text_a;

// Kind 4009 (NEWS.CPP 0x102c63cc): a person (field 2) with a date (fields 3-5) and two clubs (fields 1 and 0).
// The short text names only club 1; the long text names both and quotes the same length for club 1 three times.
void news_text_4009(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x10408f80, 0x1b0, {.first_gpr = 21});
    const auto text = c.toc(text_cell), buffer = c.toc(buffer_cell);
    const auto people_cell = c.toc(toc::people);
    const Clubs clubs{c, c.toc(toc::clubs)};
    constexpr std::uint32_t description = 0x50;
    keep(c, 27, item);
    keep(c, 29, buffer);
    keep(c, 30, people_cell);
    keep(c, 23, full);
    keep(c, 31, clubs.cell);
    keep(c, 22, output);
    keep(c, 21, text);
    const auto club = field(c, item, 1, 0x10408fb4);
    keep(c, 28, club);
    const auto person = field(c, item, 2, 0x10408fc4);
    keep(c, 26, person);
    const auto day = field(c, item, 3, 0x10408fd8);
    keep(c, 25, day);
    const auto month = field(c, item, 4, 0x10408fec);
    keep(c, 24, month);
    const auto year = field(c, item, 5, 0x10409000);
    keep(c, 23, year);
    const auto record = [&] { return c.u32(people_cell) + person * person_size; };
    if (std::int8_t(full) != 0) {
        const auto other = field(c, item, 0, 0x10409018);
        keep(c, 27, other);
        describe(c, 0x1040903c, day, month, year, frame.at(description), 300, 1, person);
        const auto club_offset = club * club_size, other_offset = other * club_size;
        keep(c, 28, club_offset);
        keep(c, 27, other_offset);
        say(c, frame, 0x104090a4, buffer, text + text_page - 0x43b1,
            {100, clubs.long_len(other), clubs.long_len(other), clubs.long_len(club), clubs.long_len(club),
             clubs.long_len(club), clubs.long_len(club), frame.at(description), clubs.short_name(other), clubs.short_name(club),
             clubs.short_name(club)});
        begin_text(c, 0x104090b0, output, buffer);
        const auto person_offset = person * person_size;
        keep(c, 21, person_offset);
        if (known(c, 0x104090d0, record(), day, month, year)) link(c, 0x104090f0, output, frame.at(description), link_person, record());
        link(c, 0x10409108, output, clubs.long_name(club), link_club, clubs.at(club));
        link(c, 0x10409120, output, clubs.long_name(other), link_club, clubs.at(other));
    } else {
        describe(c, 0x10409144, day, month, year, frame.at(description), 300, 5, person);
        const auto club_offset = club * club_size;
        keep(c, 27, club_offset);
        say(c, frame, 0x1040917c, buffer, text + text_page - 0x42c3,
            {100, clubs.long_len(club), clubs.long_len(club), frame.at(description), clubs.long_name(club)});
        begin_text(c, 0x10409188, output, buffer);
        const auto person_offset = person * person_size;
        keep(c, 21, person_offset);
        if (known(c, 0x104091a8, record(), day, month, year)) link(c, 0x104091c8, output, frame.at(description), link_person, record());
        link(c, 0x104091e0, output, clubs.long_name(club), link_club, clubs.at(club));
    }
}

// Kind 4031 (0x102c6674): a person, a date, a club (field 4) and a competition (the 0x6c-byte records) (field 5);
// the long text adds a fee (field 7) and, when `status` (field 6) is 3, another sentence.
void news_text_4031(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x104101f0, 0x210, {.first_gpr = 18});
    const auto text = c.toc(text_cell), buffer = c.toc(buffer_cell);
    const auto people_cell = c.toc(toc::people);
    const Clubs clubs{c, c.toc(toc::clubs)};
    const auto competitions = c.toc(-0x7bbc);
    constexpr std::uint32_t money_slot = 0x40, money = 0x44, description = 0x10c;
    constexpr std::uint32_t competition_size = 0x6c, competition_name = 0x1a, competition_len = 0x6a, link_competition = 0x7d0;
    keep(c, 19, item);
    keep(c, 28, competitions);
    keep(c, 29, buffer);
    keep(c, 18, full);
    keep(c, 30, people_cell);
    keep(c, 23, output);
    keep(c, 31, clubs.cell);
    keep(c, 20, text);
    const auto person = field(c, item, 0, 0x10410228);
    keep(c, 27, person);
    const auto day = field(c, item, 1, 0x1041023c);
    keep(c, 26, day);
    const auto month = field(c, item, 2, 0x10410250);
    keep(c, 25, month);
    const auto year = field(c, item, 3, 0x10410264);
    keep(c, 24, year);
    const auto record = [&] { return c.u32(people_cell) + person * person_size; };
    if (std::int8_t(full) != 0) {
        const auto club = field(c, item, 4, 0x1041027c);
        keep(c, 21, club);
        const auto competition_index = field(c, item, 5, 0x1041028c);
        keep(c, 22, competition_index);
        const auto status = std::int8_t(field(c, item, 6, 0x1041029c));
        keep_signed(c, 18, status);
        c.put32(frame.at(money_slot), field(c, item, 7, 0x104102ac));
        const auto amount = invoke(c, 0x10009f20, 0x104102c0, {frame.at(money_slot), 6});
        invoke(c, 0x1000b410, 0x104102d8, {frame.at(money), 200, amount, c.r(4), 6});
        describe(c, 0x104102f4, day, month, year, frame.at(description), 200, 1, person);
        const auto competition = [&] { return c.u32(competitions) + competition_index * competition_size; };
        const auto length = [&] { return std::uint32_t(std::int32_t(c.i8(clubs.at(club) + club_len_long))); };
        const auto competition_length = [&] { return std::uint32_t(std::int32_t(c.i8(competition() + competition_len))); };
        // Both texts take the same values; only the format differs (status 3 against anything else).
        say(c, frame, status == 3 ? 0x1041035c : 0x104103c0, buffer, text + text_page + (status == 3 ? 0x6ad : 0x7a3),
            {length(), length(), competition_length(), competition_length(), frame.at(description), clubs.long_name(club), frame.at(money),
             competition() + competition_name});
        begin_text(c, 0x104103cc, output, buffer);
        const auto person_offset = person * person_size;
        keep(c, 19, person_offset);
        if (known(c, 0x104103ec, record(), day, month, year)) link(c, 0x1041040c, output, frame.at(description), link_person, record());
        link(c, 0x10410428, output, clubs.long_name(club), link_club, clubs.at(club));
        const auto flags = c.u32(c.toc(-0x7b5c));
        if (c.u32(flags + competition_index * 4) != 0) link(c, 0x1041045c, output, competition() + competition_name, link_competition, competition());
    } else {
        describe(c, 0x10410480, day, month, year, frame.at(description), 200, 5, person);
        say(c, frame, 0x10410498, buffer, text + text_page + 0x8a1, {100, frame.at(description)});
        begin_text(c, 0x104104a4, output, buffer);
        const auto person_offset = person * person_size;
        keep(c, 19, person_offset);
        if (known(c, 0x104104c4, record(), day, month, year)) link(c, 0x104104e4, output, frame.at(description), link_person, record());
    }
}
}
