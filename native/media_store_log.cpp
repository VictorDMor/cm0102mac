// The press's log records: a record is begun with a kind, filled field by field, and
// handed to the log (the object at -0x7bf0). Each routine here builds one kind.
#include "media_records.hpp"
#include "media_store.hpp"
#include "media_store_detail.hpp"
#include "media_values_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::store;
using namespace detail::records;
namespace {
// (sample + seed) % 2, signed as the original computes it.
std::uint32_t parity(std::uint32_t sum) { return std::uint32_t(std::int32_t(sum) % 2); }
}

// Kind 0x1790 for a club that has a +0x2c record, a +0x3c record with something at +0x10, and a +0x30 record.
// A club of reputation 9000 or more whose +0x30 record has a level (+0x18) above 12 goes to the headlines.
void log_kind_1790(Cpu& c, std::uint32_t club) {
    Frame frame(c, 0x10688e80, 0x140, {.first_gpr = 26});
    const auto stream = Stream::world(c);
    keep(c, {{30, club}, {31, c.toc(slots::log_queue)}, {26, stream.seed}, {27, stream.start}, {28, stream.end}, {29, stream.cursor}});
    const Club home{c, club};
    if (compare(c, std::int32_t(club)) == 0) return;
    if (compare_logical(c, home.nation()) == 0) return;
    if (compare_logical(c, home.ground()) == 0) return;
    c.set(3, home.ground());
    if (compare_logical(c, home.ground_record().location()) == 0) return;
    if (compare_logical(c, home.division()) == 0) return;
    const Item record{c, frame.at(0x38)};
    record.begin(0x1790, 0x10688ef0);
    record.put(0, home.id(), 0x10688f00);
    record.put(1, extended(home.reputation()), 0x10688f10);
    stream.advance(0x10688f30);
    record.put(2, parity(stream.sample()), 0x10688f68);
    if (compare(c, home.reputation(), 0x2328) >= 0 && compare_logical(c, home.division()) != 0 &&
        compare(c, home.division_record().level(), 0xc) > 0)
        record.post_headline(0x10688f9c);
    else record.post_for_manager(home.nation(), 0x10688fb0);
}

// Kind 0x178e: a club with a manager and a second person (each with the three ids behind their
// +0x28, +0x2c and +0x30 records), then four values the caller passes: a signed byte, a byte, a
// halfword and a byte. A club above reputation 4750 is logged for its +0x2c record.
void log_kind_178e(Cpu& c, std::uint32_t club, std::uint32_t other, std::uint32_t first, std::uint32_t second,
                   std::uint32_t third, std::uint32_t fourth) {
    Frame frame(c, 0x106a15f0, 0x140, {.first_gpr = 24});
    keep(c, {{31, club}, {30, c.toc(slots::log_queue)}, {24, other}, {25, first}, {26, second}, {27, third}, {28, fourth}});
    const Club home{c, club};
    const Person partner{c, other};
    if (compare(c, std::int32_t(club)) == 0) return;
    if (compare_logical(c, home.nation()) == 0) return;
    if (compare_logical(c, home.manager()) == 0) return;
    if (compare_logical(c, other) == 0) return;
    // Past the gates the manager, the other person and the +0x2c record all exist: the original's
    // tests for them again (and its -1 fields for a missing one) cannot take their other side.
    const Item record{c, frame.at(0x38)};
    record.begin(0x178e, 0x106a1650);
    record.put(0, home.id(), 0x106a1660);
    const Kept<29> manager_at(c, home.manager());
    const Person manager{c, manager_at};
    compare_logical(c, manager_at);
    record.put(1, manager.id(), 0x106a167c);
    record.put(2, manager.first_name(), 0x106a1690);
    record.put(3, manager.second_name(), 0x106a16a4);
    record.put(4, manager.common_name(), 0x106a16b8);
    record.put(5, std::uint32_t(byte_word(first)), 0x106a170c);
    compare_logical(c, other);
    record.put(6, partner.id(), 0x106a1724);
    record.put(7, partner.first_name(), 0x106a1738);
    record.put(8, partner.second_name(), 0x106a174c);
    record.put(9, partner.common_name(), 0x106a1760);
    record.put(10, std::uint32_t(byte_word(second)), 0x106a17b4);
    record.put(11, extended(std::int16_t(third)), 0x106a17c4);
    compare_logical(c, home.nation());
    record.put(12, home.nation_record().id(), 0x106a17e4);
    record.put(13, std::uint32_t(byte_word(fourth)), 0x106a1808);
    if (compare_logical(c, home.nation()) != 0 && compare(c, home.reputation(), 0x128e) > 0) record.post_for_manager(home.nation(), 0x106a1830);
    else record.post(club, 0x106a1844);
}

// Kind 0xbcb: a player of reputation 3500 or more (the value at +0xa of his +0x14 record), the club he
// met and the match. 0x1030d4a0 and 0x1030dcd0 complete the record from the match; then one more field
// tells which side of the match the club was on (the match's signed goal bytes at +0x49 and +0x4a, its
// two clubs at +0x1c and +0x20), and the entries 0x103fc710 lists for the player are logged.
void log_kind_bcb(Cpu& c, std::uint32_t player, std::uint32_t club, std::uint32_t match) {
    Frame frame(c, 0x1069d1a0, 0x1f0, {.first_gpr = 23});
    const auto stream = Stream::world(c);
    keep(c, {{30, c.toc(-0x7b40)}, {29, player}, {31, c.toc(slots::log_queue)}, {24, club}, {26, stream.seed}, {25, match},
             {27, stream.start}, {23, stream.end}, {28, stream.cursor}});
    const Person about{c, player};
    Program program{c};
    program.call(library::today, 0x1069d1dc, {frame.at(0x3c)});
    if (compare_logical(c, player) == 0) return;
    if (compare_logical(c, about.player()) == 0) return;
    if (compare_logical(c, club) == 0) return;
    c.set(3, about.player());
    if (compare(c, about.player_record().reputation(), 0xdac) < 0) return;
    const Item record{c, frame.at(0xd8)};
    record.begin(0xbcb, 0x1069d218);
    record.put(0, about.id(), 0x1069d228);
    record.put(1, about.first_name(), 0x1069d23c);
    record.put(2, about.second_name(), 0x1069d250);
    record.put(3, about.common_name(), 0x1069d264);
    record.put(4, Club{c, club}.id(), 0x1069d274);
    stream.advance(0x1069d294);
    record.put(5, parity(stream.sample()), 0x1069d2c8);
    program.call(0x1030d4a0, 0x1069d2d8, {match, club, frame.at(0x3c)});
    // The byte at sp+0x38 is the next free field of the record; 0x1030dcd0 moves it on.
    const auto next = frame.at(0x38);
    c.put8(next, 6);
    program.call(0x1030dcd0, 0x1069d2f0, {frame.at(0x3c), record.at, next});
    const auto home_goals = std::int8_t(c.u8(match + 0x49)), away_goals = std::int8_t(c.u8(match + 0x4a));
    // 0: no result; 1: the club is the side that scored more; 2: it is not.
    const auto side = [&](std::uint32_t value, std::uint32_t back) {
        const auto index = c.u8(next);
        c.put8(next, index + 1);
        record.put(index, value, back);
    };
    if (compare(c, home_goals, -1) == 0) side(0, 0x1069d318);
    else if (compare(c, home_goals, away_goals) > 0) {
        if (compare_logical(c, c.u32(match + 0x1c), club) == 0) side(1, 0x1069d358);
        else side(2, 0x1069d374);
    } else if (compare(c, home_goals, away_goals) < 0) {
        if (compare_logical(c, c.u32(match + 0x20), club) == 0) side(1, 0x1069d3b4);
        else side(2, 0x1069d3d0);
    }
    const auto listing = frame.at(0x60);
    if (compare_logical(c, c.u32(c.toc(-0x7b40))) != 0)
        program.call(0x103fc710, 0x1069d3f4, {c.u32(c.toc(-0x7b40)), player, 0xda, listing, 0x1e});
    else c.set(3, 0);
    Kept<23> listed(c, c.r(3));
    c.put8(next, 0);
    for (;;) {
        c.set(3, std::uint32_t(byte_word(c.u8(next))));
        if (compare(c, std::int8_t(c.u8(next)), std::int8_t(std::uint32_t(listed))) >= 0) break;
        const auto entry = c.u32(listing + std::uint32_t(byte_word(c.u8(next))) * 4);
        record.post(entry, 0x1069d42c);
        c.put8(next, c.u8(next) + 1);
    }
}

// Kind 0x2e for a club with a manager and a second record: the manager's three ids, both ids, a draw,
// the roll for the club, the result of 0x106a95c0, a flag, a rating, whether the club's +0x30 record
// is the second one, and the days 0x10718130 finds from the manager's +0x4c and +0x50; the club's
// reputation is added and the record logged when that is 180 or more. When 0x100c7d00 (which compares
// the club's id with a limit kept in two words) answers nonzero, no record is made.
void log_kind_2e(Cpu& c, std::uint32_t club, std::uint32_t other, std::uint32_t flag) {
    Frame frame(c, 0x106a6e60, 0x150, {.first_gpr = 22});
    const auto stream = Stream::world(c);
    keep(c, {{31, club}, {22, c.toc(-0x7570)}, {29, other}, {23, stream.seed}, {27, flag},
             {24, stream.start}, {25, stream.end}, {26, stream.cursor}});
    const Kept<28> ratings_slot(c, c.toc(tables::club_ratings));
    Kept<30> counter(c, 0);
    Program program{c};
    const Club home{c, club};
    if (compare(c, std::int32_t(club)) == 0) return;
    if (compare_logical(c, home.manager()) == 0) return;
    if (compare_logical(c, other) == 0) return;
    program.call(0x100c7d00, 0x106a6eb8, {club});
    if (compare(c, std::int8_t(c.r(3))) != 0) return;
    const Item record{c, frame.at(0x38)};
    record.begin(0x2e, 0x106a6ed0);
    // Each field takes the next index; r30 is already past it when the call is made.
    const auto emit = [&](std::uint32_t value, std::uint32_t back) {
        const auto index = std::uint32_t(counter);
        counter = index + 1;
        record.put(index, value, back);
    };
    const auto manager = home.manager_record();
    emit(manager.id(), 0x106a6ee8);
    emit(manager.first_name(), 0x106a6f04);
    emit(manager.second_name(), 0x106a6f20);
    emit(manager.common_name(), 0x106a6f3c);
    emit(home.id(), 0x106a6f50);
    emit(c.u32(other), 0x106a6f64);
    stream.advance(0x106a6f84);
    emit(parity(stream.sample()), 0x106a6fc0);
    enter(c, 0x106a6fc8, {club});
    Kept<23> roll(c, story_roll_for_club(c, club));
    emit(roll, 0x106a6fe0);
    const bool has_region = compare_logical(c, home.nation()) != 0;
    emit(program.call(0x106a95c0, has_region ? 0x106a7004 : 0x106a701c,
                      {c.u32(c.r(22)), club, std::uint32_t(byte_word(roll)), has_region ? std::uint32_t(home.nation_record().id()) : no_id}),
         0x106a7030);
    emit(std::uint32_t(byte_word(flag)), 0x106a7044);
    // 5000 unless the table behind the word at -0x79f0 exists; its row for this club (0x4c bytes each)
    // has a halfword at +6. The club is not null here, so the original's 0 for that case cannot occur.
    std::uint32_t rating = 0x1388;
    if (compare_logical(c, c.u32(ratings_slot)) != 0) {
        compare_logical(c, club);
        rating = extended(home.table_rating());
    }
    emit(rating, 0x106a7090);
    emit(compare_logical(c, home.division(), other) == 0 ? 1 : 0, 0x106a70b4);
    Kept<22> days(c, program.call(0x10718130, 0x106a70c8, {c.toc(tables::today), manager.joined_first().get(), manager.joined_second().get()}));
    emit(days, 0x106a70e0);
    if (compare(c, std::int32_t(std::uint32_t(days)), 0xb4) < 0) return;
    record.put(std::uint32_t(counter), extended(home.reputation()), 0x106a70f8);
    record.post(club, 0x106a710c);
}
}
