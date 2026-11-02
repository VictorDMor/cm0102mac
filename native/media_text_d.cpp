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

// The user's record (rows of 0x2c0 bytes at the selected slot) and the manager it holds at +4.
std::uint32_t Story::user_offset() const { return extended(c.i16(c.toc(user_slot) + 0x2c58)) * 0x2c0; }
std::uint32_t Story::users_manager() const { return c.u32(c.toc(user_slot) + user_offset() + 4); }

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
    match_ending(c, std::int32_t(outcome), first_back, quote, body, [&] {
        return std::array<std::uint32_t, 4>{home_marker(), home_marker(), home_short(), clubs()};
    });
}

// The goals as two numbers, the larger first. The compiler picked the two with compares and branches, and the
// compares leave cr0 as the second one makes it.
void Story::score(std::uint32_t back) const {
    const auto smaller = cmp(home_goals(), away_goals()) < 0 ? home_goals() : away_goals();
    const auto larger = cmp(home_goals(), away_goals()) > 0 ? home_goals() : away_goals();
    c.set({{3, score_text}, {4, phrase(0x1cc7)}, {5, std::uint32_t(larger)}, {6, std::uint32_t(smaller)}});
    c.call(sprintf_entry, back);
}

// The record, field by field. Fields 0x14 and on are read with a counter kept in the frame. The original
// keeps most of the record's words in non-volatile registers while it calls (the callees save them into
// their frames): kind in r27, the club in r26, the competition in r25, the story code in r24, the subject in
// r23, the title-holder flag in r22, the two hosts in r21 and r20, the subject's club and job in r19 and r18,
// and the run length, the former-player flag, the outcome and the domestic flag in r14, r13, r17 and r16
// (the story members of those names).
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
    manager_id = read(0xb, 0x1068a568);
    c.put32(at(0x2eec), manager_id);
    manager_first = read(0xc, 0x1068a57c);
    c.put32(at(0x2ee8), manager_first);
    manager_middle = read(0xd, 0x1068a590);
    c.put32(at(0x2ee4), manager_middle);
    manager_last = read(0xe, 0x1068a5a4);
    c.put32(at(0x2ee0), manager_last);
    subject_id = read(0xf, 0x1068a5b8);
    c.set(23, subject_id);
    subject_first = read(0x10, 0x1068a5cc);
    c.put32(at(0x2edc), subject_first);
    subject_middle = read(0x11, 0x1068a5e0);
    c.put32(at(0x2ed8), subject_middle);
    subject_last = read(0x12, 0x1068a5f4);
    c.put32(at(0x2ed4), subject_last);
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
    run_length = counted(0x1068a640);
    title_holder = counted(0x1068a658);
    c.set(22, title_holder);
    host_one = counted(0x1068a670);
    c.set(21, host_one);
    host_two = counted(0x1068a688);
    c.set(20, host_two);
    spent = counted(0x1068a6a0);
    c.put32(at(0x2ecc), spent);
    staff_club = counted(0x1068a6b8);
    c.set(19, staff_club);
    staff_job = counted(0x1068a6d0);
    c.set(18, staff_job);
    const auto former_flag = counted(0x1068a6e8);
    c.set(5, former_flag);  // (r5 keeps the raw byte for the next formatter call to spill)
    former_player = sb(former_flag);
    outcome = counted(0x1068a704);
    outlet_region = counted(0x1068a71c);
    c.put32(at(0x2ec8), outlet_region);

    // Whether the user's manager is the club's manager of the story: the story is then about the user's own club.
    users_club = (clz(users_manager() - manager_row()) >> 5) & 0xff;
    c.put8(at(0x2ec4), users_club);
    c.set({{3, manager_first}, {4, manager_middle}, {5, manager_last}, {6, manager_full}, {7, 0x78}, {8, 1}, {9, manager_id}, {10, manager_row()}});
    c.call(entry::person_name, 0x1068a780);
    if (compare(c, std::int32_t(subject_id), -1) > 0) {
        c.set({{3, subject_first}, {4, subject_middle}, {5, subject_last}, {6, subject_full}, {7, 0x78}, {8, 1}, {9, subject_id}});
        c.call(entry::person_name, 0x1068a7a8);
        c.set({{3, subject_first}, {4, subject_middle}, {5, subject_last}, {6, subject_short}, {7, 0x78}, {8, 5}, {9, subject_id}});
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
// country (`domestic`, r16), and whether the world's setting matches that country's (r0, not kept).
void Story::choose_outlet() {
    std::uint32_t same_country = 0, same_league = 0;
    domestic = 0;
    const auto manager = users_manager();
    compare_logical(c, manager, 0);
    if (manager != 0) {
        c.set(5, user_offset());  // (r5 keeps the user's record offset)
        const auto manager_club = records::Person{c, manager}.club().get();
        compare_logical(c, manager_club, 0);
        // The manager's country: his club's, or his own when he has none.
        const auto manager_nation = manager_club != 0 ? records::Club{c, manager_club}.nation().get() : records::Person{c, manager}.nation().get();
        if (compare_logical(c, manager_nation, nation()) == 0) {
            same_country = 1;
            domestic = 1;
            if (compare(c, std::int32_t(c.u32(c.toc(-0x7a10))), records::Nation{c, manager_nation}.id().get()) == 0) same_league = 1;
        }
    }
    bool outlet_by_region = false;
    if (cmp(club_id, -1) != 0 && cmp(outlet_kind, -1) != 0 && cmp(outlet_region, -1) != 0) {
        if (compare_logical(c, nation(), 0) != 0 && compare(c, sb(same_country)) != 0) {
            outlet_by_region = true;
        } else if (compare(c, sb(same_league)) != 0) {
            c.set(3, home());
            c.call(club_flag, 0x1068a974);  // (a flag of the club row: meaning not established)
            const auto club_flag_set = sb(c.r(3));
            if (compare(c, club_flag_set) != 0) {
                // The remainders are computed with shifts: the club id's sign stays in r5, and the outlet text keeps it when it is not the kind's name.
                c.set(5, club_id >> 31);
                outlet_by_region = cmp(rem2(club_id), rem2(code)) == 0;
            }
        }
    }
    if (outlet_by_region) {
        outlet_from_region(c, outlet_kind, outlet_region, outlet_text, at(0x87), 0x1068a9c8);
    } else {
        outlet_name();
    }
}

// The story for the code, from the table of codes in the order the original tests them.
void Story::stories() {
    struct Entry { std::uint32_t code; void (Story::*story)(); };
    static constexpr Entry ladder[]{
        {0x50, &Story::unbeaten_run_ends}, {0x51, &Story::winless_run_ends}, {0x3f, &Story::relegation_battle},
        {0x5f, &Story::promotion_tipped}, {0x60, &Story::relegation_escape_tipped}, {0x42, &Story::title_written_off},
        {0x43, &Story::midtable_written_off}, {0x44, &Story::tournament_written_off}, {0x5a, &Story::tournament_favourites},
        {0x5c, &Story::cup_favourites}, {0x5b, &Story::big_spenders}, {0x52, &Story::starved_striker},
        {0x3e, &Story::relegation_mire}, {0x5e, &Story::winning_form_praised}, {0x3c, &Story::fans_losing_patience},
        {0x3d, &Story::fans_protest_match}, {0x63, &Story::title_challenge_asked}, {0x61, &Story::title_challenge_doubted},
        {0x5d, &Story::title_favourites}, {0x62, &Story::title_run_unstoppable}, {0x40, &Story::promotion_bid_stalling},
        {0x41, &Story::title_pressure}};
    for (const auto& entry : ladder) {
        if (cmp(code, entry.code) == 0) return (this->*entry.story)();
    }
    // Any other code: a question about the subject's ability to progress.
    c.set({{3, body}, {4, phrase(0x3915b)}, {5, subject_full}});
    c.call(sprintf_entry, 0x106971d0);
}

void Story::run() {
    read_record();
    choose_outlet();
    if (compare(c, sb(c.u8(at(0x2f73)))) == 0) {  // (`full`, as stored in the frame)
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
