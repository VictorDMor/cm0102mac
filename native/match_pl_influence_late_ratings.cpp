#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void PlayerInfluence::late_ratings() {
    c.set(2, c.u32(at(0x14)));
    c.set_f(3, c.f64(doubles + 0x40));
    c.set_f(2, c.f64(doubles + 0x10));
    c.set_f(1, fp.fdiv(c.f(1), c.f(3)));
    c.set_f(0, c.f64(doubles + 0x88));
    c.set_f(1, fp.frsp(fp.fadd(c.f(2), c.f(1))));
    c.put_f32(actor.address + 0xd0, float(c.f(1)));
    floor_attribute(0xd0);

    c.set_f(1, c.f32(actor.address + 0xd0));
    c.set_f(0, c.f64(doubles + 0x160));
    fp.fcmp(0, c.f(1), c.f(0));
    if ((c.cr(0) & 4) && compare(c, std::int8_t(c.r(28))) != 0) {
        enter(c, 0x102298d0, {actor.address});
        current_role(c, actor.address);
        if (compare(c, std::int16_t(c.r(3)), 1) != 0) {
            c.set_f(1, c.f32(actor.address + 0xd0));
            c.set_f(0, c.f64(doubles + 0x160));
            c.set_f(2, c.f64(doubles + 0x190));
            c.set_f(1, fp.fsub(c.f(1), c.f(0)));
            c.call(0x10780fc0, 0x102298f0);
            c.set(2, c.u32(at(0x14)));
            c.set_f(0, c.f64(doubles + 0x160));
            c.set_f(0, fp.frsp(fp.fadd(c.f(0), c.f(1))));
            c.put_f32(actor.address + 0xd0, float(c.f(0)));
        }
    }

    aged_profile_rating(0x3a, 4);
    c.set(3, std::int32_t(c.i16(actor.address + 0x3c)));
    c.put32(at(0x38), 0x43300000);
    c.set_f(2, c.f64(doubles + 0x80));
    c.put32(at(0x3c), c.r(3) ^ 0x80000000);
    c.set_f(3, c.f64(doubles + 0xc8));
    c.set_f(0, c.f64(at(0x38)));
    c.set_f(1, c.f64(doubles + 0xb8));
    c.set_f(2, fp.fsub(c.f(0), c.f(2)));
    c.set_f(0, c.f64(doubles + 0x88));
    c.set_f(2, fp.multiply_add(0x102299e0, c.f(3), c.f(4), c.f(2)));
    c.set_f(2, fp.frsp(fp.fsub(c.f(2), c.f(27))));
    c.set_f(1, fp.frsp(fp.fdiv(c.f(2), c.f(1))));
    c.put_f32(actor.address + 0xd4, float(c.f(1)));
    floor_attribute(0xd4);

    derived_attribute(0x3e, 0xe0, 0x10229a64, 0x178);
    derived_attribute(0x41, 0xec, 0x10229ae4, 0, false);

    c.set(3, c.u32(actor.address + 0x70));
    if (compare(c, c.i8(c.r(3) + 0x43), 10) <= 0) {
        c.set(3, c.u32(actor.address + 0x70));
        c.put32(at(0x38), 0x43300000);
        const auto trait = c.i8(c.r(3) + 0x43);
        c.set_f(1, c.f64(doubles + 0x80));
        c.put32(at(0x3c), std::uint32_t(std::int32_t(trait)) ^ 0x80000000);
        c.set_f(0, c.f64(at(0x38)));
        c.set_f(25, fp.fsubs(c.f(0), c.f(1)));
    } else {
        c.set(3, c.u32(actor.address + 0x70));
        const bool low_ability = compare(c, c.i16(c.r(3) + 4), 80) < 0;
        bool scale_trait = low_ability;
        if (!low_ability) {
            c.set(3, c.u32(actor.address + 0x70));
            scale_trait = compare(c, c.i16(c.r(3) + 4), 125) < 0;
        }
        // The strongest players retain incoming F25 on this branch.
        if (scale_trait) {
            c.set(3, c.u32(actor.address + 0x70));
            c.put32(at(0x38), 0x43300000);
            const auto trait = c.i8(c.r(3) + 0x43);
            c.set_f(2, c.f64(doubles + 0x80));
            c.set(3, std::int32_t(trait));
            c.set_f(3, c.f64(doubles + (low_ability ? 0x30 : 0x1a0)));
            c.set_f(0, c.f64(doubles + 0xb8));
            c.put32(at(0x3c), (c.r(3) - 10) ^ 0x80000000);
            c.set_f(1, c.f64(at(0x38)));
            c.set_f(1, fp.fsubs(c.f(1), c.f(2)));
            const auto site = low_ability ? 0x10229b64u : 0x10229bb4u;
            c.set_f(25, fp.frsp(fp.multiply_add(site, c.f(3), c.f(1), c.f(0))));
        }
    }
    c.set(3, actor.match());
    c.set(4, actor.side());
    c.call(0x101fedd0, 0x10229bf4);
}

void PlayerInfluence::aged_profile_rating(unsigned profile_offset, unsigned result_register) {
    const auto age = std::int8_t(c.r(26));
    unsigned weight = 0;
    if (compare(c, age, 39) > 0) weight = 0x130;
    else if (compare(c, age, 37) > 0) weight = 0x1a8;
    c.set(3, c.u32(actor.address + 0x70));
    c.put32(at(0x38), 0x43300000);
    const auto trait = c.i8(c.r(3) + profile_offset);
    c.set_f(1, c.f64(doubles + 0x80));
    if (weight) c.set_f(2, c.f64(doubles + weight));
    c.put32(at(0x3c), std::uint32_t(std::int32_t(trait)) ^ 0x80000000);
    c.set_f(0, c.f64(at(0x38)));
    if (weight) {
        c.set_f(0, fp.fsub(c.f(0), c.f(1)));
        c.set_f(result_register, fp.frsp(fp.fmul(c.f(2), c.f(0))));
    } else c.set_f(result_register, fp.fsubs(c.f(0), c.f(1)));
}
}
