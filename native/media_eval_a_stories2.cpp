// 0x106584d0: the helpers the stories share, and the kinds of social and rated stories (0, 0x11, 0x52,
// 0xd, 0xe, 2, 0x21-0x23, 9, 0xc; 8, 0x25; 1; 0x1e, 0x20).
#include "media_eval_a_detail.hpp"

namespace cm::aot::media::eval_a {

std::int32_t Verdict::roll_range(unsigned into, std::int32_t n, std::uint32_t refill_back, bool range_after,
                                 bool entry) {
    const auto value = roll(0, into, n, refill_back, 0xe8);
    if (n == 0) return value;
    if (!beyond_table(n)) {
        const auto cursor = c.u32(rng.cursor);
        leave(c, {{3, std::uint32_t(n)}, {4, rng.sample()}, {5, entry ? std::uint32_t(c.u16(cursor)) : cursor}});
    } else if (range_after) {
        c.set(3, std::uint32_t(n));
    }
    return value;
}

// The amount (r30) plus the remainder of one sample by 10 and less that of the next: the compiler overlapped the
// two draws, so the first one's scratch registers stand while the second refills.
void Verdict::swing_amount(std::uint32_t first_back, std::uint32_t second_back) {
    rng.advance(first_back);
    const auto entry = c.u16(c.u32(rng.cursor));
    const auto first = reduced_by(rng.sample(), divisors::by_10);
    story_amount = std::uint32_t(sum(amount(), first.rest));
    rng.advance(second_back, {{3, first.sign}, {4, std::uint32_t(first.high)}, {5, first.sample}, {6, entry}});
    const auto second = reduced_by(rng.sample(), divisors::by_10);
    leave(c, {{3, second.sign}, {4, second.sample}});
    story_amount = std::uint32_t(difference(amount(), second.rest));
}

std::int32_t Verdict::draw_sum(std::uint32_t query_back, std::uint32_t refill_back) {
    const auto management = effective(Skill::man_management, query_back);
    const auto loyalty = boss().loyalty().get();
    const auto total = skill(Skill::standing) + management + loyalty;
    leave(c, {{3, std::uint32_t(loyalty)},
              {4, boss().non_player()},
              {5, std::uint8_t(loyalty)},
              {6, std::uint32_t(management)}});
    return roll(19, 18, total, refill_back);
}

std::int32_t Verdict::draw_sum_plain(std::uint32_t refill_back) {
    const auto loyalty = boss().loyalty().get();
    const auto total = loyalty + skill(Skill::standing);
    leave(c, {{3, std::uint32_t(loyalty)}, {4, std::uint8_t(loyalty)}});
    return roll(19, 18, total, refill_back);
}

bool Verdict::kind_in(std::initializer_list<std::int32_t> kinds) {
    for (const auto wanted : kinds)
        if (compare(c, kind_half(), wanted) == 0) return true;
    return false;
}
// cmplwi (kind - first) & 0xffff, last - first; ble
bool Verdict::kind_between(std::uint32_t first, std::uint32_t last) {
    return compare_logical(c, (kind - first) & 0xffff, last - first) <= 0;
}

// Kinds 0, 0x11, 0x52, 0xd, 0xe, 2, 0x21-0x23, 9 and 0xc: how the manager takes news of the player's
// contract (when it is one of the states 1-3) and then his own standing with the staff.
bool Verdict::story_social() {
    const auto story = kind_half();
    const bool mine = compare(c, story, 0) == 0 || compare(c, story, 0x11) == 0 || compare(c, story, 0x52) == 0 ||
                      kind_between(0xd, 0xe) || compare(c, story, 2) == 0 || kind_between(0x21, 0x23) ||
                      compare(c, story, 9) == 0 || compare(c, story, 0xc) == 0;
    if (!mine) return false;
    const auto state = contract_state();
    if (compare(c, state, 3) == 0 || compare(c, state, 1) == 0 || compare(c, state, 2) == 0) {
        const auto moves = c.u32(c.u32(c.toc(world_slot)) + 0x988);
        const auto found = program.call(callee::moves_in, 0x1065b9d0, {moves, club, player, 0, 0});
        if (compare_logical(c, found) != 0) {
            const auto at = c.u32(found + 0x10);
            c.set(3, at);
            if (compare(c, std::int32_t(at), std::int32_t(c.u32(player))) == 0 && compare(c, amount(), -0xa) > 0) {
                const auto d = draw_of(over::manager(Trait::loyalty, 0x1065ba24));
                compare(c, d);  // (a test of the draw whose both branches go on)
            }
        }
    }
    if (all({{over::effective(Skill::man_management, 0x1065bb74, 0x1065bba4), Test::below, by::n10, 0x1065bd00},
             {over::manager(Trait::temperament, 0x1065bd74), Test::below, by::n10, 0x1065bed0},
             {over::staff(Skill::standing, 0x1065bf48), Test::below, by::n10, 0x1065c0a4},
             {over::manager(Trait::loyalty, 0x1065c118), Test::below, by::n10, 0x1065c274},
             {over::amount(0x1065c2e4), Test::below, by::minus5, 0x1065c440}}))
        return verdict_is(2);
    const auto total = draw_sum(0x1065c49c, 0x1065c4e8);
    if (compare_remainder(total, by::n36, 0x1065c644) > 0 &&
        passes({over::amount(0x1065c6b4), Test::above, by::minus5, 0x1065c810}))
        return verdict_is(1);
    return verdict_is(3);
}

// Kinds 8 and 0x25.
bool Verdict::story_8_25() {
    if (!kind_in({8, 0x25})) return false;
    const auto doubled = std::int32_t(std::uint32_t(skill(Skill::standing)) << 1);
    c.set(3, boss().non_player());
    if (compare(c, roll(19, 18, doubled, 0x1065c8b8)) == 0)
        program.call(relations::fall_out, 0x1065ca0c, {manager, player, 1});
    if (compare(c, effective(Skill::man_management, 0x1065ca1c), 0xa) < 0 &&
        all({{over::manager(Trait::temperament, 0x1065ca5c), Test::below, by::n10, 0x1065cbb8},
             {over::staff(Skill::standing, 0x1065cc30), Test::below, by::n10, 0x1065cd8c},
             {over::effective(Skill::man_management, 0x1065cddc, 0x1065ce0c), Test::zero}}))
        return verdict_is(2);
    const auto total = draw_sum(0x1065cf68, 0x1065cfb4);
    return compare_remainder(total, by::n36, 0x1065d110) > 0 ? verdict_is(1) : verdict_is(3);
}

// Kind 1.
bool Verdict::story_1() {
    if (compare(c, kind_half(), 1) != 0) return false;
    if (all({{over::effective(Skill::man_management, 0x1065d17c, 0x1065d1ac), Test::zero},
             {over::manager(Trait::temperament, 0x1065d324), Test::below, by::n10, 0x1065d480},
             {over::staff(Skill::standing, 0x1065d4f8), Test::below, by::n5, 0x1065d654},
             {over::effective(Skill::man_management, 0x1065d6a4, 0x1065d6d4), Test::zero}}))
        return verdict_is(2);
    const auto total = draw_sum(0x1065d830, 0x1065d87c);
    return compare_remainder(total, by::n30, 0x1065d9d8) > 0 ? verdict_is(1) : verdict_is(3);
}

// Kinds 0x1e and 0x20: the player's reputation against the club's decides how hard the manager leans.
bool Verdict::story_1e_20() {
    if (!kind_in({0x1e, 0x20})) return false;
    if (all({{over::manager(Trait::temperament, 0x1065da78), Test::below, by::n20, 0x1065dbd4},
             {over::staff(Skill::standing, 0x1065dc4c), Test::below, by::n15, 0x1065dda8}}))
        return verdict_is(2);
    if (compare_remainder(draw_sum_plain(0x1065de38), by::n150, 0x1065df94) > 0) {
        const auto player_rep = subject().player_record().reputation().get(), club_rep = employer().reputation().get();
        leave(c, {{3, std::uint32_t(club_rep)}, {4, std::uint32_t(player_rep)}});
        if (compare(c, player_rep, club_rep + 0x1f4) > 0) {
            const Step firm{over::staff(Skill::attitude, 0x1065e024), Test::below, by::n5, 0x1065e180};
            return passes(firm) ? verdict_is(1) : verdict_is(3);
        }
        return verdict_is(2);
    }
    if (passes({over::staff(Skill::attitude, 0x1065e210), Test::below, by::n5, 0x1065e36c})) return verdict_is(1);
    if (passes({over::staff(Skill::attitude, 0x1065e3ec), Test::above, by::n20, 0x1065e548})) return verdict_is(2);
    return verdict_is(3);
}
}
