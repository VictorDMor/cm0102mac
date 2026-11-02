// The media response dialog (0x1067bea0): the press has said something about the club, and the
// manager is asked how to answer. The headline depends on the kind of story.
#include "media_text_small.hpp"
#include "media_text_small_detail.hpp"
#include <algorithm>

namespace cm::aot::media {
using namespace detail;
using namespace detail::small;

namespace {
namespace routines {
constexpr std::uint32_t init_text = 0x1074b6e0, nation_club = 0x10713050,
    refresh = 0x102a1070, text_size = 0x100c7a50, staff_name = 0x100caeb0, set_title = 0x10744250, show_title = 0x107442f0,
    scale = 0x1076d470, open_pane = 0x107198c0, append_text = 0x1074bcd0, add_button = 0x107444e0, close_text = 0x1074b7c0;
}

// How a headline's arguments follow its text: nothing, the staff member's name, a leading 100 and the name, or a
// club's article and name with the clubs table (Nation, Continent; the Continent text without a staff member
// names the continent of the club's competition instead).
enum class Shape { Bare, Name, Marked, Staff, Nation, Continent };
// One headline, chosen by the jump table's target in the executable. `staff_*` is the text (and the call site that
// formats it) when the story names a staff member (an index above -1); `plain_*` the one without. Shapes that
// do not depend on a staff member use `staff_*` only.
struct Headline {
    std::uint32_t target;
    Shape shape;
    unsigned staff_phrase;
    std::uint32_t staff_back;
    unsigned plain_phrase;
    std::uint32_t plain_back;
};
constexpr Headline headlines[] = {
    {0x1067c0e0, Shape::Name, 0x1ed5f, 0x1067c0f4, 0, 0},
    {0x1067c0f8, Shape::Bare, 0x1edd9, 0x1067c108, 0, 0},
    {0x1067c10c, Shape::Staff, 0x1ee48, 0x1067c128, 0x1eeab, 0x1067c13c},
    {0x1067c140, Shape::Staff, 0x1eef7, 0x1067c15c, 0x1ef5b, 0x1067c170},
    {0x1067c174, Shape::Staff, 0x1efa8, 0x1067c190, 0x1f011, 0x1067c1a4},
    {0x1067c1a8, Shape::Staff, 0x1f063, 0x1067c1c4, 0x1f0db, 0x1067c1d8},
    {0x1067c1dc, Shape::Staff, 0x1f13c, 0x1067c1f8, 0x1f19d, 0x1067c20c},
    {0x1067c210, Shape::Nation, 0x1f1e7, 0x1067c264, 0x1f25e, 0x1067c2b0},
    {0x1067c2b4, Shape::Nation, 0x1f2be, 0x1067c308, 0x1f331, 0x1067c354},
    {0x1067c358, Shape::Continent, 0x1f38d, 0x1067c3ac, 0x1f40b, 0x1067c410},
    {0x1067c414, Shape::Bare, 0x1f472, 0x1067c424, 0, 0},
    {0x1067c428, Shape::Name, 0x1f4e8, 0x1067c43c, 0, 0},
    {0x1067c440, Shape::Bare, 0x1f571, 0x1067c450, 0, 0},
    {0x1067c454, Shape::Staff, 0x1f5cb, 0x1067c470, 0x1f642, 0x1067c484},
    {0x1067c488, Shape::Staff, 0x1f6a2, 0x1067c4a4, 0x1f715, 0x1067c4b8},
    {0x1067c4bc, Shape::Staff, 0x1f776, 0x1067c4d8, 0x1f7e1, 0x1067c4ec},
    {0x1067c4f0, Shape::Staff, 0x1f6a2, 0x1067c50c, 0x1f715, 0x1067c520},
    {0x1067c524, Shape::Marked, 0x1f83a, 0x1067c53c, 0, 0},
    {0x1067c540, Shape::Name, 0x1f8a8, 0x1067c554, 0, 0},
    {0x1067c558, Shape::Marked, 0x1f94c, 0x1067c570, 0, 0},
    {0x1067c574, Shape::Name, 0x1f9bd, 0x1067c588, 0, 0},
    {0x1067c58c, Shape::Marked, 0x1fa2d, 0x1067c5a4, 0, 0},
    {0x1067c5a8, Shape::Name, 0x1fab3, 0x1067c5bc, 0, 0},
    {0x1067c5c0, Shape::Name, 0x1fb29, 0x1067c5d4, 0, 0},
    {0x1067c5d8, Shape::Name, 0x1fb96, 0x1067c5ec, 0, 0},
    {0x1067c5f0, Shape::Name, 0x1fc20, 0x1067c604, 0, 0},
    {0x1067c608, Shape::Name, 0x1fca3, 0x1067c61c, 0, 0},
    {0x1067c620, Shape::Marked, 0x1fd1f, 0x1067c638, 0, 0},
    {0x1067c63c, Shape::Name, 0x1fda7, 0x1067c650, 0, 0},
    {0x1067c654, Shape::Marked, 0x1fe33, 0x1067c66c, 0, 0},
    {0x1067c670, Shape::Marked, 0x1feb8, 0x1067c688, 0, 0},
    {0x1067c68c, Shape::Marked, 0x1ff3e, 0x1067c6a4, 0, 0},
    {0x1067c6a8, Shape::Marked, 0x1ffa6, 0x1067c6c0, 0, 0},
    {0x1067c6c4, Shape::Marked, 0x2001e, 0x1067c6dc, 0, 0},
    {0x1067c6e0, Shape::Marked, 0x2008f, 0x1067c6f8, 0, 0},
    {0x1067c6fc, Shape::Marked, 0x20119, 0x1067c714, 0, 0},
    {0x1067c718, Shape::Marked, 0x2019a, 0x1067c730, 0, 0},
};
// The target for kinds beyond the table (a kind above 0x82).
constexpr std::uint32_t other_target = 0x1067c718;
constexpr std::uint32_t kinds_in_table = 0x83;
}

// 0x1067bea0: reads the story from the working record (0x1073ef70: value 1 the club, 2 the kind, 3 the
// answer to preselect, 4 a staff member, 5-7 his name key), opens the pane, writes the headline and adds
// the three answers and the buttons.
void media_response_dialog(Cpu& c) {
    Frame frame(c, 0x1067bea0, 0x170, {.first_gpr = 16});
    keep(c, {{25, c.toc(-0x7bd0)}, {27, c.toc(-0x7bc4)}, {24, c.toc(-0x7b94)}, {20, c.toc(-0x7bc8)}, {28, c.toc(slot::text)},
             {29, c.toc(-0x7bb0)}, {23, c.toc(slot::clubs)}, {30, c.toc(slot::pool)}});
    const auto text = c.r(28), world = c.r(29);
    const auto buffer = frame.at(0x58), name = frame.at(0x60), pane = frame.at(0x48), position = frame.at(0x54),
               window = frame.at(0x50), close_box = frame.at(0x4c);
    // The world record's current window holds the parent pane.
    // The four words at the bottom of the outgoing arguments (the pane's 11th to 14th).
    const auto extra_words = [&](std::uint32_t first, std::uint32_t second, std::uint32_t third, std::uint32_t fourth) {
        c.put32(frame.at(0x38), first);
        c.put32(frame.at(0x3c), second);
        c.put32(frame.at(0x40), third);
        c.put32(frame.at(0x44), fourth);
    };
    c.set({{3, buffer}, {4, 0}});
    c.call(routines::init_text, 0x1067bedc);
    c.put16(close_box, c.u16(c.r(2) - 0x3668));
    const auto pane_value = c.u16(c.r(2) - 0x3664);
    c.put16(pane, pane_value);
    c.set(7, pane_value);  // (stays in r7 until the first formatter call spills it)
    c.put32(position, 0);
    const auto defaults = c.toc(-0x4238);
    for (std::uint32_t at = 0; at < 200; at += 4) c.put32(name + at, c.u32(defaults + at));
    Kept<21> club_index{c, world_value(c, world, 1, 0x1067bf28)};
    Kept<26> kind{c, world_value(c, world, 2, 0x1067bf3c)};
    Kept<31> answer{c, world_value(c, world, 3, 0x1067bf50)};
    Kept<22> staff{c, world_value(c, world, 4, 0x1067bf64)};
    Kept<16> first{c, world_value(c, world, 5, 0x1067bf78)};
    Kept<17> second{c, world_value(c, world, 6, 0x1067bf8c)};
    const auto third = world_value(c, world, 7, 0x1067bfa0);
    Kept<18> club{c, club_record(c, club_index)};
    if (compare(c, staff, -1) > 0) person_name(c, first, second, third, name, 1, staff, 0x1067bfd4);
    c.set({{3, club}, {4, 0}, {5, 0}});
    c.call(routines::nation_club, 0x1067bfe4);
    c.set({{3, 4}, {4, 0}});
    c.call(routines::refresh, 0x1067bff0);
    c.set({{3, club}, {4, 1}, {5, window + 2}, {6, window}});
    c.call(routines::text_size, 0x1067c004);
    c.set({{3, c.u32(window_record(c, world) + 4)}, {4, text}, {5, 0x7d0}, {6, 1}});
    c.call(routines::staff_name, 0x1067c024);
    c.set({{3, world}, {4, c.u16(window)}, {5, c.u16(window + 2)}, {6, text}});
    c.call(routines::set_title, 0x1067c038);
    format(c, text, 0x1ed49, 0x1067c048);  // the dialog title
    c.set({{3, world}, {4, text}});
    c.call(routines::show_title, 0x1067c054);
    Kept<18> top{c, 0x8c};
    Kept<19> bottom{c, 0x221};
    c.set(3, 3);
    c.call(routines::scale, 0x1067c064);
    // (The division comes first: a zero scale faults before the outgoing words are stored.)
    Kept<17> step{c, sign8(divide_signed<4>(bottom - top, c.r(3)))};
    extra_words(0, 2, 0, 0xffffffffu);
    c.set({{3, c.u32(window_record(c, world) + 0x24)}, {4, 0x6e}, {5, top}, {6, 0x30c}, {7, bottom}, {8, 7}, {9, pane}, {10, step & 0xff}});
    c.call(routines::open_pane, 0x1067c0c0);
    Kept<16> container{c, c.r(3)};

    // The headline: the jump table in the executable sends each kind to its text.
    std::uint32_t target = other_target;
    if (compare_logical(c, kind, kinds_in_table - 1) <= 0) target = c.u32(c.toc(-0x423c) + kind * 4);
    const auto headline = std::ranges::find(headlines, target, &Headline::target);
    if (headline == std::end(headlines)) throw std::logic_error("media_response_dialog: jump table target outside the routine");
    const Say say{c, text};
    const auto clubs = c.u32(c.r(23));
    const auto record = clubs + club_index * club_size;
    const bool named = compare(c, staff, -1) > 0;
    switch (headline->shape) {
    case Shape::Bare: say(headline->staff_phrase, headline->staff_back, {}); break;
    case Shape::Name: say(headline->staff_phrase, headline->staff_back, {name}); break;
    case Shape::Marked: say(headline->staff_phrase, headline->staff_back, {0x64, name}); break;
    case Shape::Staff:
        if (named) say(headline->staff_phrase, headline->staff_back, {name});
        else say(headline->plain_phrase, headline->plain_back, {});
        break;
    case Shape::Nation: case Shape::Continent: {
        const auto article = sign8(c.u8(record + club_article));
        if (named) say(headline->staff_phrase, headline->staff_back, {article, article, name, record + club_name, clubs});
        else if (headline->shape == Shape::Nation)
            say(headline->plain_phrase, headline->plain_back, {article, article, record + club_name, clubs});
        else {
            const auto continent = c.u32(c.u32(record + 0x2c) + 0x74);
            const auto continent_article = sign8(c.u8(continent + 0xc3));
            say(headline->plain_phrase, headline->plain_back, {continent_article, continent_article, continent + 0x10, continent});
        }
        break;
    }
    }

    copy_text(c, buffer, text, 0x1067c73c);
    c.put32(position, 0);
    c.put32(frame.at(0x38), 0);
    c.set({{3, buffer}, {4, container}, {5, 0x6e}, {6, 0x30c}, {7, position}, {8, 3}, {9, c.u16(c.r(20))}, {10, 0}});
    c.call(routines::append_text, 0x1067c770);
    c.put32(position, c.u32(position) + 2);
    extra_words(0, 1, 0, 0xffffffffu);
    c.set({{3, c.u32(window_record(c, world) + 0x24)}, {4, 0x6e}, {5, top}, {6, 0x30c}, {7, bottom}, {8, 2}, {9, pane}, {10, step & 0xff}});
    c.call(routines::open_pane, 0x1067c7cc);
    step = 1;
    container = c.r(3);
    for (; std::int8_t(step) <= 3; step = step + 1) {
        // The manager's line for this answer; the answer chosen last time is highlighted.
        enter(c, 0x1067c7f0, {c.u32(c.r(24)), std::uint32_t(sign16(kind)), step, text, 0x7d0});
        manager_reply_text(c, c.r(3), std::int32_t(sign16(kind)), std::int32_t(step), text, 0x7d0);
        const bool chosen = compare(c, std::int32_t(answer), std::int8_t(step)) == 0;
        c.put32(frame.at(0x38), c.u16(chosen ? c.r(25) : c.r(27)));
        c.put32(frame.at(0x3c), text);
        c.put32(frame.at(0x40), 0x1485);
        c.put32(frame.at(0x44), sign8(step));
        c.set({{3, world}, {4, container}, {5, 1}, {6, c.u32(position)}, {7, 1}, {8, 0}, {9, 1}, {10, 3}});
        c.call(routines::add_button, chosen ? 0x1067c880 : 0x1067c83c);
        if (compare(c, std::int8_t(step), 1) == 0) {
            c.set({{3, world}, {4, 8}, {5, sign16(c.r(3))}, {6, 0}});
            c.call(library::set_field, 0x1067c8a4);
        }
        c.put32(position, c.u32(position) + 1);
    }

    extra_words(0, 1, 0, 0xffffffffu);
    c.set({{3, c.u32(window_record(c, world) + 0x24)}, {4, 0x64}, {5, 0x22b}, {6, 0x316}, {7, 0x24e}, {8, 2}, {9, close_box}, {10, 1}});
    c.call(routines::open_pane, 0x1067c910);
    container = c.r(3);
    format(c, text, 0x201ff, 0x1067c928);  // the cancel button
    const auto button = [&](std::uint32_t id, std::uint32_t default_button, std::uint32_t back) {
        c.put32(frame.at(0x38), c.u16(c.r(20)));
        c.put32(frame.at(0x3c), text);
        c.put32(frame.at(0x40), id);
        c.put32(frame.at(0x44), 0);
        c.set({{3, world}, {4, container}, {5, default_button}, {6, 0}, {7, 0x30}, {8, c.u16(c.r(27))}, {9, 0xc}, {10, 3}});
        c.call(routines::add_button, back);
    };
    button(0x109c, 0, 0x1067c968);
    format(c, text, 0x20206, 0x1067c978);  // "Ok"
    button(0x109b, 1, compare(c, answer) == 0 ? 0x1067c9c0 : 0x1067ca04);
    c.set({{3, buffer}, {4, 0xffffffffu}});
    c.call(routines::close_text, 0x1067ca10);
}
}
