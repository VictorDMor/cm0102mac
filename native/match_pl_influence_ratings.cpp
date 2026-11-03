#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void PlayerInfluence::floor_attribute(unsigned player_offset) {
    c.set_f(1, c.f32(actor.address + player_offset)); fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 8) { c.set_f(0, c.f32(floats)); c.put_f32(actor.address + player_offset, float(c.f(0))); }
}

void PlayerInfluence::role_attribute_scale(unsigned player_offset, std::uint32_t first_back, std::uint32_t second_back) {
    enter(c, first_back, {actor.address}); current_role(c, actor.address);
    if (compare(c, c.r(3) & 0x20) == 0) return;
    enter(c, second_back, {actor.address}); current_role(c, actor.address);
    if (compare(c, c.r(3) & 0xc) == 0) return;
    c.set_f(1, c.f64(doubles + 0x148)); c.set_f(0, c.f32(actor.address + player_offset));
    c.set_f(0, fp.fmul(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + player_offset, float(c.f(0)));
}

void PlayerInfluence::soften_attribute(unsigned player_offset, unsigned threshold, unsigned exponent, std::uint32_t power_back, bool club_gate) {
    c.set_f(1, c.f32(actor.address + player_offset)); c.set_f(0, c.f64(doubles + threshold)); fp.fcmp(0, c.f(1), c.f(0));
    if (!(c.cr(0) & 4) || compare(c, std::int8_t(c.r(28))) == 0) return;
    if (club_gate) {
        c.set(3, c.u32(actor.address + 0xfc)); c.call(0x100c7d00, 0x10228b94);
        if (compare(c, std::int8_t(c.r(3))) != 0) return;
    }
    c.set_f(1, c.f32(actor.address + player_offset)); c.set_f(0, c.f64(doubles + threshold)); c.set_f(2, c.f64(doubles + exponent));
    c.set_f(1, fp.fsub(c.f(1), c.f(0))); c.call(0x10780fc0, power_back); c.set(2, c.u32(at(0x14)));
    c.set_f(0, c.f64(doubles + threshold)); c.set_f(0, fp.fadd(c.f(0), c.f(1))); c.set_f(0, fp.frsp(c.f(0)));
    c.put_f32(actor.address + player_offset, float(c.f(0)));
}

void PlayerInfluence::derived_attribute(unsigned profile_offset, unsigned player_offset, std::uint32_t multiply_site, unsigned penalty_weight) {
    // These original attribute formulas share their integer staging and result
    // layout. Weighted penalties have a different FPR layout and use one fused
    // negative multiply-subtract rather than a separate subtraction.
    c.set(5, c.u32(actor.address + 0x70)); c.set(4, 0x43300000); c.set(3, std::int32_t(c.i16(actor.address + 0x3c)));
    c.set(5, c.u8(c.r(5) + profile_offset)); c.set(3, c.r(3) ^ 0x80000000); c.put32(at(0x38), c.r(4));
    c.set(4, std::int32_t(std::int8_t(c.r(5)))); c.set_f(3, c.f64(doubles + 0x80)); c.set(4, c.r(4) ^ 0x80000000);
    c.put32(at(0x44), c.r(3)); c.set_f(penalty_weight ? 1 : 2, c.f64(doubles + 0x80)); c.put32(at(0x3c), c.r(4));
    c.set_f(4, c.f64(doubles + 0xc8)); c.put32(at(0x40), 0x43300000);
    c.set_f(penalty_weight ? 2 : 1, c.f64(at(0x38))); c.set_f(0, c.f64(at(0x40)));
    c.set_f(3, fp.fsub(c.f(penalty_weight ? 2 : 1), c.f(3)));
    if (penalty_weight) {
        c.set_f(5, c.f64(doubles + penalty_weight)); c.set_f(2, fp.fsub(c.f(0), c.f(1))); c.set_f(1, c.f64(doubles + 0xb8));
    } else {
        c.set_f(1, c.f64(doubles + 0xb8)); c.set_f(2, fp.fsub(c.f(0), c.f(2)));
    }
    c.set_f(0, c.f64(doubles + 0x88)); c.set_f(2, fp.multiply_add(multiply_site, c.f(4), c.f(3), c.f(2)));
    c.set_f(2, penalty_weight ? fp.multiply_add(multiply_site + 4, c.f(5), c.f(27), c.f(2)) : fp.fsub(c.f(2), c.f(27)));
    c.set_f(2, fp.frsp(c.f(2))); c.set_f(1, fp.fdiv(c.f(2), c.f(1))); c.set_f(1, fp.frsp(c.f(1)));
    c.put_f32(actor.address + player_offset, float(c.f(1))); floor_attribute(player_offset);
}

void PlayerInfluence::technical_attributes() {
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        enter(c, 0x102288cc, {actor.address}); both_roles_masked(c, actor.address);
        if (compare(c, std::int8_t(c.r(3))) != 0) {
            enter(c, 0x102288dc, {actor.address}); current_role(c, actor.address);
            bool attacking = compare(c, c.r(3) & 0x60) != 0;
            if (!attacking) {
                enter(c, 0x102288ec, {actor.address}); secondary_role(c, actor.address);
                attacking = compare(c, c.r(3) & 0x60) != 0;
            }
            if (attacking) {
                c.set_f(1, c.f64(doubles + 0x158)); c.set_f(0, c.f32(actor.address + 0x90));
                c.set_f(0, fp.fmul(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + 0x90, float(c.f(0)));
            }
        }
    }
    c.set_f(1, c.f32(actor.address + 0x90)); c.set_f(0, c.f64(doubles + 0x88)); fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 8) { c.set_f(0, c.f32(floats)); c.put_f32(actor.address + 0x90, float(c.f(0))); }
    soften_attribute(0x90, 0x160, 0x168, 0x1022894c);
    derived_attribute(0x24, 0x94, 0x102289b4);

    // This attribute prepares r3 for the role query while using r6 for the
    // profile byte; preserve those live arguments and its distinct FPR layout.
    c.set(3, c.u32(actor.address + 0x70)); c.set(5, 0x43300000); c.set(4, std::int32_t(c.i16(actor.address + 0x3c)));
    c.set(6, c.u8(c.r(3) + 0x26)); c.set(3, actor.address); c.set(4, c.r(4) ^ 0x80000000); c.put32(at(0x38), c.r(5));
    c.set(5, std::int32_t(std::int8_t(c.r(6)))); c.set_f(3, c.f64(doubles + 0x80)); c.set(5, c.r(5) ^ 0x80000000);
    c.put32(at(0x44), c.r(4)); c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x3c), c.r(5)); c.set_f(4, c.f64(doubles + 0xc8));
    c.put32(at(0x40), 0x43300000); c.set_f(2, c.f64(at(0x38))); c.set_f(0, c.f64(at(0x40)));
    c.set_f(2, fp.fsub(c.f(2), c.f(3))); c.set_f(3, c.f64(doubles + 0x170)); c.set_f(1, fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, c.f64(doubles + 0xb8)); c.set_f(1, fp.multiply_add(0x10228a38, c.f(4), c.f(2), c.f(1)));
    c.set_f(1, fp.multiply_add(0x10228a3c, c.f(3), c.f(27), c.f(1))); c.set_f(1, fp.frsp(c.f(1)));
    c.set_f(0, fp.fdiv(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + 0x98, float(c.f(0)));
    role_attribute_scale(0x98, 0x10228a54, 0x10228a64);
    c.set_f(1, c.f32(actor.address + 0x98)); c.set_f(0, c.f64(doubles + 0x88)); fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 8) { c.set_f(0, c.f32(floats)); c.put_f32(actor.address + 0x98, float(c.f(0))); }

    derived_attribute(0x27, 0x9c, 0x10228af0, 0x178);
    enter(c, 0x10228b24, {actor.address}); both_roles_masked(c, actor.address);
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        c.set_f(1, c.f64(doubles + 0x180)); c.set_f(0, c.f32(actor.address + 0x9c));
        c.set_f(0, fp.fmul(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + 0x9c, float(c.f(0)));
    }
    role_attribute_scale(0x9c, 0x10228b48, 0x10228b58);
    soften_attribute(0x9c, 0x160, 0x168, 0x10228bb0, true);
    derived_attribute(0x2a, 0xa8, 0x10228c18);
    derived_attribute(0x2b, 0xac, 0x10228c9c, 0x150);
    soften_attribute(0xac, 0x188, 0x190, 0x10228cf4);
    derived_attribute(0x31, 0xb4, 0x10228d5c);
    derived_attribute(0x32, 0xb8, 0x10228de0, 0x28);
}
}
