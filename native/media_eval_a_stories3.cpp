// 0x106584d0: the verdict on the rated stories (0x42-0x44, 0x3f-0x41, 0x5a-0x63), 3, 4, 5 and 6.
#include "media_eval_a_detail.hpp"

namespace cm::aot::media::eval_a {

// Kinds 0x5a-0x5d, 0x42-0x44, 0x5f-0x63, 0x3f-0x41: the amount is reset to a bias by kind (+10 for 0x61, +5 for
// 0x5d, -7 for 0x62) and moved by two draws if the manager's professionalism draw comes up zero.
bool Verdict::story_rated() {
    if (!(kind_between(0x5a, 0x5d) || kind_between(0x42, 0x44) || kind_between(0x5f, 0x63) || kind_between(0x3f, 0x40) ||
          compare(c, kind_half(), 0x41) == 0))
        return false;
    const auto story = kind_half();
    c.set(30, 0);
    if (compare(c, story, 0x61) == 0) c.set(30, 10);
    else if (compare(c, story, 0x61) > 0) {
        if (compare(c, story, 0x63) < 0) c.set(30, std::uint32_t(-7));
    } else if (compare(c, story, 0x5d) == 0) c.set(30, 5);
    if (compare(c, draw_manager(personality::professionalism, 0x1065e654)) == 0) swing_amount(0x1065e7b8, 0x1065e814);
    const auto attitude = draw_staff(attribute::attribute_34, 0x1065e888);
    const auto limit = roll_range(19, 15 - amount(), 0x1065e9f8, true, true);
    if (compare(c, attitude, limit) > 0) return verdict_is(2);
    const auto ambition = draw_manager(personality::ambition, 0x1065eb98);
    const auto reach = roll_range(17, amount() + 0xf, 0x1065ed04, false, false);
    return compare(c, ambition, reach) > 0 ? verdict_is(1) : verdict_is(3);
}

// Kinds 3 and 4.
bool Verdict::story_3_4() {
    if (!kind_in({3, 4})) return false;
    if (compare_remainder(draw_staff(attribute::attribute_34, 0x1065eebc), by::n50, 0x1065f018) > 0 && compare(c, draw_staff(attribute::attribute_3d, 0x1065f090)) == 0 &&
        compare_remainder(draw_amount(0x1065f204), by::minus5, 0x1065f360) < 0) {
        program.call(ask::praise, 0x1065f3b4, {manager, player, 1});
        return verdict_is(2);
    }
    const auto state = contract_state();
    if (!(compare(c, state, 0) == 0 || compare(c, state, 7) == 0 || compare(c, state, 8) == 0) &&
        !asks(ask::calm_down, 0x1065f3f4, player, 0) && compare_remainder(draw_manager(personality::loyalty, 0x1065f430), by::n6, 0x1065f58c) > 0 &&
        compare_remainder(draw_trait(attribute::attribute_39, 0x1065f5d8, 0x1065f608), by::n6, 0x1065f764) > 0 &&
        compare_remainder(draw_amount(0x1065f7d0), by::minus5, 0x1065f92c) > 0)
        return verdict_is(1);
    if ((compare(c, state, 7) == 0 || compare(c, state, 4) == 0) && compare_remainder(draw_manager(personality::loyalty, 0x1065f9cc), by::n5, 0x1065fb28) < 0 &&
        compare_remainder(draw_manager(personality::sportsmanship, 0x1065fb9c), by::n5, 0x1065fcf8) < 0 &&
        compare_remainder(draw_amount(0x1065fd68), by::minus5, 0x1065fec4) < 0)
        return verdict_is(2);
    return verdict_is(3);
}

// Kind 5.
bool Verdict::story_5() {
    if (compare(c, kind_half(), 5) != 0) return false;
    if (compare(c, draw_manager(personality::loyalty, 0x1065ff58)) != 0 && compare(c, draw_trait(attribute::attribute_39, 0x106600ac, 0x106600dc)) != 0 &&
        compare(c, draw(byte_word(c.u8(player + personality::sportsmanship)), 0x10660254)) != 0 &&
        compare_remainder(draw_amount(0x106603c8), by::minus5, 0x10660524) > 0)
        return verdict_is(1);
    if (compare(c, draw_trait(attribute::attribute_39, 0x10660580, 0x106605b0)) == 0 && compare_remainder(draw(byte_word(c.u8(player + personality::professionalism)), 0x10660728), by::n10, 0x10660884) < 0 &&
        compare_remainder(draw_amount(0x106608f4), by::minus5, 0x10660a50) < 0)
        return verdict_is(2);
    return verdict_is(3);
}

// Kind 6.
bool Verdict::story_6() {
    if (compare(c, kind_half(), 6) != 0) return false;
    if (compare(c, draw_manager(personality::loyalty, 0x10660ae4)) != 0 && compare_remainder(draw_staff(attribute::attribute_3f, 0x10660c60), by::n10, 0x10660dbc) > 0 &&
        compare_remainder(draw(byte_word(c.u8(player + personality::sportsmanship)), 0x10660e30), by::n5, 0x10660f8c) > 0 &&
        compare_remainder(draw(player_byte(0x25), 0x10661004), by::n30, 0x10661160) < 0 &&
        compare_remainder(draw_amount(0x106611d4), by::minus5, 0x10661330) > 0)
        return verdict_is(1);
    if (compare(c, draw_trait(attribute::attribute_39, 0x1066138c, 0x106613bc)) == 0 && compare(c, draw_staff(attribute::attribute_3f, 0x10661538)) == 0 &&
        compare_remainder(draw(player_byte(0x25), 0x106616b4), by::n50, 0x10661810) > 0 &&
        compare_remainder(draw_amount(0x10661880), by::minus5, 0x106619dc) < 0)
        return verdict_is(2);
    return verdict_is(3);
}
}
