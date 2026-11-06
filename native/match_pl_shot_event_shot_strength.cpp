#include "match_pl_shot_event_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static unsigned strength_scale(Cpu& c, std::int32_t action) {
    c.compare(0, action, 0x1b);
    if (c.cr(0) & 2) return 0x2c;
    if (c.cr(0) & 8) {
        c.compare(0, action, 0x17);
        if (c.cr(0) & 2) return 0x114;
        if (c.cr(0) & 8) {
            c.compare(0, action, 5);
            return (c.cr(0) & 2) ? 0x118 : 0x34;
        }
        c.compare(0, action, 0x19);
        if (c.cr(0) & 2) return 0x11c;
        return (c.cr(0) & 8) ? 0x40 : 0x110;
    }
    c.compare(0, action, 0x39);
    if (c.cr(0) & 2) return 0x110;
    if (c.cr(0) & 4) {
        c.compare(0, action, 0x3b);
        return (c.cr(0) & 8) ? 8 : 0x34;
    }
    c.compare(0, action, 0x35);
    if (c.cr(0) & 2) return 0xe4;
    if (c.cr(0) & 4) return 0x34;
    c.compare(0, action, 0x1d);
    return (c.cr(0) & 8) ? 0x114 : 0x34;
}

static void compute_shot_strength(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    c.set(6, c.u8(stack + 0x45));
    c.set(3, std::uint32_t(std::int16_t(c.r(16))));
    c.set(4, std::uint32_t(c.i16(actor + 0x3a)));
    c.set(7, std::uint32_t(std::int8_t(c.r(6))));
    c.set(0, c.u8(stack + 0x1cf));
    c.set(6, 0u - c.r(7));
    c.set(4, c.r(3) - c.r(4));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(5, std::uint32_t(c.i16(stack + 0x132)));
    c.set(0, c.r(4) - 100);
    c.set(6, c.r(6) | c.r(7));
    c.set(4, 100);
    c.compare(0, std::int32_t(c.r(3)), 0x1b);
    const auto mask = std::int32_t(c.r(6));
    c.s.wr<1>(reg::xer_ca, mask < 0 && (std::uint32_t(mask) & 0x7fffffff));
    c.set(6, std::uint32_t(mask >> 31));
    c.set(0, c.r(5) + c.r(0));
    c.set(4, c.r(4) & c.r(6));
    c.set(0, c.r(0) - c.r(4));
    const auto scale = strength_scale(c, std::int32_t(c.r(3)));
    c.set(3, c.toc(-0x51e0));
    c.set_f(2, c.f32(c.r(3) + scale));

    Fpu fp(c);
    c.compare(0, std::int32_t(c.r(0)), 0);
    const bool negative = (c.cr(0) & 8) != 0;
    const unsigned input = negative ? 0x104 : 0x114;
    const unsigned result = negative ? 0x10c : 0xfc;
    c.set(3, c.r(0) ^ 0x80000000);
    c.set(0, 0x43300000);
    c.put32(stack + input + 4, c.r(3));
    c.set_f(1, c.f64(c.r(27) + 0x80));
    c.put32(stack + input, c.r(0));
    c.set_f(0, fp.fsubs(c.f64(stack + input), c.f(1)));
    c.set_f(0, negative ? fp.fdivs(c.f(0), c.f(2)) : fp.fmuls(c.f(0), c.f(2)));
    c.put64(stack + result, fp.fctiwz(0, c.f(0)));
    c.set(0, c.u32(stack + result + 4));
}

void publish_shot_strength(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    c.set(0, c.u8(stack + 0x1cf));
    c.compare(0, std::int32_t(c.r(0)), 0x39);
    if (c.cr(0) & 2) {
        c.set(3, c.u8(actor + 0x2c));
        c.set(0, c.r(3) + 2);
        c.put8(actor + 0x2c, c.r(0));
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.set(5, stack + 0x4b);
    c.set(4, c.u8(actor + 0x27));
    c.set({{6, stack + 0x4a}, {7, stack + 0x49}, {8, stack + 0x48}, {9, stack + 0x47}, {10, 1}});
    c.call(0x101f21d0, 0x1025be2c);
    compute_shot_strength(c, actor, stack);
    c.set(4, c.u32(actor + 0x1a4));
    c.set(3, std::uint32_t(std::int8_t(c.r(13))));
    c.compare(0, std::int32_t(c.r(3)), 1);
    c.set(3, c.u32(c.r(4) + 0x310c));
    c.set(0, c.r(3) + c.r(0));
    c.put32(c.r(4) + 0x310c, c.r(0));
    if (!(c.cr(0) & 2)) {
        c.set(0, std::uint32_t(std::int8_t(c.r(13))));
        c.compare(0, std::int32_t(c.r(0)), 14);
        if (!(c.cr(0) & 2)) c.compare(0, std::int32_t(c.r(0)), 15);
    }
    if (c.cr(0) & 2) c.s.pc = 0x10262198;
    else {
        c.set(0, c.u8(stack + 0x128));
        c.compare(0, std::int32_t(c.r(0)), 2);
        c.s.pc = (c.cr(0) & 2) ? 0x1025bff0 : 0x1025c28c;
    }
}
}
