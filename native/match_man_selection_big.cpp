// MATCH_MAN.CPP selection block, 0x1021a240 (match_man_selection.hpp): replaces the holders of places that a
// rule rules out, with the best of the people left.
//
// Like 0x10210c50 the original keeps its loop variables in r13-r31 and in a 0x510-byte frame whose local arrays
// (flags, ratings, slots and roles of the places that need a new holder) are part of the stack image the game
// reads. Each non-volatile register keeps the original's value (Kept) and every local is written where the
// original writes it. Offsets and registers are the original's.
#include "match_man_selection.hpp"
#include "match_man_selection_detail.hpp"
#include "match_man_adapter.hpp"
#include "guest_values.hpp"

namespace cm::aot::match_man {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::Field;
using guest::Frame;
using guest::Kept;
using guest::nonzero_logical;

constexpr std::int32_t s8(std::uint32_t value) { return std::int8_t(value); }
constexpr std::uint32_t reg(std::int32_t value) { return std::uint32_t(value); }

constexpr std::uint32_t entry_point = 0x1021a240;
constexpr std::uint32_t places_entry = 0x10210c50, place_rating_entry = 0x10211e70;
// 0x100cab10 and 0x100ca6a0 (DATABASE.CPP): the two tests the rule applies to a person, with the club's value at +0x2c.
constexpr std::uint32_t test_one = 0x100cab10, test_two = 0x100ca6a0;
constexpr std::uint32_t club_value = 0x2c;
constexpr std::int32_t no_rating = -1000000;  // lis -0xf, subi 0x4240: a holder with nobody to take its place

using Word = Field<std::uint32_t>;
using Byte = Field<std::uint8_t>;
// The return addresses of the database calls of one of the four places the rule is written out.
struct RuleSite { std::uint32_t first, second, only; };
}

void refill_places(Cpu& c, std::uint32_t club_argument, std::uint32_t judge_argument, std::uint32_t config_argument,
                   std::uint32_t mode_argument, std::uint32_t places_argument, std::uint32_t pool_count_argument,
                   std::uint32_t count_argument, std::uint32_t holders_argument) {
    Frame frame(c, entry_point, 0x510, {.first_gpr = 13});
    const auto at = [&](std::uint32_t offset) { return frame.at(std::int32_t(offset)); };
    Kept<13> r13(c, c.r(13));      // the holders of the places, then a pointer into them
    Kept<14> club(c, c.r(14));
    Kept<15> judge(c, c.r(15));
    Kept<16> config(c, c.r(16));   // six bytes: the rule's kind, a limit, and counts
    Kept<17> mode(c, c.r(17));
    Kept<18> candidate_count(c, c.r(18));
    Kept<19> candidates(c, c.r(19));
    Kept<20> chosen(c, c.r(20));
    Kept<21> side(c, c.r(21));
    Kept<22> r22(c, c.r(22));
    Kept<23> r23(c, c.r(23));
    Kept<24> r24(c, c.r(24));
    Kept<25> r25(c, c.r(25));
    Kept<26> r26(c, c.r(26));
    Kept<27> r27(c, c.r(27));
    Kept<28> r28(c, c.r(28));
    Kept<29> r29(c, c.r(29));
    Kept<30> r30(c, c.r(30));
    Kept<31> slots(c, c.r(31));
    // Stack arguments (the caller's frame, +0x548 on) and the locals.
    Word pool(c, at(0x548)), candidates_argument(c, at(0x54c)), chosen_argument(c, at(0x550)), flags_argument(c, at(0x554)),
        ratings_argument(c, at(0x558)), slots_argument(c, at(0x55c)), roles_argument(c, at(0x560)), side_argument(c, at(0x564));
    Byte places_byte(c, at(0x53b)), pool_count_byte(c, at(0x53f));
    Byte changed(c, at(0x4a8)), holes(c, at(0x4a9)), wanted_first(c, at(0x4aa)), wanted_second(c, at(0x4ab)), holes_second(c, at(0x4ac));
    Word places_extended(c, at(0x4b0)), holes_extended(c, at(0x4b4)), pool_extended(c, at(0x4b8)), first_places_extended(c, at(0x4bc));
    // The local arrays: bytes at 0x58 (which place each hole is) and 0x8c (its role), halfwords at 0xc0 (rating) and
    // 0x124 (flags), words at 0x188 (people to choose from) and 0x250 (the choices), 8-byte slots at 0x318.
    const std::uint32_t hole_place = at(0x58), hole_role = at(0x8c), hole_rating = at(0xc0), hole_flags = at(0x124),
                        people_list = at(0x188), choices = at(0x250), hole_slot = at(0x318);

    // The rule of the config's first byte for `person`: kind 1 rules out one that passes the first test and fails the
    // second, kind 2 one that passes the first, kind 3 one older than the config's byte 1, other kinds none.
    // `null_first`: the person was tested for null before the rule (the pool loop), not inside each kind.
    const auto ruled_out = [&](std::uint32_t person, const RuleSite& site, bool null_first = false) {
        const std::int32_t kind = s8(c.u8(config));
        const auto first_test = [&](std::uint32_t back) {
            c.set({{4, c.u32(club + club_value)}, {3, person}, {5, 1}});
            c.call(test_one, back);
            return compare(c, s8(c.r(3))) != 0;
        };
        const auto present = [&] { return null_first || nonzero_logical(c, person); };
        compare(c, kind, 2);
        if (kind == 2) return present() && first_test(site.only);
        if (kind > 2) {
            if (compare(c, kind, 4) >= 0) return false;
            if (!present()) return false;
            const std::int32_t age = s8(reg(c.i16(person + selection_person::age)));
            return compare(c, age, s8(c.u8(config + 1))) > 0;
        }
        if (compare(c, kind, 1) < 0) return false;
        if (!present() || !first_test(site.first)) return false;
        c.set({{4, c.u32(club + club_value)}, {3, person}});
        c.call(test_two, site.second);
        return compare(c, s8(c.r(3))) == 0;
    };
    // The rating of `person` for a place (0x10211e70): the place's flags and rating halfwords, and (when `entry_at` is
    // given) its slot words and role; the extra word is `extra` (the side's value in the first phase, 0 in the second).
    const auto rate = [&](std::uint32_t back, std::uint32_t person, std::uint32_t flag_at, std::uint32_t rating_at, std::uint32_t entry_at,
                          std::uint32_t role_at, std::uint32_t extra) {
        c.put32(at(0x38), 0xffffffffu);
        const auto first = entry_at ? c.u32(entry_at) : 0;
        const auto second = entry_at ? c.u32(entry_at + 4) : 0;
        c.put32(at(0x40), second);
        c.put32(at(0x3c), first);
        c.put32(at(0x44), entry_at ? c.u8(role_at) : 0);
        c.put32(at(0x48), extra);
        c.put32(at(0x4c), reg(c.i16(rating_at)));
        c.put32(at(0x50), 0);
        c.set({{3, person}, {4, c.u32(person + person::player)}, {5, 0}, {6, reg(c.i16(flag_at))}, {7, mode}, {8, club}, {9, side}, {10, judge}});
        match_man::call(c, place_rating_entry, back);
        return std::int32_t(c.r(3));
    };
    // The call of 0x10210c50 that chooses people for the collected holes.
    const auto choose = [&](std::uint32_t list, std::uint32_t count, std::uint32_t flags_at, std::uint32_t primary, std::uint32_t chosen_at,
                            std::uint32_t slots_at, std::uint32_t roles_at, std::uint32_t ratings_at, std::uint32_t weights, std::uint32_t back,
                            std::uint32_t leftover) {
        c.put32(at(0x38), side);
        c.put32(at(0x3c), weights);
        c.put32(at(0x40), 0);
        c.put32(at(0x44), slots_at);
        c.put32(at(0x48), roles_at);
        c.put32(at(0x4c), leftover);
        c.put32(at(0x50), ratings_at);
        c.set({{3, list}, {4, count}, {5, flags_at}, {6, primary}, {7, 0}, {8, chosen_at}, {9, mode}, {10, club}});
        match_man::call(c, places_entry, back);
    };
    // ---- Prologue.
    club = club_argument;
    judge = judge_argument;
    config = config_argument;
    mode = mode_argument;
    candidate_count = count_argument;
    r13 = holders_argument;
    r24 = 0;
    const std::int32_t kind = s8(c.u8(config));
    candidates = candidates_argument.get();
    compare(c, kind);
    chosen = chosen_argument.get();
    wanted_first = 0;
    slots = slots_argument.get();
    places_byte = places_argument;
    side = side_argument.get();
    pool_count_byte = pool_count_argument;
    changed = 0;
    if (kind == 0) { c.set(3, club_argument); return; }
    const std::int32_t at_four = s8(c.u8(config + 4)), at_five = s8(c.u8(config + 5));
    c.set(3, reg(at_four));
    if (compare(c, at_four, at_five) <= 0) return;
    if (nonzero_logical(c, side)) {
        c.set(3, c.u32(side + 0x30));
        r24 = c.u32(c.r(3) + 0x588);
    }

    // ---- People the rule rules out leave the candidates.
    r25 = 0;
    for (;;) {
        c.set(3, reg(s8(r25)));
        if (compare(c, s8(r25), s8(candidate_count)) >= 0) break;
        if (!nonzero_logical(c, candidates)) break;
        const std::uint32_t at_bytes = reg(s8(r25)) << 2;
        r22 = at_bytes;
        r23 = 0;
        r26 = c.u32(candidates + at_bytes);
        if (ruled_out(r26, {0x1021a334, 0x1021a348, 0x1021a370})) r23 = 1;
        if (compare(c, s8(r23)) != 0) {
            const std::int32_t count = s8(candidate_count);
            c.set({{4, reg(count)}, {3, reg(s8(r25))}});
            if (compare(c, s8(r25), count - 1) < 0) {
                const std::uint32_t last = reg(count) << 2;
                c.set({{3, last}, {4, last - 4}});
                candidate_count = candidate_count - 1;
                const auto moved = c.u32(candidates + (last - 4));
                c.set(3, moved);
                r25 = r25 - 1;
                c.put32(candidates + r22, moved);
                c.put32(candidates + (last - 4), 0);
            } else {
                candidate_count = candidate_count - 1;
                c.put32(candidates + r22, 0);
            }
        }
        r25 = r25 + 1;
    }

    // ---- How many places are wanted from each group (a: those the first people fill, b: the second).
    const std::int32_t at_two = s8(c.u8(config + 2));
    const std::uint32_t byte_five = c.u8(config + 5);
    c.set({{5, c.u8(config + 2)}, {4, byte_five}});
    c.set(3, reg(at_two));
    if (compare(c, at_two, s8(byte_five)) <= 0) {
        const std::uint32_t four = c.u8(config + 4);
        wanted_second = reg(s8(four - byte_five));
    } else {
        wanted_first = reg(s8(c.u8(config + 2) - byte_five));
        c.set(3, c.u8(config + 3));
        wanted_second = reg(s8(c.u8(config + 3)));
    }

    // ---- The first group: holders of the first places that the rule rules out leave a hole each.
    if (compare(c, s8(wanted_first)) != 0) {
        places_extended = reg(s8(places_byte));
        r28 = 0;
        r23 = flags_argument.get();
        r25 = r28;
        r22 = ratings_argument.get();
        r26 = r28;
        r27 = r28;
        holes = 0;
        for (;;) {
            c.set(3, reg(s8(r28)));
            if (compare(c, s8(r28), std::int32_t(places_extended.get())) >= 0) break;
            r29 = 0;
            r30 = c.u32(r13);
            if (ruled_out(r30, {0x1021a4d8, 0x1021a4ec, 0x1021a514})) r29 = 1;
            if (compare(c, s8(r29)) != 0) {
                const std::uint32_t where = reg(s8(holes));
                c.put16(hole_flags + r25, c.u16(r23));
                c.put8(hole_place + where, r28);
                const bool rated = nonzero_logical(c, ratings_argument.get());
                c.put16(hole_rating + r25, rated ? c.u16(r22) : 0);
                std::uint32_t slot_first = 0, slot_second = 0;
                if (nonzero_logical(c, slots)) {
                    slot_first = c.u32(slots + r26);
                    slot_second = c.u32(slots + r26 + 4);
                }
                c.put32(hole_slot + r27 + 4, slot_second);
                c.put32(hole_slot + r27, slot_first);
                const bool roled = nonzero_logical(c, roles_argument.get());
                const std::uint32_t role = roled ? reg(s8(c.u8(roles_argument.get() + reg(s8(r28))))) : 0;
                r25 = r25 + 2;
                c.put8(hole_role + where, role);
                r27 = r27 + 8;
                c.set({{3, holes.get() + 1u}, {4, role}, {5, slot_first}, {6, slot_second}});
                holes = holes + 1;
            }
            r13 = r13 + 4;
            r22 = r22 + 2;
            r23 = r23 + 2;
            r26 = r26 + 8;
            r28 = r28 + 1;
        }

        // The people to choose from: the pool's that the rule leaves, then the candidates.
        r25 = 0;
        r26 = pool.get();
        r13 = 0;
        r27 = reg(s8(pool_count_byte));
        for (;;) {
            if (compare(c, s8(r13), s8(r27)) >= 0) break;
            r23 = c.u32(r26);
            r22 = 0;
            // A null pool entry is skipped altogether (nothing is stored for it).
            if (nonzero_logical(c, r23)) {
                if (ruled_out(r23, {0x1021a684, 0x1021a698, 0x1021a6b8}, true)) r22 = 1;
                if (compare(c, s8(r22)) == 0) {
                    const std::uint32_t at_bytes = reg(s8(r25)) << 2;
                    r25 = r25 + 1;
                    c.put32(people_list + at_bytes, r23);
                    c.set(3, people_list);
                }
            }
            r26 = r26 + 4;
            r13 = r13 + 1;
        }
        std::uint32_t listed = candidates, passed = 0;
        c.set({{3, candidates}, {5, people_list}, {7, 0}});
        for (;;) {
            c.set(4, reg(s8(passed)));
            if (compare(c, s8(passed), s8(candidate_count)) >= 0 || !nonzero_logical(c, candidates)) break;
            const std::uint32_t at_bytes = reg(s8(r25)) << 2;
            const auto value = c.u32(listed);
            c.put32(people_list + at_bytes, value);
            ++passed;
            r25 = r25 + 1;
            listed += 4;
            c.set({{4, at_bytes}, {6, value}, {7, passed}, {3, listed}});
        }
        if (compare(c, s8(r25)) != 0) {
            choose(people_list, r25, hole_flags, holes, choices, hole_slot, hole_role, hole_rating, judge, 0x1021a7b4, r24);
            r13 = choices;
            r23 = 0;
            pool_extended = reg(s8(pool_count_byte));
            holes_extended = reg(s8(holes));
            while (compare(c, s8(wanted_first)) != 0 && compare(c, s8(holes)) != 0) {
                r25 = 0;
                r22 = reg(-1);
                r30 = r25;
                r29 = r25;
                r28 = r25;
                for (;;) {
                    c.set(3, reg(s8(r25)));
                    if (compare(c, s8(r25), std::int32_t(holes_extended.get())) >= 0) break;
                    r27 = reg(s8(r25));
                    const std::int32_t index = s8(c.u8(hole_place + r27));
                    if (compare(c, index, -1) != 0) {
                        const auto holder = c.u32(chosen + (reg(index) << 2));
                        c.set(3, holder);
                        if (nonzero_logical(c, holder)) {
                            r26 = hole_slot + r29;
                            const auto flags_at = hole_flags + r30, rating_at = hole_rating + r30, role_at = hole_role + r27;
                            // The holder's rating for its own hole (kept in r31), against the one the new pick would get.
                            slots = reg(rate(0x1021a884, holder, flags_at, rating_at, r26, role_at, r24));
                            c.set(4, choices);
                            const auto replaced = c.u32(choices + r28);
                            c.set(3, replaced);
                            std::int32_t other = no_rating;
                            if (nonzero_logical(c, replaced)) other = rate(0x1021a8f8, replaced, flags_at, rating_at, r26, role_at, r24);
                            c.set(3, reg(other));
                            if (compare(c, std::int32_t(r22), -1) == 0 || compare(c, std::int32_t(slots) - other, std::int32_t(r23)) < 0) {
                                r22 = r27;
                                r23 = slots - reg(other);
                            }
                        }
                    }
                    r30 = r30 + 2;
                    r29 = r29 + 8;
                    r28 = r28 + 4;
                    r25 = r25 + 1;
                }
                if (compare(c, std::int32_t(r22), -1) <= 0) break;
                const std::uint32_t at_bytes = reg(std::int32_t(r22)) << 2;
                const std::uint32_t place = reg(s8(c.u8(hole_place + r22)));
                const std::uint32_t replacement = c.u32(r13 + at_bytes);
                c.put32(chosen + (place << 2), replacement);
                c.set({{3, candidates}, {4, reg(s8(candidate_count))}, {5, place << 2}, {6, replacement}, {7, 0}, {8, 0}});
                // The replacement leaves the candidates (the last one moves into its place) ...
                std::uint32_t walk = candidates, advanced = 0;
                for (std::uint32_t index = 0;; ++index, walk += 4, advanced += 4) {
                    c.set(5, reg(s8(index)));
                    if (compare(c, s8(index), s8(candidate_count)) >= 0) break;
                    c.set({{6, c.u32(walk)}, {5, c.u32(r13 + at_bytes)}});
                    if (c.u32(walk) != c.u32(r13 + at_bytes)) { c.set({{7, advanced + 4}, {3, walk + 4}, {8, index + 1}}); continue; }
                    const std::int32_t count = s8(candidate_count);
                    c.set({{5, reg(count)}, {4, reg(s8(index))}, {3, reg(count) - 1}});
                    if (compare(c, s8(index), count - 1) < 0) {
                        const std::uint32_t last = reg(count) << 2;
                        candidate_count = candidate_count - 1;
                        const auto moved = c.u32(candidates + (last - 4));
                        c.put32(candidates + (reg(s8(index)) << 2), moved);
                        c.put32(candidates + (last - 4), 0);
                        c.set({{3, 0}, {4, reg(s8(index)) << 2}, {5, moved}, {6, last - 4}});
                    } else {
                        c.put32(candidates + (reg(s8(index)) << 2), 0);
                        candidate_count = candidate_count - 1;
                        c.set({{3, reg(s8(index)) << 2}, {4, 0}});
                    }
                    break;
                }
                // ... and from the pool (which then needs choosing again).
                std::uint32_t pooled = pool.get(), moved_on = 0;
                c.set({{8, 0}, {4, pool.get()}, {7, 0}, {6, 0}});
                for (std::uint32_t index = 0;; ++index, pooled += 4, moved_on += 4) {
                    c.set({{3, pool_extended.get()}, {5, reg(s8(index))}});
                    if (compare(c, s8(index), std::int32_t(pool_extended.get())) >= 0) break;
                    const auto wanted = c.u32(r13 + at_bytes);
                    c.set(5, wanted);
                    if (nonzero_logical(c, wanted)) {
                        c.set(3, c.u32(pooled));
                        if (c.u32(pooled) == wanted) {
                            c.set(3, 1);
                            c.put32(pooled, 0);
                            changed = 1;
                        }
                    }
                    c.set({{7, moved_on + 4}, {4, pooled + 4}, {8, index + 1}});
                }
                c.put8(hole_place + r22, 0xff);
                c.set(3, wanted_first.get() - 1u);
                wanted_first = wanted_first - 1;
            }
        }
    }

    // ---- The pool changed: choose again for the places beyond the ones just filled.
    if (compare(c, s8(changed)) != 0) {
        const std::int32_t places = s8(places_byte);
        std::uint32_t count = 0;
        // The pool's people, then the candidates, into the choices list (the loops leave r3-r8 as they do here).
        const auto collect = [&](std::uint32_t from, std::int32_t number) {
            std::uint32_t listed = from, passed = 0;
            for (;;) {
                c.set(5, reg(s8(passed)));
                if (compare(c, s8(passed), number) >= 0) break;
                const auto value = c.u32(listed);
                c.set(7, value);
                if (nonzero_logical(c, value)) {
                    const std::uint32_t at_bytes = reg(s8(count)) << 2;
                    c.set(5, at_bytes);
                    ++count;
                    c.set(4, count);
                    c.put32(choices + at_bytes, value);
                }
                listed += 4;
                ++passed;
                c.set({{3, listed}, {8, passed}});
            }
        };
        c.set({{4, 0}, {3, pool.get()}, {8, 0}, {6, choices}});
        collect(pool.get(), s8(pool_count_byte));
        c.set({{3, candidates}, {6, choices}, {8, 0}});
        collect(candidates, s8(candidate_count));
        if (compare(c, s8(count)) != 0)
            choose(choices, count, flags_argument.get() + (reg(places) << 1), c.u8(at(0x53f)), chosen + (reg(places) << 2), 0, 0,
                   ratings_argument.get() + (reg(places) << 1), judge, 0x1021ab80, 0);
    }

    // ---- The second group: holders of the places after the first ones that the rule rules out.
    if (compare(c, s8(wanted_second)) == 0) return;
    holes_second = 0;
    r24 = holes_second;
    r13 = reg(s8(places_byte));
    r22 = r24;
    r26 = reg(s8(pool_count_byte));
    while (compare(c, s8(r24), s8(r26)) < 0) {
        c.set(3, reg(s8(r24)));
        r23 = reg(s8(r24)) + r13;
        r25 = 0;
        c.set(3, r23 << 2);
        r27 = c.u32(chosen + (r23 << 2));
        if (ruled_out(r27, {0x1021ac08, 0x1021ac1c, 0x1021ac44})) r25 = 1;
        if (compare(c, s8(r25)) != 0) {
            const std::uint32_t twice = r23 << 1;
            const std::uint32_t where = reg(s8(holes_second));
            const auto flags_value = c.u16(flags_argument.get() + twice);
            c.put16(hole_flags + r22, flags_value);
            c.put8(hole_place + where, r24);
            c.put16(hole_rating + r22, c.u16(ratings_argument.get() + twice));
            r22 = r22 + 2;
            c.set({{7, twice}, {5, hole_rating}, {6, reg(std::int16_t(flags_value))}, {4, hole_place}, {3, holes_second.get() + 1u}});
            holes_second = holes_second + 1;
        }
        r24 = r24 + 1;
    }
    // The candidates that hold none of the places after the first ones.
    std::uint32_t count = 0;
    {
        const std::int32_t first_places = s8(places_byte), pool_count = s8(pool_count_byte);
        std::uint32_t listed = candidates, outer = 0;
        c.set({{4, 0}, {3, candidates}, {9, reg(first_places)}, {10, 0}, {8, reg(pool_count)}});
        for (;;) {
            c.set(5, reg(s8(outer)));
            if (compare(c, s8(outer), s8(candidate_count)) >= 0 || !nonzero_logical(c, candidates)) break;
            bool held = false;
            const auto mine = c.u32(listed);
            c.set(7, 0);
            for (std::uint32_t walk = 0;; ++walk) {
                c.set(5, reg(s8(walk)));
                if (compare(c, s8(walk), pool_count) >= 0) break;
                const auto there = c.u32(chosen + ((reg(s8(walk)) + reg(first_places)) << 2));
                c.set({{6, mine}, {5, there}});
                compare_logical(c, mine, there);
                if (mine == there) { held = true; break; }
                c.set(7, walk + 1);
            }
            c.set(5, held ? 1 : 0);
            if (compare(c, held ? 1 : 0) == 0) {
                const std::uint32_t at_bytes = reg(s8(count)) << 2;
                c.set({{5, people_list}, {6, at_bytes}, {7, mine}, {4, count + 1}});
                c.put32(people_list + at_bytes, mine);
                ++count;
            }
            listed += 4;
            ++outer;
            c.set({{3, listed}, {10, outer}});
        }
    }
    if (compare(c, s8(count)) == 0) return;
    choose(people_list, count, hole_flags, holes_second, choices, 0, 0, hole_rating, judge, 0x1021adc8, 0);
    r22 = hole_place;
    r27 = choices;
    r29 = 0;
    r13 = reg(s8(holes_second));
    first_places_extended = reg(s8(places_byte));
    while (compare(c, s8(wanted_second)) != 0 && compare(c, s8(holes_second)) != 0) {
        r28 = 0;
        r30 = reg(-1);
        r23 = r28;
        r24 = r28;
        while (compare(c, s8(r28), s8(r13)) < 0) {
            r26 = reg(s8(r28));
            const std::int32_t index = s8(c.u8(r22 + r26));
            c.set(3, reg(index));
            if (compare(c, index, -1) != 0) {
                const auto holder = c.u32(chosen + ((reg(index) + reg(s8(places_byte))) << 2));
                c.set(3, holder);
                if (nonzero_logical(c, holder)) {
                    slots = hole_rating;  // addi r31, r25: the arrays the rating reads from
                    r25 = hole_flags;
                    const auto flags_at = hole_flags + r23, rating_at = hole_rating + r23;
                    config = reg(rate(0x1021ae80, holder, flags_at, rating_at, 0, 0, 0));
                    c.set(4, choices);
                    const auto replaced = c.u32(choices + r24);
                    c.set(3, replaced);
                    std::int32_t other = no_rating;
                    if (nonzero_logical(c, replaced)) other = rate(0x1021aedc, replaced, flags_at, rating_at, 0, 0, 0);
                    c.set(3, reg(other));
                    if (compare(c, std::int32_t(r30), -1) == 0 || compare(c, std::int32_t(config) - other, std::int32_t(r29)) < 0) {
                        r30 = r26;
                        r29 = config - reg(other);
                    }
                }
            }
            r23 = r23 + 2;
            r24 = r24 + 4;
            r28 = r28 + 1;
        }
        if (compare(c, std::int32_t(r30), -1) <= 0) break;
        const std::uint32_t idx = reg(s8(c.u8(r22 + r30)));
        const std::uint32_t replacement = c.u32(r27 + (r30 << 2));
        const std::uint32_t target = (idx + first_places_extended.get()) << 2;
        c.set({{6, r30 << 2}, {8, 0}, {5, replacement}, {4, target}, {3, candidates}, {7, 0}});
        c.put32(chosen + target, replacement);
        std::uint32_t walk = candidates, advanced = 0;
        for (std::uint32_t index = 0;; ++index, walk += 4, advanced += 4) {
            c.set(4, reg(s8(index)));
            if (compare(c, s8(index), s8(candidate_count)) >= 0) break;
            c.set({{5, c.u32(walk)}, {4, c.u32(r27 + (r30 << 2))}});
            if (c.u32(walk) != c.u32(r27 + (r30 << 2))) { c.set({{7, advanced + 4}, {3, walk + 4}, {8, index + 1}}); continue; }
            const std::int32_t cnt = s8(candidate_count);
            c.set({{4, reg(cnt)}, {3, reg(s8(index))}});
            if (compare(c, s8(index), cnt - 1) < 0) {
                const std::uint32_t last = reg(cnt) << 2;
                const auto moved = c.u32(candidates + (last - 4));
                candidate_count = candidate_count - 1;
                c.put32(candidates + (reg(s8(index)) << 2), moved);
                c.put32(candidates + (last - 4), 0);
                c.set({{4, moved}, {3, reg(s8(index)) << 2}, {5, last - 4}});
            } else {
                c.put32(candidates + (reg(s8(index)) << 2), 0);
                candidate_count = candidate_count - 1;
                c.set({{3, 0}});
            }
            break;
        }
        c.put8(r22 + r30, 0xff);
        c.set(3, wanted_second.get() - 1u);
        wanted_second = wanted_second - 1;
    }
}
}
