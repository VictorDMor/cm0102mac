// Stories about a staff member and a match, as press text: the record names the club, the
// staff member (three name parts) and a variant; 1030dee0 reads the match behind it (date,
// competition, both clubs, score).
#include "media_text_mid.hpp"
#include "media_text_mid_detail.hpp"
#include <algorithm>

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::text;

namespace {
constexpr std::uint32_t match_exists = 0x1029eb20, sprintf_entry = 0x1077b720;
constexpr int match_slot = -0x7b4c;
constexpr std::uint32_t score_phrase = 0x1cc7;  // the score as two numbers
// The first return address of the four endings of a draw story (variants 1-4), for each of the two forms.
constexpr std::uint32_t ending_backs[2]{0x1069d744, 0x1069dbe8};
}

// The text for a staff member in a match: `full` has the detailed text with the score, the
// competition and links to club, competition, match and staff; otherwise a short one-liner.
void staff_match_text(Cpu& c, std::uint32_t record, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1069d460, 0x580, {.first_gpr = 18});
    keep(c, {{26, c.toc(slot::competitions)}, {27, c.toc(slot::text)}, {19, full}, {28, c.toc(slot::clubs)},
             {31, output}, {29, c.toc(staff_slot)}, {30, c.toc(slot::pool)}});
    Kept<18> held(c, record);
    Program program{c};
    const auto match = DecodedMatch{c, frame.at(0x68)};
    program.call(today, 0x1069d498, {match.at});
    RecordReader read{.c = c, .record = record, .back = 0x1069d4a4};
    Kept<25> staff_id(c, read(0));
    Kept<24> first(c, read(1));
    Kept<23> middle(c, read(2));
    Kept<22> last(c, read(3));
    Kept<21> club_id(c, read(4));
    Kept<20> kind(c, read(5));
    const auto cursor = frame.at(0x58);
    c.put8(cursor, 6);
    program.call(decode_match, 0x1069d524, {record, cursor, match.at});
    const auto next = c.u8(cursor);
    c.put8(cursor, next + 1);
    const auto variant_value = program.call(entry::read_field, 0x1069d538, {record, next});
    const bool detailed = compare(c, signed_byte(full)) != 0;
    Kept<19> variant(c, variant_value);
    const auto name = frame.at(0x3ac), short_name = frame.at(0x21c), score = frame.at(0x5c), ending = frame.at(0x8c);
    const auto text = c.toc(slot::text);
    const Output out{c, output};
    if (!detailed) {
        program.call(entry::person_name, 0x1069e0c0, {first, middle, last, name, 0x190, 5, staff_id});
        // Two phrases by the staff member's id and the kind, mod 7, signed as the original divides.
        const auto spread = std::int32_t((std::uint32_t(kind) + 1) * std::uint32_t(staff_id)) % 7;
        if (compare(c, spread, 3) > 0) format(c, text, 0x3daab, 0x1069e10c, {0x64, name});
        else format(c, text, 0x3dadf, 0x1069e128, {0x64, name});
        out.set(text, 0x1069e134);
        out.staff_link(staff_at(c, staff_id), first, middle, last, name, 0x1069e154, 0x1069e178, 0x7da);
        return;
    }
    program.call(entry::person_name, 0x1069d564, {first, middle, last, name, 0x190, 1, staff_id});
    program.call(entry::person_name, 0x1069d584, {first, middle, last, short_name, 0x190, 5, staff_id});
    const auto one = club_at(c, club_id), two = club_at(c, match.second_club());
    const auto competition = competition_at(c, match.competition());
    const auto home = match.first_goals(), away = match.second_goals();
    compare(c, home, away);
    program.call(sprintf_entry, 0x1069d5e0, {score, c.toc(slot::pool) + score_phrase, signed_byte(std::uint8_t(std::max(home, away))),
                                              signed_byte(std::uint8_t(std::min(home, away)))});
    const unsigned form = compare(c, std::int32_t(std::uint32_t(kind))) != 0 ? 1 : 0;
    // A draw ends with the variant's closing sentence, appended.
    const auto closing = [&] {
        match_ending(c, std::int32_t(std::uint32_t(variant)), ending_backs[form], ending, text, [&] {
            return std::array<std::uint32_t, 4>{one.short_marker(), one.short_marker(), one.short_name(), table(c, -0x7be8)};
        });
    };
    const auto draw = compare(c, home, away) == 0;
    if (form == 0) {
        if (draw) {
            { StackWords{c} << competition.marker() << one.short_name() << name << two.long_name() << score << competition.name(); format(c, text, 0x3d27e, 0x1069d6f8, {0x64, one.short_marker(), one.short_marker(), two.long_marker(), two.long_marker(), competition.marker()}); }
            closing();
        } else {
            const bool home_won = compare(c, home, away) > 0;
            { StackWords{c} << 0x64 << competition.marker() << competition.marker() << name << two.long_name() << one.short_name() << score << competition.name(); format(c, text, home_won ? 0x3d3d4 : 0x3d529, home_won ? 0x1069d984 : 0x1069da8c, {0x64, 0x64, two.long_marker(), two.long_marker(), one.short_marker(), one.short_marker()}); }
        }
    } else {
        held = one.at;
        const auto won = draw ? 0 : compare(c, home, away);
        { StackWords{c} << one.short_name() << score << competition.name() << two.long_name() << name << short_name; format(c, text, draw ? 0x3d681 : won > 0 ? 0x3d7e2 : 0x3d945, draw ? 0x1069db9c : won > 0 ? 0x1069de18 : 0x1069df10, {one.short_marker(), one.short_marker(), competition.marker(), competition.marker(), two.long_marker(), two.long_marker()}); }
        if (draw) closing();
    }
    out.set(text, 0x1069df1c);
    out.link(one.long_name(), link_kind::club, one.at, 0x1069df44, table(c, -0x7be8));
    out.link(one.short_name(), link_kind::club, one.at, 0x1069df6c, table(c, -0x7be8));
    out.link(two.long_name(), link_kind::club, two.at, 0x1069df9c, table(c, -0x7be8));
    out.link(two.short_name(), link_kind::club, two.at, 0x1069dfcc, table(c, -0x7be8));
    out.link(competition.name(), link_kind::competition, competition.at, 0x1069dffc, table(c, -0x7bbc));
    if (compare(c, std::int8_t(program.call(match_exists, 0x1069e020, {table(c, match_slot), match.id(), match.first_club(), match.second_club(),
                                                                       std::uint16_t(match.day()), match.year()}))) != 0)
        out.link(score, 0x7d3, match.id(), 0x1069e03c);
    const auto member = staff_at(c, staff_id);
    if (out.named(member, first, middle, last, 0x1069e05c)) {
        out.link(name, link_kind::staff, member, 0x1069e080);
        out.link(short_name, link_kind::staff, member, 0x1069e09c);
    }
}
}
