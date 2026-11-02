// The press items of the reactions: 0xbe6 (two people and their clubs), 0xbe8 (a person with a standing at his
// club), 0xbe5 (a person and a club) and 0xbe4 (a club's manager and two people). Each names the people by
// their id and three links, adds a share of the random stream, and posts to the shared queue.
#include "media_pick_c.hpp"
#include "media_pick_c_detail.hpp"
#include "media_store.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::pick_c;

namespace {
constexpr std::uint32_t manager_of = 0x90, club_of = 0x24, contract_of = 0x14;

// The checks both item 0xbe8 and 0xbe5 make first: a person with a link, a club that has a manager and a +0x3c
// record, and a contract.
bool person_ready(Cpu& c, std::uint32_t person) {
    const Person who{c, person};
    if (compare(c, std::int32_t(person)) == 0) return false;
    if (compare_logical(c, who.nation()) == 0) return false;
    if (compare_logical(c, who.club()) == 0) return false;
    c.set(3, who.club());
    if (compare_logical(c, Club{c, c.r(3)}.manager()) == 0) return false;
    c.set(3, who.club());
    if (compare_logical(c, Club{c, c.r(3)}.ground()) == 0) return false;
    return compare_logical(c, who.player()) != 0;
}

// The label routine's text for a club under a shape (0x106a95c0): with the id of the club's +0x2c record, or -1 when
// it has none. The result is left in r3 for the field that follows.
std::uint32_t club_label(Cpu& c, std::uint32_t club, std::uint32_t shape, std::uint32_t back_with_record, std::uint32_t back_without) {
    const Club home{c, club};
    if (compare_logical(c, home.nation()) != 0) {
        c.set(6, home.nation_record().id());
        module_call(c, callee::label, back_with_record, {c.u32(c.toc(slots::labels)), club, std::uint32_t(byte_word(shape)), c.r(6)});
    } else {
        module_call(c, callee::label, back_without, {c.u32(c.toc(slots::labels)), club, std::uint32_t(byte_word(shape)), 0xffffffff});
    }
    return c.r(3);
}

// 0x106584d0's verdict on the manager of the person's club (the code says which kind of item asks), unless the club
// is barred (0x1014c2e0 answers true), in which case 0.
std::uint32_t manager_verdict(Cpu& c, std::uint32_t self, std::uint32_t person, std::uint32_t code, std::uint32_t barred_back,
                              std::uint32_t verdict_back) {
    const auto home = Person{c, person}.club();
    const auto barred = Program{c}.call(callee::club_flag, barred_back, {home, 0, 1, 0, 0});
    if (compare(c, byte_word(barred)) != 0) return 0;
    module_call(c, callee::verdict, verdict_back, {self, Club{c, home}.manager(), person, code});
    return c.r(3);
}
}

// 0x106a1f00
void post_item_be6(Cpu& c, std::uint32_t self, std::uint32_t first, std::uint32_t second, std::uint32_t fixture) {
    Frame frame(c, 0x106a1f00, 0x180, {.first_gpr = 23});
    const auto rng = Stream::world(c);
    // r23: the stream's end, then the manager of one club or the verdict; r24: the queue; r26: self; r27: the fixture; r30, r31: the two people.
    Kept<23> scratch(c, rng.end);
    keep(c, {{24, c.toc(slots::queue)}, {25, rng.start}, {26, self}, {27, fixture}, {28, rng.seed}, {29, rng.cursor}, {30, second}, {31, first}});
    c.put8(frame.at(0x38), 0);
    if (compare(c, std::int32_t(first)) == 0) return;
    if (compare_logical(c, c.u32(first + 0xc)) == 0) return;
    if (compare_logical(c, c.u32(first + club_of)) == 0) return;
    c.set(3, c.u32(first + club_of));
    if (compare_logical(c, c.u32(c.r(3) + manager_of)) == 0) return;
    if (compare_logical(c, c.u32(first + contract_of)) == 0) return;
    if (compare_logical(c, second) == 0) return;
    if (compare_logical(c, c.u32(second + club_of)) == 0) return;
    c.set(3, c.u32(second + club_of));
    if (compare_logical(c, c.u32(c.r(3) + manager_of)) == 0) return;
    if (compare_logical(c, c.u32(second + contract_of)) == 0) return;

    const Item item{c, frame.at(0x60)};
    const StackFields fields{c, item, frame.at(0x38)};
    Fpu fpu(c);
    const Program program{c};
    item.begin(0xbe6, 0x106a1fb4);
    fields.name(first, 0x106a1fcc, 0x106a1fe8, 0x106a2004, 0x106a2020);
    fields.name(second, 0x106a2038, 0x106a2054, 0x106a2070, 0x106a208c);
    rng.advance(0x106a20ac);
    c.set(6, entry_of(c, rng));
    fields.put(signed_mod2(rng.sample()), 0x106a20ec);
    fields.put([&] { return c.u32(c.u32(first + club_of)); }, 0x106a2108);
    // The original tests the second person and his club again here (and would put -1); both hold.
    fields.put([&] { return c.u32(c.u32(second + club_of)); }, 0x106a2140);
    // A pooled share of the first person's contract, through the float unit's truncation.
    c.set_f(1, double(c.f32(c.toc(-0x4200))));
    program.call(0x10303560, 0x106a2160, {c.u32(c.toc(-0x7b3c)), c.u32(first + contract_of), 0xb, 2, c.toc(-0x4200)});
    c.put64(frame.at(0x144), fpu.fctiwz(0, c.f(1)));
    fields.put([&] { return c.u32(frame.at(0x148)); }, 0x106a2180);
    // The manager is loaded between the counter's store and the load of his id.
    fields.put([&] { scratch = c.u32(c.u32(first + club_of) + manager_of); return c.u32(scratch); }, 0x106a21a0);
    fields.put([&] { return linked(c, scratch, 0x28); }, 0x106a21bc);
    fields.put([&] { return linked(c, scratch, 0x2c); }, 0x106a21d8);
    fields.put([&] { return linked(c, scratch, 0x30); }, 0x106a21f4);
    fields.put([&] { scratch = c.u32(c.u32(second + club_of) + manager_of); return c.u32(scratch); }, 0x106a2214);
    fields.put([&] { return linked(c, scratch, 0x28); }, 0x106a2230);
    fields.put([&] { return linked(c, scratch, 0x2c); }, 0x106a224c);
    fields.put([&] { return linked(c, scratch, 0x30); }, 0x106a2268);
    scratch = 0;
    scratch = manager_verdict(c, self, first, 0x82, 0x106a2284, 0x106a22a4);
    fields.put(scratch, 0x106a22c0);
    program.call(callee::today, 0x106a22c8, {frame.at(0x3c)});
    program.call(0x1030d4a0, 0x106a22d8, {fixture, c.u32(first + club_of), frame.at(0x3c)});
    program.call(0x1030dcd0, 0x106a22e8, {frame.at(0x3c), frame.at(0x60), frame.at(0x38)});
    c.set(3, c.u32(first + contract_of));
    if (compare(c, std::int32_t(c.i16(c.r(3) + 0xc)), 0x128e) <= 0) {
        item.post(c.u32(first + club_of), 0x106a231c);
    } else {
        item.post_for_manager(c.u32(first + 0xc), 0x106a2308);
    }
}

// 0x106a40a0
void post_item_be8(Cpu& c, std::uint32_t self, std::uint32_t person, std::uint32_t flag) {
    Frame frame(c, 0x106a40a0, 0x150, {.first_gpr = 20});
    const auto rng = Stream::world(c);
    // r20: the stream's seed, then the person's shape; r24: the labels; r25: the club's manager, then the club;
    // r26: the queue; r27: self; r28: the flag; r29: the person's standing; r30: the person; r31: the field counter.
    Kept<20> shape(c, rng.seed);
    Kept<25> club(c, c.r(25));
    Kept<29> standing(c, c.r(29));
    Kept<31> counter(c, 0);
    keep(c, {{21, rng.start}, {22, rng.end}, {23, rng.cursor}, {24, c.toc(slots::labels)}, {26, c.toc(slots::queue)}, {27, self}, {28, flag}, {30, person}});
    if (!person_ready(c, person)) return;
    const Program program{c};
    standing = program.call(callee::standing_of, 0x106a4134, {c.u32(c.toc(slots::standings)), person, c.u32(person + club_of)});
    if (compare(c, std::int32_t(standing)) == 0) return;

    const Item item{c, frame.at(0x38)};
    const RegisterFields<31> fields{c, item, counter};
    item.begin(0xbe8, 0x106a414c);
    fields.name(person, 0x106a4160, 0x106a4178, 0x106a4190, 0x106a41a8);
    fields.put_after([&] { club = c.u32(c.u32(person + club_of) + manager_of); return c.u32(club); }, 0x106a41c4);
    fields.link(club, 0x28, 0x106a41dc);
    fields.link(club, 0x2c, 0x106a41f4);
    fields.link(club, 0x30, 0x106a420c);
    club = c.u32(person + club_of);
    fields.put(c.u32(club), 0x106a4228);
    fields.put(program.call(callee::region_of, 0x106a4230, {person}), 0x106a4244);
    rng.advance(0x106a4264);
    fields.put(signed_mod2(rng.sample()), 0x106a42a0);
    enter(c, 0x106a42a8, {person});
    shape = story_roll_for_player(c, person);
    fields.put(shape, 0x106a42c0);
    fields.put(club_label(c, club, shape, 0x106a42e4, 0x106a42fc), 0x106a4310);
    fields.put(program.call(callee::fame, 0x106a431c, {person, club}), 0x106a4330);
    fields.put(extended(std::int16_t(program.call(callee::grade, 0x106a4338, {c.u32(person + contract_of)}))), 0x106a434c);
    fields.put(flag, 0x106a4360);
    fields.put(std::uint32_t(byte_word(c.u8(standing + 0x4c))), 0x106a4378);
    fields.put(std::uint32_t(byte_word(c.u16(person + 0x5c))), 0x106a4394);
    shape = 0;
    shape = manager_verdict(c, self, person, 0x83, 0x106a43b0, 0x106a43d0);
    fields.put(shape, 0x106a43e8);
    fields.put(c.u8(standing + 0x60) & 0xf, 0x106a4404);
    // r5 is 1 when the person's two traits (a contract byte and a person byte) sum below 10 while a third exceeds 10.
    const auto contract = c.u32(person + contract_of);
    std::uint32_t trait_flag = 0;
    c.set(6, c.u8(contract + 0x1c));
    if (compare(c, byte_word(c.r(6) + c.u8(person + 0x69)), 10) < 0 && compare(c, byte_word(c.u8(person + 0x63)), 10) > 0) trait_flag = 1;
    fields.put(trait_flag, 0x106a4448);
    // The last field leaves the counter as it is.
    item.put(counter, std::uint32_t(byte_word(program.call(callee::person_flag, 0x106a4454, {person, 0}))), 0x106a4464);
    if (compare(c, std::int32_t(flag)) != 0) {
        item.post(c.u32(person + club_of), 0x106a44b4);
        return;
    }
    c.set(3, c.u32(person + contract_of));
    if (compare(c, std::int32_t(c.i16(c.r(3) + 0xc)), 0x128e) <= 0) item.post(c.u32(person + club_of), 0x106a44a0);
    else item.post_for_manager(c.u32(person + 0xc), 0x106a448c);
}

// 0x106a5e90
void post_item_be5(Cpu& c, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t kind, std::uint32_t flag,
                   std::uint32_t other) {
    Frame frame(c, 0x106a5e90, 0x150, {.first_gpr = 20});
    const auto rng = Stream::world(c);
    // r20: the labels, then the club's verdict or manager; r21: the stream's seed, then the person's shape; r25: the
    // flag; r26: self; r27: the kind; r28: the club; r29: the other person; r30: the field counter; r31: the person.
    Kept<20> verdict(c, c.toc(slots::labels));
    Kept<21> shape(c, rng.seed);
    Kept<30> counter(c, 0);
    keep(c, {{22, rng.start}, {23, rng.end}, {24, rng.cursor}, {25, flag}, {26, self}, {27, kind}, {28, club}, {29, other}, {31, person}});
    if (!person_ready(c, person)) return;
    // Without a second person, kind 2 (the one that names him) is left alone.
    if (compare_logical(c, other) == 0 && compare(c, byte_word(kind), 2) == 0) return;

    const Item item{c, frame.at(0x38)};
    const RegisterFields<30> fields{c, item, counter};
    const Program program{c};
    item.begin(0xbe5, 0x106a5f3c);
    fields.name(person, 0x106a5f50, 0x106a5f68, 0x106a5f80, 0x106a5f98);
    fields.put_after([&] { return c.u32(c.u32(person + club_of)); }, 0x106a5fb0);
    rng.advance(0x106a5fd0);
    fields.put(signed_mod2(rng.sample()), 0x106a600c);
    enter(c, 0x106a6014, {person});
    shape = story_roll_for_player(c, person);
    fields.put(shape, 0x106a602c);
    fields.put(club_label(c, club, shape, 0x106a6050, 0x106a6068), 0x106a607c);
    fields.put(extended(std::int16_t(program.call(callee::grade, 0x106a6084, {c.u32(person + contract_of)}))), 0x106a6098);
    fields.put(std::uint32_t(byte_word(kind)), 0x106a60ac);
    verdict = 0;
    verdict = manager_verdict(c, self, person, 0x82, 0x106a60c8, 0x106a60e8);
    fields.put(verdict, 0x106a6100);
    if (compare(c, byte_word(kind), 2) == 0) fields.name(other, 0x106a6120, 0x106a6138, 0x106a6150, 0x106a6168);
    c.set(5, 0);
    if (compare(c, std::int32_t(c.i16(club + 0x1ec)), 0x1c52) > 0) c.set(5, 1);
    fields.put(c.r(5), 0x106a618c);
    fields.put_after([&] { verdict = c.u32(c.u32(person + club_of) + manager_of); return c.u32(verdict); }, 0x106a61a8);
    fields.link(verdict, 0x28, 0x106a61c0);
    fields.link(verdict, 0x2c, 0x106a61d8);
    fields.link(verdict, 0x30, 0x106a61f0);
    item.put(counter, std::uint32_t(byte_word(flag)), 0x106a6200);
    item.post(c.u32(person + club_of), 0x106a6214);
}

// 0x106a7d70
void post_item_be4(Cpu& c, std::uint32_t, std::uint32_t club, std::uint32_t record, std::uint32_t first, std::uint32_t second) {
    Frame frame(c, 0x106a7d70, 0x150, {.first_gpr = 22});
    const auto rng = Stream::world(c);
    // r22: the record, then the club's shape; r23, r24: the two people; r25: the labels; r30: the club; r31: the field counter.
    Kept<22> shape(c, record);
    Kept<31> counter(c, 0);
    keep(c, {{23, first}, {24, second}, {25, c.toc(slots::labels)}, {26, rng.seed}, {27, rng.start}, {28, rng.end}, {29, rng.cursor}, {30, club}});
    if (compare(c, std::int32_t(club)) == 0) return;
    if (compare_logical(c, c.u32(club + manager_of)) == 0) return;
    const Program program{c};
    if (compare(c, byte_word(program.call(callee::is_national_team, 0x106a7dc0, {club}))) != 0) return;
    if (compare_logical(c, second) == 0) return;
    if (compare_logical(c, c.u32(second + contract_of)) == 0) return;
    if (compare_logical(c, first) == 0) return;
    if (compare_logical(c, c.u32(first + contract_of)) == 0) return;

    const Item item{c, frame.at(0x38)};
    const RegisterFields<31> fields{c, item, counter};
    item.begin(0xbe4, 0x106a7e00);
    fields.name(c.u32(club + manager_of), 0x106a7e18, 0x106a7e34, 0x106a7e50, 0x106a7e6c);
    fields.name(second, 0x106a7e80, 0x106a7e98, 0x106a7eb0, 0x106a7ec8);
    fields.name(first, 0x106a7edc, 0x106a7ef4, 0x106a7f0c, 0x106a7f24);
    fields.put(c.u32(club), 0x106a7f38);
    fields.put(c.u32(record), 0x106a7f4c);
    rng.advance(0x106a7f6c);
    // The second person's trait (+0x67) against a draw of 10..14: above it, a coin flip; otherwise a coin flip plus 2.
    const auto fifth = reduced_by(rng.sample(), divisors::by_5);
    const std::int32_t trait = Person{c, second}.sportsmanship().get();
    leave(c, {{3, std::uint32_t(trait)}, {4, std::uint32_t(fifth.rest)}, {5, fifth.sample}, {6, entry_of(c, rng)}});
    const bool above = compare(c, trait, fifth.rest + 10) > 0;
    rng.advance(above ? 0x106a7fd8 : 0x106a8028);
    const auto coin = signed_mod2(rng.sample());
    fields.put(above ? coin : coin + 2, 0x106a8064);
    enter(c, 0x106a806c, {club});
    shape = story_roll_for_club(c, club);
    fields.put(shape, 0x106a8084);
    fields.put(club_label(c, club, shape, 0x106a80a8, 0x106a80c0), 0x106a80d4);
    item.put(counter, 2, 0x106a80e4);
    item.post(club, 0x106a80f8);
}
}
