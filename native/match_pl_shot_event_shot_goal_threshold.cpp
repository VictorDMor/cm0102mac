#include "match_pl_shot_event_detail.hpp"
#include "guest_fpu.hpp"

#include <bit>

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

namespace {
constexpr int shot_constants = -0x51e0;

bool within_threshold(Cpu& c, Fpu& fp) {
    fp.fcmp(0, c.f(31), c.f(0));
    const auto field = c.cr(0);
    c.set_cr(0, field | ((field & 8) >> 2));
    return (c.cr(0) & 2) != 0;
}

void calculate_goal_ratio(Cpu& c, Fpu& fp, std::uint32_t actor, std::uint32_t stack) {
    const auto previous = c.i16(stack + 0x132);
    const auto contact = std::int16_t(c.r(16));
    c.set(3, std::uint32_t(previous));
    c.set(0, std::uint32_t(previous + contact));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!c.r(0)) {
        c.set(3, c.toc(shot_constants));
        c.set_f(31, c.f32(c.r(3)));
        return;
    }

    c.set({{0, std::uint32_t(contact)}, {6, 0x43300000},
           {7, std::uint32_t(contact * 100)}, {4, 0x43300000}});
    c.set(0, std::uint32_t(c.i16(stack + 0x132)));
    c.set(5, std::uint32_t(contact));
    c.set(3, c.u32(actor + 0x1a4));
    c.set({{0, c.r(5) + c.r(0)}, {5, c.r(7) ^ 0x80000000u}});
    c.put32(stack + 0xfc, c.r(6));
    c.set(0, c.r(0) ^ 0x80000000u);
    c.set_f(2, c.f64(c.r(27) + 0x80));
    c.put32(stack + 0x100, c.r(5));
    c.compare_logical(0, c.r(3), 0);
    c.set_f(1, c.f64(c.r(27) + 0x80));
    c.set_f(0, c.f64(stack + 0xfc));
    c.put32(stack + 0x108, c.r(0));
    c.set_f(2, fp.fsubs(c.f(0), c.f(2)));
    c.put32(stack + 0x104, c.r(4));
    c.set_f(0, c.f64(stack + 0x104));
    c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
    c.set_f(31, fp.fdivs(c.f(2), c.f(0)));
    if ((c.cr(0) & 2) != 0) return;

    c.set(3, c.toc(shot_constants));
    c.set_f(0, c.f32(c.r(3) + 0x120));
    if (!within_threshold(c, fp)) return;
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, c.u8(c.r(3) + 1));
    c.compare(0, std::int32_t(c.r(0)), 2);
    if (!(c.cr(0) & 2)) {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(0, c.u8(c.r(3) + 1));
        c.compare(0, std::int32_t(c.r(0)), 8);
        if (!(c.cr(0) & 2)) return;
    }
    c.set(3, c.toc(shot_constants));
    c.set_f(0, c.f32(c.r(3) + 0x2c));
    c.set_f(31, fp.fadds(c.f(31), c.f(0)));
}

bool is_goalkeeper_reaction(Cpu& c, std::uint32_t stack, unsigned flag, std::uint32_t back) {
    const auto active = c.i8(stack + flag);
    c.set(0, std::uint32_t(active));
    c.compare(0, active, 0);
    if (!active) return false;
    c.set(3, c.r(24));
    call_shot_owner(c, 0x10289560, back);
    const auto role = std::int16_t(c.r(3));
    c.set(0, std::uint32_t(role));
    c.compare(0, role, 1);
    return role == 1;
}

}

void apply_shot_morale(Cpu& c, std::uint32_t player, unsigned range, std::uint32_t refill_return,
                       std::uint32_t morale_return, std::uint32_t refresh_return) {
    advance_shot_stream(c, refill_return);
    if (range == 4 || range == 8) {
        c.set(4, c.u32(c.r(31)));
        c.set(3, player);
        c.set(0, c.u32(c.r(28)));
        const auto sample = c.u16(c.r(4)) + c.r(0);
        const auto sign = sample >> 31;
        const auto shift = range == 4 ? 2u : 3u;
        const auto mask = UINT32_MAX << (32 - shift);
        c.set(4, sign);
        c.set(0, std::rotl((std::rotr(sample, int(shift)) & mask) - sign, int(shift)) + sign);
    } else {
        c.set(3, c.u32(c.r(31)));
        c.set(4, range == 6 ? 0x2aab0000 : 0x92490000);
        c.set(0, c.u32(c.r(28)));
        c.set(4, range == 6 ? 0x2aaaaaab : 0x92492493);
        c.set(5, c.u16(c.r(3)));
        c.set(3, player);
        const auto sample = c.r(5) + c.r(0);
        c.set(5, sample);
        const auto high = std::uint32_t((std::int64_t(std::int32_t(c.r(4))) * std::int32_t(sample)) >> 32);
        if (range == 6) {
            c.set(4, high);
            c.set(0, sample - (high + (high >> 31)) * 6);
        } else {
            const auto quotient = std::int32_t(high + sample);
            c.s.wr<1>(reg::xer_ca, quotient < 0 && (quotient & 3));
            const auto shifted = std::uint32_t(quotient >> 2);
            c.set(4, shifted >> 31);
            c.set(0, sample - (shifted + c.r(4)) * 7);
        }
    }
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    call_shot_owner(c, 0x10288f30, morale_return);
    const auto changed = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(changed));
    c.compare(0, changed, 0);
    if (changed) {
        c.set({{3, player}, {4, 0}});
        call_shot_owner(c, 0x1022ba10, refresh_return);
    }
}

namespace {
void react_to_near_goal(Cpu& c, bool stronger_reaction) {
    const auto keeper = c.r(24);
    c.set(3, std::uint32_t(c.i16(keeper + 0x36)));
    c.set(0, c.r(3) + (stronger_reaction ? 600 : 350));
    c.put16(keeper + 0x36, c.r(0));

    const auto modifier = c.i8(keeper + 0x17);
    c.set(0, std::uint32_t(modifier));
    c.set(3, std::uint32_t(modifier * 20));
    const auto minimum = stronger_reaction ? 20u : 50u;
    const auto bound = std::int32_t(c.r(3) + minimum);
    c.s.wr<1>(reg::xer_ca, c.r(3) > UINT32_MAX - minimum);
    c.set(16, std::uint32_t(bound));
    // The signed-byte modifier times 20 plus at most 50 fits the narrow draw.
    draw_narrow_shot_sample(c, bound, 14, stronger_reaction ? 0x102624e8 : 0x102622cc);
    c.compare(0, std::int32_t(c.r(14)), 0);
    if (c.r(14)) return;

    if (stronger_reaction) apply_shot_morale(c, keeper, 7, 0x1026264c, 0x10262694, 0x102626a8);
    else apply_shot_morale(c, keeper, 4, 0x10262430, 0x10262468, 0x1026247c);
}
}

void evaluate_shot_goal_threshold(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    Fpu fp(c);
    calculate_goal_ratio(c, fp, actor, stack);
    c.set(3, c.toc(shot_constants));
    c.set_f(0, c.f32(c.r(3) + 0x124));
    if (within_threshold(c, fp)) {
        if (is_goalkeeper_reaction(c, stack, 0x122, 0x10262278)) react_to_near_goal(c, false);
        else if (is_goalkeeper_reaction(c, stack, 0x123, 0x10262494)) react_to_near_goal(c, true);
    }
    c.set(3, c.toc(shot_constants));
    c.set_f(0, c.f32(c.r(3) + 0x124));
    c.set_f(0, fp.fmuls(c.f(0), c.f(29)));
    fp.fcmp(0, c.f(31), c.f(0));
    c.s.pc = (c.cr(0) & 4) ? 0x102626bc : 0x10267b6c;
}
}
