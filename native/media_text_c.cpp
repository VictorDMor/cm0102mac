// The press story about a staff member's praise, a match, a rumour or a former star's words: the record
// names the people, clubs, nations and the competition involved, and numbers that choose the wording.
// A long story is two paragraphs (the story, then a closing sentence on the club's season); a short one
// is one line. Each ends with the text going to the output object and a link for every name in it.
#include "media_text_c.hpp"
#include "media_text_c_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::report;
using namespace detail::text;

void Report::say(std::uint32_t destination, unsigned phrase, std::uint32_t back, std::initializer_list<Words> args,
                 std::initializer_list<RegisterValue> leftovers) const {
    std::array<std::uint32_t, 24> words{};
    unsigned count = 0;
    for (const auto& arg : args)
        for (unsigned i = 0; i < arg.n; ++i) words[count++] = arg.v[i];
    // The compiler reused the dead story variables as temporaries; the formatter's prologue saves them.
    keep(c, leftovers);
    c.set(3, destination);
    c.set(4, pool(phrase));
    for (unsigned i = 0; i < count; ++i) c.set(5 + i, words[i]);  // (the words past the sixth are stored by the caller)
    c.call(formatter, back);
}

// The press outlet that carries the story, written to `outlet` with the marker for the article that goes
// before it (text_shared.hpp).
void Report::outlet_text() {
    // A region and a kind both known: the outlet comes from the region's own table.
    if (cmpl(clubs[club].nation(), 0) != 0 && cmp(outlet_kind, -1) != 0 && cmp(outlet_region, -1) != 0 && cmp(signed_byte(domestic), 0) != 0) {
        outlet_from_region(c, outlet_kind, outlet_region, outlet, article_marker.address(), 0x106804cc);
        return;
    }
    write_outlet(c, {0x106804dc, outlet, nationality, article_marker.address()}, std::int32_t(std::uint32_t(outlet_kind)), std::uint32_t(domestic), clubs[club].at);
}

// The famous person the story quotes: a staff member's name, or one of four from a short list.
void Report::legend_text() {
    if (cmp(legend, -1) > 0) {
        call(entry::person_name, 0x106807b8, {legend_first, legend_middle, legend_last, legend_name, 0x78, 1, legend});
        return;
    }
    const std::int32_t rest = std::int32_t(std::uint32_t(subject) + std::uint32_t(tone)) % 4;
    unsigned name = 0x216ec;  // the first stand-in legend
    std::uint32_t back = 0x10680844;
    if (const auto against_one = cmp(rest, 1); against_one == 0) { name = 0x216d8; back = 0x1068081c; }  // the second
    else if (against_one > 0) {
        if (cmp(rest, 3) < 0) { name = 0x216e7; back = 0x10680830; }  // the third
    } else if (cmp(rest, 0) >= 0) { name = 0x216c9; back = 0x10680808; }  // the fourth
    call(copy_entry, back, {legend_name, pool(name)});
}

// The story's variables from the record, field after field through 0x102cb570.
void Report::read_record(std::uint32_t record) {
    const auto field = [&](std::uint32_t back, std::uint32_t index) { return call(entry::read_field, back, {record, index}); };
    subject = field(0x1067ff10, 0);
    subject_first = field(0x1067ff24, 1);
    subject_middle = field(0x1067ff38, 2);
    subject_last = field(0x1067ff4c, 3);
    club = field(0x1067ff60, 4);
    kind = field(0x1067ff74, 5);
    field(0x1067ff88, 6);  // a field the original reads and drops
    nation = field(0x1067ff94, 7);
    tone = field(0x1067ffa8, 8);
    other = field(0x1067ffbc, 9);
    other_first = field(0x1067ffd0, 10);
    other_middle = field(0x1067ffe4, 11);
    other_last = field(0x1067fff8, 12);
    closing = field(0x1068000c, 13);
    flags = field(0x10680020, 14);
    amount = extended(std::int16_t(field(0x10680034, 15)));
    competition = field(0x10680048, 16);
    const auto matched = field(0x1068005c, 17);
    // A match, when the record has one, is decoded from the fields that follow; the counter in the frame says where they continue.
    const auto counter = frame.at(0x8b);
    c.set(4, 0x12);
    c.put8(counter, 0x12);
    const bool has_match = compare(c, signed_byte(matched)) != 0;
    with_match = signed_byte(matched);
    if (has_match) call(decode_match, 0x10680080, {record, counter, frame.at(0xdc)});
    const auto next = [&](std::uint32_t back) {
        const auto index = c.u8(counter);
        c.put8(counter, index + 1);
        return field(back, index);
    };
    suitor = next(0x10680094);
    transfer_agreed = next(0x106800ac);
    const auto day = next(0x106800c4);
    due_day = std::int16_t(day);
    c.set(5, day);
    const auto year = next(0x106800e4);
    due_year = std::uint16_t(year);
    c.set(5, year);
    const auto rumour = next(0x10680104);
    transfer_rumour = signed_byte(rumour);
    c.set(5, rumour);
    legend = next(0x10680124);
    legend_first = next(0x1068013c);
    legend_middle = next(0x10680154);
    legend_last = next(0x1068016c);
    nation_a = next(0x10680184);
    nation_b = next(0x1068019c);
    outcome = next(0x106801b4);
    legend_job = next(0x106801cc);
    previous_club = next(0x106801e4);
    outlet_kind = next(0x106801fc);
    outlet_region = next(0x10680214);
    position = next(0x1068022c);
    second_division = next(0x10680244);
    // A club outside the table is none.
    if (compare(c, previous_club, 0) < 0) previous_club = std::uint32_t(-1);
    else {
        const auto total = c.toc(-0x7b50);
        c.set(3, total);
        if (compare(c, previous_club, c.u32(total)) >= 0) previous_club = std::uint32_t(-1);
    }
}

void press_story_text(Cpu& c, std::uint32_t record, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1067fed0, 0x1f20, {.first_gpr = 13});
    keep(c, {{14, record}, {15, output}, {28, c.toc(-0x7bbc)}, {29, c.toc(-0x7b98)}, {30, c.toc(slot::clubs)}, {31, c.toc(slot::pool)}});
    Report r(c, frame, output);
    r.full = full;
    Program{c}.call(today, 0x1067ff04, {frame.at(0xdc)});
    r.read_record(record);

    // Who the user's manager is decides which outlet speaks and whether a club that wants the player is named.
    const auto user = c.toc(-0x7bb0);
    const auto user_row = user + std::uint32_t(std::int32_t(c.i16(user + 0x2c58))) * 0x2c0;
    r.domestic = 0;
    const auto subject_row = c.u32(c.toc(-0x7be0)) + std::uint32_t(r.subject) * staff_size;
    c.set(27, subject_row);
    const auto manager_row = c.u32(user_row + 4);  // (read before the frame's two words are written)
    r.other_row = c.u32(c.toc(-0x7be0)) + std::uint32_t(r.other) * staff_size;
    r.users_manager = manager_row;
    if (compare_logical(c, r.users_manager, 0) != 0) {
        const auto manager = std::uint32_t(r.users_manager);
        const auto manager_club = [&] { return c.u32(manager + 0x24); };
        // (the club's nation is compared with the manager's club's, or with the manager's own when he has none)
        if (compare(c, r.transfer_agreed, 0) == 0 && compare(c, std::int8_t(std::uint32_t(r.transfer_rumour))) == 0 &&
            compare(c, r.suitor, -1) > 0 && compare(c, std::int32_t(std::uint32_t(r.subject)) % 4) != 0) {
            if (compare_logical(c, manager_club(), 0) != 0) {
                if (compare_logical(c, c.u32(manager_club() + 0x2c), r.clubs[r.suitor].nation()) == 0) r.suitor = std::uint32_t(-1);
            } else if (compare_logical(c, r.clubs[r.suitor].nation(), c.u32(manager + 0xc)) == 0) r.suitor = std::uint32_t(-1);
        }
        if (compare_logical(c, manager_club(), 0) != 0) {
            if (compare_logical(c, c.u32(manager_club() + 0x2c), r.clubs[r.club].nation()) == 0) r.domestic = 1;
        } else if (compare_logical(c, r.clubs[r.club].nation(), c.u32(manager + 0xc)) == 0) r.domestic = 1;
    }
    // The amount's wording; when the staff member and the other are the same person the text says so instead.
    c.set(7, c.u32(c.toc(-0x7be0)));
    const auto amount_flag = r.call(amount_text_entry, 0x10680450, {r.amount, r.amount_text, 0xc8, 1});
    const bool same_person = compare_logical(c, r.other_row, subject_row) == 0;
    r.amount_flag = amount_flag;
    if (same_person) {
        format(c, r.amount_text, 0xcc2, 0x1068046c);
        r.amount_flag = 0;
    }
    r.outlet_text();
    r.legend_text();
    r.call(entry::person_name, 0x10680864, {r.subject_first, r.subject_middle, r.subject_last, r.subject_full, 0x78, 1, r.subject});
    r.call(entry::person_name, 0x10680884, {r.subject_first, r.subject_middle, r.subject_last, r.subject_short, 0x78, 5, r.subject});
    r.call(entry::person_name, 0x106808a4, {r.other_first, r.other_middle, r.other_last, r.other_full, 0x78, 1, r.other});
    r.call(entry::person_name, 0x106808c4, {r.other_first, r.other_middle, r.other_last, r.other_short, 0x78, 5, r.other});

    if (compare(c, signed_byte(r.full)) != 0) {
        r.long_story();
        // The two paragraphs are joined in the shared text buffer.
        r.call(sprintf_entry, 0x10688304, {c.toc(slot::text), r.pool(0xc024), r.story, r.paragraph});
        r.out.set(c.toc(slot::text), 0x10688310);
    } else {
        r.short_story();
        r.out.set(r.story, 0x106886c0);
    }
    r.links();
}

// The story by its kind (0x29-0x34; any other is the one of 0x2c), then the closing sentence.
void Report::long_story() {
    cmpl(kind - 0x29, 0xb);
    switch (std::uint32_t(kind) - 0x29) {
    case 0: kind_29(); break;
    case 1: kind_2a(); break;
    case 2: kind_2b(); break;
    case 4: kind_2d(); break;
    case 5: kind_2e(); break;
    case 6: kind_2f(); break;
    case 7: kind_30(); break;
    case 8: kind_31(); break;
    case 9: kind_32(); break;
    case 10: kind_33(); break;
    case 11: kind_34(); break;
    default: kind_2c(); break;
    }
    paragraph_two();
}

// Every name the story printed becomes a link.
void Report::links() {
    const auto add_link = [&](std::uint32_t text, std::uint32_t token_kind, std::uint32_t target, std::uint32_t back,
                              std::optional<std::uint32_t> leftover = {}) { out.link(text, token_kind, target, back, leftover); };
    // A staff member who is named in the text gets a link for his long and short names.
    const auto staff_links = [&](std::uint32_t member, std::uint32_t first, std::uint32_t middle, std::uint32_t last, std::uint32_t long_name,
                                 std::uint32_t short_name, std::uint32_t named_back, std::uint32_t long_back, std::uint32_t short_back) {
        if (!out.named(member, first, middle, last, named_back)) return;
        add_link(long_name, link_kind::staff, member, long_back);
        add_link(short_name, link_kind::staff, member, short_back);
    };
    const auto club_links = [&](std::uint32_t id, std::uint32_t short_back, std::uint32_t long_back) {
        add_link(clubs[id].short_name(), link_kind::club, clubs[id], short_back, clubs.base());
        add_link(clubs[id].long_name(), link_kind::club, clubs[id], long_back, clubs.base());
    };
    if (cmp(signed_byte(with_match), 0) != 0) {
        const auto found = call(match_known, 0x106886f0, {c.u32(c.toc(-0x7b4c)), match_id, first_club, second_club, c.u16(frame.at(0xdc)), today_year});
        if (compare(c, std::int8_t(found)) != 0) add_link(score, 0x7d3, match_id, 0x1068870c);
        add_link(clubs[second_club].long_name(), link_kind::club, clubs[second_club], 0x1068873c, clubs.base());
    }
    staff_links(people[subject], subject_first, subject_middle, subject_last, subject_full, subject_short, 0x10688760, 0x10688788, 0x106887a8);
    if (cmp(other, -1) > 0)
        staff_links(people[other], other_first, other_middle, other_last, other_full, other_short, 0x106887dc, 0x10688808, 0x1068882c);
    if (cmp(legend, -1) > 0 && out.named(people[legend], legend_first, legend_middle, legend_last, 0x10688858))
        add_link(legend_name, link_kind::staff, people[legend], 0x10688880);
    if (cmp(previous_club, -1) != 0) club_links(previous_club, 0x106888b0, 0x106888d8);
    if (cmp(suitor, -1) != 0) club_links(suitor, 0x10688908, 0x10688930);
    if (cmp(club, -1) != 0) {
        club_links(club, 0x10688960, 0x10688988);
        // The club's nation: its team is linked by each way the nation is written.
        const auto club_nation = clubs[club].nation();
        const auto team_link = [&](std::uint32_t text, std::uint32_t team_back, std::uint32_t link_back, NationView nation_view) {
            add_link(text, link_kind::club, call(nation_team, team_back, {nation_view}), link_back);
        };
        team_link(club_nation.short_name(), 0x1068899c, 0x106889c0, club_nation);
        team_link(club_nation.name(), 0x106889d4, 0x106889f8, club_nation);
        // The nationality as an adjective, in each of the ten forms.
        for (unsigned form = 0; form < 10; ++form) {
            const auto back = 0x10688a18 + 0x48 * form;
            call(nationality_of, back, {club_nation, nationality, 0x78, 0x16 + form});
            team_link(nationality, back + 0x14, back + 0x28, club_nation);
        }
    }
    // A nation's team is linked by the nation's name, then by its short name.
    const auto nation_links = [&](NationView nation_view, std::array<std::uint32_t, 4> backs) {
        add_link(nation_view.name(), link_kind::club, call(nation_team, backs[0], {nation_view}), backs[1]);
        add_link(nation_view.short_name(), link_kind::club, call(nation_team, backs[2], {nation_view}), backs[3]);
    };
    if (cmp(nation, -1) != 0) nation_links(nations[nation], {0x10688ce8, 0x10688d0c, 0x10688d20, 0x10688d44});
    if (cmp(nation_a, -1) != 0) nation_links(nations[nation_a], {0x10688d5c, 0x10688d7c, 0x10688d8c, 0x10688dac});
    if (cmp(nation_b, -1) != 0) nation_links(nations[nation_b], {0x10688dc4, 0x10688de4, 0x10688df4, 0x10688e14});
    if (cmp(competition, -1) != 0) {
        add_link(competitions[competition].short_name(), link_kind::competition, competitions[competition], 0x10688e44, competitions.base());
        add_link(competitions[competition].name(), link_kind::competition, competitions[competition], 0x10688e6c, competitions.base());
    }
}
}
