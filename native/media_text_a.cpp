// 0x1064cf80: the press writes about a player (or his manager): a column in some paper, radio or website,
// and, when the item is the long form, the manager's reply to it. The item holds the player and his
// club, the kind of story, tone and traits that choose the wording, and the people the text links to.
#include "media_text_a.hpp"
#include "media_text_a_detail.hpp"
#include <algorithm>

namespace cm::aot::media {
namespace detail::column {

std::uint32_t Story::value(Arg arg) const {
    switch (arg) {
    case Arg::plain: return 0x64;
    case Arg::player_name: return at(0x398);
    case Arg::player_short: return at(0x320);
    case Arg::manager_name: return at(0x1b8);
    case Arg::manager_short: return at(0x140);
    case Arg::second_name: return at(0x230);
    case Arg::third_name: return at(0xc8);
    case Arg::adjective_text: return at(0x2a8);
    case Arg::outlet_name: return at(0x94);
    case Arg::score_text: return at(0x64);
    case Arg::position_text: return at(0x410);
    case Arg::date_text: return at(0x4d8);
    case Arg::club_article: return sign8(c.u8(club_of(club) + club_article_offset));
    case Arg::club_name: return club_of(club) + club_name_offset;
    case Arg::club_short: return club_of(club) + club_short_name_offset;
    case Arg::clubs_base: return clubs();
    case Arg::club_short_article: return sign8(c.u8(club_of(club) + club_short_article_offset));
    case Arg::nation: return c.u32(club_of(club) + 0x2c);
    case Arg::nation_article: return sign8(c.u8(c.u32(club_of(club) + 0x2c) + 0x116));
    case Arg::nation_name: return c.u32(club_of(club) + 0x2c) + 0xe1;
    case Arg::rival_short_article: return sign8(c.u8(club_of(std::int32_t(rival_club())) + club_short_article_offset));
    case Arg::rival_short: return club_of(std::int32_t(rival_club())) + club_short_name_offset;
    case Arg::other_article: return sign8(c.u8(club_of(rival) + club_article_offset));
    case Arg::other_name: return club_of(rival) + club_name_offset;
    case Arg::person_article: return sign8(c.u8(person_at(person) + 0x115));
    case Arg::person_text: return person_at(person) + 0xae;
    case Arg::team_article: return sign8(c.u8(team_at() + 0x36));
    case Arg::team_name: return team_at() + 0x1c;
    case Arg::teams_base: return c.u32(c.toc(teams_slot));
    case Arg::position_kind: return sign8(status);
    case Arg::outlet_article: return sign8(c.u8(at(0x61)));
    default: return sign8(c.u8(at(0x3218)));  // Arg::age_flag_arg
    }
}

void Story::say(const Line& line) {
    if (line.adjective_kind) {
        const auto of = line.adjective_of == Of::nation ? c.u32(club_of(club) + 0x2c) : person_at(person);
        c.set({{3, of}, {4, at(0x2a8)}, {5, 0x78}, {6, line.adjective_kind}});
        c.call(entry::adjective, line.adjective_back);
    }
    // The original works out the words past the sixth argument and stores them, in order, before it reads
    // anything for the registers: a read that faults (a club whose nation is null) leaves them written.
    for (unsigned i = 6; i < line.count; ++i) c.put32(c.sp() + 0x38 + (i - 6) * 4, value(line.args[i]));
    std::array<std::uint32_t, 6> registers{};
    for (unsigned i = 0; i < std::min<unsigned>(line.count, 6); ++i) registers[i] = value(line.args[i]);
    c.set(3, at(unsigned(line.dest)));
    c.set(4, c.toc(slot::pool) + line.phrase);
    for (unsigned i = 0; i < std::min<unsigned>(line.count, 6); ++i) c.set(5 + i, registers[i]);
    c.call(line.sprintf_form ? entry::sprintf : formatter, line.back);
}

// `player % 8` the way the compiler wrote it for a signed value: the tone is taken from the player's number
// when no other rule fits (a negative number keeps its sign, so it never settles).
void Story::reseed_tone() {
    const std::uint32_t x = std::uint32_t(player), sign = x >> 31;
    tone = std::int32_t(std::rotl((x << 29) - sign, 3) + sign);
    c.set(25, std::uint32_t(tone));
}

// The three-way pick of the headline: the player plus the club divided by the figure (a zero divisor gives the
// divw convention), modulo 3.
std::int32_t Story::pick_three() const {
    const auto divisor = std::uint32_t(figure) + 1;
    const auto quotient = std::uint32_t(divide_signed<4>(std::uint32_t(club), divisor));
    const auto sum = std::int32_t(std::uint32_t(player) + quotient);
    return sum % 3;
}

// The score as two numbers, the larger first, into the score text.
void Story::score(std::uint32_t back) {
    const auto home = goals_home(), away = goals_away();
    compare(c, home, away);
    const std::uint32_t smaller = home >= away ? c.u8(at(0x8d)) : c.u8(at(0x8c));
    compare(c, home, away);
    const std::uint32_t larger = home > away ? c.u8(at(0x8c)) : c.u8(at(0x8d));
    c.set({{3, at(0x64)}, {4, c.toc(slot::pool) + 0x1cc7}, {5, sign8(larger)}, {6, sign8(smaller)}});
    c.call(entry::sprintf, back);
}

// The player's position text; a player-manager's stays empty (status 0).
void Story::position(std::uint32_t back) {
    c.set({{3, std::uint32_t(player)}, {4, extended(std::int16_t(c.u16(at(0x31da))))}, {5, c.u32(at(0x31dc))},
            {6, at(0x410)},
           {7, 0xc8}, {8, status}, {9, 1}});
    c.call(entry::position_text, back);
    status = c.r(3);
    c.set(28, status);
}

void Story::link(std::uint32_t text, std::uint32_t kind, std::uint32_t record, std::uint32_t back, bool clubs_in_r7) {
    if (clubs_in_r7) c.set(7, clubs());
    link_text(c, output, text, kind, record, back);
}

}  // namespace detail::column

using namespace detail::column;

void press_column_story(Cpu& c, std::uint32_t, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1064cf80, 0x3270, {.first_gpr = 13});
    keep(c,
            {{29, c.toc(slot::clubs)}, {16, item}, {30, c.toc(rows_slot)}, {13, full}, {31, c.toc(slot::pool)},
            {15, output}});
    c.set(3, frame.at(0x70));
    c.call(entry::today, 0x1064cfb0);
    Story story(c, frame, item, output);
    if (compare(c, std::int8_t(full)) != 0) story.long_form();
    else story.short_form();
}

}  // namespace cm::aot::media
