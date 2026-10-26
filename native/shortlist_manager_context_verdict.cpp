// SHORTLIST_MANAGER.CPP 0x10366470, the candidate context evaluation: the
// comparison of the considered club with the one he has (0x10368968) and
// the optional refinement by the reference quote (0x10368ee8).
#include "shortlist_manager_context_detail.hpp"

namespace cm::aot::shortlist_manager::context {
void Evaluation::compare_clubs() {
    // 0x10368968: a club in no division or in the minor one (TOC): x0.1.
    const Club ref{c, reference};
    const std::uint32_t division = club.division();
    if (!nonzero_logical(c, division) ||
        compare(c, Division{c, division}.id(), c.i32(c.toc(slot::minor_division))) == 0)
        score = fpu.fmul(score, 0.1);
    // 0x10368990: the club's reputation against his average reputation,
    // applied at the end: 1 + (reputation - average) / 8000.
    const StaffRecord staff{c, person.non_player()};
    const std::int32_t world = staff.world_reputation(), home = staff.home_reputation();
    const std::int32_t current = staff.reputation();
    const std::int32_t ours = half_of(club_value), theirs = half_of(other_value);
    const std::int32_t high = multiply_high(0x55555556, std::int32_t(word(current) + word(home) + word(world)));
    const std::int32_t average = std::int32_t(word(high) + word(sign_bit(high)));
    leave(c, {{4, word(theirs)}, {5, word(ours)}, {6, 0x43300000}, {7, word(high)}, {8, word(world)}, {9, 0x55555556}});
    const double gap = integer(0xac, std::int32_t(word(club.reputation()) - word(average)));
    const double gap_part = fpu.fdiv(gap, 8000.0);
    const double standing = fpu.fadd(1.0, gap_part);
    // The gain in value (single precision): a better club by (0.75 +
    // ambition / 11.4) per point; a worse one 1.5 per point, or for the
    // manager categories 1 per point, x clamp(1.1 - score / 100, 0.1, 1)
    // when the score is over 10.
    double gain;
    if (compare(c, ours, theirs) > 0) {
        const double ambition = integer(0xac, byte_of(person.ambition()));
        const double difference = stacked(0xb4, std::int32_t(word(ours) - word(theirs)));
        const double ambition_part = fpu.fdiv(ambition, 11.4);
        const double per_point = fpu.fadd(0.75, ambition_part);
        const double points = fpu.fsub(difference, k::signed_bias);
        gain = fpu.frsp(fpu.fmul(per_point, points));
    } else if (!manager_category()) {
        const double points = integer(0xac, std::int32_t(word(ours) - word(theirs)));
        gain = fpu.frsp(fpu.fmul(1.5, points));
    } else if (compare(c, ours, theirs) <= 0 && fcompare(score, 10.0).greater) {
        const double part = fpu.fdiv(score, 100.0);
        const double rest = fpu.frsp(fpu.fsub(1.1, part));
        double clamped = 0.1;
        if (!fcompare(0.1, rest).greater) clamped = fcompare(1.0, rest).less ? 1.0 : rest;
        const double weight = fpu.frsp(clamped);
        const double points = fpu.fsubs(stacked(0xac, std::int32_t(word(ours) - word(theirs))), k::signed_bias);
        gain = fpu.fmuls(weight, points);
    } else {
        gain = fpu.fsubs(stacked(0xac, std::int32_t(word(ours) - word(theirs))), k::signed_bias);
    }
    // 0x10368b40: a gain counts less when he has not served long (f29
    // under 1): x((150 - g) f29 + g) / 150, g = min(100, gain / 50).
    if (fcompare(gain, 0.0).greater && fcompare(service, 1.0).less) {
        const double share = fpu.fdiv(gain, 50.0);
        const double capped = fcompare(100.0, share).less ? 100.0 : share;
        const bool reasons_wanted = has_reasons();
        const double rest = fpu.fsub(150.0, capped);
        const double mixed = fpu.multiply_add(0x10368b80, rest, service, capped);
        const double factor = fpu.fdiv(mixed, 150.0);
        score = fpu.fmul(score, factor);
        if (reasons_wanted && fcompare(factor, 0.6).less) add_reason(reason::step_up);
    }
    // 0x10368ba8: a loss for a manager category, not from a manager job:
    // scaled by max(1 - score / 25, 0.4) from jobs 8, 15 and 6, else by
    // max(1 - score / 10, 0.2).
    if (!manager_job() && manager_category() && fcompare(gain, 0.0).less) {
        const bool coaching = compare(c, job8(), 8) == 0 || compare(c, job8(), 15) == 0 || compare(c, job8(), 6) == 0;
        const double part = fpu.fdiv(score, coaching ? 25.0 : 10.0);
        const double rest = fpu.fsub(1.0, part);
        const double floor = coaching ? 0.4 : 0.2;
        gain = fpu.fmul(gain, fcompare(rest, floor).less ? floor : rest);
    }
    // 0x10368c44: x max(1 + gain / 4500, 0.001).
    const double gain_part = fpu.fdiv(gain, 4500.0);
    const double raised = fpu.fadd(1.0, gain_part);
    const double factor = fcompare(raised, 0.001).less ? 0.001 : raised;
    score = fpu.fmul(score, factor);
    if (has_reasons()) {
        const auto unprotected = [&] {
            return !nonzero_logical(c, contract) || !nonzero(c, byte_of(Contract{c, contract}.listed()));
        };
        if (fcompare(factor, 0.6).less) {
            const bool step_up = fcompare(capacity_weight, 1.2).greater && nonzero_logical(c, reference_stadium) &&
                                 !nonzero_logical(c, stadium_record) && fcompare(score, 1.25).greater &&
                                 manager_job() && unprotected();
            add_reason(step_up ? reason::step_up_big : reason::worse);
        } else if (fcompare(factor, 1.5).greater && fcompare(capacity_weight, 1.2).greater &&
                   !nonzero_logical(c, reference_stadium) && nonzero_logical(c, stadium_record) &&
                   manager_category() && unprotected()) {
            add_reason(reason::step_down_big);
        }
    }
    // 0x10368d48: nations of different levels: x clamp(their level x our
    // division value / (our level x max(500, theirs)), 0.01, 10), and x0.2
    // more when it is a step down to another continent.
    const auto continent = [&](std::uint32_t nation) { return std::uint32_t(Nation{c, nation}.continent()); };
    if (nonzero_logical(c, reference) && compare(c, byte_of(reference_level), byte_of(nation_level)) != 0) {
        const std::int32_t their_level = byte_of(reference_level), our_level = byte_of(nation_level);
        const std::int32_t our_division = half_of(club_division), their_division = half_of(reference_division);
        const double their_level_value = integer(0xac, their_level);
        const double our_division_value = integer(0xb4, our_division);
        const double our_level_stacked = stacked(0xbc, our_level);
        const double numerator = fpu.fmul(their_level_value, our_division_value);
        const double our_level_value = fpu.fsub(our_level_stacked, k::signed_bias);
        const std::int32_t floor = compare(c, their_division, 500) > 0 ? their_division : 500;
        const double floor_value = integer(0xc4, floor);
        const double denominator = fpu.fmul(our_level_value, floor_value);
        const double ratio = fpu.fdiv(numerator, denominator);
        double level_factor = 0.01;
        if (!fcompare(0.01, ratio).greater) level_factor = fcompare(10.0, ratio).less ? 10.0 : ratio;
        const std::uint32_t ours_continent = continent(club.nation());
        leave(c, {{4, ours_continent}, {5, signed_word(our_division)}, {6, 0x43300000}});
        score = fpu.fmul(score, level_factor);
        if (compare_logical(c, ours_continent, continent(ref.nation())) != 0 && fcompare(level_factor, 1.0).less) {
            score = fpu.fmul(score, 0.2);
            note(reason::continent);
        }
    } else if (nonzero_logical(c, reference)) {
        // 0x10368e58: from his home continent (TOC) to a club of another,
        // not the reference club's: x0.4.
        const std::uint32_t his = continent(person.nation());
        c.set(4, his);
        if (nonzero_logical(c, his)) {
            const std::int32_t home_id = c.i32(c.toc(slot::home_continent));
            c.set(4, word(home_id));
            if (compare(c, c.i32(his + 8), home_id) == 0) {
                const std::uint32_t ours_continent = continent(club.nation());
                c.set(5, ours_continent);
                if (nonzero_logical(c, ours_continent) && compare(c, c.i32(ours_continent + 8), home_id) != 0 &&
                    compare_logical(c, ours_continent, continent(ref.nation())) != 0) {
                    score = fpu.fmul(score, 0.4);
                    note(reason::continent);
                }
            }
        }
    }
    // 0x10368ec4
    score = fpu.fmul(score, standing);
    if (has_reasons() && fcompare(standing, 0.6).less) add_reason(reason::worse);
}

double Evaluation::refine_by_quote() {
    // 0x10368ee8: with the refine byte and a score of 1.75 or more, the
    // reference quote (0x1037b6f0) against the club's budget for the
    // category (0x100f4790) and the contract's figure.
    if (!nonzero(c, c.i8(frame.at(0x197))) || fcompare(score, 1.75).less) return score;
    const std::uint32_t draft = frame.at(0x48);
    program.call(call::initialize_draft, 0x10368f10, {draft});
    quote_record = program.call(call::finance_record, 0x10368f20, {c.u32(c.toc(slot::finances)), club.address});
    if (!nonzero(c, std::int32_t(quote_record))) return 0.0;
    program.call(call::reference_quote, 0x10368f50, {draft, person.address, club.address, 0, 0xffffffff, category, 0});
    double quoted = c.f(1);
    const double floor = fcompare(0.1, score).greater ? 0.1 : double(score);
    quote_factor = fpu.fdiv(3.5, floor);
    if (fcompare(quote_factor, 0.9).less) quote_factor = 0.9;
    // Quirk: unreachable here (the score is at least 1.75), kept as written.
    if (fcompare(score, 1.75).less) quoted = fpu.fadd(quoted, 2500.0);
    quote = fpu.fmul(quote_factor, quoted);
    const std::int32_t budget = program.call(call::budget_for, 0x10368fa0, {quote_record, category});
    const double available = integer(0xac, budget);
    if (fcompare(quote, available).greater && fcompare(quote, 0.0).greater) {
        const double share = fpu.fdiv(available, quote);
        score = fpu.fmul(score, share);
        quote = available;
    }
    if (!nonzero_logical(c, contract)) return score;
    const double figure = integer(0xac, Contract{c, contract}.wage());
    const double divisor = fcompare(figure, 10.0).less ? 10.0 : figure;
    const double ratio = fpu.fdiv(quote, divisor);
    double clamped = 0.1;
    if (!fcompare(0.1, ratio).greater) clamped = fcompare(10.0, ratio).less ? 10.0 : ratio;
    const auto order = fcompare(clamped, 1.0);
    merge_equal(false);
    if (order.greater || order.equal) {
        const double share = fpu.fdiv(clamped, quote_factor);
        score = fpu.fmul(score, share);
    }
    return score;
}
}
