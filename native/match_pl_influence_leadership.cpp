#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void PlayerInfluence::leadership_attribute() {
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        enter(c, 0x10229408, {actor.address}); both_roles_masked(c, actor.address);
        if (compare(c, std::int8_t(c.r(3))) != 0) {
            enter(c, 0x10229418, {actor.address}); current_role(c, actor.address);
            bool attacking = compare(c, c.r(3) & 0x60) != 0;
            if (!attacking) {
                enter(c, 0x10229428, {actor.address}); secondary_role(c, actor.address);
                attacking = compare(c, c.r(3) & 0x60) != 0;
            }
            if (attacking) {
                c.set_f(1, c.f64(doubles + 0x1c8)); c.set_f(0, c.f32(actor.address + 0xc8));
                c.set_f(0, fp.fmul(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + 0xc8, float(c.f(0)));
            }
        }
    }
    soften_attribute(0xc8, 0x188, 0x168, 0x10229470);
    derived_attribute(0x38, 0xcc, 0x102294dc, 0x28);

    const bool veteran = compare(c, std::int8_t(c.r(26)), 36) > 0;
    c.set(3, c.u32(actor.address + 0x70)); const unsigned staging = veteran ? 0x38 : 0x40;
    c.put32(at(staging), 0x43300000); const auto leadership = c.i8(c.r(3) + 0x39); c.set_f(1, c.f64(doubles + 0x80));
    if (veteran) c.set_f(2, c.f64(doubles + 0x30));
    c.put32(at(staging + 4), std::uint32_t(std::int32_t(leadership)) ^ 0x80000000); c.set_f(0, c.f64(at(staging)));
    if (veteran) {
        c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(28, fp.fmul(c.f(2), c.f(0))); c.set_f(28, fp.frsp(c.f(28)));
    } else c.set_f(28, fp.fsubs(c.f(0), c.f(1)));
    const auto scale_leadership = [&](unsigned weight, unsigned source = 28) {
        c.set_f(0, c.f64(doubles + weight)); c.set_f(28, fp.fmul(c.f(0), c.f(source))); c.set_f(28, fp.frsp(c.f(28)));
    };
    enter(c, 0x1022957c, {actor.address}); both_roles_masked(c, actor.address);
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        enter(c, 0x1022958c, {actor.address}); current_role(c, actor.address);
        if (compare(c, c.r(3) & 4) != 0) scale_leadership(0x1b0);
        else {
            enter(c, 0x102295ac, {actor.address}); current_role(c, actor.address);
            if (compare(c, c.r(3) & 8) != 0) scale_leadership(0x1d0);
            else {
                enter(c, 0x102295cc, {actor.address}); current_role(c, actor.address);
                bool related_role = compare(c, c.r(3) & 0x10) != 0;
                if (!related_role) {
                    enter(c, 0x102295dc, {actor.address}); secondary_role(c, actor.address);
                    related_role = compare(c, c.r(3) & 0xc) != 0;
                }
                // Original 0x102295e8 deliberately takes the positional value
                // F26, not the leadership value F28 used by the other branches.
                if (related_role) scale_leadership(0x158, 26);
            }
        }
    }
    enter(c, 0x102295f8, {actor.address}); current_role(c, actor.address);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) {
        bool inexperienced = compare(c, std::int8_t(c.r(26)), 21) < 0;
        if (!inexperienced) {
            c.set(3, c.u32(actor.address + 0x70)); inexperienced = compare(c, c.i16(c.r(3) + 0xc), 3750) < 0;
        }
        if (inexperienced) scale_leadership(0x1b8);
        else if (compare(c, std::int8_t(c.r(26)), 23) < 0) scale_leadership(0x1d8);
    }
    enter(c, 0x10229650, {actor.address}); current_role(c, actor.address);
    bool defensive_role = compare(c, c.r(3) & 4) != 0;
    if (!defensive_role) {
        enter(c, 0x10229660, {actor.address}); secondary_role(c, actor.address);
        defensive_role = compare(c, c.r(3) & 4) != 0;
    }
    if (defensive_role) {
        enter(c, 0x10229670, {actor.address}); current_role(c, actor.address);
        const auto primary_role = std::int16_t(c.r(3));
        c.set(3, actor.address); keep(c, {{20, std::uint32_t(std::int32_t(primary_role))}});
        enter(c, 0x10229680, {actor.address}); secondary_role(c, actor.address);
        if (compare(c, std::int16_t(c.r(3)), std::int32_t(c.r(20))) != 0) {
            enter(c, 0x10229694, {actor.address}); role_distance(c, actor.address);
            scale_leadership(compare(c, std::int8_t(c.r(3)), 1) > 0 ? 0x1e0 : 0x148);
        }
    }
    c.set(3, std::int32_t(c.i16(actor.address + 0x3c))); c.put32(at(0x38), 0x43300000); c.set_f(1, c.f64(doubles + 0x80));
    c.put32(at(0x3c), c.r(3) ^ 0x80000000); c.set_f(2, c.f64(doubles + 0xc8)); c.set_f(0, c.f64(at(0x38)));
    c.set_f(3, c.f64(doubles + 0x150)); c.set_f(1, fp.fsub(c.f(0), c.f(1))); c.set_f(0, c.f64(doubles + 0xb8));
    c.set_f(1, fp.multiply_add(0x102296e8, c.f(2), c.f(28), c.f(1))); c.set_f(1, fp.multiply_add(0x102296ec, c.f(3), c.f(27), c.f(1)));
    c.set_f(1, fp.fadd(c.f(31), c.f(1))); c.set_f(1, fp.frsp(c.f(1))); c.set_f(0, fp.fdiv(c.f(1), c.f(0)));
    c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + 0xd0, float(c.f(0)));
    const auto side = actor.side(); c.set(3, actor.match()); const bool away = compare(c, std::int8_t(side)) != 0;
    c.set(3, c.u32(c.r(3) + 0x3118)); c.set(3, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
    c.set({{4, 0}, {5, 1}, {6, 0}, {7, 0}}); c.call(0x1014c2e0, 0x1022973c);
    weighted_coaching(0xd0, 26, 8, 0x1c0, 0x1022981c, 0x10229828, 0x10229850);
}
}
