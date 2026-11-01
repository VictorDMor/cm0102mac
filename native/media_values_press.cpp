// The press items the values block posts about a player and a club: each builds a numbered record
// on its stack (kind 0xbdb, 0xbcf, 0xbdc) and posts it to the shared queue.
#include "media_leaves.hpp"
#include "media_records.hpp"
#include "media_values_detail.hpp"

namespace cm::aot::media {
using namespace detail;
using namespace detail::records;

// 0x10649900: item 0xbdb about `player` and `club`. Draws a mod-4 flag for some kinds.
void post_item_bdb(Cpu& c, std::uint32_t player, std::uint32_t club, std::uint32_t kind_register) {
    Frame frame(c, 0x10649900, 0x140, {.first_gpr = 25});
    const auto rng = Stream::world(c);
    keep(c, {{25, player}, {26, kind_register}, {27, rng.seed}, {28, rng.start}, {29, rng.end}, {30, rng.cursor}, {31, club}});
    const Person about{c, player};
    if (!nonzero(c, std::int32_t(player))) return;
    if (!nonzero_logical(c, about.player())) return;
    if (!nonzero_logical(c, club)) return;
    const Item item{c, frame.at(0x38)};
    item.begin(0xbdb, 0x10649954);
    item.put(0, about.id(), 0x10649964);
    item.put(1, about.first_name(), 0x10649978);
    item.put(2, about.second_name(), 0x1064998c);
    item.put(3, about.common_name(), 0x106499a0);
    // The club is nonnull here, so the original's -1 branch for field 4 never runs.
    compare_logical(c, club);
    item.put(4, Club{c, club}.id(), 0x106499b8);
    const auto kind = half_word(kind_register);
    item.put(5, std::uint32_t(kind), 0x106499dc);
    item.put(6, 0, 0x106499ec);
    // The compiler's decision tree over the kind: it draws for 0, 2-5, 13, 14, 17 and 35.
    const bool draws = [&] {
        if (compare(c, kind, 0xf) >= 0) {
            const auto order = compare(c, kind, 0x23);
            if (order != 0) return order < 0 && compare(c, kind, 0x11) == 0;
            return true;
        }
        const auto order = compare(c, kind, 1);
        if (order == 0) return false;
        if (order < 0) return compare(c, kind, 0) >= 0;
        if (compare(c, kind, 0xd) >= 0) return true;
        return compare(c, kind, 6) < 0;
    }();
    std::uint32_t flag = 0;
    if (draws) {
        rng.advance(0x10649a5c);
        flag = std::uint32_t(std::int32_t(rng.sample()) % 4);  // the original's shift trick for % 4
    }
    item.put(7, flag, 0x10649aa0);
    item.post(club, 0x10649ab4);
}

// 0x1067d040: item 0xbcf for `player` and `club` (only when 0x1014c2e0 accepts the club) with a
// tier (1 above rating 4250, else 2) and a rank of 1 to 3.
void post_item_bcf(Cpu& c, std::uint32_t player, std::uint32_t club, std::uint32_t kind_register, std::uint32_t extra_register) {
    Frame frame(c, 0x1067d040, 0x140, {.first_gpr = 27});
    keep(c, {{27, player}, {28, club}, {29, kind_register}, {30, extra_register}});
    Kept<31> today_slot(c, c.toc(tables::today));
    const Person about{c, player};
    if (!nonzero(c, std::int32_t(player))) return;
    if (!nonzero_logical(c, about.player())) return;
    if (!nonzero_logical(c, club)) return;
    Program{c}.call(0x1014c2e0, 0x1067d094, {club, 0, 1, 0, 0});
    if (!nonzero(c, byte_word(c.r(3)))) return;
    const Item item{c, frame.at(0x38)};
    item.begin(0xbcf, 0x1067d0ac);
    item.put(0, about.id(), 0x1067d0bc);
    item.put(1, about.first_name(), 0x1067d0d0);
    item.put(2, about.second_name(), 0x1067d0e4);
    item.put(3, about.common_name(), 0x1067d0f8);
    item.put(4, Club{c, club}.id(), 0x1067d108);
    const auto kind = byte_word(kind_register);
    // Rank 3 for an established player of kind 32, 1 or 30 whose distance (0x10718130) exceeds 210, else 2;
    // rank 1 when both ability bytes are low. The two rating tiers differ in which bytes they test.
    const auto distant = [&](std::uint32_t back) {
        if (!(compare(c, kind, 0x20) == 0 || compare(c, kind, 1) == 0 || compare(c, kind, 0x1e) == 0)) return false;
        if (compare(c, about.loyalty(), 0xf) <= 0 || compare(c, about.sportsmanship(), 0xa) <= 0) return false;
        const auto ahead = Program{c}.call(0x10718130, back, {today_slot, about.joined_first().get(), about.joined_second().get()});
        return compare(c, std::int32_t(ahead), 0xd2) > 0;
    };
    std::uint32_t tier, rank;
    if (compare(c, about.player_record().reputation(), 0x109a) > 0) {
        tier = 1;
        if (compare(c, about.ambition(), 0xf) > 0 && compare(c, about.trait_69(), 6) < 0) rank = 1;
        else rank = distant(0x1067d198) ? 3 : 2;
    } else {
        tier = 2;
        if (compare(c, about.sportsmanship(), 0xa) < 0 && compare(c, about.loyalty(), 6) < 0) rank = 1;
        else rank = distant(0x1067d238) ? 3 : 2;
    }
    today_slot = rank;  // r31 is reused for the rank
    item.put(5, tier, 0x1067d264);
    item.put(6, rank, 0x1067d274);
    item.put(7, std::uint32_t(kind), 0x1067d284);
    item.put(8, std::uint32_t(half_word(extra_register)), 0x1067d294);
    Kept<27> wanted(c, 0);
    const auto accepted = Program{c}.call(0x100d3150, 0x1067d2a4, {player, 0});
    const bool flag = nonzero(c, byte_word(accepted));
    if (flag) wanted = 1;
    item.put(9, flag, 0x1067d2c0);
    item.post(club, 0x1067d2d4);
}

// 0x1067d830: item 0xbdc about `player` and `club`: a tier 0-2 from the player's standing against
// the club (0x1007c140, 0x10718130), the club's rating against 4000, and one post of the item per
// person 0x103fc710 lists for the player.
void post_item_bdc(Cpu& c, std::uint32_t player, std::uint32_t club, std::uint32_t figure, std::uint32_t grade_register) {
    Frame frame(c, 0x1067d830, 0x1c0, {.first_gpr = 23});
    keep(c, {{23, player}, {24, club}, {25, figure}, {26, grade_register}});
    const Kept<29> lists_slot(c, c.toc(-0x7b40));
    const Kept<30> contracts_slot(c, c.toc(-0x7b7c));
    const Kept<31> log_slot(c, c.toc(-0x7bf0));
    const Person about{c, player};
    const Club home{c, club};
    if (!nonzero(c, std::int32_t(player))) return;
    if (!nonzero_logical(c, about.player())) return;
    if (!nonzero_logical(c, club)) return;
    if (!nonzero_logical(c, home.manager())) return;
    const Kept<27> contract_at(c, Program{c}.call(0x10082f90, 0x1067d890, {c.u32(contracts_slot), player, club}));
    if (!nonzero(c, std::int32_t(contract_at))) return;
    const Contract contract{c, contract_at};
    const Item item{c, frame.at(0xb0)};
    item.begin(0xbdc, 0x1067d8a8);
    // r28: a young, strong player of a club that is in one of the flagged states.
    bool is_young = false;
    if (nonzero(c, std::int32_t(contract.flags() & 0x2000)) || nonzero(c, std::int32_t(contract.flags() & 8)))
        is_young = compare(c, about.sportsmanship(), 0xa) < 0 && compare(c, about.trait_69(), 0xa) > 0 &&
                   compare(c, about.loyalty(), 0xf) < 0 && compare(c, about.age(), 0x17) > 0;
    const Kept<28> young(c, is_young);
    item.put(0, about.id(), 0x1067d91c);
    item.put(1, about.first_name(), 0x1067d930);
    item.put(2, about.second_name(), 0x1067d944);
    item.put(3, about.common_name(), 0x1067d958);
    // The club is nonnull here, so the original's -1 branch for field 4 never runs.
    compare_logical(c, club);
    item.put(4, home.id(), 0x1067d970);
    const auto owner_rating = [&] { return std::int32_t(about.player_record().reputation()); };
    std::uint32_t tier = 0;
    const auto grade = byte_word(grade_register);
    if (compare(c, owner_rating(), 0x109a) > 0 && compare(c, owner_rating(), home.reputation()) > 0) {
        const auto standing = [&](std::uint32_t back) {
            return byte_word(Program{c}.call(0x1007c140, back, {c.u32(contracts_slot), player, contract_at}));
        };
        if ((compare(c, standing(0x1067d9b8), 1) == 0 || compare(c, standing(0x1067d9d4), 2) == 0) && compare(c, about.age(), 0x16) > 0) {
            if (compare(c, about.loyalty(), 0xa) < 0 && compare(c, about.ambition(), 0xf) > 0 && compare(c, about.trait_69(), 6) < 0) {
                // Young needs +0x69 above 10 and this branch +0x69 below 6, so the original's tier 3 (young
                // and grade 2) never happens: only its test of r28 remains.
                compare(c, byte_word(young));
                tier = 1;
            } else {
                const auto distance = Program{c}.call(0x10718130, 0x1067da54, {c.toc(tables::today), about.joined_first().get(), about.joined_second().get()});
                if (compare(c, std::int32_t(distance), 0x3e8) > 0 && compare(c, about.loyalty(), 0xf) > 0 && !nonzero(c, std::int32_t(contract.flags() & 0x181))) {
                    // Young needs +0x65 below 15 and this branch +0x65 above 15, so the original's tier 4 never
                    // happens either: only its test of r28 remains.
                    compare(c, byte_word(young));
                    tier = 2;
                }
            }
        }
    }
    item.put(5, tier, 0x1067dab8);
    item.put(6, std::uint32_t(grade), 0x1067dac8);
    // Field 7: whether the club's rating (its row of the ratings table) is below 4000, by the
    // compiler's bit trick for a < b. It leaves 4000 in r6.
    compare_logical(c, club);
    const auto rating = std::int32_t(home.table_rating());
    const std::int32_t limit = 0xfa0, difference = rating ^ limit;
    const auto below = (std::uint32_t((difference >> 1) - (difference & limit)) >> 31) & 1;
    leave(c, {{6, 0xfa0}});
    item.put(7, std::uint32_t(byte_word(below)), 0x1067db24);
    item.put(8, figure, 0x1067db34);
    const auto manager = home.manager_record();
    item.put(9, manager.id(), 0x1067db48);
    item.put(10, manager.first_name(), 0x1067db60);
    item.put(11, manager.second_name(), 0x1067db78);
    item.put(12, manager.common_name(), 0x1067db90);
    const auto list = frame.at(0x38);
    std::uint32_t count = 0;
    if (nonzero_logical(c, c.u32(lists_slot))) count = Program{c}.call(0x103fc710, 0x1067dbb4, {c.u32(lists_slot), player, 0x21a, list, 0x1e});
    else c.set(3, 0);
    Kept<23> listed(c, count);
    Kept<24> posted(c, 0);
    for (;;) {
        // extsb r3,r24 at every test: the loop leaves the routine's result (the sign-extended count of posts).
        c.set(3, std::uint32_t(byte_word(posted)));
        if (compare(c, byte_word(posted), byte_word(listed)) >= 0) break;
        const auto person = c.u32(list + (std::uint32_t(byte_word(posted)) << 2));
        item.post(person, 0x1067dbe4);
        posted = posted + 1;
    }
}

// 0x106548e0: item 0xbda about `person` and `club`; `kind` is picked by draws from the person's
// state, and a draw may also raise the person's morale byte (the player record's +0x44).
void post_item_bda(Cpu& c, std::uint32_t query, std::uint32_t person, std::uint32_t club, std::uint32_t kind_register, std::uint32_t extra_register) {
    Frame frame(c, 0x106548e0, 0x150, {.first_gpr = 22});
    const auto rng = Stream::world(c);
    keep(c, {{22, query}, {24, extra_register}, {26, rng.start}, {27, rng.end}, {28, person}, {29, rng.seed}, {30, rng.cursor}, {31, club}});
    Kept<23> kind_kept(c, kind_register);
    const Person about{c, person};
    const Club home{c, club};
    if (!nonzero(c, std::int32_t(person))) return;
    if (!nonzero_logical(c, about.player())) return;
    if (!nonzero_logical(c, club)) return;
    if (!nonzero_logical(c, home.manager())) return;
    const Kept<25> contract_at(c, Program{c}.call(0x10082f90, 0x1065494c, {c.u32(c.toc(-0x7b7c)), person, club}));
    if (!nonzero(c, std::int32_t(contract_at))) return;
    const Contract contract{c, contract_at};
    const Item item{c, frame.at(0x38)};
    item.begin(0xbda, 0x10654964);
    item.put(0, about.id(), 0x10654974);
    item.put(1, about.first_name(), 0x10654988);
    item.put(2, about.second_name(), 0x1065499c);
    item.put(3, about.common_name(), 0x106549b0);
    // The club is nonnull here, so the original's -1 branch for field 4 never runs.
    compare_logical(c, club);
    item.put(4, home.id(), 0x106549c8);
    enter(c, 0x106549e8, {query, person});
    const auto known = lists_kind_two(c, query, person);
    // A kind that can turn into 18 or 33 by a draw against the person's +0x69 byte.
    if (nonzero(c, byte_word(known)) || nonzero(c, std::int32_t(contract.flags() & 0x2000)) || nonzero(c, std::int32_t(contract.flags() & 8))) {
        const auto kind = half_word(kind_kept);
        if (kind == 0xe || kind == 0 || kind == 2 || kind == 9 || kind == 0x23 || kind == 4 || kind == 7) {
            rng.advance(0x10654a78);
            const auto x = std::int32_t(rng.sample());
            kind_kept = about.trait_69() < x % 10 ? 0x21 : 0x12;
        }
    }
    const auto spoken = Program{c}.call(0x1018c340, 0x10654adc, {c.u32(c.toc(-0x7ba0)), person});
    if (nonzero_logical(c, spoken)) kind_kept = 0xc;
    const auto kind = half_word(kind_kept);
    item.put(5, std::uint32_t(kind), 0x10654af8);
    item.put(6, std::uint32_t(byte_word(c.r(24))), 0x10654b08);
    // The value of field 7 by kind: the remainder of a draw (modulo 2, 3, 4, 5 or 10), or 0.
    const auto draw = [&](std::uint32_t back) {
        rng.advance(back);
        return std::int32_t(rng.sample());
    };
    std::uint32_t value = 0;
    switch (kind) {
    case 0: case 6: case 0xe: case 0x11: {
        const auto record = about.player_record();
        if (nonzero_logical(c, about.player()) && record.morale() < 0xa) {
            value = std::uint32_t(byte_word(std::uint32_t(draw(0x10654d70) % 3 + 2)));
        } else {
            const auto rest = draw(0x10654dd4) % 5;
            value = std::uint32_t(rest);
            // Morale rises on a draw of 0 or 1 while it is below 20.
            if (nonzero(c, rest) && compare(c, rest, 1) != 0) break;
            if (!nonzero_logical(c, about.player())) break;
            if (compare(c, about.player_record().morale(), 0x14) >= 0) break;
            record.morale() = std::int8_t(record.morale() + 1);
        }
        break;
    }
    case 2: case 0x1e: value = std::uint32_t(draw(kind == 2 ? 0x10654c9c : 0x10654e70) % 2); break;
    case 4: value = std::uint32_t(draw(0x10654cec) % 3); break;
    case 5: case 7: value = std::uint32_t(draw(0x10654bec) % 4); break;
    case 9: value = std::uint32_t(draw(0x10654c40) % 3); break;
    case 0x21: case 0x22: case 0x25: value = std::uint32_t(draw(0x10654ec0) % 3); break;
    default: break;  // 0xd and 0x23 and the rest draw nothing
    }
    item.put(7, value, 0x10654f14);
    const auto manager = home.manager_record();
    item.put(8, manager.id(), 0x10654f28);
    item.put(9, manager.first_name(), 0x10654f40);
    item.put(10, manager.second_name(), 0x10654f58);
    item.put(11, manager.common_name(), 0x10654f70);
    item.put(12, 0x420e9, 0x10654f84);
    if (nonzero_logical(c, home.deputy())) {
        const auto deputy = home.deputy_record();
        item.put(13, deputy.id(), 0x10654fa4);
        item.put(14, deputy.first_name(), 0x10654fbc);
        item.put(15, deputy.second_name(), 0x10654fd4);
        item.put(16, deputy.common_name(), 0x10654fec);
    } else {
        item.put(13, 0xffffffffu, 0x10655000);
    }
    item.post(club, 0x10655014);
}
}
