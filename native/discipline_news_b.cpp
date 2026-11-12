// DISCIPLINE.CPP news block B (discipline_news_b.hpp): the text of a ban story, native-first. The decision tree
// picks a format of the module's pool and an argument shape; one function per shape makes the call.
#include "discipline_news_b.hpp"
#include "discipline_adapter.hpp"
#include <initializer_list>

namespace cm::aot::discipline {
using guest::Cpu;
using guest::Frame;

namespace {
namespace callee {
constexpr std::uint32_t init_date = 0x10715880, field = 0x102cb570, read_words = 0x1030e2a0, read_date = 0x1030dee0,
                        other_story = 0x100e0b70, person_known = 0x100cbde0, fixture_known = 0x1029eb20,
                        person_name = 0x100cb590, date_text = 0x1030e880, format = 0x10727890, set_text = 0x1074b830,
                        link_text = 0x1074bab0, describe_bodies = 0x100e7490, format_text = 0x1077b720;
}
namespace toc {
constexpr std::int32_t pool = -0x5608, persons = -0x7be0, text_buffer = -0x7bb8, clubs = -0x7be8, fixtures = -0x7b4c;
}
namespace at {  // offsets in the routine's frame (0x810 bytes)
constexpr std::int32_t counter = 0x50, bodies = 0x54, short_text = 0x58, date_text = 0x64, key = 0x84, person_row = 0x90,
                       person_date = 0x94, fixture_date = 0x9c, home_club = 0xb0, away_club = 0xb4, home_goals = 0xb8,
                       away_goals = 0xb9, fixture_id = 0xbc, bodies_text = 0xc0, names = 0x124, plain_story = 0x1ec,
                       story = 0x3e0;
}
namespace link_kind {  // 0x1074bab0's third argument
constexpr std::uint32_t person = 0x7d9, club = 0x7d5, fixture = 0x7d3;
}
namespace format_at {  // offsets in the module's pool
constexpr std::uint32_t headline_one = 0x81, headline_many = 0xba, headline_date = 0x106, score = 0x150,
                        sentence_tail = 0x8213, joined_tail = 0x82ae, tail = 0x82b5;
}
constexpr std::uint32_t club_size = 0x248, person_size = 0x70, standing_at = 0x23b, club_name_at = 0x221;

std::uint32_t sext8(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }
std::uint32_t sext16(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int16_t(value))); }

// A format of the pool and the return address of its call.
struct Site {
    std::uint32_t format, back;
};
enum Compare { home_ahead, away_ahead, level };

// The formats of the body text. Where a ban kind compares the two clubs' goals, three rows (home ahead, away ahead,
// level), each with a text for a length of exactly 1 and one for any other length (0 or more, the date form
// belongs to a negative length: those texts are the second table).
namespace sites {
// Length 0 or more (the class 0x100e7490 returns first).
constexpr Site kind0[2] = {{0x82b8, 0x100e5d6c}, {0x8354, 0x100e5d98}};
constexpr Site kind1[3][2] = {{{0x8403, 0x100e5e44}, {0x84f6, 0x100e5ec4}},
                              {{0x85fc, 0x100e5f4c}, {0x86f0, 0x100e5fcc}},
                              {{0x87f7, 0x100e6050}, {0x88eb, 0x100e60d0}}};
constexpr Site kind1_without_count[2] = {{0x89f2, 0x100e6100}, {0x8a94, 0x100e612c}};
constexpr Site kind4[2] = {{0x8b49, 0x100e6164}, {0x8c01, 0x100e6190}};
constexpr Site kind2[3][2] = {{{0x8ccc, 0x100e6234}, {0x8db5, 0x100e62b4}},
                              {{0x8eb1, 0x100e633c}, {0x8f9b, 0x100e63bc}},
                              {{0x9098, 0x100e6440}, {0x9182, 0x100e64c0}}};
constexpr Site kind3[3][2] = {{{0x927f, 0x100e6564}, {0x9368, 0x100e65e4}},
                              {{0x9464, 0x100e666c}, {0x954e, 0x100e66ec}},
                              {{0x964b, 0x100e6770}, {0x9735, 0x100e67f0}}};
// A negative length (a ban by date): one text per case, no text for the length.
constexpr Site negative_kind0 = {0x9832, 0x100e684c};
constexpr Site negative_kind1[3] = {{0x98ce, 0x100e68f4}, {0x99c1, 0x100e6978}, {0x9ab5, 0x100e69f8}};
constexpr Site negative_kind1_without_count = {0x9ba9, 0x100e6a24};
constexpr Site negative_kind4 = {0x9c4b, 0x100e6a58};
constexpr Site negative_kind2[3] = {{0x9d03, 0x100e6af8}, {0x9df3, 0x100e6b7c}, {0x9ee4, 0x100e6bfc}};
constexpr Site negative_kind3[3] = {{0x9fd5, 0x100e6c9c}, {0xa0be, 0x100e6d20}, {0xa1a8, 0x100e6da0}};
}

class BanStory {
public:
    BanStory(Cpu& cpu, const Frame& frame, std::uint32_t owner, std::uint32_t message, std::uint32_t body_flag,
             std::uint32_t text, std::uint32_t entry_r28)
        : c(cpu), f(frame), owner(owner), message(message), body_flag(body_flag), text(text), entry_r28(entry_r28) {}

    void run() {
        keep(17, c.toc(toc::persons));
        keep(19, f.at(at::person_date));
        keep(20, c.toc(toc::text_buffer));
        keep(22, text);
        keep(24, body_flag);
        keep(25, c.toc(toc::clubs));
        keep(26, owner);
        keep(28, entry_r28);
        keep(27, message);
        keep(29, c.toc(toc::pool));
        c.put8(f.at(at::counter), 0);
        c.set(3, f.at(at::person_date));
        call(callee::init_date, 0x100e59f4);
        c.set(3, f.at(at::fixture_date));
        call(callee::init_date, 0x100e59fc);
        read_message();
        if (disc != 0) {
            // 0x100e0b70 saves what it does not change of r16-r31 in its frames, and its link rows copy them.
            c.set({{3, owner}, {4, message}, {5, body_flag}, {6, text}});
            call_module(callee::other_story, 0x100e5b74);
            return;
        }
        c.set({{3, person_row()}, {4, c.u32(f.at(at::key))}, {5, c.u32(f.at(at::key + 4))}, {6, c.u32(f.at(at::key + 8))},
               {7, f.at(at::person_date)}});
        call(callee::person_known, 0x100e5b9c);
        keep(27, c.r(3));
        person_listed = std::int8_t(c.r(3)) != 0;
        c.set({{3, c.u32(c.toc(toc::fixtures))}, {4, c.u32(f.at(at::fixture_id))}, {5, c.u32(f.at(at::home_club))},
               {6, c.u32(f.at(at::away_club))}, {7, c.u16(f.at(at::fixture_date))}, {8, c.u16(f.at(at::fixture_date) + 2)}});
        call(callee::fixture_known, 0x100e5bc4);
        keep(19, c.r(3));
        fixture_listed = std::int8_t(c.r(3)) != 0;
        if (std::int8_t(body_flag) == 0) headline();
        else body();
    }

private:
    Cpu& c;
    const Frame& f;
    std::uint32_t owner, message, body_flag, text, entry_r28;
    // The message's fields, as the original takes them (signed bytes and halfwords sign-extended).
    std::int8_t has_context = 0, kind = 0, disc = 0;
    std::uint32_t length = 0, days = 0, count = 0;
    bool person_listed = false, fixture_listed = false;

    std::uint32_t pool() const { return c.toc(toc::pool); }
    std::uint32_t clubs() const { return c.u32(c.toc(toc::clubs)); }
    std::uint32_t person_row() const { return c.u32(c.toc(toc::persons)) + c.u32(f.at(at::person_row)) * person_size; }
    std::uint32_t club_row(std::int32_t slot) const { return clubs() + c.u32(f.at(slot)) * club_size; }
    std::uint32_t buffer() const { return c.toc(toc::text_buffer); }

    // The original keeps its values in r16..r31 (callee-saved). The formatter saves them in its frame, and 0x1074bab0
    // copies 0xd0 bytes of its own frame, which then holds the register save area of a callee before it, into the
    // link row it appends to the text object (the reader; the same as original_registers in
    // manager_manager_standing.cpp). So the calls that format a text set the registers the original holds by then
    // (checked: without them the rows' last six words differ); the ones it has not written yet keep the entry's.
    std::uint32_t reg_value[32] = {};
    std::uint32_t reg_known = 0;
    void keep(unsigned n, std::uint32_t value) {
        reg_value[n] = value;
        reg_known |= 1u << n;
    }
    void original_registers() {
        for (unsigned n = 16; n < 32; ++n)
            if (reg_known >> n & 1) c.set(n, reg_value[n]);
    }
    // Every guest call sees the original's r16..r31: its prologue saves them in frames that later link rows copy.
    void call(std::uint32_t entry, std::uint32_t back) {
        original_registers();
        c.call(entry, back);
    }
    void call_module(std::uint32_t entry, std::uint32_t back) {
        original_registers();
        discipline::call(c, entry, back);
    }

    // The next message field: the counter byte in the frame says which (after `skip` unread ones), is advanced past
    // it, and the value comes back in r3.
    std::uint32_t field(std::uint32_t skip, std::uint32_t back) {
        const auto index = c.u8(f.at(at::counter)) + skip;
        c.put8(f.at(at::counter), index + 1);
        c.set({{3, message}, {4, index}});
        call(callee::field, back);
        return c.r(3);
    }
    // The aggregate readers take the counter's address and advance it by what they read.
    void aggregate(std::uint32_t entry, std::uint32_t skip, std::uint32_t destination, std::uint32_t back) {
        c.put8(f.at(at::counter), c.u8(f.at(at::counter)) + skip);
        c.set({{3, message}, {4, f.at(at::counter)}, {5, destination}});
        call(entry, back);
    }

    void read_message() {
        has_context = std::int8_t(field(0, 0x100e5a10));
        keep(18, sext8(has_context));
        field(0, 0x100e5a28);
        length = sext8(field(0, 0x100e5a3c));
        keep(31, length);
        days = sext16(field(0, 0x100e5a54));
        keep(21, days);
        c.put8(f.at(at::bodies), field(2, 0x100e5a78));
        c.put8(f.at(at::bodies + 1), field(0, 0x100e5a90));
        c.put8(f.at(at::bodies + 2), field(0, 0x100e5aa8));
        c.put8(f.at(at::bodies + 3), field(0, 0x100e5ac0));
        kind = std::int8_t(field(0, 0x100e5ad8));
        keep(23, sext8(kind));
        count = sext8(field(0, 0x100e5af0));
        keep(30, count);
        disc = std::int8_t(field(0, 0x100e5b08));
        keep(16, sext8(disc));
        field(0, 0x100e5b20);
        aggregate(callee::read_words, 3, f.at(at::key), 0x100e5b3c);
        aggregate(callee::read_date, 1, f.at(at::fixture_date), 0x100e5b58);
    }

    // `bl 0x10727890`: buffer, format, then the values: the first six in r5..r10, the rest in the parameter area.
    void format(std::uint32_t destination, std::uint32_t format_offset, std::initializer_list<std::uint32_t> values,
                std::uint32_t back) {
        c.set({{3, destination}, {4, pool() + format_offset}});
        unsigned index = 0;
        for (const auto value : values) {
            if (index < 6) c.set(5 + index, value);
            else c.put32(c.sp() + 0x38 + (index - 6) * 4, value);
            ++index;
        }
        call(callee::format, back);
    }
    void format_site(Site site, std::initializer_list<std::uint32_t> values) { format(f.at(at::story), site.format, values, site.back); }

    void person_name(std::uint32_t mode) {
        c.set({{3, c.u32(f.at(at::key))}, {4, c.u32(f.at(at::key + 4))}, {5, c.u32(f.at(at::key + 8))}, {6, f.at(at::names)},
               {7, 200}, {8, mode}, {9, c.u32(f.at(at::person_row))}});
        call(callee::person_name, mode == 5 ? 0x100e5bf0 : 0x100e5cac);
    }
    void set_text(std::uint32_t source, std::uint32_t back) {
        c.set({{3, text}, {4, source}});
        call(callee::set_text, back);
    }
    void link(std::uint32_t source, std::uint32_t kind_of_link, std::uint32_t record, std::uint32_t back) {
        c.set({{3, text}, {4, source}, {5, kind_of_link}, {6, record}});
        call(callee::link_text, back);
    }
    void link_person(std::uint32_t back) { link(f.at(at::names), link_kind::person, person_row(), back); }

    // ---- the headline: the person's name and how long the ban is
    void headline() {
        person_name(5);
        const auto signed_length = std::int32_t(length);
        if (signed_length < 0) {
            c.set({{3, days}, {4, f.at(at::date_text)}, {5, 0x1e}, {6, 0}});
            call(callee::date_text, 0x100e5c40);
            format(buffer(), format_at::headline_date, {f.at(at::names), f.at(at::date_text)}, 0x100e5c54);
        } else if (signed_length == 1) {
            format(buffer(), format_at::headline_one, {f.at(at::names)}, 0x100e5c10);
        } else {
            format(buffer(), format_at::headline_many, {f.at(at::names), length}, 0x100e5c28);
        }
        set_text(buffer(), 0x100e5c60);
        if (person_listed) link_person(0x100e5c88);
    }

    // ---- the body
    void body() {
        person_name(1);
        const auto home_goals = std::int8_t(c.u8(f.at(at::home_goals))), away_goals = std::int8_t(c.u8(f.at(at::away_goals)));
        // The score text takes the higher goals first: r5 is the higher, r6 the lower (the original loaded r6 with the
        // away goals before the compare, and sets only r5 when the home goals are not below).
        if (home_goals >= away_goals) format(f.at(at::short_text), format_at::score, {sext8(home_goals), sext8(away_goals)}, 0x100e5cd4);
        else format(f.at(at::short_text), format_at::score, {sext8(away_goals), sext8(home_goals)}, 0x100e5cec);
        const auto compare = home_goals > away_goals ? home_ahead : home_goals < away_goals ? away_ahead : level;
        if (std::int32_t(length) > -1) story_for_length(compare);
        else story_for_date(compare);
        if (has_context != 0) {
            format(f.at(at::plain_story), format_at::sentence_tail, {}, 0x100e6db8);
            c.set({{3, buffer()}, {4, pool() + format_at::joined_tail}, {5, f.at(at::story)}, {6, f.at(at::plain_story)}});
            call(callee::format_text, 0x100e6dd0);
        } else {
            c.set({{3, buffer()}, {4, pool() + format_at::tail}, {5, f.at(at::story)}});
            call(callee::format_text, 0x100e6de8);
        }
        set_text(buffer(), 0x100e6df4);
        if (person_listed) link_person(0x100e6e1c);
        link(club_row(at::home_club) + club_name_at, link_kind::club, club_row(at::home_club), 0x100e6e3c);
        link(club_row(at::away_club) + club_name_at, link_kind::club, club_row(at::away_club), 0x100e6e5c);
        if (fixture_listed) link(f.at(at::short_text), link_kind::fixture, c.u32(f.at(at::fixture_id)), 0x100e6e78);
    }

    // The two clubs' standings (a signed byte of each club record) and names.
    struct Clubs {
        std::uint32_t home_standing, away_standing, home_name, away_name;
    };
    Clubs clubs_now() const {
        const auto home = c.u32(f.at(at::home_club)) * club_size, away = c.u32(f.at(at::away_club)) * club_size;
        return {sext8(c.u8(clubs() + standing_at + home)), sext8(c.u8(clubs() + standing_at + away)),
                clubs() + home + club_name_at, clubs() + away + club_name_at};
    }

    // Length 0 or more: 0x100e7490 describes the bodies first (the original's r8 is 5 for a length of 1, else 6).
    void story_for_length(Compare compare) {
        const bool one = length == 1;
        c.set({{3, owner}, {4, f.at(at::bodies)}, {5, f.at(at::bodies_text)}, {6, 0x64}, {7, 0}, {8, one ? 5u : 6u}});
        call_module(callee::describe_bodies, one ? 0x100e5d18 : 0x100e5d38);
        const auto bodies = sext8(c.r(3));
        const auto names = f.at(at::names), bodies_text = f.at(at::bodies_text);
        const auto plain = [&](const Site (&row)[2]) {
            if (one) format_site(row[0], {bodies, bodies, names, bodies_text, count});
            else format_site(row[1], {bodies, bodies, names, length, bodies_text, count});
        };
        const auto with_clubs = [&](const Site (&row)[3][2]) {
            const auto k = clubs_now();
            const auto site = row[compare][one ? 0 : 1];
            if (one) format_site(site, {bodies, bodies, k.home_standing, k.home_standing, k.away_standing, k.away_standing, names,
                                        bodies_text, k.home_name, f.at(at::short_text), k.away_name});
            else format_site(site, {bodies, bodies, k.home_standing, k.home_standing, k.away_standing, k.away_standing, names,
                                    length, bodies_text, k.home_name, f.at(at::short_text), k.away_name});
        };
        switch (kind) {
            case 0: plain(sites::kind0); break;
            case 1:
                if (count == 1) with_clubs(sites::kind1);
                else plain(sites::kind1_without_count);
                break;
            case 4: plain(sites::kind4); break;
            case 2: with_clubs(sites::kind2); break;
            case 3: with_clubs(sites::kind3); break;
            default: break;  // another kind: no text, the story buffer is formatted as the frame left it
        }
    }

    // A negative length (a ban by date). The original reads r28 for the class here: 0x100e7490 is only called on this
    // path for a length of 1, which a negative length is not, so r28 still holds the caller's (dead branch 0x100e67f8).
    void story_for_date(Compare compare) {
        const auto bodies = sext8(entry_r28);
        const auto names = f.at(at::names), bodies_text = f.at(at::bodies_text);
        const auto plain = [&](Site site) { format_site(site, {0x64, bodies, bodies, names, bodies_text, count}); };
        const auto with_clubs = [&](const Site (&row)[3]) {
            const auto k = clubs_now();
            format_site(row[compare], {0x64, bodies, bodies, k.home_standing, k.home_standing, k.away_standing, k.away_standing,
                                       names, bodies_text, k.home_name, f.at(at::short_text), k.away_name});
        };
        switch (kind) {
            case 0: plain(sites::negative_kind0); break;
            case 1:
                if (count == 1) with_clubs(sites::negative_kind1);
                else plain(sites::negative_kind1_without_count);
                break;
            case 4: plain(sites::negative_kind4); break;
            case 2: with_clubs(sites::negative_kind2); break;
            case 3: with_clubs(sites::negative_kind3); break;
            default: break;
        }
    }
};
}

void render_ban_story(Cpu& c, std::uint32_t owner, std::uint32_t message, std::uint32_t body_flag, std::uint32_t text) {
    // The original's only caller, NEWS 0x102be040 (`bl` at 0x102c5604), holds the TOC cell address of the clubs table
    // (`lwz r28,-0x7be8(r2)` at 0x102be070) in r28, and the routine reads it for a ban length below 0 (the class
    // argument of those texts). NEWS is C++ now and does not keep the register, so the value is taken here.
    const auto r28 = c.toc(-0x7be8);
    Frame frame(c, 0x100e59b0, 0x810, {.first_gpr = 16});
    BanStory(c, frame, owner, message, body_flag, text, r28).run();
}
}
