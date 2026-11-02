// 0x1069ed10: story 0x25 about a member of `club`'s squad who is unsettled there and shares the
// region of `person`. Walks the club's 50 squad slots; each member who qualifies draws two rolls
// (against his own ability bytes and the club's region tier) and, when the first is lower, gets the
// story posted (once per call) and the contract flag 0x800.
#include "media_pick_a.hpp"
#include "media_pick_a_detail.hpp"
#include "media_pick_b.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::pick;

namespace {
namespace callee {
constexpr std::uint32_t region_of = 0x100db1b0,   // (person): the region the person belongs to
    region_tier = 0x100db120,                     // (club, region): signed byte
    contract_with = 0x10082f90,                   // (contracts, person, club)
    link_kind = 0x10187680,                       // (object, person): signed byte
    staff_test = 0x100d3150,                      // (person, club): signed byte
    contract_flags = 0x1037fa70, set_contract_flag = 0x1037f8a0;
}
namespace slots {
constexpr int contracts = -0x7b7c, link_object = -0x7ba0, press = -0x7b94, tiers = -0x7b5c;
}
constexpr std::uint32_t squad_first = 0x98, squad_slots = 50;
constexpr std::uint32_t story = 0x25, unsettled_flag = 0x800, unhappy_flags = 0x101;
}

void post_squad_story(Cpu& c, std::uint32_t person, std::uint32_t club) {
    Frame frame(c, 0x1069ed10, 0xa0, {.first_gpr = 15});
    const auto rng = Stream::world(c);
    const Program program{c};
    keep(c, {{15, c.toc(slots::contracts)}, {17, c.toc(slots::link_object)}, {19, c.toc(slots::press)}, {20, rng.end}, {21, rng.start},
             {22, 0}, {23, rng.seed}, {28, c.toc(slot::floats)}, {29, rng.cursor}, {30, person}, {31, club}});
    if (!nonzero(c, std::int32_t(person))) return;
    if (!nonzero_logical(c, Person{c, person}.player())) return;
    if (!nonzero_logical(c, club)) return;
    const auto region = program.call(callee::region_of, 0x1069ed70, {person});
    const auto tier_word = program.call(callee::region_tier, 0x1069ed7c, {club, region});
    keep(c, {{27, tier_word}});
    const auto tier = byte_word(tier_word);
    if (compare(c, tier, 2) <= 0) return;
    const auto link = Club{c, club}.division().get();
    if (!nonzero_logical(c, link)) return;
    // The club's link leads to a table entry that has to be nonzero.
    const auto table = c.u32(c.toc(slots::tiers));
    leave(c, {{3, table}, {4, c.toc(slots::tiers)}});
    if (!nonzero_logical(c, c.u32(table + c.u32(link) * 4))) return;
    const auto contract = Person{c, person}.player();
    c.set(3, contract);
    if (compare(c, Person{c, person}.player_record().reputation(), 0x128e) <= 0) return;

    for (std::uint32_t slot = 0;; ++slot) {
        keep(c, {{26, slot}});
        if (compare(c, std::int32_t(slot), std::int32_t(squad_slots)) >= 0) return;
        c.set(3, club + slot * 4);
        const auto member = c.u32(club + slot * 4 + squad_first);
        keep(c, {{25, member}});
        if (!nonzero_logical(c, member)) continue;
        const Person who{c, member};
        if (!nonzero_logical(c, who.player())) continue;
        if (member == person) continue;
        const auto standing = program.call(callee::contract_with, 0x1069ee08, {c.u32(c.r(15)), member, club});
        keep(c, {{24, standing}});
        if (!nonzero(c, std::int32_t(standing))) continue;
        const Contract deal{c, standing};
        if (compare(c, deal.happiness().get()) <= 0) continue;
        const auto kind = program.call(callee::link_kind, 0x1069ee3c, {c.u32(c.r(17)), person});
        if (compare(c, byte_word(kind), 2) == 0) continue;
        if (nonzero(c, std::int32_t(deal.flags() & unhappy_flags))) continue;
        if (nonzero(c, byte_word(program.call(callee::staff_test, 0x1069ee60, {member, club})))) continue;
        // Quirk: the halfword at +0x5c is tested through its low byte (extsb after lha).
        if (compare(c, who.age(), 0x14) <= 0) continue;
        const auto mode = deal.squad_status() & 0xf;
        if (!(compare_logical(c, mode, 1) == 0 || compare_logical(c, mode, 2) == 0 || compare_logical(c, mode, 3) == 0)) continue;
        const std::uint8_t low = who.professionalism().get(), high = who.sportsmanship().get();
        leave(c, {{3, std::uint32_t(byte_word(low))}, {4, low}});
        if (compare(c, byte_word(low), tier + 5) >= 0) continue;
        leave(c, {{3, std::uint32_t(byte_word(high))}, {4, high}});
        if (compare(c, byte_word(high), tier + 10) >= 0) continue;
        // The first roll is against his own ability, the second against the tier (extsb. r27 is
        // never zero here: the tier is above 2).
        keep(c, {{16, std::uint32_t(byte_word(low))}});
        const auto own = draw_narrow(c, rng, byte_word(low), 0x1069ef0c, 18);
        const auto regional = draw_narrow(c, rng, tier, 0x1069f078, 16);
        // The second site leaves n, the sample and the cursor in r3-r5.
        leave(c, {{4, c.r(3)}, {3, std::uint32_t(tier)}, {5, c.u32(rng.cursor)}});
        if (compare(c, own, regional) >= 0) continue;
        const auto member_region = program.call(callee::region_of, 0x1069f1e0, {member});
        keep(c, {{16, member_region}});
        if (compare(c, std::int32_t(program.call(callee::region_of, 0x1069f1ec, {person})), std::int32_t(member_region)) != 0) continue;
        const auto member_contract = who.player();
        c.set(3, member_contract);
        if (compare(c, who.player_record().reputation(), 0xea6) <= 0) continue;
        if (!nonzero(c, byte_word(c.r(22)))) {
            typed_call(c, 0x1069f228, post_item_bd0, c.u32(c.r(19)), member, club, story, 0, person);
            keep(c, {{22, 1}});
        }
        const auto flags = program.call(callee::contract_flags, 0x1069f234, {standing});
        if (nonzero(c, std::int32_t(flags & unsettled_flag))) continue;
        program.call(callee::set_contract_flag, 0x1069f248, {standing, unsettled_flag});
    }
}
}
