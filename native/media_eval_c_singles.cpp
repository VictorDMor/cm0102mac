// Kind 0x43: the story whose amounts are single-precision functions of the factor alone (each limited by
// the pool's single 0x8), chosen by the club's flags. Side 2 loses, side 1 gains (with a draw and the
// queries for the first two flags), side 3 only reports.
#include "media_eval_c_detail.hpp"

namespace cm::aot::media::eval_c {
namespace {
constexpr std::uint32_t rate_floor = 8;  // the pool's single that limits every amount
}

void Press::story_singles() {
    by_side([&] { singles_up(); }, [&] { singles_down(); }, [&] { post(4, 0, 0x10676748); });
}

// Side 2: the loss by the flag (the factor times a rate, less an offset for the later flags).
// fmadds / fmsubs / fnmsubs at `site`: rate * factor + offset (the offset less rate * factor for the later flags of side 2).
double Press::single_fused(std::uint32_t site, std::uint32_t rate, std::uint32_t offset) {
    return fpu.multiply_add(site, single_at(rate), factor(), single_at(offset));
}
double Press::single_negative() { return fpu.fmuls(single_at(0x50), -factor()); }
double Press::single_floored_negative() {
    const auto negative = [&] { return single_negative(); };
    return limited_by(single_at(rate_floor), negative, negative);
}

void Press::singles_down() {
    const auto floored = [&](auto&& first, auto&& second) { return limited_by(single_at(rate_floor), first, second); };
    if (flag(0x20)) {
        change_a(0, 0x10675d30);
        change_b(0, 0, 0x10675d50);
        post(4, 0, 0x10675d74);
    } else if (flag(0x10)) {
        change_a(truncated(0x38, single_floored_negative()), 0x10675df8);
        change_b(truncated(0x40, single_floored_negative()), 0, 0x10675e4c);
        post(4, truncated(0x48, single_floored_negative()), 0x10675ea4);
    } else if (flag(0x9)) {
        const auto amount = [&](std::uint32_t first, std::uint32_t second) {
            return floored([&] { return single_fused(first, 0x50, 0x58); }, [&] { return single_fused(second, 0x50, 0x58); });
        };
        change_a(truncated(0x38, amount(0x10675ef4, 0x10675f14)), 0x10675f2c);
        change_b(truncated(0x40, amount(0x10675f44, 0x10675f64)), 0, 0x10675f80);
        post(8, truncated(0x48, amount(0x10675f98, 0x10675fc0)), 0x10675fd8);
    } else if (flag(0x6)) {
        const auto amount = [&](std::uint32_t first, std::uint32_t second) {
            return floored([&] { return single_fused(first, 0x60, 0x5c); }, [&] { return single_fused(second, 0x60, 0x5c); });
        };
        change_a(truncated(0x38, amount(0x10676024, 0x10676044)), 0x1067605c);
        change_b(truncated(0x40, amount(0x10676074, 0x10676094)), 0, 0x106760b0);
        // (the item takes the 0x50/0x58 pair, as the flag 9 case does)
        const auto reported = [&](std::uint32_t site) { return single_fused(site, 0x50, 0x58); };
        post(8, truncated(0x48, floored([&] { return reported(0x106760c8); }, [&] { return reported(0x106760f0); })), 0x10676108);
    } else {
        post(8, 0, 0x10676130);
    }
}

void Press::singles_up() {
    const auto times = [&](std::uint32_t rate) { return fpu.fmuls(single_at(rate), factor()); };
    const auto drawn_queries = [&](std::uint32_t refill, std::uint32_t first, std::uint32_t second) {
        return rolled_above(refill) && queries_pass({0, first, second, 0, 0, 0, 0, 0});
    };
    if (flag(0x20)) {
        change_a(truncated(0x38, single_fused(0x1067617c, 4, 0x64)), 0x10676194);
        if (drawn_queries(0x106761b4, 0x10676224, 0x10676238)) {
            edit_flags({mask(0x1b, 0x19), 0});
            edit_flags({~0u, 0x10});
            change_b(truncated(0x38, single_fused(0x106762ac, 4, 0x68)), 0, 0x106762c8);
        } else {
            change_b(truncated(0x38, single_fused(0x106762e4, 4, 0x64)), 0, 0x10676300);
        }
        post(0x50, truncated(0x38, single_fused(0x10676318, 4, 0x64)), 0x10676338);
    } else if (flag(0x10)) {
        change_a(truncated(0x38, single_fused(0x10676384, 0x50, 0x64)), 0x1067639c);
        if (drawn_queries(0x106763bc, 0x1067642c, 0x10676440)) {
            edit_flags({mask(0x1c, 0x1a), 0});
            edit_flags({~0u, 0x40});
            change_b(truncated(0x38, single_fused(0x106764b4, 0x50, 0x68)), 0, 0x106764d0);
        } else {
            change_b(truncated(0x38, single_fused(0x106764ec, 0x50, 0x64)), 0, 0x10676508);
        }
        post(0x50, truncated(0x38, single_floored_negative()), 0x10676560);
    } else if (flag(0x9)) {
        change_a(truncated(0x38, times(4)), 0x106765c4);
        change_b(truncated(0x40, times(4)), 0, 0x106765f4);
        post(0x50, truncated(0x48, times(4)), 0x10676628);
    } else if (flag(0x6)) {
        change_a(truncated(0x38, single_fused(0x10676674, 4, 0x48)), 0x1067668c);
        change_b(truncated(0x40, single_fused(0x106766a4, 4, 0x48)), 0, 0x106766c0);
        post(0x50, truncated(0x48, single_fused(0x106766d8, 4, 0x48)), 0x106766f8);
    } else {
        post(0x50, 0, 0x10676720);
    }
}
}
