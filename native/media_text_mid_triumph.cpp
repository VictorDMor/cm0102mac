// The press's reaction to a club winning a competition: the board's and the supporters' view, as a
// story record with the staff member, the club, the competition and the season's figures.
#include "media_text_mid.hpp"
#include "media_text_mid_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::text;

namespace {
constexpr std::uint32_t sprintf_entry = 0x1077b720, joined_phrase = 0xc024, single_phrase = 0x3f438;
}

void competition_win_text(Cpu& c, std::uint32_t record, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x106a7120, 0x4240, {.first_gpr = 13});
    keep(c, {{16, record}, {29, c.toc(user_slot)}, {30, c.toc(slot::clubs)}, {15, output}, {31, c.toc(slot::pool)}, {28, 0}});
    c.put8(frame.at(0x4263), full);
    constexpr std::uint32_t backs[]{0x106a7160, 0x106a7174, 0x106a7188, 0x106a719c, 0x106a71b0, 0x106a71c4, 0x106a71d8,
                                    0x106a71e8, 0x106a71fc, 0x106a7210, 0x106a7224, 0x106a7238, 0x106a724c};
    RecordReader read{.c = c, .record = record, .cursor = 17, .backs = backs};
    Kept<17> cursor(c, 0);
    Kept<25> staff_id(c, read.next());
    Kept<24> first(c, read.next());
    Kept<23> middle(c, read.next());
    Kept<22> last(c, read.next());
    Kept<21> club_id(c, read.next());
    const auto competition_id = frame.at(0x41e4);
    c.put32(competition_id, read.next());
    read.next();  // a field the original reads and drops
    Kept<20> outlet_kind(c, read.next());
    Kept<13> region(c, read.next());
    Kept<19> type(c, read.next());
    Kept<18> rating(c, read.next());
    Kept<14> first_season(c, read.next());
    cursor = read.next();
    // The last field is read with r17 already holding the previous one.
    Kept<16> points(c, Program{c}.call(entry::read_field, 0x106a725c, {record, 13}));
    Kept<27> club_held(c, table(c, -0x7be8) + std::uint32_t(club_id) * club_size);
    Kept<26> competition_held(c, table(c, -0x7bbc) + c.u32(competition_id) * competition_size);
    const Club club{c, club_held};
    const Competition competition{c, competition_held};
    const auto manager_name = frame.at(0x5bc), manager_short = frame.at(0x42c), first_text = frame.at(0x2e5c), second_text = frame.at(0x1ad4),
               text = frame.at(0x74c);
    Program program{c};
    program.call(entry::person_name, 0x106a72a0, {first, middle, last, manager_name, 0x190, 1, staff_id});
    program.call(entry::person_name, 0x106a72c0, {first, middle, last, manager_short, 0x190, 5, staff_id});
    describe_outlet(c, {0x106a73b4, frame.at(0x238), frame.at(0x44), frame.at(0x40), true}, outlet_kind, club_id, region);
    const Output out{c, output};
    const auto weeks = std::int32_t(std::uint32_t(cursor)), before = std::int32_t(std::uint32_t(rating)), now = std::int32_t(std::uint32_t(points));
    // Read where a story uses them: a competition of another type has neither.
    const auto level = [&] { return std::int32_t(c.i16(competition.at + 0x18)); };
    const auto marker = [&] { return competition.marker(); };
    Kept<28> found(c, 0);
    const auto say = [&](std::uint32_t phrase, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        format(c, second_text, phrase, back, arguments);
        found = 1;
    };
    // The reputation gained since the competition's last winner, against its level.
    const auto below_level = [&] { return compare(c, std::int32_t(std::uint32_t(now) - 0x5dc), std::int32_t(std::uint32_t(level()) * 0x1f4)) < 0; };
    if (compare(c, signed_byte(c.u8(frame.at(0x4263)))) == 0) {
        program.call(entry::person_name, 0x106a7c38, {first, middle, last, manager_name, 0x190, 5, staff_id});
        const auto against_two = compare(c, std::int32_t(std::uint32_t(type)), 2);
        if (against_two > 0 && compare(c, std::int32_t(std::uint32_t(type)), 4) < 0) {
            if (compare(c, before, 0x1482) < 0) format(c, text, 0x41896, 0x106a7c84, {marker(), marker(), competition.name()});
            else format(c, text, 0x418d8, 0x106a7cac, {marker(), marker(), competition.name()});
        } else if (against_two < 0 && compare(c, std::int32_t(std::uint32_t(type)), 1) >= 0) {
            format(c, text, 0x4191c, 0x106a7cd4, {marker(), marker(), competition.name()});
        } else program.call(sprintf_entry, 0x106a7ce8, {text, c.toc(slot::pool) + 0x41960});
        out.set(text, 0x106a7cf4);
        out.staff_link(staff_at(c, staff_id), first, middle, last, manager_name, 0x106a7d18, 0x106a7d40);
        out.link(competition.name(), link_kind::competition, competition.at, 0x106a7d54);
        return;
    }
    const auto against_two = compare(c, std::int32_t(std::uint32_t(type)), 2);
    const bool third = against_two > 0 && compare(c, std::int32_t(std::uint32_t(type)), 4) < 0;
    const bool second = against_two < 0 && compare(c, std::int32_t(std::uint32_t(type)), 1) >= 0;
    std::uint32_t joined_back = 0, single_back = 0;
    if (third) {
        const bool announced = compare(c, std::int32_t(std::uint32_t(first_season))) != 0;
        if (announced) {
            format(c, first_text, 0x40f15, 0x106a76dc, {club.long_marker(), club.long_marker(), marker(), marker(), club.long_name(), competition.name()});
            joined_back = 0x106a77b4; single_back = 0x106a77cc;
            if (compare(c, weeks, 0x16d) < 0) say(0x40fca, 0x106a76f8, {manager_short});
            else if (below_level()) say(0x41068, 0x106a773c, {club.short_marker(), club.short_marker(), club.short_name(), manager_short});
            else if (compare(c, now, 0x1c52) < 0 && compare(c, before, 0x1c52) > 0) say(0x41122, 0x106a7768, {manager_short});
            else if (compare(c, now, 0x1c52) > 0 && compare(c, before, 0x1a5e) < 0) say(0x4119b, 0x106a7790, {});
        } else {
            const bool low = compare(c, before, 0x128e) < 0;
            format(c, first_text, low ? 0x411d1 : 0x4125a, low ? 0x106a7810 : 0x106a784c,
                   {club.long_marker(), club.long_marker(), marker(), marker(), club.long_name(), competition.name()});
            joined_back = 0x106a79a8; single_back = 0x106a79c0;
            if (compare(c, before, 0x128e) < 0) say(0x412ee, 0x106a7864, {});
            else if (compare(c, weeks, 0x16d) < 0) say(0x40fca, 0x106a7888, {manager_short});
            else if (below_level()) say(0x4135f, 0x106a78cc, {club.short_marker(), club.short_marker(), club.short_name(), manager_short});
            else if (compare(c, now, 0x1482) < 0 && compare(c, before, 0x1c52) > 0) say(0x41414, 0x106a78f8, {manager_short});
            else if (compare(c, now, 0x1c52) < 0 && compare(c, before, 0x1c52) > 0) say(0x41122, 0x106a7924, {manager_short});
            else if (compare(c, level(), 0xa) > 0 && compare(c, now, 0x1c52) > 0 && compare(c, before, 0x1a5e) < 0) say(0x4119b, 0x106a7958, {});
            else if (compare(c, now, 0x1c52) < 0 && compare(c, before, 0x1a5e) < 0) say(0x41485, 0x106a7984, {manager_short});
        }
    } else if (second) {
        const bool announced = compare(c, std::int32_t(std::uint32_t(first_season))) != 0;
        { StackWords{c} << manager_name << competition.name(); format(c, first_text, announced ? 0x41507 : 0x415c7, announced ? 0x106a7a14 : 0x106a7a60, {club.long_marker(), club.long_marker(), 0x64, marker(), marker(), club.long_name()}); }
        joined_back = 0x106a7b20; single_back = 0x106a7b38;
        if (compare(c, level(), 0xa) > 0 && compare(c, now, 0x1a5e) < 0 && compare(c, before, 0x1f40) > 0) say(0x4166d, 0x106a7a90, {manager_short});
        else if (compare(c, now, 0x1c52) < 0 && compare(c, before, 0x1a5e) < 0) say(0x416e8, 0x106a7ac0, {0x64, manager_short});
        else if (compare(c, level(), 0xa) > 0 && compare(c, now, 0x1c52) < 0 && compare(c, now, 0x1482) > 0) say(0x41781, 0x106a7afc, {0x64, manager_short});
    }
    if (third || second) {
        if (compare(c, signed_byte(found)) != 0) program.call(sprintf_entry, joined_back, {text, c.toc(slot::pool) + joined_phrase, first_text, second_text});
        else program.call(sprintf_entry, single_back, {text, c.toc(slot::pool) + single_phrase, first_text});
    } else program.call(sprintf_entry, 0x106a7b4c, {text, c.toc(slot::pool) + 0x417f6});
    out.set(text, 0x106a7b58);
    out.link(club.short_name(), link_kind::club, club.at, 0x106a7b6c);
    out.link(club.long_name(), link_kind::club, club.at, 0x106a7b80);
    const auto member = staff_at(c, staff_id);
    if (out.named(member, first, middle, last, 0x106a7ba4)) {
        out.link(manager_name, link_kind::staff, member, 0x106a7bcc);
        out.link(manager_short, link_kind::staff, member, 0x106a7bec);
    }
    out.link(competition.at + 0x1a, link_kind::competition, competition.at, 0x106a7c00);
    out.link(competition.name(), link_kind::competition, competition.at, 0x106a7c14);
}
}
