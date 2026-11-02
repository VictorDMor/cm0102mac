// A staff member sent off or losing his temper in a match, as press text: the record names the
// staff members involved, a variant and the reply the manager gave; 1030dee0 reads the match
// behind it.
#include "media_text_mid.hpp"
#include "media_text_mid_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::text;

namespace {
constexpr std::uint32_t render_week = 0x107467a0, copy_string = 0x1077dbf0, sprintf_entry = 0x1077b720;
constexpr int selected_slot = -0x7bb0;
constexpr std::uint32_t score_phrase = 0x3e6fe, joined_phrase = 0xc024, single_phrase = 0x3f438;
// The same staff member is the manager of the club the user plays: the user's record (a table at
// the selected slot, 0x2c0 bytes each) holds his staff record at +4.
bool is_users_manager(Cpu& c, std::uint32_t member) {
    const auto base = c.toc(selected_slot);
    const auto user = base + std::uint32_t(std::int32_t(c.i16(base + 0x2c58))) * 0x2c0;
    return compare(c, c.u32(user + 4) == member ? 1 : 0) != 0;
}
}

// The press text of a sent-off or temper story; `full` has the whole story with the match and its replies.
void sent_off_text(Cpu& c, std::uint32_t record, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x106a2330, 0x45b0, {.first_gpr = 13});
    keep(c, {{30, c.toc(staff_slot)}, {31, c.toc(slot::pool)}, {15, output}});
    Kept<16> held(c, record);
    Program program{c};
    const auto cursor = frame.at(0x50), match_at = frame.at(0x54);
    c.put8(cursor, 0);
    c.put8(frame.at(0x45d3), full);
    program.call(today, 0x106a2364, {match_at});
    Kept<28> other_club(c, 0);
    std::uint32_t back = 0x106a237c;
    const auto next = [&] {
        const auto index = c.u8(cursor);
        c.put8(cursor, index + 1);
        const auto value = program.call(entry::read_field, back, {record, index});
        back += 0x18;
        return value;
    };
    // The record: the first staff member (id and name parts), a second one, a reply, the club, a
    // third and a fourth staff member, and the reply code. Some parts wait in the frame.
    const auto first_part = frame.at(0x4550), second_part = frame.at(0x454c), third_part = frame.at(0x4548);
    Kept<27> staff_a(c, next());
    Kept<26> a_first(c, next());
    Kept<25> a_middle(c, next());
    Kept<24> a_last(c, next());
    Kept<23> staff_b(c, next());
    c.put32(first_part, next());
    c.put32(second_part, next());
    c.put32(third_part, next());
    Kept<22> kind(c, next());
    Kept<29> club_id(c, next());
    const auto second_club_id = frame.at(0x4544);
    c.put32(second_club_id, next());
    Kept<21> week(c, next());
    Kept<20> staff_c(c, next());
    const auto c_first = frame.at(0x4540), c_middle = frame.at(0x453c), c_last = frame.at(0x4538);
    c.put32(c_first, next());
    c.put32(c_middle, next());
    c.put32(c_last, next());
    Kept<19> staff_d(c, next());
    Kept<13> d_first(c, next());
    Kept<14> d_middle(c, next());
    Kept<18> d_last(c, next());
    Kept<17> reply(c, next());
    // r23 is compared with -1 here too, but the compare of the second club's id replaces it at once.
    club_id = table(c, -0x7be8) + std::uint32_t(club_id) * club_size;
    if (compare(c, c.i32(second_club_id), -1) != 0) other_club = table(c, -0x7be8) + c.u32(second_club_id) * club_size;
    const Club mine{c, club_id}, other{c, other_club};
    const DecodedMatch match{c, match_at};
    program.call(decode_match, 0x106a25a4, {record, cursor, match_at});
    const Output out{c, output};
    const auto buffer_a = frame.at(0x910), short_a = frame.at(0x5f0), buffer_b = frame.at(0x780), buffer_c = frame.at(0x460),
               buffer_d = frame.at(0x2d0), text = frame.at(0x31b0);
    if (compare(c, signed_byte(c.u8(frame.at(0x45d3)))) == 0) {
        program.call(entry::person_name, 0x106a2d64, {a_first, a_middle, a_last, buffer_a, 0x190, 5, staff_a});
        // Only a kind of 0 is the quiet one.
        const bool quiet = compare(c, std::int32_t(std::uint32_t(kind)), 1) < 0 && compare(c, std::int32_t(std::uint32_t(kind)), 0) >= 0;
        if (quiet) format(c, text, 0x3f43b, 0x106a2d90, {buffer_a});
        else format(c, text, 0x3f479, 0x106a2da8, {buffer_a});
        out.set(text, 0x106a2db4);
        out.staff_link(staff_at(c, staff_a), a_first, a_middle, a_last, buffer_a, 0x106a2dd4, 0x106a2df8);
        return;
    }
    program.call(entry::person_name, 0x106a25d0, {a_first, a_middle, a_last, buffer_a, 0x190, 1, staff_a});
    program.call(entry::person_name, 0x106a25f0, {a_first, a_middle, a_last, short_a, 0x190, 5, staff_a});
    program.call(entry::person_name, 0x106a2610, {c.u32(first_part), c.u32(second_part), c.u32(third_part), buffer_b, 0x190, 1, staff_b});
    program.call(entry::person_name, 0x106a2630, {c.u32(c_first), c.u32(c_middle), c.u32(c_last), buffer_c, 0x190, 5, staff_c});
    program.call(entry::person_name, 0x106a2650, {d_first, d_middle, d_last, buffer_d, 0x190, 5, staff_d});
    const auto weekday = frame.at(0x78);
    program.call(copy_string, 0x106a2668, {weekday, program.call(render_week, 0x106a265c, {week, 0xffffffff})});
    // The score as the club sees it, and how the story reads it: 1 won, 0 drawn, -1 lost. (A match where the
    // club was the second side and the first side scored more reads as 0 here, not -1.)
    const auto home = match.first_goals(), away = match.second_goals();
    const bool is_home = compare(c, std::int32_t(match.first_club()), std::int32_t(c.u32(mine.at))) == 0;
    const auto goals = [&](std::int8_t left, std::int8_t right, int outcome, std::uint32_t score_back) {
        held = std::uint32_t(outcome);
        format(c, frame.at(0xdc), score_phrase, score_back, {signed_byte(std::uint8_t(left)), signed_byte(std::uint8_t(right))});
    };
    if (is_home) {
        if (compare(c, home, away) > 0) goals(home, away, 1, 0x106a26b4);
        else if (compare(c, home, away) == 0) goals(home, away, 0, 0x106a26f4);
        else goals(away, home, -1, 0x106a271c);
    } else {
        if (compare(c, home, away) > 0) goals(away, home, 0, 0x106a275c);
        else if (compare(c, home, away) == 0) goals(home, away, 0, 0x106a279c);
        else goals(home, away, 1, 0x106a27c4);
    }
    const auto story = frame.at(0x1e28), reply_text = frame.at(0xaa0), score = frame.at(0xdc);
    const bool sent_off = compare(c, std::int32_t(std::uint32_t(kind)), 1) < 0 && compare(c, std::int32_t(std::uint32_t(kind)), 0) >= 0;
    const auto outcome = std::int8_t(std::uint32_t(held));
    const auto outcome_index = compare(c, outcome, 0) == 0 ? 0 : compare(c, outcome, 1) == 0 ? 1 : 2;
    if (sent_off) {
        constexpr std::uint32_t phrases[]{0x3e72a, 0x3e8f8, 0x3eac5}, backs[]{0x106a2844, 0x106a28b4, 0x106a2918};
        { StackWords{c} << mine.long_name() << score << other.long_name() << buffer_d << buffer_c; format(c, story, phrases[outcome_index], backs[outcome_index], {mine.long_marker(), mine.long_marker(), other.long_marker(), other.long_marker(), buffer_a, weekday}); }
    } else {
        constexpr std::uint32_t phrases[]{0x3ec95, 0x3ee2b, 0x3efc4}, backs[]{0x106a2980, 0x106a29ec, 0x106a2a4c};
        { StackWords{c} << mine.long_name() << score << other.long_name() << short_a << week; format(c, story, phrases[outcome_index], backs[outcome_index], {0x64, mine.long_marker(), mine.long_marker(), other.long_marker(), other.long_marker(), buffer_a}); }
    }
    if (compare(c, std::int32_t(std::uint32_t(reply))) != 0) {
        const bool users = is_users_manager(c, staff_at(c, staff_a));
        const auto answer = reply_of(c, std::int32_t(std::uint32_t(reply)));
        if (users) {
            if (answer == 1) format(c, reply_text, 0x3f180, 0x106a2ad8, {0x64, buffer_a});
            else if (answer == 2) format(c, reply_text, 0x3f1ed, 0x106a2af4, {0x64, buffer_a});
            else if (answer == 3) format(c, reply_text, 0x3f15b, 0x106a2abc, {});
        } else {
            if (answer == 1) format(c, reply_text, 0x3f2f7, 0x106a2b64, {0x64, 0x64, buffer_c, buffer_a});
            else if (answer == 2) format(c, reply_text, 0x3f399, 0x106a2b88, {0x64, 0x64, buffer_c, buffer_a});
            else if (answer == 3) format(c, reply_text, 0x3f259, 0x106a2b40, {mine.long_marker(), mine.long_marker(), mine.short_name(), buffer_c});
        }
        program.call(sprintf_entry, 0x106a2ba0, {text, c.toc(slot::pool) + joined_phrase, story, reply_text});
    } else {
        program.call(sprintf_entry, 0x106a2bb8, {text, c.toc(slot::pool) + single_phrase, story});
    }
    out.set(text, 0x106a2bc4);
    out.link(other.short_name(), link_kind::club, other.at, 0x106a2bd8);
    out.link(other.long_name(), link_kind::club, other.at, 0x106a2bec);
    out.link(mine.short_name(), link_kind::club, mine.at, 0x106a2c00);
    out.link(mine.long_name(), link_kind::club, mine.at, 0x106a2c14);
    const auto member_a = staff_at(c, staff_a);
    if (out.named(member_a, a_first, a_middle, a_last, 0x106a2c34)) {
        out.link(buffer_a, link_kind::staff, member_a, 0x106a2c58);
        out.link(short_a, link_kind::staff, member_a, 0x106a2c74);
    }
    out.staff_link(staff_at(c, staff_c), c.u32(c_first), c.u32(c_middle), c.u32(c_last), buffer_c, 0x106a2c94, 0x106a2cb8);
    out.staff_link(staff_at(c, staff_b), c.u32(first_part), c.u32(second_part), c.u32(third_part), buffer_b, 0x106a2cd8, 0x106a2cfc);
    out.staff_link(staff_at(c, staff_d), d_first, d_middle, d_last, buffer_d, 0x106a2d1c, 0x106a2d40);
}
}
