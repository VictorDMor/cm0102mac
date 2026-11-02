// A player's outburst reported by the press (0x10656450): which outlet carries it, what the player
// said, and, in the long form, how his manager and an assistant answered.
#include "media_text_small.hpp"
#include "media_text_small_detail.hpp"

namespace cm::aot::media {
using namespace detail;
using namespace detail::small;

namespace {
namespace routines {
constexpr std::uint32_t today = 0x10715880, read_date = 0x1030dee0, nationality = 0x10168a00, outlet_name = 0x106a9f10,
    outlet_is_magazine_class = 0x10727880, sprintf = 0x1077b720, outlet_owner = 0x100c76c0, match_link = 0x1029eb20;
}
// The story's kinds: 0x52 is the player blasting a lack of service.
constexpr std::int32_t blast_story = 0x52;

// The outlets: the kind the item carries (1-6, anything else a website), the short name used for the
// manager's own country, the long name with a nationality, and the nationality's index.
struct Outlet {
    std::int32_t kind;
    unsigned short_phrase;
    std::uint32_t short_back;
    unsigned long_phrase;
    std::uint32_t long_back;
    std::uint32_t nationality_back;
    std::uint32_t nationality;
};
constexpr Outlet outlets[] = {
    {1, 0xcd1, 0x10656868, 0xce4, 0x10656894, 0x10656880, 0x16},
    {2, 0xd19, 0x106568b4, 0xd29, 0x106568e0, 0x106568cc, 0x17},
    {3, 0xd5b, 0x10656900, 0xd72, 0x1065692c, 0x10656918, 0x18},
    {4, 0xda2, 0x1065694c, 0xdb6, 0x10656978, 0x10656964, 0x18},
    {5, 0xdec, 0x10656998, 0xdf5, 0x106569c4, 0x106569b0, 0x19},
    {6, 0xe20, 0x106569fc, 0xe2d, 0x10656a28, 0x10656a14, 0x1a},
};
constexpr Outlet website{0, 0xe5c, 0x10656a48, 0xe64, 0x10656a74, 0x10656a60, 0x1a};

// The story's working state: the item's values (kept in the registers the original keeps them in)
// and the buffers on the stack frame.
struct Story {
    Cpu& c;
    const Frame& frame;
    std::uint32_t item, output;
    // Stack locals.
    std::uint32_t counter() const { return frame.at(0x41); }
    std::uint32_t text() const { return frame.at(0x1dac); }
    std::uint32_t name() const { return frame.at(0xec); }        // the player, full form
    std::uint32_t partner() const { return frame.at(0x74); }     // the second name of the story
    std::uint32_t outlet_text() const { return frame.at(0x164); }
    std::uint32_t nationality_text() const { return frame.at(0x1dc); }
    std::uint32_t reply() const { return frame.at(0x254); }
    std::uint32_t assistant_text() const { return frame.at(0x448); }
    std::uint32_t both() const { return frame.at(0x830); }
    std::uint32_t score() const { return frame.at(0x44); }
    std::uint32_t date() const { return frame.at(0x50); }
    std::uint32_t kind() const { return c.u32(frame.at(0x313c)); }
    std::uint32_t club_at(std::uint32_t index) const { return club_record(c, index); }

    // The next value of the item: its index is a byte on the stack that counts up as the original reads.
    std::uint32_t next(std::uint32_t back, bool skip = false) const {
        if (skip) c.put8(counter(), c.u8(counter()) + 1);
        const auto index = c.u8(counter());
        c.put8(counter(), index + 1);
        return item_value(c, item, index, back);
    }
    void say(unsigned phrase, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) const {
        format(c, text(), phrase, back, arguments);
    }
};
}

// 0x10656450: the long form (`full`) tells the story with the manager's and the assistant's answers.
void player_outburst_story(Cpu& c, std::uint32_t, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x10656450, 0x31a0, {.first_gpr = 13});
    keep(c, {{30, c.toc(slot::clubs)}, {14, item}, {31, c.toc(slot::pool)}, {15, output}});
    c.put8(frame.at(0x41), 0);
    c.put8(frame.at(0x31c3), full);
    Story s{c, frame, item, output};
    c.set(3, s.date());
    c.call(routines::today, 0x10656484);
    Kept<29> player{c, s.next(0x10656498)};
    Kept<28> first{c, s.next(0x106564b0)};
    Kept<27> second{c, s.next(0x106564c8)};
    Kept<26> third{c, s.next(0x106564e0)};
    Kept<25> club_index{c, s.next(0x106564f8)};
    const auto sixth = s.next(0x10656510);
    c.set(5, sixth);
    c.put8(frame.at(0x3140), sign8(sixth));
    c.put32(frame.at(0x313c), s.next(0x10656530));
    Kept<24> tone{c, s.next(0x10656548)};
    Kept<23> assisted{c, s.next(0x10656560)};
    Kept<22> reaction{c, s.next(0x10656584, true)};
    Kept<21> assistant{c, s.next(0x1065659c)};
    c.put32(frame.at(0x3138), s.next(0x106565b4));
    c.put32(frame.at(0x3134), s.next(0x106565cc));
    Kept<13> assistant_third{c, s.next(0x106565e4)};
    Kept<20> match{c, sign8(s.next(0x106565fc))};
    if (compare(c, std::int8_t(match)) != 0) {
        c.set({{3, item}, {4, frame.at(0x41)}, {5, s.date()}});
        c.call(routines::read_date, 0x10656614);
        s.next(0x10656628);
    }
    Kept<19> outlet_kind{c, s.next(0x1065663c)};
    Kept<14> outlet_id{c, s.next(0x10656654)};
    Kept<18> club_fan{c, 0};
    const auto club = s.club_at(club_index);
    Kept<16> owner{c, c.u32(club + 0x2c)};
    const auto world = c.toc(-0x7bb0);
    const auto window = window_record(c, world);
    Kept<17> window_owner{c, c.u32(window + 4)};
    c.set({{5, c.u32(c.r(30))}, {6, window_index(c, world) * window_size}});
    if (compare(c, std::int32_t(s.kind()), blast_story) == 0) {
        // Sometimes a blast is read as the owner of the player's club speaking.
        const auto rng = Stream::world(c);
        // (The compiler left the cursor's address and the stepped cursor in r3 and r4 for the refill.)
        rng.advance(0x106566c4, {{3, rng.cursor}, {4, c.u32(rng.cursor) + 2}});
        if (compare(c, std::int32_t(signed_mod2(rng.sample()))) != 0) {
            const auto row = person_row(c, player);
            if (compare(c, std::int32_t(c.i16(c.u32(row + 0xc) + 0xac)), 0x1388) > 0) owner = c.u32(row + 0xc);
        }
    }
    // Whether the manager's own club's country is the one named (the media are local to him).
    bool local = false;
    if (window_owner != 0) {
        const auto pane = c.u32(window_owner + 0x24);
        if (pane != 0) local = c.u32(pane + 0x2c) == owner;
        else local = owner == c.u32(window_owner + 0xc);
    }
    if (c.u32(club + 0x2c) != 0 && compare(c, outlet_kind, -1) != 0 && compare(c, outlet_id, -1) != 0 && compare(c, std::int8_t(local)) != 0) {
        c.set({{3, c.u32(c.toc(-0x7570))}, {4, sign8(outlet_kind)}, {5, sign16(outlet_id)}, {6, s.outlet_text()}, {7, frame.at(0x40)}});
        c.call(routines::outlet_name, 0x1065681c);
    } else {
        // The kind of outlet and its name; with a nationality unless it is the manager's own.
        const Outlet* outlet = &website;
        for (const auto& row : outlets) if (row.kind == std::int32_t(outlet_kind)) outlet = &row;
        if (local) {
            format(c, s.outlet_text(), outlet->short_phrase, outlet->short_back);
        } else {
            c.set({{3, owner}, {4, s.nationality_text()}, {5, 0x78}, {6, outlet->nationality}});
            c.call(routines::nationality, outlet->nationality_back);
            format(c, s.outlet_text(), outlet->long_phrase, outlet->long_back, {0x64, s.nationality_text()});
        }
        if (outlet->kind == 5) {
            // A magazine is "an" when its class (0x10727880) is 8.
            c.call(routines::outlet_is_magazine_class, 0x106569c8);
            c.put8(frame.at(0x40), std::uint32_t(sign8(c.r(3)) == 8));
        } else {
            c.put8(frame.at(0x40), 0);
        }
    }
    const auto fan_club = c.u32(club + 0x90);
    if (fan_club != 0) club_fan = fan_club;
    const auto player_row = person_row(c, player);
    const auto link_player = [&](std::uint32_t span, std::uint32_t back) { link_text(c, output, span, 0x7d7, player_row, back); };
    const auto link_club = [&](std::uint32_t back) {
        c.set(7, c.u32(c.r(30)));
        link_text(c, output, club + club_name, 0x7d5, club, back);
    };
    if (compare(c, std::int8_t(c.u8(frame.at(0x31c3)))) == 0) {
        // The short form: the player's remark alone.
        person_name(c, first, second, third, s.name(), 5, player, 0x10656ad0, 0x78);
        if (compare(c, std::int32_t(s.kind()), blast_story) == 0) s.say(0xe13a, 0x10656af0, {s.name()});
        else if (compare(c, assisted) != 0 && compare(c, assistant, -1) > 0) s.say(0xe172, 0x10656b14, {});
        else if (compare(c, tone) != 0 && compare(c, std::int8_t(c.u8(frame.at(0x3140)))) == 0) s.say(0xe192, 0x10656b44, {0x64, s.name()});
        else s.say(0xe1c4, 0x10656b5c, {s.name()});
        copy_text(c, output, s.text(), 0x10656b68);
        if (compare(c, assisted) != 0 && compare_logical(c, window_owner, club_fan) == 0) return;
        if (person_listed(c, player_row, first, second, third, 0x10656b9c) == 0) return;
        link_player(s.name(), 0x10656bc4);
        link_club(0x10656bec);
        return;
    }
    // The long form.
    person_name(c, first, second, third, s.name(), 1, player, 0x10656c10, 0x78);
    person_name(c, first, second, third, s.partner(), 1, player, 0x10656c30, 0x78);
    if (compare(c, std::int8_t(match)) != 0) {
        // The score, higher goals first.
        const auto home = c.u8(frame.at(0x6c)), away = c.u8(frame.at(0x6d));
        std::uint32_t low = home, high = away;
        if (compare(c, std::int8_t(home), std::int8_t(away)) < 0) low = home; else low = away;
        if (compare(c, std::int8_t(home), std::int8_t(away)) > 0) high = home; else high = away;
        c.set({{3, s.score()}, {4, c.toc(slot::pool) + 0x1cc7}, {5, sign8(high)}, {6, sign8(low)}});
        c.call(routines::sprintf, 0x10656c94);
    }
    const auto rival_club = [&] { return s.club_at(c.u32(frame.at(0x68))); };
    const auto s8_rival = [&] { return sign8(c.u8(rival_club() + club_short_article)); };
    const auto s8_club = sign8(c.u8(club + club_article));
    const auto article = sign8(c.u8(frame.at(0x40)));
    if (compare(c, std::int32_t(s.kind()), blast_story) == 0) {
        if (compare(c, tone) == 0) s.say(0xe1f2, 0x10656d10, {s8_club, s8_club, article, article, club + club_name, s.name(), s.outlet_text()});
        else s.say(0xe2e2, 0x10656d80, {article, article, s8_club, s8_club, s.outlet_text(), club + club_name, s.name(), s.partner()});
    } else if (compare(c, assisted) != 0 && compare(c, assistant, -1) > 0) {
        // An assistant answers: his own words follow the score, the tone and the incident's detail.
        person_name(c, c.u32(frame.at(0x3138)), c.u32(frame.at(0x3134)), assistant_third, s.assistant_text(), 1, assistant, 0x10656db4);
        const bool captain = compare(c, std::int8_t(c.u8(frame.at(0x3140)))) != 0;
        struct Wording { unsigned phrase; std::uint32_t back; };
        // Rows: [captain?][tone != 0?] -> draw, home win, home loss (and without a match report).
        static constexpr Wording words[2][2][3] = {
            {{{0xebe2, 0x10657158}, {0xecbc, 0x106571d4}, {0xed95, 0x10657238}}, {{0xef13, 0x106572d8}, {0xefbd, 0x10657354}, {0xf066, 0x106573b8}}},
            {{{0xe451, 0x10656e4c}, {0xe569, 0x10656ec8}, {0xe680, 0x10656f2c}}, {{0xe87d, 0x10656fcc}, {0xe967, 0x10657048}, {0xea50, 0x106570ac}}}};
        static constexpr Wording without_match[2][2] = {{{0xee6f, 0x10657254}, {0xf113, 0x106573d4}}, {{0xe79b, 0x10656f48}, {0xeb3a, 0x106570c8}}};
        const bool upset = compare(c, tone) != 0;
        if (compare(c, std::int8_t(match)) == 0) {
            const auto& w = without_match[captain][upset];
            if (upset) s.say(w.phrase, w.back, {0x64, s.name()});
            else s.say(w.phrase, w.back, {s.assistant_text(), s.name()});
        } else {
            const auto home = std::int8_t(c.u8(frame.at(0x6c))), away = std::int8_t(c.u8(frame.at(0x6d)));
            unsigned outcome = 0;
            if (compare(c, home, away) != 0) outcome = compare(c, home, away) > 0 ? 1 : 2;
            const auto& w = words[captain][upset][outcome];
            if (upset) s.say(w.phrase, w.back, {0x64, s8_rival(), s8_rival(), s.name(), s.score(), rival_club() + club_short_name});
            else s.say(w.phrase, w.back, {s8_rival(), s8_rival(), s.assistant_text(), s.name(), s.score(), rival_club() + club_short_name});
        }
    } else {
        const bool captain = compare(c, std::int8_t(c.u8(frame.at(0x3140)))) != 0;
        struct Wording { unsigned phrase; std::uint32_t back; };
        static constexpr Wording own[2][2] = {{{0xf34f, 0x10657530}, {0xf42a, 0x10657598}}, {{0xf187, 0x10657454}, {0xf26c, 0x106574bc}}};
        const auto& w = own[captain][compare(c, tone) != 0];
        if (compare(c, tone) == 0) s.say(w.phrase, w.back, {s8_club, s8_club, article, article, club + club_name, s.name(), s.outlet_text()});
        else s.say(w.phrase, w.back, {article, article, s8_club, s8_club, s.outlet_text(), club + club_name, s.name()});
    }
    // How the player's own manager answered, depending on whether the story is that of his club's fan.
    const bool agrees = compare_logical(c, window_owner, club_fan) == 0;
    const auto empty = [&](std::uint32_t back) {
        c.set({{3, s.reply()}, {4, c.toc(slot::pool) + 0x9f58}});
        c.call(routines::sprintf, back);
    };
    // The reaction (value 10) picks the sentence: 2 and 1 have one each, any other value none.
    if (agrees) {
        if (std::int32_t(reaction) == 2) format(c, s.reply(), 0xf548, 0x106575e4);
        else if (std::int32_t(reaction) == 1) format(c, s.reply(), 0xf506, 0x106575d0);
        else empty(0x106575f8);
    } else {
        if (std::int32_t(reaction) == 2) format(c, s.reply(), 0xf5dd, 0x10657640);
        else if (std::int32_t(reaction) == 1) format(c, s.reply(), 0xf5a1, 0x1065762c);
        else empty(0x10657654);
    }
    c.set({{3, s.both()}, {4, c.toc(slot::pool) + 0xc024}, {5, s.text()}, {6, s.reply()}});
    c.call(routines::sprintf, 0x1065766c);
    copy_text(c, output, s.both(), 0x10657678);
    if (person_listed(c, player_row, first, second, third, 0x1065769c) != 0) {
        link_player(s.name(), 0x106576c4);
        link_player(s.partner(), 0x106576e4);
    }
    if (compare(c, assistant, -1) > 0) {
        const auto assistant_row = person_row(c, assistant);
        if (person_listed(c, assistant_row, c.u32(frame.at(0x3138)), c.u32(frame.at(0x3134)), assistant_third, 0x10657710) != 0)
            link_text(c, output, s.assistant_text(), 0x7d7, assistant_row, 0x10657738);
    }
    link_club(0x10657760);
    // The outlets that carry the story, by nationality: the country, its adjective and the outlet names.
    const auto owner_link = [&] { return c.u32(club + 0x2c); };
    c.set(3, owner_link());
    c.call(routines::outlet_owner, 0x10657774);
    link_text(c, output, owner_link() + 0xae, 0x7d5, c.r(3), 0x10657798);
    c.set(3, owner_link());
    c.call(routines::outlet_owner, 0x106577ac);
    link_text(c, output, owner_link() + 0xe1, 0x7d5, c.r(3), 0x106577d0);
    for (std::uint32_t index = 0; index < 10; ++index) {
        const auto back = 0x106577f0 + 0x48 * index;
        c.set({{3, owner_link()}, {4, s.nationality_text()}, {5, 0x78}, {6, 0x16 + index}});
        c.call(routines::nationality, back);
        c.set(3, owner_link());
        c.call(routines::outlet_owner, back + 0x14);
        link_text(c, output, s.nationality_text(), 0x7d5, c.r(3), back + 0x28);
    }
    if (compare(c, std::int8_t(match)) != 0) {
        c.set({{3, c.u32(c.toc(-0x7b4c))}, {4, c.u32(frame.at(0x70))}, {5, c.u32(frame.at(0x64))}, {6, c.u32(frame.at(0x68))},
               {7, c.u16(frame.at(0x50))}, {8, c.u16(frame.at(0x52))}});
        c.call(routines::match_link, 0x10657acc);
        if (compare(c, std::int8_t(c.r(3))) != 0)
            link_text(c, output, s.score(), 0x7d3, c.u32(frame.at(0x70)), 0x10657ae8);
        const auto rival = rival_club();
        c.set(7, c.u32(c.r(30)));
        link_text(c, output, rival + club_short_name, 0x7d5, s.club_at(c.u32(frame.at(0x68))), 0x10657b18);
    }
}
}
