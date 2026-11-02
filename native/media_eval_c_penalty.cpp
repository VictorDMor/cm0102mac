// Kinds 0x3f, 0x60, 0x40 and 0x41: the stories whose losses are the strength less 1000 and whose gains
// are the strength's share of the scale (limited by the pool). Side 1 gains after a draw against the
// factor, that may change the club's flags; side 2 loses and reports; side 3 is a draw-gated loss.
#include "media_eval_c_detail.hpp"

namespace cm::aot::media::eval_c {
namespace families {
namespace {
constexpr Pull pulls_a[] = {{0x20, 0x70}, {0x10, 0x80}, {0x9, 0x78}, {0x6, 0xd0}}, pulls_b[] = {{0x20, 0x10}, {0x10, 0x90}, {0x9, 0xc0}, {0x6, 0xd0}};
constexpr Step steps[] = {
    {0x10676b20, 0x8, {mask(0x1d, 0x1b), 0}, {~0u, 0x4}, 0, 0x10676c84, 0x10676c98},
    {0x10676cbc, 0x1, {mask(0, 0x1e), 0}, {~0u, 0x2}, 0, 0x10676e20, 0x10676e34},
    {0x10676e58, 0x40, {mask(0x1a, 0x18), 0}, {~0u, 0x8}, 1, 0x10676ffc, 0x10677010},
    {0x10677034, 0x40, {mask(0x1a, 0x18), 0}, {~0u, 0x1}, 2, 0x106771b8, 0x106771cc},
};
}
const Penalty penalty_3f{pulls_a,
                         {0, 0, 0, 0x10674d4c, 0x10674de8, 0, 0x10674e88, 0},
                         {0x10674f34, 0x10674fa4, 0x10674fb8, 0x10674f14, 0x1067516c, 0x106750d8, 0x106751fc, 0},
                         {0x10675220, 0, 0, 0x10675314, 0x106753b0, 0, 0x10675404, 0x1067542c},
                         true, -1, -1, -1, {}, 0x106750c4};
const Penalty penalty_60{pulls_a,
                         {0, 0, 0, 0x106755e4, 0x10675680, 0, 0x10675720, 0},
                         {0x106757c4, 0x10675834, 0x10675848, 0x106757a4, 0x106759ec, 0x10675960, 0x10675a7c, 0},
                         {0x10675aa0, 0, 0, 0x10675b94, 0x10675c30, 0, 0x10675c84, 0x10675cac},
                         true, 1, 1, -1, {}, 0x1067594c};
const Penalty penalty_40{pulls_b,
                         {0, 0, 0, 0x1067690c, 0x106769a8, 0, 0x10676a48, 0},
                         {0, 0x10676ae0, 0x10676af4, 0x10676ad4, 0x10677260, 0, 0x10677384, 0x106772f4},
                         {0x106773a8, 0, 0, 0x106774a0, 0x1067753c, 0, 0x10677590, 0x106775b8},
                         false, -1, -1, -1, steps, 0};
}

void Press::story_penalty(const Penalty& penalty) {
    apply_pulls(penalty.pulls);
    by_side(
        [&] {
            if (penalty.chain.empty()) penalty_up_single(penalty);
            else penalty_up_chain(penalty);
        },
        [&] {
            penalty_losses(penalty.down);
            const auto loss = std::int32_t(0u - (std::uint32_t(strength()) + 0x3e8));
            const auto report = [&](std::uint32_t at) { return fpu.fmul(dbl(0x70), integer(at, loss)); };
            post(8, limited(dbl(0x28), 0x68, 0x70, 0x78, report, std::uint32_t(strength()) + 0x3e8), penalty.down.post);
        },
        [&] { penalty_third(penalty); });
}

// The two losses: the strength less 1000, negated, scaled (each limited by the pool's constant).
void Press::penalty_losses(const Backs& at) {
    const auto loss = std::int32_t(0u - (std::uint32_t(strength()) + 0x3e8));
    const auto leftover = std::uint32_t(strength()) + 0x3e8;
    const auto first = [&](std::uint32_t pair) { return fpu.fmul(scale(), integer(pair, loss)); };
    change_a(limited(dbl(0x28), 0x38, 0x40, 0x48, first, leftover), at.rate_a);
    const auto second = [&](std::uint32_t pair) {
        const auto k = fpu.fmul(scale(), pull());
        const auto s = integer(pair, loss);
        return fpu.fmul(s, k);
    };
    change_b(limited(dbl(0x28), 0x50, 0x58, 0x60, second, leftover), 0, at.rate_b);
}

// Side 3: a draw against the factor gates the two losses; the item reports what the first would lose
// (never above 0) or just the story.
void Press::penalty_third(const Penalty& penalty) {
    const auto& at = penalty.third;
    bool passes;
    if (penalty.upward_third) {
        passes = rolled_above(at.refill);
    } else {
        const auto value = hundred(at.refill, true);
        const auto limit = fpu.fsubs(factor(), single_at(0xc));
        const auto drawn = fpu.fsubs(stacked_integer(c, frame.at(0x38), signed_word(value)), bias());
        fpu.fcmp(0, limit, drawn);
        passes = limit < drawn;
    }
    if (!passes) {
        post(4, 0, at.post_other);
        return;
    }
    penalty_losses(at);
    // max(-(strength + 1000), 0) mirrored: the loss when it is not above 0, else 0 (subfc/subfe/andc)
    const auto loss = std::int32_t(0u - (std::uint32_t(strength()) + 0x3e8));
    post(8, extended(std::int16_t(loss > 0 ? 0 : loss)), at.post);
}

// One draw above the factor's limit: single(0xc) + factor against the table's sample reduced by 100.
bool Press::rolled_above(std::uint32_t back) {
    const auto value = hundred(back, false);
    const auto limit = fpu.fadds(single_at(0xc), factor());
    const auto drawn = fpu.fsubs(stacked_integer(c, frame.at(0x38), signed_word(value)), bias());
    fpu.fcmp(0, limit, drawn);
    return limit > drawn;
}

// 0x10459b50 about the club: none, or a kind from 0x19 on, fails.
bool Press::queries_pass(const Backs& at) {
    if (!nonzero_logical(c, Program{c}.call(club_query, at.query_first, {c.r(18), club}))) return false;
    const auto second = Program{c}.call(club_query, at.query_second, {c.r(18), club});
    return compare_logical(c, c.u8(second + 4), 0x19) < 0;
}

// The strength (negated for sign < 0) over the scale, limited by the pool's 0xb8: the amounts of side 1.
double Press::ratio(int sign, std::uint32_t first, std::uint32_t second) {
    const auto value = sign > 0 ? strength() : std::int32_t(0u - std::uint32_t(strength()));
    // r7 before the second pair: the strength xor 0x80000000 for a plain one, the negated strength otherwise
    const auto leftover = sign > 0 ? signed_word(value) : std::uint32_t(value);
    return limited_by(dbl(0xb8), [&] { return fpu.fdiv(integer(first, value), scale()); }, [&] {
        c.set(7, leftover);
        return fpu.fdiv(integer(second, value), scale());
    });
}

// Side 1 of 0x3f and 0x60: the first rating gains; a draw, the club's queries and a flag decide whether the
// second gains boosted (and the flag moves from 0x20 to 0x10).
void Press::penalty_up_single(const Penalty& penalty) {
    const auto& at = penalty.up;
    change_a(truncated(0x48, ratio(penalty.sign_a, 0x38, 0x40)), at.rate_a);
    bool boosted = rolled_above(at.refill) && queries_pass(at) && flag(0x20);
    if (boosted) {
        edit_flags({mask(0x1b, 0x19), 0});
        edit_flags({~0u, 0x10});
        const auto share = ratio(penalty.sign_b, 0x38, 0x40);
        change_b(truncated(0x48, fpu.multiply_add(penalty.boost_site, pull(), share, dbl(0xd8))), 1, at.rate_boost);
    } else {
        change_b(truncated(0x48, fpu.fmul(pull(), ratio(penalty.sign_b, 0x38, 0x40))), 0, at.rate_b);
    }
    post(0x50, truncated(0x48, ratio(penalty.sign_post, 0x38, 0x40)), at.post);
}

// The group record checks of the later draws of 0x40 and 0x41.
bool Press::group_check(int check) {
    const auto record = [&] {
        const auto group = records::Club{c, club}.division().get(), table = c.u32(c.r(17));
        const auto at = c.u32(table + (c.u32(group) << 2));
        leave(c, {{3, at}, {4, table}});
        return at;
    };
    if (check == 0) return true;
    if (check == 2) return compare(c, c.i32(record() + 0x1c), -1) == 0;
    if (compare(c, c.i32(record() + 0x1c), -1) == 0) return false;
    return nonzero(c, byte_word(c.u8(record() + 0xc1)));
}

// Side 1 of 0x40 and 0x41: the queries first, then up to four draws, each with its own flag and edits.
void Press::penalty_up_chain(const Penalty& penalty) {
    const auto& at = penalty.up;
    change_a(truncated(0x48, ratio(penalty.sign_a, 0x38, 0x40)), at.rate_a);
    if (!queries_pass(at)) {
        change_b(truncated(0x48, fpu.fmul(pull(), ratio(penalty.sign_b, 0x38, 0x40))), 0, at.post_other);
    } else {
        bool boosted = false;
        for (const auto& step : penalty.chain) {
            if (!rolled_above(step.refill) || !flag(step.flag) || !group_check(step.check)) continue;
            edit_flags(step.first);
            edit_flags(step.second);
            const auto share = ratio(penalty.sign_b, 0x38, 0x40);
            change_b(truncated(0x48, fpu.multiply_add(step.boost_site, pull(), share, dbl(0xd8))), 1, step.rate_boost);
            boosted = true;
            break;
        }
        if (!boosted) change_b(truncated(0x48, fpu.fmul(pull(), ratio(penalty.sign_b, 0x38, 0x40))), 0, at.rate_b);
    }
    post(0x50, truncated(0x48, ratio(penalty.sign_post, 0x38, 0x40)), at.post);
}
}
