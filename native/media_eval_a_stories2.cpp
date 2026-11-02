// 0x106584d0: the helpers the stories share, and the kinds of social and rated stories (0, 0x11, 0x52,
// 0xd, 0xe, 2, 0x21-0x23, 9, 0xc; 8, 0x25; 1; 0x1e, 0x20).
#include "media_eval_a_detail.hpp"

namespace cm::aot::media::eval_a {

std::int32_t Verdict::roll_range(unsigned into, std::int32_t n, std::uint32_t refill_back, bool range_after, bool entry) {
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
    c.set(30, std::uint32_t(std::int32_t(c.r(30)) + first.rest));
    rng.advance(second_back, {{3, first.sign}, {4, std::uint32_t(first.high)}, {5, first.sample}, {6, entry}});
    const auto second = reduced_by(rng.sample(), divisors::by_10);
    leave(c, {{3, second.sign}, {4, second.sample}});
    c.set(30, std::uint32_t(std::int32_t(c.r(30)) - second.rest));
}

std::int32_t Verdict::draw_sum(std::uint32_t query_back, std::uint32_t refill_back) {
    const auto trait = staff_trait(attribute::attribute_39, query_back);
    const auto loyalty = c.u8(manager + personality::loyalty);
    const auto sum = byte_word(c.u8(staff() + attribute::attribute_3d)) + trait + byte_word(loyalty);
    leave(c, {{3, std::uint32_t(byte_word(loyalty))}, {4, staff()}, {5, loyalty}, {6, std::uint32_t(trait)}});
    return roll(19, 18, sum, refill_back);
}

std::int32_t Verdict::draw_sum_plain(std::uint32_t refill_back) {
    const auto loyalty = c.u8(manager + personality::loyalty);
    const auto sum = byte_word(loyalty) + staff_byte(attribute::attribute_3d);
    leave(c, {{3, std::uint32_t(byte_word(loyalty))}, {4, loyalty}});
    return roll(19, 18, sum, refill_back);
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
    if (!(compare(c, story, 0) == 0 || compare(c, story, 0x11) == 0 || compare(c, story, 0x52) == 0 || kind_between(0xd, 0xe) ||
          compare(c, story, 2) == 0 || kind_between(0x21, 0x23) || compare(c, story, 9) == 0 || compare(c, story, 0xc) == 0))
        return false;
    const auto state = contract_state();
    if (compare(c, state, 3) == 0 || compare(c, state, 1) == 0 || compare(c, state, 2) == 0) {
        const auto found = program.call(ask::moves_in, 0x1065b9d0, {c.u32(c.u32(c.toc(-0x7b40)) + 0x988), club, player, 0, 0});
        if (compare_logical(c, found) != 0) {
            const auto at = c.u32(found + 0x10);
            c.set(3, at);
            if (compare(c, std::int32_t(at), std::int32_t(c.u32(player))) == 0 && compare(c, amount(), -0xa) > 0) {
                const auto d = draw_manager(personality::loyalty, 0x1065ba24);
                compare(c, d);  // (a test of the draw whose both branches go on)
            }
        }
    }
    const auto n = draw_trait(attribute::attribute_39, 0x1065bb74, 0x1065bba4);
    if (compare_remainder(n, by::n10, 0x1065bd00) < 0 &&
        compare_remainder(draw_manager(personality::trait_69, 0x1065bd74), by::n10, 0x1065bed0) < 0 &&
        compare_remainder(draw_staff(attribute::attribute_3d, 0x1065bf48), by::n10, 0x1065c0a4) < 0 &&
        compare_remainder(draw_manager(personality::loyalty, 0x1065c118), by::n10, 0x1065c274) < 0 &&
        compare_remainder(draw_amount(0x1065c2e4), by::minus5, 0x1065c440) < 0)
        return verdict_is(2);
    const auto sum = draw_sum(0x1065c49c, 0x1065c4e8);
    if (compare_remainder(sum, by::n36, 0x1065c644) > 0 && compare_remainder(draw_amount(0x1065c6b4), by::minus5, 0x1065c810) > 0)
        return verdict_is(1);
    return verdict_is(3);
}

// Kinds 8 and 0x25.
bool Verdict::story_8_25() {
    if (!kind_in({8, 0x25})) return false;
    const auto doubled = std::int32_t(std::uint32_t(staff_byte(attribute::attribute_3d)) << 1);
    c.set(3, staff());
    if (compare(c, roll(19, 18, doubled, 0x1065c8b8)) == 0) program.call(ask::praise, 0x1065ca0c, {manager, player, 1});
    if (compare(c, staff_trait(attribute::attribute_39, 0x1065ca1c), 0xa) < 0 && compare_remainder(draw_manager(personality::trait_69, 0x1065ca5c), by::n10, 0x1065cbb8) < 0 &&
        compare_remainder(draw_staff(attribute::attribute_3d, 0x1065cc30), by::n10, 0x1065cd8c) < 0 && compare(c, draw_trait(attribute::attribute_39, 0x1065cddc, 0x1065ce0c)) == 0)
        return verdict_is(2);
    const auto sum = draw_sum(0x1065cf68, 0x1065cfb4);
    return compare_remainder(sum, by::n36, 0x1065d110) > 0 ? verdict_is(1) : verdict_is(3);
}

// Kind 1.
bool Verdict::story_1() {
    if (compare(c, kind_half(), 1) != 0) return false;
    if (compare(c, draw_trait(attribute::attribute_39, 0x1065d17c, 0x1065d1ac)) == 0 && compare_remainder(draw_manager(personality::trait_69, 0x1065d324), by::n10, 0x1065d480) < 0 &&
        compare_remainder(draw_staff(attribute::attribute_3d, 0x1065d4f8), by::n5, 0x1065d654) < 0 && compare(c, draw_trait(attribute::attribute_39, 0x1065d6a4, 0x1065d6d4)) == 0)
        return verdict_is(2);
    const auto sum = draw_sum(0x1065d830, 0x1065d87c);
    return compare_remainder(sum, by::n30, 0x1065d9d8) > 0 ? verdict_is(1) : verdict_is(3);
}

// Kinds 0x1e and 0x20: the player's reputation against the club's decides how hard the manager leans.
bool Verdict::story_1e_20() {
    if (!kind_in({0x1e, 0x20})) return false;
    if (compare_remainder(draw_manager(personality::trait_69, 0x1065da78), by::n20, 0x1065dbd4) < 0 &&
        compare_remainder(draw_staff(attribute::attribute_3d, 0x1065dc4c), by::n15, 0x1065dda8) < 0)
        return verdict_is(2);
    if (compare_remainder(draw_sum_plain(0x1065de38), by::n150, 0x1065df94) > 0) {
        const auto player_rep = half_word(c.u16(c.u32(player + 0x14) + 0xa)), club_rep = half_word(c.u16(club + 0x1ec));
        leave(c, {{3, std::uint32_t(club_rep)}, {4, std::uint32_t(player_rep)}});
        if (compare(c, player_rep, club_rep + 0x1f4) > 0) {
            if (compare_remainder(draw_staff(attribute::attribute_34, 0x1065e024), by::n5, 0x1065e180) < 0) return verdict_is(1);
            return verdict_is(3);
        }
        return verdict_is(2);
    }
    if (compare_remainder(draw_staff(attribute::attribute_34, 0x1065e210), by::n5, 0x1065e36c) < 0) return verdict_is(1);
    if (compare_remainder(draw_staff(attribute::attribute_34, 0x1065e3ec), by::n20, 0x1065e548) > 0) return verdict_is(2);
    return verdict_is(3);
}
}
