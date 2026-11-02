// The press story writer (media.cpp's largest routine): reads a story record, names the outlet, writes
// the story's body for the story code and the manager's quote, and links the names in the text.
#include "media_text_d.hpp"
#include "media_text_d_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::text;

namespace detail::story {
constexpr std::int32_t user_slot = -0x7bb0;
constexpr std::uint32_t sprintf_entry = 0x1077b720, copy_text = 0x1077dbf0, club_flag = 0x100c7d00;

Story::Story(Cpu& cpu, const Frame& f, std::uint32_t record_, std::uint32_t full_, std::uint32_t output_)
    : c(cpu), frame(f), record(record_), full(full_), output(output_), pool(cpu.toc(slot::pool)), clubs_slot(cpu.toc(slot::clubs)) {}

void Story::say(std::uint32_t destination, std::uint32_t text, std::uint32_t back, std::initializer_list<std::uint32_t> args) const {
    c.set(3, destination);
    c.set(4, text);
    // The arguments past the sixth were stored as outgoing words by the story, where the original stores them.
    unsigned index = 0;
    for (const auto argument : args) c.set(5 + index++, argument);
    c.call(formatter, back);
}

void Story::call(std::uint32_t entry, std::uint32_t back, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers) const {
    c.set(registers);
    c.call(entry, back);
}

void Story::level_ending(std::uint32_t first_back) const {
    match_ending(c, std::int32_t(c.r(17)), first_back, quote, body, [&] {
        return std::array<std::uint32_t, 4>{home_marker(), home_marker(), home_short(), clubs()};
    });
}

// The goals as two numbers, the larger first. The compiler picked the two with compares and branches, and the
// compares leave cr0 as the second one makes it.
void Story::score(std::uint32_t back) const {
    const auto home_raw = c.u8(at(0xe8)), away_raw = c.u8(at(0xe9));
    const auto smaller = cmp(sb(home_raw), sb(away_raw)) < 0 ? home_raw : away_raw;
    const auto larger = cmp(sb(home_raw), sb(away_raw)) > 0 ? home_raw : away_raw;
    c.set({{3, score_text}, {4, phrase(0x1cc7)}, {5, sb(larger)}, {6, sb(smaller)}});
    c.call(sprintf_entry, back);
}

// The record, field by field. Fields 0x14 and on are read with a counter kept in the frame.
void Story::read_record() {
    const auto read = [&](std::uint32_t index, std::uint32_t back) { return Program{c}.call(entry::read_field, back, {record, index}); };
    const auto counted = [&](std::uint32_t back) {
        const auto index = c.u8(at(0x86));
        c.put8(at(0x86), index + 1);
        return read(index, back);
    };
    c.set(3, at(0xcc));
    c.call(today, 0x1068a4a4);  // the match result that the record may hold, cleared
    read(0, 0x1068a4b0);
    kind = read(1, 0x1068a4bc);
    c.set(27, kind);
    outlet_kind = read(2, 0x1068a4d0);
    c.put32(at(0x2efc), outlet_kind);
    read(3, 0x1068a4e4);
    club_id = read(4, 0x1068a4f0);
    c.set(26, club_id);
    field5 = read(5, 0x1068a504);
    c.put32(at(0x2ef8), field5);
    field6 = read(6, 0x1068a518);
    c.put32(at(0x2ef4), field6);
    competition_id = read(7, 0x1068a52c);
    c.set(25, competition_id);
    code = read(9, 0x1068a540);
    c.set(24, code);
    mood = read(0xa, 0x1068a554);
    c.put32(at(0x2ef0), mood);
    other_staff = read(0xb, 0x1068a568);
    c.put32(at(0x2eec), other_staff);
    other_first = read(0xc, 0x1068a57c);
    c.put32(at(0x2ee8), other_first);
    other_middle = read(0xd, 0x1068a590);
    c.put32(at(0x2ee4), other_middle);
    other_last = read(0xe, 0x1068a5a4);
    c.put32(at(0x2ee0), other_last);
    staff_id = read(0xf, 0x1068a5b8);
    c.set(23, staff_id);
    staff_first = read(0x10, 0x1068a5cc);
    c.put32(at(0x2edc), staff_first);
    staff_middle = read(0x11, 0x1068a5e0);
    c.put32(at(0x2ed8), staff_middle);
    staff_last = read(0x12, 0x1068a5f4);
    c.put32(at(0x2ed4), staff_last);
    // Whether the record holds a match result: if it does, 0x1030dee0 decodes it from field 0x14 on.
    const auto result = read(0x13, 0x1068a608);
    compare(c, std::int8_t(result));
    c.put8(at(0x86), 0x14);
    has_match = result & 0xff;
    c.put8(at(0x2ed0), has_match);
    c.set(4, 0x14);
    if (std::int8_t(result) != 0) {
        c.set({{3, record}, {4, at(0x86)}, {5, at(0xcc)}});
        c.call(decode_match, 0x1068a62c);
    }
    c.set(14, counted(0x1068a640));
    field15 = counted(0x1068a658);
    c.set(22, field15);
    field16 = counted(0x1068a670);
    c.set(21, field16);
    field17 = counted(0x1068a688);
    c.set(20, field17);
    field18 = counted(0x1068a6a0);
    c.put32(at(0x2ecc), field18);
    field19 = counted(0x1068a6b8);
    c.set(19, field19);
    field1a = counted(0x1068a6d0);
    c.set(18, field1a);
    const auto field1b = counted(0x1068a6e8);
    c.set(5, field1b);
    c.set(13, sb(field1b));
    c.set(17, counted(0x1068a704));
    field1d = counted(0x1068a71c);
    c.put32(at(0x2ec8), field1d);

    // Whether the user's manager is the other person of the story (the user's record is at the selected slot).
    const auto user_row = [&] { return c.toc(user_slot) + extended(c.i16(c.toc(user_slot) + 0x2c58)) * 0x2c0; };
    own_club = (clz(c.u32(user_row() + 4) - other_row()) >> 5) & 0xff;
    c.put8(at(0x2ec4), own_club);
    c.set({{3, other_first}, {4, other_middle}, {5, other_last}, {6, other_full}, {7, 0x78}, {8, 1}, {9, other_staff}, {10, other_row()}});
    c.call(entry::person_name, 0x1068a780);
    if (compare(c, std::int32_t(staff_id), -1) > 0) {
        c.set({{3, staff_first}, {4, staff_middle}, {5, staff_last}, {6, subject_full}, {7, 0x78}, {8, 1}, {9, staff_id}});
        c.call(entry::person_name, 0x1068a7a8);
        c.set({{3, staff_first}, {4, staff_middle}, {5, staff_last}, {6, subject_short}, {7, 0x78}, {8, 5}, {9, staff_id}});
        c.call(entry::person_name, 0x1068a7c8);
    } else {
        // Nobody named: a stand-in legend in both forms.
        c.set({{3, subject_full}, {4, pool + 0x2b6a9}});
        c.call(copy_text, 0x1068a7dc);
        c.set({{3, subject_short}, {4, pool + 0x2b6b4}});
        c.call(copy_text, 0x1068a7ec);
    }
}

// Which outlet runs the story. The user's manager decides two flags: whether he comes from the club's
// country (r16), and whether the world's setting matches that country's (r0).
void Story::choose_outlet() {
    const auto user_row = [&] { return c.toc(user_slot) + extended(c.i16(c.toc(user_slot) + 0x2c58)) * 0x2c0; };
    std::uint32_t same_country = 0, same_league = 0;
    c.set(16, 0);
    const auto manager = c.u32(user_row() + 4);
    compare_logical(c, manager, 0);
    if (manager != 0) {
        c.set(5, extended(c.i16(c.toc(user_slot) + 0x2c58)) * 0x2c0);
        const auto manager_club = c.u32(manager + 0x24);
        compare_logical(c, manager_club, 0);
        // The manager's country: his club's, or his own when he has none.
        const auto manager_nation = manager_club != 0 ? c.u32(manager_club + 0x2c) : c.u32(manager + 0xc);
        if (compare_logical(c, manager_nation, nation()) == 0) {
            same_country = 1;
            c.set(16, 1);
            if (compare(c, std::int32_t(c.u32(c.toc(-0x7a10))), std::int32_t(c.u32(manager_nation + 0x68))) == 0) same_league = 1;
        }
    }
    bool kind_name = false;
    if (cmp(club_id, -1) != 0 && cmp(outlet_kind, -1) != 0 && cmp(field1d, -1) != 0) {
        if (compare_logical(c, nation(), 0) != 0 && compare(c, sb(same_country)) != 0) {
            kind_name = true;
        } else if (compare(c, sb(same_league)) != 0) {
            c.set(3, home());
            c.call(club_flag, 0x1068a974);
            if (compare(c, sb(c.r(3))) != 0) {
                // The remainders are computed with shifts: the club id's sign stays in r5, and the outlet text keeps it when it is not the kind's name.
                c.set(5, club_id >> 31);
                kind_name = cmp(rem2(club_id), rem2(code)) == 0;
            }
        }
    }
    if (kind_name) {
        outlet_from_region(c, outlet_kind, field1d, outlet_text, at(0x87), 0x1068a9c8);
    } else {
        outlet_name();
    }
}

// The story for the code, from the table of codes in the order the original tests them.
void Story::stories() {
    struct Entry { std::uint32_t code; void (Story::*story)(); };
    static constexpr Entry ladder[]{
        {0x50, &Story::story_50}, {0x51, &Story::story_51}, {0x3f, &Story::story_3f}, {0x5f, &Story::story_5f},
        {0x60, &Story::story_60}, {0x42, &Story::story_42}, {0x43, &Story::story_43}, {0x44, &Story::story_44},
        {0x5a, &Story::story_5a}, {0x5c, &Story::story_5c}, {0x5b, &Story::story_5b}, {0x52, &Story::story_52},
        {0x3e, &Story::story_3e}, {0x5e, &Story::story_5e}, {0x3c, &Story::story_3c}, {0x3d, &Story::story_3d},
        {0x63, &Story::story_63}, {0x61, &Story::story_61}, {0x5d, &Story::story_5d}, {0x62, &Story::story_62},
        {0x40, &Story::story_40}, {0x41, &Story::story_41}};
    for (const auto& entry : ladder) {
        if (cmp(code, entry.code) == 0) return (this->*entry.story)();
    }
    // Any other code: a question about the staff member's ability to progress.
    c.set({{3, body}, {4, phrase(0x3915b)}, {5, subject_full}});
    c.call(sprintf_entry, 0x106971d0);
}

void Story::run() {
    read_record();
    choose_outlet();
    if (compare(c, sb(c.u8(at(0x2f73)))) == 0) {
        // The short version: a headline over the club.
        c.set(8, home());
        headline();
        c.set({{3, output}, {4, body}});
        c.call(entry::set_text, 0x10698b68);
        c.set({{3, output}, {4, home_short()}, {5, 0x7d5}, {6, home()}, {7, clubs()}});
        c.call(entry::add_link, 0x10698b90);
        return;
    }
    stories();
    quote_text();
    links();
}
}

void press_feature_text(Cpu& c, std::uint32_t record, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1068a470, 0x2f50, {.first_gpr = 13});
    keep(c, {{16, record}, {15, output}, {28, c.toc(-0x7bbc)}, {29, c.toc(-0x7b98)}, {30, c.toc(slot::clubs)}, {31, c.toc(slot::pool)}});
    c.put8(frame.at(0x2f73), full);
    detail::story::Story story(c, frame, record, full, output);
    story.run();
}
}
