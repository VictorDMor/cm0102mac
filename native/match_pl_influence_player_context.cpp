#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;

// After the first role query, the retained fixture can differ from the
// actor's live match if an external service updated the actor.
void PlayerInfluence::contextual_ratings() {
    const auto fixture = c.r(20);
    if (compare(c, std::int16_t(c.r(3)), 1) != 0 &&
        compare_logical(c, fixture) != 0 && compare_logical(c, c.u32(fixture + 0x14)) != 0) {
        c.set(4, c.u32(fixture + 0x14));
        c.set(3, c.toc(-0x7b70));
        c.set(4, c.u32(c.r(4)));
        if (compare(c, std::int32_t(c.r(4)), std::int32_t(c.u32(c.r(3)))) == 0) {
            c.set(3, c.u32(actor.address + 0x6c));
            if (compare_logical(c, c.u32(c.r(3) + 0xc)) != 0) {
                bool shared_identity = true;
                for (const int identity : {-0x7a20, -0x653c, -0x74dc}) {
                    c.set(4, c.u32(actor.address + 0x6c));
                    c.set(3, c.toc(identity));
                    c.set(4, c.u32(c.r(4) + 0xc));
                    const auto expected = c.u32(c.r(3));
                    c.set(3, c.u32(c.r(4) + 0x68));
                    if (compare(c, std::int32_t(c.r(3)), std::int32_t(expected)) != 0) {
                        shared_identity = false;
                        break;
                    }
                }
                if (!shared_identity) {
                    c.set_f(1, c.f32(actor.address + 0xf8));
                    c.set_f(0, c.f32(floats + 0x14));
                    c.set_f(0, fp.fmuls(c.f(1), c.f(0)));
                    c.put_f32(actor.address + 0xf8, float(c.f(0)));
                }
            }

            c.set_f(2, c.f32(actor.address + 0xa0));
            c.set_f(0, c.f32(floats + 0x68));
            c.set_f(1, c.f32(floats + 0x68));
            c.set_f(2, fp.fmuls(c.f(2), c.f(0)));
            c.set_f(0, c.f32(floats + 0x14));
            c.put_f32(actor.address + 0xa0, float(c.f(2)));
            c.set_f(2, c.f32(actor.address + 0xf4));
            c.set_f(1, fp.fmuls(c.f(2), c.f(1)));
            c.put_f32(actor.address + 0xf4, float(c.f(1)));
            c.set_f(1, c.f32(actor.address + 0xbc));
            c.set_f(0, fp.fmuls(c.f(1), c.f(0)));
            c.put_f32(actor.address + 0xbc, float(c.f(0)));
        }
    }
    enter(c, 0x1022a8f8, {actor.address});
    current_role(c, actor.address);
    if (compare(c, std::int16_t(c.r(3)), 1) != 0) {
        c.set(3, c.u32(actor.address + 0x70));
        bool limited = compare(c, c.u8(c.r(3) + 0xf), 20) == 0;
        for (unsigned offset : {0x11u, 0x12u, 0x13u, 0x14u, 0x15u}) {
            if (!limited) break;
            c.set(3, c.u32(actor.address + 0x70));
            limited = compare(c, c.i8(c.r(3) + offset), offset == 0x13 ? 15 : 10) < 0;
        }
        if (limited) limit_contextual_ratings();
    }
    personality_rating();
}

void PlayerInfluence::limit_contextual_ratings() {
    c.set_f(2, c.f32(actor.address + 0xa0));
    c.set_f(1, c.f32(floats + 0x6c));
    c.set_f(0, c.f32(floats + 0x70));
    c.set_f(1, fp.fmuls(c.f(2), c.f(1)));
    for (unsigned f : {8u, 7u, 6u}) c.set_f(f, c.f32(floats + 0x70));
    c.put_f32(actor.address + 0xa0, float(c.f(1)));
    c.set_f(5, c.f32(floats + 0x10));
    c.set_f(1, c.f32(actor.address + 0xf4));
    c.set_f(4, c.f32(floats + 0xc));
    c.set_f(0, fp.fmuls(c.f(1), c.f(0)));
    c.set_f(3, c.f32(floats + 8));
    c.set_f(2, c.f32(floats + 8));
    c.set_f(1, c.f32(floats + 0xc));
    c.put_f32(actor.address + 0xf4, float(c.f(0)));
    c.set_f(0, c.f32(floats + 0xc));
    constexpr unsigned ratings[]{0x98, 0x94, 0x80, 0xb0, 0xac, 0xd0, 0xc8, 0xe0, 0xbc};
    for (unsigned i = 0; i < std::size(ratings); ++i) {
        const unsigned coefficient = 8 - i;
        c.set_f(coefficient + 1, c.f32(actor.address + ratings[i]));
        c.set_f(coefficient, fp.fmuls(c.f(coefficient + 1), c.f(coefficient)));
        c.put_f32(actor.address + ratings[i], float(c.f(coefficient)));
    }
}

void PlayerInfluence::personality_rating() {
    c.set_f(5, c.f64(doubles + 0x28));
    const bool older = compare(c, std::int8_t(c.r(26)), 36) >= 0;
    c.set(3, c.u32(actor.address + 0x70));
    const unsigned staging = older ? 0x40 : 0x38;
    c.put32(at(staging), 0x43300000);
    const auto trait = c.i8(c.r(3) + 0x44);
    c.set_f(1, c.f64(doubles + 0x80));
    if (older) c.set_f(2, c.f64(doubles + 0x30));
    c.put32(at(staging + 4), std::uint32_t(std::int32_t(trait)) ^ 0x80000000);
    c.set_f(0, c.f64(at(staging)));
    if (older) {
        c.set_f(0, fp.fsub(c.f(0), c.f(1)));
        c.set_f(4, fp.frsp(fp.fmul(c.f(2), c.f(0))));
    } else c.set_f(4, fp.fsubs(c.f(0), c.f(1)));

    c.set(3, c.u32(actor.address + 0x6c));
    c.set(5, c.u8(actor.address + 0x103));
    c.set(4, 0x43300000);
    c.set(3, c.u8(c.r(3) + 0x67));
    c.set(5, std::int32_t(std::int8_t(c.r(5))));
    c.put32(at(0x50), 0x43300000);
    const auto temperament = std::int32_t(std::int8_t(c.r(3)));
    c.set(3, temperament);
    c.set_f(1, c.f64(doubles + 0x80));
    c.set(5, c.r(5) ^ 0x80000000);
    const auto biased = temperament + (temperament < 0);
    c.put32(at(0x4c), c.r(5));
    c.s.wr<1>(reg::xer_ca, biased < 0 && (biased & 1));
    c.set(3, biased >> 1);
    c.set_f(3, c.f64(doubles + 0x80));
    c.put32(at(0x48), c.r(4));
    c.put32(at(0x54), (c.r(3) + 10) ^ 0x80000000);
    c.set_f(2, c.f64(at(0x48)));
    c.set_f(0, c.f64(at(0x50)));
    c.set_f(2, fp.fsubs(c.f(2), c.f(3)));
    c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
    c.set_f(1, fp.fadds(c.f(4), c.f(2)));
    c.set_f(0, fp.fdivs(c.f(27), c.f(0)));
    c.set_f(0, fp.fsubs(c.f(1), c.f(0)));
    c.set_f(0, fp.frsp(fp.multiply_add(0x1022ab00, c.f(29), c.f(5), c.f(0))));
    c.put_f32(actor.address + 0xf8, float(c.f(0)));
}
}
