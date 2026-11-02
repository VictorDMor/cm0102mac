// The stories of one shape: the club's rating moves by the story's strength, scaled by the club's
// flags; side 2 loses and reports, side 1 gains after a draw and may change the club's flags, side 3
// is a draw-gated loss. Four families of kinds share the code and differ in their pool constants,
// flag bits and the original's call sites.
#include "media_eval_c_detail.hpp"

namespace cm::aot::media::eval_c {
namespace families {
namespace {
constexpr Pull scorer_pulls[] = {{0x2, 0x78}, {0x1, 0xa8}}, striker_pulls[] = {{0x2, 0xc0}, {0x1, 0xc8}},
               back_pulls[] = {{0x4, 0x78}, {0x8, 0xa8}}, keeper_pulls[] = {{0x400, 0x78}};
constexpr Edit scorer_edits[] = {{mask(0, 0x1e), 0}, {mask(0x1c, 0x1a), 0}, {mask(0x1b, 0x19), 0}, {mask(0x1a, 0x18), 0}, {~0u, 0x2}},
               back_edits[] = {{mask(0x1d, 0x1b), 0}, {mask(0x1c, 0x1a), 0}, {mask(0x1b, 0x19), 0}, {mask(0x1a, 0x18), 0}, {~0u, 0x4}},
               keeper_edits[] = {{~0u, 0x400}};
}
const Family kind_63{scorer_pulls, scorer_edits, 0x106735b0, 0xb8, true, false, false,
                     {0, 0, 0, 0x10673290, 0x10673324, 0, 0x106733b4, 0}, {0x10673420, 0x10673488, 0x1067349c, 0x10673400, 0x10673618, 0x106735c4, 0x1067366c, 0},
                     {0x10673690, 0, 0, 0x10673740, 0x10673790, 0, 0x106737b4, 0}};
const Family kind_62{striker_pulls, scorer_edits, 0x10673c14, 0x68, true, true, true,
                      {0, 0, 0, 0x106738f4, 0x10673988, 0, 0x10673a18, 0}, {0x10673a84, 0x10673aec, 0x10673b00, 0x10673a64, 0x10673c7c, 0x10673c28, 0x10673d6c, 0},
                      {0x10673d90, 0, 0, 0x10673e40, 0x10673e90, 0, 0x10673eb4, 0}};
const Family kind_5f{back_pulls, back_edits, 0x10674308, 0xb8, true, false, true,
                   {0, 0, 0, 0x10673fe8, 0x1067407c, 0, 0x1067410c, 0}, {0x10674178, 0x106741e0, 0x106741f4, 0x10674158, 0x10674370, 0x1067431c, 0x10674460, 0},
                   {0x10674484, 0, 0, 0x10674534, 0x10674584, 0, 0x106745a8, 0}};
const Family kind_5c{keeper_pulls, keeper_edits, 0x106748f4, 0xb8, false, false, true,
                     {0, 0, 0, 0x106746a0, 0x10674734, 0, 0x106747c4, 0}, {0x10674830, 0, 0, 0x10674810, 0x1067495c, 0x10674908, 0x10674a4c, 0},
                     {0x10674a70, 0, 0, 0x10674b20, 0x10674b70, 0, 0x10674b94, 0}};
}

void Press::story_family(const Family& family) {
    // The club's flags scale the story: the first flag set decides.
    apply_pulls(family.pulls);
    by_side([&] { family_up(family); }, [&] { family_down(family); }, [&] { family_third(family); });
}

// Side 2: the club loses by the strength (each amount limited), reported with item 8.
void Press::family_down(const Family& family) {
    const auto& at = family.down;
    const auto loss = [&](std::uint32_t slot) { return fpu.fmul(-scale(), strength_at(slot)); };
    change_a(limited(dbl(0x28), 0x38, 0x40, 0x48, loss), at.rate_a);
    const auto pulled = [&](std::uint32_t slot) {
        const auto k = fpu.fmul(-scale(), pull());
        return fpu.fmul(strength_at(slot), k);
    };
    change_b(limited(dbl(0x28), 0x50, 0x58, 0x60, pulled), 0, at.rate_b);
    const auto report = [&](std::uint32_t slot) { return fpu.fmul(dbl(0xb0), strength_at(slot)); };
    post(8, limited(dbl(0x28), 0x68, 0x70, 0x78, report), at.post);
}

// Side 1: the club gains; a draw against the factor may also pay the second column more and change the
// club's flags.
void Press::family_up(const Family& family) {
    const auto& at = family.up;
    change_a(truncated(0x40, fpu.fmul(scale(), strength_at(0x38))), at.rate_a);
    const auto value = family.twenty ? roll(at.refill, 0x66666667, 3, 20, false) : hundred(at.refill, false);
    const auto drawn = fpu.fsubs(stacked_integer(c, frame.at(0x38), signed_word(value)), bias());
    fpu.fcmp(0, factor(), drawn);
    bool boosted = factor() > drawn;
    if (boosted && family.queries) boosted = queries_pass(at);
    if (boosted) {
        for (const auto& edit : family.edits) edit_flags(edit);
        const auto s = strength_at(0x38);
        const auto k = fpu.fmul(scale(), pull());
        change_b(truncated(0x40, fpu.multiply_add(family.boost_site, s, k, dbl(family.boost_pool))), 1, at.rate_boost);
    } else {
        const auto k = fpu.fmul(scale(), pull());
        const auto s = strength_at(0x38);
        change_b(truncated(0x40, fpu.fmul(s, k)), 0, at.rate_b);
    }
    if (family.column_after) {
        move_third_column();
        post(0x50, 0, at.post);
    } else {
        post(0x10, truncated(0x40, fpu.fmul(dbl(0x40), strength_at(0x38))), at.post);
    }
}

// Side 3: a downward draw against the factor; when it passes, both ratings fall by the scaled strength.
void Press::family_third(const Family& family) {
    const auto& at = family.third;
    const auto value = hundred(at.refill, true);
    const auto limit = fpu.fsubs(factor(), single_at(0xc));
    const auto drawn = fpu.fsubs(stacked_integer(c, frame.at(0x38), signed_word(value)), bias());
    fpu.fcmp(0, limit, drawn);
    if (limit < drawn) {
        change_a(truncated(0x40, fpu.fmul(scale(), strength_at(0x38))), at.rate_a);
        const auto k = fpu.fmul(scale(), pull());
        const auto s = strength_at(0x48);
        change_b(truncated(0x50, fpu.fmul(s, k)), 0, at.rate_b);
    }
    post(4, 0, at.post);
}
}
