// The two forms of the story: the long one (an outlet's column and the manager's reply) and the short
// one (a headline), with how each reads its item and ends with the links.
#include "media_text_a_detail.hpp"

namespace cm::aot::media::detail::column {
namespace {
std::uint32_t signed_half(std::uint32_t value) { return extended(std::int16_t(value)); }
}  // namespace

void Story::long_form() {
    text::RecordReader read{.c = c, .record = item, .back = 0x1064cfc8};
    const auto put = [&](unsigned offset, std::uint32_t value) { c.put32(at(offset), value); };
    const auto kept = [&](unsigned reg, std::int32_t& field,
            std::uint32_t value) { field = std::int32_t(value); c.set(reg, value); };
    kept(27, player, read(0));
    put(0x3214, read(1)); put(0x3210, read(2)); put(0x320c, read(3));
    kept(26, club, read(4));
    kept(25, tone, read(5));
    kept(14, medium, read(6));
    kept(24, person, read(7));
    put(0x3208, read(8));
    kept(23, kind, read(9));
    put(0x3204, read(0xa)); put(0x3200, read(0xb));
    put(0x31fc, read(0xc)); put(0x31f8, read(0xd)); put(0x31f4, read(0xe));
    put(0x31f0, read(0xe));  // the item's field 14 is read twice
    kept(22, answer, read(0x10));
    kept(21, level, read(0x11));
    kept(20, traits, read(0x12));
    kept(19, manager, read(0x17));
    put(0x31ec, read(0x18)); put(0x31e8, read(0x19)); put(0x31e4, read(0x1a));
    kept(18, rival, read(0x1b));
    const auto match = read(0x1c);
    compare(c, std::int8_t(match));
    c.put8(at(0x31e0), match);
    if (std::int8_t(match) != 0) {
        // A story about a match: its fields follow a position the decoder of the match record moves along.
        c.put8(at(0x60), 0x1d);
        c.set({{3, item}, {4, at(0x60)}, {5, at(0x70)}});
        c.call(entry::decode_match, 0x1064d1cc);
        const auto next = [&](std::uint32_t back) {
            const auto cursor = c.u8(at(0x60));
            c.put8(at(0x60), cursor + 1);
            return text::RecordReader{.c = c, .record = item}.field(cursor, back);
        };
        kept(17, result, next(0x1064d1e0));
        put(0x31dc, next(0x1064d1f8));
        const auto home = next(0x1064d210);
        c.set(5, home);  // left in r5 by the compiler for the next read
        c.put16(at(0x31da), home);
        c.put16(at(0x31d8), next(0x1064d230));
        kept(13, third, 0xffffffffu);
        put(0x31d4, 0); c.put8(at(0x31d1), 0); c.put8(at(0x31d0), 0);
        put(0x31cc, 0xffffffffu); put(0x31c8, 0xffffffffu); put(0x31c4, 0xffffffffu);
    } else {
        text::RecordReader more{.c = c, .record = item, .back = 0x1064d27c};
        put(0x31dc, more(0x1d));
        c.put16(at(0x31da), more(0x1e, 0x18));
        c.put16(at(0x31d8), more(0x1f, 0x18));
        put(0x31d4, more(0x20));
        c.put8(at(0x31d1), more(0x21, 0x18));
        c.put8(at(0x31d0), more(0x22, 0x18));
        kept(13, third, more(0x23, 0x14));
        put(0x31cc, more(0x24)); put(0x31c8, more(0x25)); put(0x31c4, more(0x26));
    }

    c.set({{3, c.u32(at(0x31d4))}, {4, at(0x4d8)}, {5, 0}});
    c.call(entry::age_text, 0x1064d354);
    c.put8(at(0x3218), c.r(3));
    const auto rows = c.u32(c.toc(rows_slot));
    const auto player_row = rows + std::uint32_t(player) * row_size,
            manager_row = rows + std::uint32_t(manager) * row_size;
    status = player_row;
    c.set(28, status);
    c.set(16, manager_row);
    const auto keys = [&](unsigned a, unsigned b, unsigned d) { return std::array<std::uint32_t,
            3>{c.u32(at(a)), c.u32(at(b)), c.u32(at(d))}; };
    const auto name = [&](std::array<std::uint32_t, 3> k, unsigned buffer, std::uint32_t mode, std::uint32_t index,
            std::uint32_t back) {
        person_name(c, k[0], k[1], k[2], at(buffer), mode, index, back, 0x78);
    };
    name(keys(0x3214, 0x3210, 0x320c), 0x398, 1, std::uint32_t(player), 0x1064d394);
    name(keys(0x3214, 0x3210, 0x320c), 0x320, 5, std::uint32_t(player), 0x1064d3b4);
    name(keys(0x31ec, 0x31e8, 0x31e4), 0x1b8, 1, std::uint32_t(manager), 0x1064d3d4);
    name(keys(0x31ec, 0x31e8, 0x31e4), 0x140, 5, std::uint32_t(manager), 0x1064d3f4);
    // The player's position text; a player who is also the manager has none.
    c.set({{3, signed_half(c.u16(at(0x31da)))}, {4, at(0x410)}, {5, 0xc8}, {6, 1}});
    c.call(entry::match_text, 0x1064d408);
    compare_logical(c, manager_row, player_row);
    status = c.r(3);
    c.set(28, status);
    if (manager_row == player_row) {
        format(c, at(0x410), 0xcc2, 0x1064d420);
        status = 0;
        c.set(28, 0);
    }
    if (compare(c, second_person(), -1) != 0) name(keys(0x31fc, 0x31f8, 0x31f4), 0x230, 1, std::uint32_t(player),
            0x1064d450);
    else c.put8(at(0x230), 0);
    if (compare(c, third, -1) != 0) name(keys(0x31cc, 0x31c8, 0x31c4), 0xc8, 1, std::uint32_t(third), 0x1064d490);
    else c.put8(at(0xc8), 0);

    // Whether the story is told in the user's own country: the nation of the user's club (or of his
    // manager's record, when he has no club) is the nation of the story's club.
    const auto base = c.toc(user_slot);
    const auto user_row = base + std::uint32_t(std::int32_t(std::int16_t(c.u16(base + 0x2c58)))) * 0x2c0;
    domestic = 0;
    c.set(16, 0);
    const auto user_manager = c.u32(user_row + 4);
    if (compare_logical(c, user_manager) != 0) {
        const auto user_club = c.u32(user_manager + 0x24);
        const auto club_nation = c.u32(club_of(club) + 0x2c);
        if (compare_logical(c, user_club) != 0) {
            if (compare_logical(c, c.u32(user_club + 0x2c), club_nation) == 0) domestic = 1;
        } else if (compare_logical(c, club_nation, c.u32(user_manager + 0xc)) == 0) domestic = 1;
    }
    c.set(16, std::uint32_t(domestic));

    outlet();
    write_column_by_kind();
    reply();
    finish_long();
}

// The outlet: a national or local paper or radio station, a magazine, a sports paper or a website,
// named for the club's nation unless the story is told at home.
void Story::outlet() {
    if (compare(c, club, -1) != 0 && compare(c, medium, -1) != 0 && compare(c, half_31d8(), -1) != 0 &&
        compare_logical(c, c.u32(club_of(club) + 0x2c)) != 0 && compare(c, std::int8_t(domestic)) != 0) {
        // The outlet the user's own side names for this medium and team.
        text::outlet_from_region(c, std::uint32_t(medium), half_31d8(), at(0x94), at(0x61), 0x1064d5a8);
        return;
    }
    text::write_outlet(c, {0x1064d5a8, at(0x94), at(0x2a8), at(0x61)}, medium, std::uint32_t(domestic), club_of(club));
}

// The manager's reply: for the user's own player what he told the press or admitted, otherwise what
// another manager did; the item's variant says which (1 told, 2 admitted, 3 ignored, else nothing).
void Story::reply() {
    const auto blank = [&](std::uint32_t back) {
        c.set({{3, at(0x6cc)}, {4, c.toc(slot::pool) + 0x9f58}});
        c.call(entry::sprintf, back);
    };
    const auto rows = c.u32(c.toc(rows_slot));
    const auto base = c.toc(user_slot);
    const auto user_row = base + std::uint32_t(std::int32_t(std::int16_t(c.u16(base + 0x2c58)))) * 0x2c0;
    const bool users = c.u32(user_row + 4) == rows + std::uint32_t(manager) * row_size;
    compare(c, std::int8_t(users));
    if (users) {
        if (const auto r = cmp(variant(), 2); r == 0) return user_admitted();
        else if (r > 0) {
            if (cmp(variant(), 4) >= 0) return blank(0x10652f38);
            return say(L(0xa7c4, 0x10652f24).to(reply_text));  // the reply for a report the user ignores
        }
        if (const auto r0 = cmp(variant(), 0); r0 == 0) return blank(0x10652f38);
        else if (r0 > 0) return user_told();
        return blank(0x10652f38);
    }
    if (const auto r = cmp(variant(), 2); r == 0) return other_said();
    else if (r > 0) return blank(0x10653c94);
    if (const auto r0 = cmp(variant(), 0); r0 == 0) return blank(0x10653c94);
    else if (r0 > 0) return other_told();
    blank(0x10653c94);
}

// The two texts joined and put in the output, then every name in them made a link.
void Story::finish_long() {
    c.set({{3, at(0x8c0)}, {4, c.toc(slot::pool) + 0xc024}, {5, at(0x1e3c)}, {6, at(0x6cc)}});
    c.call(entry::sprintf, 0x10653cac);
    copy_text(c, output, at(0x8c0), 0x10653cb8);
    const auto rows = c.u32(c.toc(rows_slot));
    const auto person_row_of = [&](std::int32_t index) { return rows + std::uint32_t(index) * row_size; };
    if (compare(c, second_person(), -1) > 0 &&
        person_listed(c, person_row_of(second_person()), c.u32(at(0x31fc)), c.u32(at(0x31f8)), c.u32(at(0x31f4)),
                0x10653ce8) != 0)
        link(at(0x230), 0x7d7, person_row_of(second_person()), 0x10653d10);
    if (compare(c, player, -1) != 0 &&
        person_listed(c, person_row_of(player), c.u32(at(0x3214)), c.u32(at(0x3210)), c.u32(at(0x320c)),
                0x10653d38) != 0) {
        link(at(0x398), 0x7d7, person_row_of(player), 0x10653d5c);
        link(at(0x320), 0x7d7, person_row_of(player), 0x10653d78);
    }
    if (compare(c, third, -1) != 0 &&
        person_listed(c, person_row_of(third), c.u32(at(0x31cc)), c.u32(at(0x31c8)), c.u32(at(0x31c4)),
                0x10653da0) != 0)
        link(at(0xc8), 0x7d7, person_row_of(third), 0x10653dc4);
    if (compare(c, manager, -1) != 0 &&
        person_listed(c, person_row_of(manager), c.u32(at(0x31ec)), c.u32(at(0x31e8)), c.u32(at(0x31e4)),
                0x10653dec) != 0) {
        link(at(0x140), 0x7d7, person_row_of(manager), 0x10653e10);
        link(at(0x1b8), 0x7d7, person_row_of(manager), 0x10653e2c);
    }
    if (compare(c, club, -1) != 0) {
        // The club's nation: its name and, written out in each adjective form, the links of the text.
        const auto nation_of = [&] { return c.u32(club_of(club) + 0x2c); };
        const auto name_of_nation = [&](std::uint32_t text_offset, std::uint32_t named_back, std::uint32_t link_back) {
            c.set(3, nation_of());
            c.call(entry::nation_text, named_back);
            link(nation_of() + text_offset, 0x7d5, c.r(3), link_back);
        };
        name_of_nation(0xae, 0x10653e48, 0x10653e6c);
        name_of_nation(0xe1, 0x10653e80, 0x10653ea4);
        for (std::uint32_t form = 0; form < 10; ++form) {
            c.set({{3, nation_of()}, {4, at(0x2a8)}, {5, 0x78}, {6, 0x16 + form}});
            c.call(entry::adjective, 0x10653ec4 + 0x48 * form);
            c.set(3, nation_of());
            c.call(entry::nation_text, 0x10653ed8 + 0x48 * form);
            link(at(0x2a8), 0x7d5, c.r(3), 0x10653eec + 0x48 * form);
        }
        link(club_of(club) + club_name_offset, 0x7d5, club_of(club), 0x1065419c, true);
        link(club_of(club) + club_short_name_offset, 0x7d5, club_of(club), 0x106541c4, true);
    }
    if (compare(c, has_match()) != 0) {
        // A match story links the fixture (when the calendar knows it) and the other club.
        c.set({{3, c.u32(c.toc(calendar_slot))}, {4, c.u32(at(0x90))}, {5, c.u32(at(0x84))}, {6, c.u32(at(0x88))},
               {7, c.u16(at(0x70))}, {8, c.u16(at(0x72))}});
        c.call(entry::fixture, 0x106541f4);
        if (compare(c, std::int8_t(c.r(3))) != 0) link(at(0x64), 0x7d3, c.u32(at(0x90)), 0x10654210);
        link(club_of(std::int32_t(rival_club())) + club_short_name_offset, 0x7d5, club_of(std::int32_t(rival_club())),
                0x10654240, true);
    }
}

void Story::short_form() {
    text::RecordReader read{.c = c, .record = item, .back = 0x10654250};
    const auto kept = [&](unsigned reg, std::int32_t& field,
            std::uint32_t value) { field = std::int32_t(value); c.set(reg, value); };
    kept(13, player, read(0));
    const auto first = read(1); c.set(14, first);
    const auto second_key = read(2); c.set(17, second_key);
    const auto third_key = read(3); c.set(18, third_key);
    kept(19, club, read(4));
    kept(20, figure, read(5));
    read(6, 0xc);
    read(6, 0xc);
    kept(21, kind, read(9));
    read(0xa, 0xc);
    kept(22, second, read(0xb));
    const auto other_first = read(0xc); c.set(23, other_first);
    const auto other_second = read(0xd); c.set(24, other_second);
    const auto other_third = read(0xe); c.set(25, other_third);
    for (const unsigned index : {0x12u, 0x17u, 0x18u, 0x19u, 0x1au, 0x20u}) read(index, 0xc);
    c.set({{4, at(0x4d8)}, {5, 1}});
    c.call(entry::age_text, 0x10654398);
    c.set(16, club_of(club));
    person_name(c, first, second_key, third_key, at(0x398), 5, std::uint32_t(player), 0x106543c4);
    if (compare(c, second, -1) != 0) person_name(c, other_first, other_second, other_third, at(0x230), 5,
            std::uint32_t(player), 0x106543ec);
    else c.put8(at(0x230), 0);
    headline();
    // The headline goes to the output; the names in it become links.
    copy_text(c, output, at(0x1e3c), 0x106547a4);
    const auto rows = c.u32(c.toc(rows_slot));
    const auto row_of = [&](std::int32_t index) { return rows + std::uint32_t(index) * row_size; };
    if (person_listed(c, row_of(player), first, second_key, third_key, 0x106547c4) != 0) link(at(0x398), 0x7d7,
            row_of(player), 0x106547e8);
    if (compare(c, second, -1) > 0 && person_listed(c, row_of(second), other_first, other_second, other_third,
            0x10654810) != 0)
        link(at(0x230), 0x7d7, row_of(second), 0x10654834);
    if (compare(c, club, -1) != 0) {
        link(club_of(club) + club_name_offset, 0x7d5, club_of(club), 0x10654864, true);
        link(club_of(club) + club_short_name_offset, 0x7d5, club_of(club), 0x1065488c, true);
        c.set(3, c.u32(club_of(club) + 0x2c));
        c.call(entry::nation_text, 0x106548a0);
        link(c.u32(club_of(club) + 0x2c) + 0xae, 0x7d5, c.r(3), 0x106548c4);
    }
}

}  // namespace cm::aot::media::detail::column
