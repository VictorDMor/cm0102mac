#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void PlayerInfluence::coaching_effects() {
    // The controller and staff are reread at each original decision: an
    // external context query can change the actor's side or match.
    bool coached = compare(c, std::int8_t(c.r(3))) == 0;
    const auto controller = [&] {
        const auto side = actor.side(); c.set(4, actor.match());
        c.set(3, std::uint32_t(std::int32_t(std::int8_t(side))) * 0xd8 + 0x7aac);
        c.set(3, c.r(4) + c.r(3)); return c.r(3);
    };
    if (coached) coached = compare_logical(c, c.u32(controller() + 0x20)) != 0;
    if (coached) {
        c.set(3, c.u32(controller() + 0x20));
        coached = compare_logical(c, c.u32(c.r(3) + 0x1c)) != 0;
    }
    if (coached) {
        const auto first_side = actor.side(); c.set(3, actor.side()); c.set(6, actor.match());
        c.set(4, std::uint32_t(std::int32_t(std::int8_t(first_side))) * 0xd8); c.set(5, actor.match());
        const auto second_side = std::int8_t(c.r(3)); c.set(4, c.r(4) + 0x7aac);
        c.set(3, std::uint32_t(std::int32_t(second_side)) * 0xd8); c.set(4, c.r(6) + c.r(4));
        c.set(6, c.r(3) + 0x7aac); c.set(3, c.u32(c.r(4) + 0x20)); c.set(6, c.r(5) + c.r(6));
        c.set(4, c.u32(c.r(3) + 0x1c)); c.set(5, c.u32(c.r(6) + 0x20));
        c.set(4, c.u8(c.r(4) + 0x41)); c.set(3, c.u32(c.r(5) + 0x1c)); c.call(0x100d2f50, 0x10228780);
        c.set(3, std::int32_t(std::int8_t(c.r(3)))); c.put32(at(0x38), 0x43300000);
        c.set_f(3, c.f64(doubles + 0x80)); c.put32(at(0x3c), (c.r(3) - 13) ^ 0x80000000);
        c.set_f(1, c.f64(doubles + 0x138)); c.set_f(2, c.f64(at(0x38))); c.set_f(0, c.f32(actor.address + 0x80));
        c.set_f(2, fp.fsubs(c.f(2), c.f(3))); c.set_f(0, fp.multiply_add(0x102287ac, c.f(1), c.f(2), c.f(0)));
        c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + 0x80, float(c.f(0)));
    } else {
        c.set_f(1, c.f64(doubles + 0x140)); c.set_f(0, c.f32(actor.address + 0x80));
        c.set_f(0, fp.fadd(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + 0x80, float(c.f(0)));
    }
    c.set_f(1, c.f32(actor.address + 0x80)); c.set_f(0, c.f64(doubles + 0x88)); fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 8) { c.set_f(0, c.f32(floats)); c.put_f32(actor.address + 0x80, float(c.f(0))); }
    enter(c, 0x102287f0, {actor.address}); current_role(c, actor.address);
    if (compare(c, c.r(3) & 0x20) != 0) {
        enter(c, 0x10228800, {actor.address}); current_role(c, actor.address);
        if (compare(c, c.r(3) & 0xc) != 0) {
            c.set_f(1, c.f64(doubles + 0x148)); c.set_f(0, c.f32(actor.address + 0x80));
            c.set_f(0, fp.fmul(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + 0x80, float(c.f(0)));
        }
    }
    c.set_f(2, c.f32(actor.address + 0x80)); c.set(3, 0x43300000); c.set_f(1, c.f64(doubles + 0x28));
    c.set_f(0, c.f64(doubles + 0x18)); c.put32(at(0x38), c.r(3));
    c.set_f(0, fp.multiply_add(0x10228834, c.f(2), c.f(1), c.f(0)));
    c.set_f(4, c.f64(doubles + 0x80)); c.put32(at(0x40), 0x43300000); c.set_f(2, c.f64(doubles + 0x80));
    c.set_f(5, c.f64(doubles + 0xc8)); c.set_f(1, fp.frsp(c.f(0))); c.set_f(6, c.f64(doubles + 0x150));
    c.set_f(0, c.f64(doubles + 0xb8)); c.put_f32(actor.address + 0x80, float(c.f(1)));
    c.set(3, c.u32(actor.address + 0x70)); const auto ability = c.i16(actor.address + 0x3c);
    c.set(3, c.u8(c.r(3) + 0x23)); c.set(3, std::int32_t(std::int8_t(c.r(3))));
    c.put32(at(0x44), std::uint32_t(std::int32_t(ability)) ^ 0x80000000); c.put32(at(0x3c), c.r(3) ^ 0x80000000);
    c.set_f(1, c.f64(at(0x40))); c.set_f(3, c.f64(at(0x38))); c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(2, fp.fsub(c.f(3), c.f(4))); c.set_f(1, fp.multiply_add(0x10228888, c.f(5), c.f(2), c.f(1)));
    c.set_f(1, fp.multiply_add(0x1022888c, c.f(6), c.f(27), c.f(1))); c.set_f(1, fp.frsp(c.f(1)));
    c.set_f(0, fp.fdiv(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + 0x90, float(c.f(0)));
}
}
