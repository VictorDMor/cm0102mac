#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void PlayerInfluence::positional_attribute() {
    c.set(2, c.u32(at(0x14))); c.set_f(3, c.f64(doubles + 0x40)); c.set_f(2, c.f64(doubles + 0x10));
    c.set_f(1, fp.fdiv(c.f(1), c.f(3))); c.set_f(0, c.f64(doubles + 0x160)); c.set_f(1, fp.fadd(c.f(2), c.f(1)));
    c.set_f(1, fp.frsp(c.f(1))); c.put_f32(actor.address + 0xb8, float(c.f(1)));
    // The threshold was loaded before the stored result is read back.
    c.set_f(1, c.f32(actor.address + 0xb8)); fp.fcmp(0, c.f(1), c.f(0));
    if ((c.cr(0) & 4) && compare(c, std::int8_t(c.r(28))) != 0) {
        c.set_f(1, c.f32(actor.address + 0xb8)); c.set_f(0, c.f64(doubles + 0x160)); c.set_f(2, c.f64(doubles + 0x190));
        c.set_f(1, fp.fsub(c.f(1), c.f(0))); c.call(0x10780fc0, 0x10228e60); c.set(2, c.u32(at(0x14)));
        c.set_f(0, c.f64(doubles + 0x160)); c.set_f(0, fp.fadd(c.f(0), c.f(1))); c.set_f(0, fp.frsp(c.f(0)));
        c.put_f32(actor.address + 0xb8, float(c.f(0)));
    }

    unsigned role_weight = 0;
    if (compare(c, std::int8_t(c.r(26)), 36) > 0) role_weight = 0x30;
    else {
        enter(c, 0x10228ec0, {actor.address}); current_role(c, actor.address);
        if (compare(c, c.r(3) & 0x40) == 0) {
            enter(c, 0x10228efc, {actor.address}); current_role(c, actor.address);
            if (compare(c, c.r(3) & 0x20) != 0) role_weight = 0x148;
            else {
                enter(c, 0x10228f44, {actor.address}); current_role(c, actor.address);
                if (compare(c, c.r(3) & 0x18) != 0) {
                    c.set(3, c.u32(actor.address + 0x70));
                    role_weight = compare(c, c.u8(c.r(3) + 0x12), 20) == 0 ? 0x1a0 : 0x1a8;
                } else role_weight = 0x130;
            }
        }
    }
    c.set(3, c.u32(actor.address + 0x70)); c.put32(at(0x38), 0x43300000);
    const auto positional_skill = c.i8(c.r(3) + 0x33); c.set_f(1, c.f64(doubles + 0x80));
    if (role_weight) c.set_f(2, c.f64(doubles + role_weight));
    c.put32(at(0x3c), std::uint32_t(std::int32_t(positional_skill)) ^ 0x80000000); c.set_f(0, c.f64(at(0x38)));
    if (role_weight) {
        c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(26, fp.fmul(c.f(2), c.f(0))); c.set_f(26, fp.frsp(c.f(26)));
    } else c.set_f(26, fp.fsubs(c.f(0), c.f(1)));

    const auto scale_position = [&](unsigned weight) {
        c.set_f(0, c.f64(doubles + weight)); c.set_f(26, fp.fmul(c.f(0), c.f(26))); c.set_f(26, fp.frsp(c.f(26)));
    };
    enter(c, 0x10229008, {actor.address}); both_roles_masked(c, actor.address);
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        enter(c, 0x10229018, {actor.address}); current_role(c, actor.address);
        if (compare(c, c.r(3) & 0x30) != 0) scale_position(0x1b0);
        else {
            enter(c, 0x10229038, {actor.address}); current_role(c, actor.address);
            bool inner_role = compare(c, c.r(3) & 8) != 0;
            if (!inner_role) {
                enter(c, 0x10229048, {actor.address}); secondary_role(c, actor.address);
                inner_role = compare(c, c.r(3) & 8) != 0;
            }
            if (inner_role) scale_position(0x158);
        }
    }
    const auto opposite = std::int8_t(actor.side()) == 0; c.set(3, actor.match()); c.set(4, opposite); c.call(0x101fee30, 0x10229078);
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        enter(c, 0x10229088, {actor.address}); both_roles_masked(c, actor.address);
        if (compare(c, std::int8_t(c.r(3))) != 0) {
            enter(c, 0x10229098, {actor.address}); current_role(c, actor.address);
            bool forward_role = compare(c, c.r(3) & 0x60) != 0;
            if (!forward_role) {
                enter(c, 0x102290a8, {actor.address}); secondary_role(c, actor.address);
                forward_role = compare(c, c.r(3) & 0x60) != 0;
            }
            if (forward_role) scale_position(0x158);
        }
    }
    c.set(3, actor.match()); c.set(4, actor.side()); c.call(0x101fedd0, 0x102290c8);
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        enter(c, 0x102290d8, {actor.address}); current_role(c, actor.address);
        bool outfield_role = compare(c, c.r(3) & 0x78) != 0;
        if (!outfield_role) {
            enter(c, 0x102290e8, {actor.address}); secondary_role(c, actor.address);
            outfield_role = compare(c, c.r(3) & 0x78) != 0;
        }
        if (outfield_role) scale_position(0x1b8);
    }
    c.set(3, std::int32_t(c.i16(actor.address + 0x3c))); c.put32(at(0x38), 0x43300000); c.set_f(1, c.f64(doubles + 0x80));
    c.put32(at(0x3c), c.r(3) ^ 0x80000000); c.set_f(2, c.f64(doubles + 0xc8)); c.set_f(0, c.f64(at(0x38)));
    c.set_f(3, c.f64(doubles + 0x130)); c.set_f(1, fp.fsub(c.f(0), c.f(1))); c.set_f(0, c.f64(doubles + 0xb8));
    c.set_f(1, fp.multiply_add(0x10229128, c.f(2), c.f(26), c.f(1))); c.set_f(1, fp.multiply_add(0x1022912c, c.f(3), c.f(27), c.f(1)));
    c.set_f(1, fp.frsp(c.f(1))); c.set_f(0, fp.fdiv(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + 0xbc, float(c.f(0)));
    const auto side = actor.side(); c.set(3, actor.match()); const bool away = compare(c, std::int8_t(side)) != 0;
    c.set(3, c.u32(c.r(3) + 0x3118)); c.set(3, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
    c.set({{4, 0}, {5, 1}, {6, 0}, {7, 0}}); c.call(0x1014c2e0, 0x10229178);
    weighted_coaching(0xbc, 28, 0x48, 0x1c0, 0x10229258, 0x10229264, 0x1022928c);
    c.set_f(3, c.f32(floats + 0x54)); c.set_f(2, c.f32(actor.address + 0xbc)); c.set_f(1, c.f32(floats + 0x50));
    c.set_f(0, c.f64(doubles + 0x88)); c.set_f(1, fp.multiply_add(0x102292bc, c.f(3), c.f(2), c.f(1)));
    c.put_f32(actor.address + 0xbc, float(c.f(1))); floor_attribute(0xbc);
    derived_attribute(0x35, 0xc0, 0x1022932c);
    derived_attribute(0x37, 0xc8, 0x102293b0, 0x170);
}

void PlayerInfluence::weighted_coaching(unsigned player_offset, unsigned coefficient_register, unsigned base_offset, unsigned fallback_offset,
    std::uint32_t coefficient_site, std::uint32_t rating_back, std::uint32_t adjustment_site) {
    bool coached = compare(c, std::int8_t(c.r(3))) == 0;
    const auto controller = [&] {
        const auto side = actor.side(); c.set(4, actor.match());
        c.set(3, std::uint32_t(std::int32_t(std::int8_t(side))) * 0xd8 + 0x7aac);
        c.set(3, c.r(4) + c.r(3)); return c.r(3);
    };
    if (coached) coached = compare_logical(c, c.u32(controller() + 0x20)) != 0;
    if (coached) {
        c.set(3, c.u32(controller() + 0x20)); coached = compare_logical(c, c.u32(c.r(3) + 0x1c)) != 0;
    }
    if (coached) {
        const auto first_side = actor.side(); c.set(4, 0x43300000); c.set(3, actor.side()); c.set(6, actor.side());
        c.set(5, std::uint32_t(std::int32_t(std::int8_t(first_side))) * 0xd8); c.set(3, std::int32_t(std::int8_t(c.r(3))));
        const auto third_side = std::int8_t(c.r(6)); c.put32(at(0x38), c.r(4)); c.set(6, actor.match());
        c.set(5, c.r(5) + 0x7aac); c.set(5, c.r(6) + c.r(5)); c.set(7, actor.match()); c.set(5, c.u32(c.r(5) + 0x20));
        c.set(4, c.r(3) * 0xd8); c.set_f(2, c.f64(doubles + 0x80)); c.set(5, c.u32(c.r(5) + 0x1c));
        c.set(3, std::uint32_t(std::int32_t(third_side)) * 0xd8); c.set(6, actor.match());
        const auto coaching_skill = c.i8(c.r(5) + 0x2e); c.set(4, c.r(4) + 0x7aac); c.set_f(3, c.f32(floats + 0x4c));
        c.set(5, c.r(3) + 0x7aac); c.put32(at(0x3c), std::uint32_t(std::int32_t(coaching_skill)) ^ 0x80000000);
        c.set(4, c.r(7) + c.r(4)); c.set(3, c.u32(c.r(4) + 0x20)); c.set(5, c.r(6) + c.r(5));
        c.set_f(1, c.f64(at(0x38))); c.set(4, c.u32(c.r(3) + 0x1c)); c.set_f(1, fp.fsubs(c.f(1), c.f(2)));
        c.set_f(0, c.f32(floats + base_offset)); c.set(5, c.u32(c.r(5) + 0x20)); c.set(4, c.u8(c.r(4) + 0x41));
        c.set_f(coefficient_register, fp.multiply_add(coefficient_site, c.f(3), c.f(1), c.f(0)));
        c.set(3, c.u32(c.r(5) + 0x1c)); c.call(0x100d2f50, rating_back);
        c.set(3, std::int32_t(std::int8_t(c.r(3)))); c.put32(at(0x40), 0x43300000); c.set_f(2, c.f64(doubles + 0x80));
        c.put32(at(0x44), (c.r(3) - 13) ^ 0x80000000); c.set_f(0, c.f32(actor.address + player_offset)); c.set_f(1, c.f64(at(0x40)));
        c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(0, fp.multiply_add(adjustment_site, c.f(coefficient_register), c.f(1), c.f(0)));
        c.put_f32(actor.address + player_offset, float(c.f(0)));
    } else {
        c.set_f(1, c.f64(doubles + fallback_offset)); c.set_f(0, c.f32(actor.address + player_offset));
        c.set_f(0, fp.fadd(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + player_offset, float(c.f(0)));
    }
}
}
