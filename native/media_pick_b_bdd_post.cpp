// 0x1067e210, second half: the item's numbered fields, the history check, the posting, and the story's
// effect on the player's ratings and on a random club's people.
#include "media_pick_b.hpp"
#include "media_pick_b_bdd_detail.hpp"

namespace cm::aot::media {
using namespace detail;
using namespace detail::pick;
using namespace detail::pick_b;

namespace {
constexpr std::uint32_t item_kind = 0xbdd;
}

// The numbered fields after the kind: the club's table rating, the player's owner, the roll, the club's manager
// and his three links.
void Bdd::put_standing_fields() {
    const auto rating = team.table_rating().get();  // club nonnull: the original's null branch gives 0
    item.put(6, rating < 0xfa0 ? 1 : 0, 0x1067f340);
    item.put(7, subject.nation_record().id(), 0x1067f354);
    item.put(8, roll, 0x1067f364);
    put_person(c, item, 9, team.manager(), {0x1067f378, 0x1067f390, 0x1067f3a8, 0x1067f3c0});
}

// Everything up to the dates: false when the routine ends there (the history already has the story, or a
// coach is missing).
bool Bdd::posts() {
    item.put(5, std::uint32_t(byte_word(kind)), 0x1067f2c0);
    const auto known = program.module_call(0x1069e700, 0x1067f2dc, {query, person, club, item_kind, std::uint32_t(byte_word(kind)), 0});
    if (compare(c, byte_word(known)) != 0) return false;
    put_standing_fields();
    const auto open = program.call(0x1014c2e0, 0x1067f3d8, {club, 0, 1, 0, 0});
    if (compare(c, byte_word(open)) == 0) r20 = program.module_call(0x106584d0, 0x1067f3fc, {query, team.manager(), person, std::uint32_t(byte_word(kind))});
    else r20 = 0;
    item.put(13, r20, 0x1067f410);
    const auto fame = program.call(0x100d71a0, 0x1067f41c, {person, club});
    item.put(14, fame, 0x1067f42c);
    item.put(15, extended(std::int16_t(program.call(0x10117490, 0x1067f434, {player_record()}))), 0x1067f448);
    if (compare(c, byte_word(kind), 0x34) == 0 && compare_logical(c, extra) != 0) {
        standing = c.u32(extra);
        item.put(16, standing, 0x1067f470);
    } else if (compare(c, byte_word(kind), 0x33) == 0 || compare(c, byte_word(kind), 0x34) == 0) {
        const auto owner = team.nation();
        if (!nonzero_logical(c, program.call(0x102b7450, 0x1067f498, {c.u32(c.r(16)), owner}))) return false;
        standing = c.u32(program.call(0x102b7450, 0x1067f4ac, {c.u32(c.r(16)), owner}));
        item.put(16, standing, 0x1067f4c4);
    } else {
        standing = c.u32(team.division());
        item.put(16, standing, 0x1067f4e4);
    }
    return true;
}

// The fields past 0x10, numbered by the counter in the byte at sp+0x38: the extra record's date, the person
// the club is dealing with and the people around the player.
void Bdd::put_dates() {
    c.put8(frame.at(0x38), 0x11);
    compare_logical(c, extra);
    if (extra != 0) {
        program.call(0x10715880, 0x1067f4fc, {frame.at(0x58)});
        numbered.put(1, 0x1067f514);
        program.call(0x1030d4a0, 0x1067f524, {extra, club, frame.at(0x58)});
        program.call(0x1030dcd0, 0x1067f534, {frame.at(0x58), item.at, frame.at(0x38)});
    } else {
        numbered.put(0, 0x1067f550);
    }
    const auto found = program.call(0x103f43b0, 0x1067f560, {slot_value(-0x7b40), person, frame.at(0x44)});
    if (compare(c, byte_word(found)) != 0) {
        const auto record = c.u32(frame.at(0x44));
        const auto link = program.call(0x103eb440, 0x1067f578, {slot_value(-0x7b40), c.u32(record + 8)});
        numbered.put_loaded([&] { return c.u32(c.u32(link) + 8); }, 0x1067f598);
        numbered.put(1, 0x1067f5b0);
        numbered.put(extended(c.i16(c.u32(frame.at(0x44)))), 0x1067f5cc);
        numbered.put(c.u16(c.u32(frame.at(0x44)) + 2), 0x1067f5e8);
    } else if (compare_logical(c, subject.club()) != 0) {
        numbered.put_loaded([&] { return c.u32(subject.club()); }, 0x1067f614);
        numbered.put(0, 0x1067f62c);
        numbered.put(none, 0x1067f644);
        numbered.put(none, 0x1067f65c);
    } else {
        numbered.put(none, 0x1067f678);
        numbered.put(0, 0x1067f690);
        numbered.put(none, 0x1067f6a8);
        numbered.put(none, 0x1067f6c0);
    }
    const auto seen = program.call(0x103f43b0, 0x1067f6d0, {slot_value(-0x7b40), person, 0});
    if (compare(c, byte_word(seen)) == 0) {
        // The high nibble of the player's byte in the table at -0x7b9c, as in the kind choice.
        const auto table = c.u32(r15);
        c.set(6, table);
        numbered.put_loaded([&] { return std::uint32_t((c.u8(table + subject.id() * 0x64 + 0xc) >> 4) & 0xf); }, 0x1067f704);
    } else {
        numbered.put(0, 0x1067f720);
    }
}

// The staff the club is dealing with (0x10647730 picks), the date lookup, the extra record's side and the
// person who stands for the player.
void Bdd::put_staff() {
    const auto staff = program.module_call(0x10647730, 0x1067f730, {query, club, person});
    roll = staff;  // r27 holds the staff record from here on
    compare(c, std::int32_t(staff));
    if (staff == 0) put_no_person(numbered, {0x1067f750, 0x1067f768, 0x1067f780, 0x1067f798});
    else put_person_loaded(c, numbered, staff, {0x1067f7b4, 0x1067f7d0, 0x1067f7ec, 0x1067f808});
    c.put32(frame.at(0x40), 0);
    c.put32(frame.at(0x3c), none);
    program.call(0x1049d330, 0x1067f830, {c.u32(c.toc(-0x7424)), standing, frame.at(0x40), frame.at(0x3c)});
    numbered.put(c.u32(frame.at(0x40)), 0x1067f848);
    numbered.put(c.u32(frame.at(0x3c)), 0x1067f860);
    // Which side of the extra record's pair the club is on: 1 on the side it holds, 2 on the other, 0 when level
    // (the only block of its kind that puts 0 for a level score).
    compare_logical(c, extra);
    if (extra == 0) numbered.put(0, 0x1067f980);
    else put_result(c, numbered, Fixture{c, extra}, club, {0x1067f890, 0x1067f8d0, 0x1067f8ec, 0x1067f92c, 0x1067f948, 0x1067f964});
    compare_logical(c, staff);
    const Person staff_person{c, staff};
    if (staff == 0) {
        numbered.put(none, 0x1067fa64);
        numbered.put(none, 0x1067fa7c);
    } else if (compare_logical(c, staff_person.club()) != 0) {
        numbered.put_loaded([&] { return std::uint32_t(staff_person.job().get()); }, 0x1067f9b0);
        numbered.put_loaded([&] { return c.u32(staff_person.club()); }, 0x1067f9cc);
    } else if (compare_logical(c, staff_person.national_team()) != 0) {
        numbered.put_loaded([&] { return std::uint32_t(byte_word(staff_person.national_job())); }, 0x1067f9f8);
        numbered.put_loaded([&] { return c.u32(staff_person.national_team()); }, 0x1067fa14);
    } else {
        numbered.put(none, 0x1067fa30);
        numbered.put(none, 0x1067fa48);
    }
}

// The last fields, then the item goes to the queue: once per person the player's story reaches, or in its
// own form for kinds 0x33 and 0x34.
void Bdd::deliver() {
    r15 = program.module_call(0x1069eb10, 0x1067fa84, {person});
    numbered.put(r15, 0x1067faa0);
    const auto owner = team.nation();
    compare_logical(c, owner);
    const auto sold = owner != 0 ? program.module_call(0x106a95c0, 0x1067fac4, {c.u32(r14), club, std::uint32_t(byte_word(r15)), c.u32(owner + 0x68)})
                                 : program.module_call(0x106a95c0, 0x1067fadc, {c.u32(r14), club, std::uint32_t(byte_word(r15)), none});
    numbered.put(sold, 0x1067faf4);
    numbered.put(std::uint32_t(age()), 0x1067fb10);
    const bool young = compare_logical(c, player_record()) != 0 && compare(c, c.u8(player_record() + 0xf), 0x14) == 0;
    numbered.put(young ? 1 : 0, 0x1067fb50);
    const auto people = slot_value(-0x7b40);
    r14 = compare_logical(c, people) != 0 ? program.call(0x103fc710, 0x1067fb74, {people, person, 0x25a, frame.at(0x7c), 0x1e}) : 0;
    if (compare(c, byte_word(kind), 0x33) == 0) {
        if (compare(c, std::int32_t(team.reputation().get()) + subject.player_record().world_reputation().get(), 0x35b6) > 0) item.post_headline(0x1067fbb0);
        else program.call(0x102cc730, 0x1067fbc8, {item.log(), item.at, c.u32(team.nation() + 0x74)});
    } else if (compare(c, byte_word(kind), 0x34) == 0) {
        item.post_headline(0x1067fbe4);
    } else {
        for (r15 = 0; compare(c, byte_word(r15), byte_word(r14)) < 0; r15 = r15 + 1)
            item.post(c.u32(frame.at(0x7c) + std::uint32_t(byte_word(r15)) * 4), 0x1067fc0c);
    }
}

// Records the story, raises the player's three ratings (capped at 10000), tells his people, and lets a random
// club of the same owner react.
void Bdd::raise_and_notify() {
    program.module_call(0x1069e190, 0x1067fc3c, {query, person, club, item_kind, std::uint32_t(byte_word(kind)), std::uint32_t(byte_word(r20))});
    // The player's three reputations (+0xa, +8 and +0xc of his record) rise and are capped at 10000.
    struct Raise { std::uint32_t offset, step; };
    for (const auto [offset, step] : {Raise{0xa, 0xc8}, Raise{8, 0xc8}, Raise{0xc, 0x64}}) {
        const auto rating = player_record() + offset;
        c.put16(rating, std::uint32_t(c.i16(rating)) + step);
        if (compare(c, c.i16(player_record() + offset), 0x2710) > 0) c.put16(player_record() + offset, 0x2710);
    }
    program.call(0x10363ca0, 0x1067fcd4, {c.u32(slot_value(-0x7b40) + 0x988), person, 1});
    r15 = c.u32(c.toc(-0x7b50));
    r14 = std::uint32_t(draw_below(c, frame, fpu, rng, pool, {0x1067fd0c, 0x1067fde4, 0x1d8, 0x1e0, 0x1e8, 14}, std::int32_t(r15)));
    const Club other{c, c.u32(c.toc(-0x7be8)) + r14 * club_size};
    if (compare_logical(c, other.nation()) != 0 && compare_logical(c, other.nation(), team.nation()) == 0)
        program.call(0x100d9960, 0x1067fe84, {c.u32(other.at + 0x80), person, 1});
    const auto open = program.call(0x1014c2e0, 0x1067fe9c, {club, 0, 1, 0, 0});
    if (compare(c, byte_word(open)) == 0) program.module_call(0x10677d60, 0x1067febc, {query, person, club, std::uint32_t(byte_word(kind)), std::uint32_t(byte_word(r20))});
}

void post_item_bdd(Cpu& c, std::uint32_t query, std::uint32_t person, std::uint32_t club, std::uint32_t kind, std::uint32_t extra) {
    Bdd story(c, query, person, club, kind, extra);
    if (!story.admits()) return;
    story.roll = std::uint32_t(story.sample(0x1067e328) % 2);
    story.item.begin(item_kind, 0x1067e364);
    put_person(c, story.item, 0, person, {0x1067e374, 0x1067e388, 0x1067e39c, 0x1067e3b0});
    story.item.put(4, story.team.id(), 0x1067e3c8);  // club nonnull: the original's -1 branch never runs
    story.choose_kind();
    if (!story.posts()) return;
    story.put_dates();
    story.put_staff();
    story.deliver();
    story.raise_and_notify();
}
}
