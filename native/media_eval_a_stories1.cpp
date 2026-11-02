// 0x106584d0: the verdict on the press items' codes (0x83, 0x82), the manager's moods that come before every
// other kind, and the kinds 0x1f, 0x3c, 0x3d and the transfer stories.
#include "media_eval_a_detail.hpp"

namespace cm::aot::media::eval_a {

// Code 0x83, the item about a person's standing at his club: a contract in its first states (1, 2) or one of type 3
// with grievances is backed (1); a manager with an attitude to spare, who sees a player of lesser reputation, sides
// against him (2); otherwise a draw of the manager's management decides between 1 and 3.
void Verdict::standing_item() {
    const auto state = contract_state();
    if (compare(c, state, 1) == 0 || compare(c, state, 2) == 0) {
        answer(1);
        return;
    }
    if (compare(c, state, 3) == 0 && compare(c, deal().grievances().get(), 1) > 0) {
        answer(1);
        return;
    }
    if (compare(c, subject().player_record().reputation().get(), 0x1482) < 0 &&
        compare(c, effective(Skill::attitude, 0x10658648), 0xf) > 0) {
        answer(2);
        return;
    }
    if (compare(c, state, 7) == 0 && compare(c, effective(Skill::attitude, 0x1065867c), 0xa) > 0) {
        answer(2);
        return;
    }
    const auto n = effective(Skill::man_management, 0x106586a0);
    const auto drawn = roll(18, 17, n, 0x106586d0);
    answer(compare(c, drawn) != 0 ? 1 : 3);
}

// Code 0x82, the items about two people or a person and his club: how the manager rates the player decides.
void Verdict::pair_item() {
    if (asks(relations::disliked_by, 0x1065883c, player, manager)) {
        if (compare(c, trait(boss(), Trait::sportsmanship), 0xf) < 0) {
            if (compare(c, draw_of(over::manager(Trait::sportsmanship, 0x10658888))) == 0) {
                const auto n = draw_of(over::effective(Skill::man_management, 0x106589dc, 0x10658a0c));
                if (compare_remainder(n, by::n20, 0x10658b68) < 0) {
                    answer(2);
                    return;
                }
            }
        }
        if (compare(c, trait(boss(), Trait::sportsmanship), 0xe) > 0) {
            if (compare(c, draw_of(over::manager(Trait::sportsmanship, 0x10658bf4)), 0xa) > 0) {
                const auto n = draw_of(over::effective(Skill::man_management, 0x10658d48, 0x10658d78));
                if (compare_remainder(n, by::n20, 0x10658ed4) > 0) {
                    answer(1);
                    return;
                }
            }
        }
        answer(3);
        return;
    }
    if (asks(relations::liked_by, 0x10658f30, player, manager)) {
        const auto d = draw_of(over::manager(Trait::sportsmanship, 0x10658f6c));
        if (compare_remainder(d, by::n10, 0x106590c8) < 0) {
            const auto n = draw_of(over::effective(Skill::man_management, 0x10659118, 0x10659148));
            if (compare_remainder(n, by::n15, 0x106592a4) > 0) {
                answer(1);
                return;
            }
        }
        answer(3);
        return;
    }
    if (compare(c, trait(boss(), Trait::temperament), 6) < 0 &&
        compare_remainder(trait(boss(), Trait::temperament), by::loaded20, 0x10659328) < 0) {
        answer(1);
        return;
    }
    const auto n = draw_of(over::effective(Skill::man_management, 0x10659388, 0x106593b8));
    if (compare_remainder(n, by::n20, 0x10659514) > 0) {
        answer(1);
        return;
    }
    if (compare(c, trait(boss(), Trait::composure), 0xf) > 0 &&
        compare_remainder(trait(boss(), Trait::composure), by::loaded10, 0x1065958c) > 0) {
        answer(2);
        return;
    }
    answer(3);
}

// Kind 0x1f: a team mate of the club's, else the contract's happiness against a draw.
void Verdict::happiness_story() {
    if (asks(callee::team_mate, 0x10659d54, player, club)) {
        answer(2);
        return;
    }
    c.set(17, std::uint8_t(deal().happiness().get()));  // (kept unsigned in r17, compared as a signed byte)
    if (compare(c, deal().happiness().get(), remainder(by::loaded100, 0x10659d88)) <= 0) {
        answer(3);
        return;
    }
    if (nonzero(c, byte_word(program.call(callee::has_flag, 0x10659dd4, {contract}))) ||
        nonzero(c, deal().listed().get()) || !nonzero(c, remainder(by::n3, 0x10659e08))) {
        answer(3);
        return;
    }
    answer(1);
}

// What comes before every other kind: a manager who dislikes the player (for a story worth under 5) or likes him
// (for one worth over -5) answers by draws of his sportsmanship and management; otherwise the kinds follow.
bool Verdict::mood() {
    if (asks(relations::disliked_by, 0x106595fc, player, manager) && compare(c, amount(), 5) < 0) {
        if (compare(c, draw_of(over::manager(Trait::sportsmanship, 0x10659640))) == 0) {
            const auto n = draw_of(over::effective(Skill::man_management, 0x10659794, 0x106597c4));
            if (compare_remainder(n, by::n20, 0x10659920) < 0) return answer(2);
        }
        return answer(3);
    }
    if (asks(relations::liked_by, 0x1065997c, player, manager) && compare(c, amount(), -5) > 0) {
        if (passes({over::manager(Trait::sportsmanship, 0x106599c0), Test::below, by::n10, 0x10659b1c})) {
            const auto n = draw_of(over::effective(Skill::man_management, 0x10659b6c, 0x10659b9c));
            if (compare_remainder(n, by::n20, 0x10659cf8) < 0) return answer(1);
        }
        return answer(3);
    }
    return false;
}

// Kinds 0x3c and 0x3d, the stories about a fixture.
bool Verdict::fixture_story() {
    if (compare(c, kind_half(), 0x3c) != 0 && compare(c, kind_half(), 0x3d) != 0) return false;
    const auto n = draw_of(over::effective(Skill::tactics, 0x10659e78, 0x10659ea8));
    if (compare_remainder(n, by::n23, 0x1065a004) > 0 &&
        passes({over::staff(Skill::standing, 0x1065a080), Test::below, by::n10, 0x1065a1dc}))
        return answer(2);
    if (compare(c, draw_of(over::manager(Trait::professionalism, 0x1065a258))) != 0 &&
        all({{over::manager(Trait::loyalty, 0x1065a3d0), Test::at_most, by::n40, 0x1065a52c},
             {over::staff(Skill::standing, 0x1065a5a4), Test::at_most, by::n50, 0x1065a700}}))
        return answer(3);
    return answer(1);
}

// The kinds a transfer story covers (0x1069ecf0 says which).
bool Verdict::transfer_story() {
    enter(c, 0x1065a75c, {reporter, std::uint32_t(kind_half())});
    is_transfer_story(c, kind_half());
    if (!nonzero(c, byte_word(c.r(3)))) return false;
    if (compare_remainder(draw(trait(subject(), Trait::sportsmanship), 0x1065a798), by::n20, 0x1065a8f4) > 0) {
        const auto n = draw_of(over::effective(Skill::man_management, 0x1065a944, 0x1065a974));
        const auto limit = roll_range(19, difference(20, amount()), 0x1065aae4, true, true);
        if (compare(c, n, limit) > 0) return answer(1);
        const auto m = effective(Skill::man_management, 0x1065ac60);
        const auto drawn = roll(18, 17, m, 0x1065ac90);
        if (compare(c, drawn) == 0 && compare(c, amount(), 0xa) < 0) return answer(2);
        return answer(3);
    }
    const auto reaction = draw_of(over::effective(Skill::man_management, 0x1065adfc, 0x1065ae2c));
    if (compare_remainder(reaction, by::n10, 0x1065af88) > 0) {
        // The morale block. r18 says whether the player has any morale; if so, a draw over his tally (first) goes
        // against a draw over 40 (20 when his row's first mark is set) (second) and r18 becomes whether first > second
        // (the compiler's xor/and/shift sequence on the two draws, which agrees with the plain comparison for draws
        // of this size). The next draw is random(r18): with r18 = 0 nothing is drawn, with r18 = 1 the range is 1, so
        // the result (r19) is 0 and `r19 == 0` below always holds either way. The one effect of the whole block is
        // whether that random(1) consumes a sample of the stream (and so moves every later draw): the extra sample
        // is the point.
        const auto morale = subject().player_record().morale().get();
        c.set(3, std::uint32_t(morale));
        std::uint32_t outdrawn = ((0u - std::uint32_t(morale)) | std::uint32_t(morale)) >> 31;  // morale != 0
        c.set(18, outdrawn);
        if (compare(c, std::int32_t(outdrawn)) != 0) {
            c.set(3, tally_row().at);
            c.set(19, tally_row().tally().get());
            const auto first = roll(19, 18, std::int32_t(tally_row().tally().get()), 0x1065b028);
            const auto id_offset = c.u32(player) * records::tally_row_size;
            leave(c, {{3, id_offset}, {4, c.u32(c.toc(records::tables::tallies))}});
            const auto second = roll(20, 19, 0x28 - 0x14 * std::int32_t(tally_row().marks().get() & 1), 0x1065b1b4);
            c.set(3, std::uint32_t(std::int32_t(std::uint32_t(first) ^ std::uint32_t(second)) >> 1));
            outdrawn = first > second;
            c.set(18, outdrawn);
        }
        if (compare(c, std::int32_t(c.r(18))) == 0) c.set(19, 0);
        else roll(18, 19, std::int32_t(c.r(18)), 0x1065b334);  // random(1): always 0, but it consumes a sample
        if (compare(c, std::int32_t(c.r(19))) == 0 && compare(c, amount(), 0xa) < 0) return answer(2);
    }
    if (compare(c, draw_of(over::subject_attribute(Attribute::versatility, 0x1065b4c0))) == 0) return answer(1);
    const auto loyalty = draw_of(over::manager(Trait::loyalty, 0x1065b638));
    const auto limit = roll_range(17, difference(20, amount()), 0x1065b7a8, true, true);
    return compare(c, loyalty, limit) > 0 ? answer(1) : answer(3);
}
}
