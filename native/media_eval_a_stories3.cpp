// 0x106584d0: the verdict on the rated stories (0x42-0x44, 0x3f-0x41, 0x5a-0x63), 3, 4, 5 and 6.
#include "media_eval_a_detail.hpp"

namespace cm::aot::media::eval_a {

// Kinds 0x5a-0x5d, 0x42-0x44, 0x5f-0x63, 0x3f-0x41: the amount is reset to a bias by kind (+10 for 0x61, +5 for
// 0x5d, -7 for 0x62) and moved by two draws if the manager's professionalism draw comes up zero.
bool Verdict::story_rated() {
    if (!(kind_between(0x5a, 0x5d) || kind_between(0x42, 0x44) || kind_between(0x5f, 0x63) ||
          kind_between(0x3f, 0x40) || compare(c, kind_half(), 0x41) == 0))
        return false;
    const auto story = kind_half();
    story_amount = 0;
    if (compare(c, story, 0x61) == 0) story_amount = 10;
    else if (compare(c, story, 0x61) > 0) {
        if (compare(c, story, 0x63) < 0) story_amount = std::uint32_t(-7);
    } else if (compare(c, story, 0x5d) == 0) story_amount = 5;
    if (compare(c, draw_of(over::manager(Trait::professionalism, 0x1065e654))) == 0)
        swing_amount(0x1065e7b8, 0x1065e814);
    const auto attitude = draw_of(over::staff(Skill::attitude, 0x1065e888));
    const auto limit = roll_range(19, difference(15, amount()), 0x1065e9f8, true, true);
    if (compare(c, attitude, limit) > 0) return verdict_is(2);
    const auto ambition = draw_of(over::manager(Trait::ambition, 0x1065eb98));
    const auto reach = roll_range(17, sum(amount(), 0xf), 0x1065ed04, false, false);
    return compare(c, ambition, reach) > 0 ? verdict_is(1) : verdict_is(3);
}

// Kinds 3 and 4.
bool Verdict::story_3_4() {
    if (!kind_in({3, 4})) return false;
    if (all({{over::staff(Skill::attitude, 0x1065eebc), Test::above, by::n50, 0x1065f018},
             {over::staff(Skill::standing, 0x1065f090), Test::zero},
             {over::amount(0x1065f204), Test::below, by::minus5, 0x1065f360}})) {
        program.call(relations::fall_out, 0x1065f3b4, {manager, player, 1});
        return verdict_is(2);
    }
    const auto state = contract_state();
    if (!(compare(c, state, 0) == 0 || compare(c, state, 7) == 0 || compare(c, state, 8) == 0) &&
        !asks(callee::calm_down, 0x1065f3f4, player, 0) &&
        all({{over::manager(Trait::loyalty, 0x1065f430), Test::above, by::n6, 0x1065f58c},
             {over::effective(Skill::man_management, 0x1065f5d8, 0x1065f608), Test::above, by::n6, 0x1065f764},
             {over::amount(0x1065f7d0), Test::above, by::minus5, 0x1065f92c}}))
        return verdict_is(1);
    if ((compare(c, state, 7) == 0 || compare(c, state, 4) == 0) &&
        all({{over::manager(Trait::loyalty, 0x1065f9cc), Test::below, by::n5, 0x1065fb28},
             {over::manager(Trait::sportsmanship, 0x1065fb9c), Test::below, by::n5, 0x1065fcf8},
             {over::amount(0x1065fd68), Test::below, by::minus5, 0x1065fec4}}))
        return verdict_is(2);
    return verdict_is(3);
}

// Kind 5.
bool Verdict::story_5() {
    if (compare(c, kind_half(), 5) != 0) return false;
    if (all({{over::manager(Trait::loyalty, 0x1065ff58), Test::nonzero},
             {over::effective(Skill::man_management, 0x106600ac, 0x106600dc), Test::nonzero},
             {over::subject(Trait::sportsmanship, 0x10660254), Test::nonzero},
             {over::amount(0x106603c8), Test::above, by::minus5, 0x10660524}}))
        return verdict_is(1);
    if (all({{over::effective(Skill::man_management, 0x10660580, 0x106605b0), Test::zero},
             {over::subject(Trait::professionalism, 0x10660728), Test::below, by::n10, 0x10660884},
             {over::amount(0x106608f4), Test::below, by::minus5, 0x10660a50}}))
        return verdict_is(2);
    return verdict_is(3);
}

// Kind 6: the player's dirtiness enters the chances.
bool Verdict::story_6() {
    if (compare(c, kind_half(), 6) != 0) return false;
    if (all({{over::manager(Trait::loyalty, 0x10660ae4), Test::nonzero},
             {over::staff(Skill::discipline, 0x10660c60), Test::above, by::n10, 0x10660dbc},
             {over::subject(Trait::sportsmanship, 0x10660e30), Test::above, by::n5, 0x10660f8c},
             {over::subject_attribute(Attribute::dirtiness, 0x10661004), Test::below, by::n30, 0x10661160},
             {over::amount(0x106611d4), Test::above, by::minus5, 0x10661330}}))
        return verdict_is(1);
    if (all({{over::effective(Skill::man_management, 0x1066138c, 0x106613bc), Test::zero},
             {over::staff(Skill::discipline, 0x10661538), Test::zero},
             {over::subject_attribute(Attribute::dirtiness, 0x106616b4), Test::above, by::n50, 0x10661810},
             {over::amount(0x10661880), Test::below, by::minus5, 0x106619dc}}))
        return verdict_is(2);
    return verdict_is(3);
}
}
