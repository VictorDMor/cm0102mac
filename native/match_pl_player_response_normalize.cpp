#include "match_pl_player_response.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void PlayerResponse::normalize_attributes() {
    constexpr auto integer_bias = 0x43300000u;
    const auto biased = [](std::int32_t value) { return std::uint32_t(value) ^ 0x80000000; };
    const auto publish = [&](unsigned slot) {
        c.put64(at(slot), fp.fctiwz(0, c.f(0)));
        c.put16(c.r(4) + 0x11e, c.u32(at(slot + 4)));
    };
    const auto penalized = [&](std::uint32_t cell, unsigned factor, unsigned base, unsigned scale, std::uint32_t site) {
        const auto penalty = c.i16(actor.address + 0x38);
        c.set(4, std::uint32_t(std::int32_t(penalty) * std::int32_t(factor)));
        c.set(5, integer_bias);
        c.put32(at(0x38), integer_bias);
        const auto score = c.i16(cell + 0x11e);
        c.set(4, c.r(4) + base);
        c.set(7, biased(score));
        c.set(6, biased(std::int32_t(c.r(4))));
        c.put32(at(0x3c), c.r(7));
        c.set_f(2, c.f64(doubles + 0x80));
        c.set_f(0, c.f64(at(0x38)));
        c.set(4, cell);
        c.put32(at(0x44), c.r(6));
        c.set_f(1, c.f64(doubles + 0x80));
        c.set_f(2, fp.fsub(c.f(0), c.f(2)));
        c.put32(at(0x40), c.r(5));
        c.set_f(3, c.f64(doubles + scale));
        c.set_f(0, c.f64(at(0x40)));
        c.set_f(0, fp.fsub(c.f(0), c.f(1)));
        c.set_f(0, fp.multiply_add(site, c.f(3), c.f(2), c.f(0)));
        publish(0x48);
    };
    const auto double_weight = [&](std::uint32_t cell, unsigned scale, std::uint32_t first_site, std::uint32_t second_site) {
        const auto penalty = c.i16(actor.address + 0x38);
        c.set(4, integer_bias);
        c.set(5, integer_bias);
        c.put32(at(0x44), biased(penalty));
        const auto score = c.i16(cell + 0x11e);
        c.put32(at(0x40), c.r(4));
        c.set(6, biased(score));
        c.set_f(1, c.f64(doubles + 0x80));
        c.set_f(0, c.f64(at(0x40)));
        c.set(4, cell);
        c.put32(at(0x3c), c.r(6));
        c.set_f(1, fp.fsub(c.f(0), c.f(1)));
        c.set_f(2, c.f64(doubles + 0xc0));
        c.put32(at(0x38), c.r(5));
        c.set_f(0, c.f64(doubles + 0xb0));
        c.set_f(4, c.f64(doubles + 0x80));
        c.set_f(3, c.f64(at(0x38)));
        c.set_f(0, fp.multiply_add(first_site, c.f(2), c.f(1), c.f(0)));
        c.set_f(2, c.f64(doubles + scale));
        c.set_f(1, fp.fsub(c.f(3), c.f(4)));
        c.set_f(0, fp.multiply_add(second_site, c.f(2), c.f(1), c.f(0)));
        publish(0x48);
    };

    // The normal activation initializes the index to zero. The original
    // signed loop condition remains separate from its unsigned table bound.
    while (compare(c, std::int16_t(c.r(3)), 63) < 0) {
        const auto index = std::int16_t(c.r(3));
        const auto cell = actor.address + std::uint32_t(std::int32_t(index)) * 2;
        if (compare_logical(c, std::uint32_t(std::int32_t(index)), 62) > 0) {
            c.set(4, cell);
            c.set(5, std::int32_t(c.i16(cell + 0x11e)));
            c.put16(cell + 0x11e, c.r(5) * 2);
        } else {
            const auto table = c.toc(-0x51f0);
            c.set(4, table);
            const auto target = c.u32(table + unsigned(index) * 4);
            c.set(4, target);
            c.s.wr<4>(reg::ctr, target);
            switch (target) {
            case 0x1022dfa8: penalized(cell, 5, 225, 0x28, 0x1022e00c); break;
            case 0x1022e024: penalized(cell, 7, 210, 0x268, 0x1022e088); break;
            case 0x1022e120: penalized(cell, 10, 250, 0xc0, 0x1022e184); break;
            case 0x1022e19c: double_weight(cell, 0x278, 0x1022e1f8, 0x1022e204); break;
            case 0x1022e21c: double_weight(cell, 0x1f8, 0x1022e278, 0x1022e284); break;
            case 0x1022e0a0: {
                const auto penalty = c.i16(actor.address + 0x38);
                c.set(4, std::int32_t(penalty) * 3);
                c.set(5, integer_bias);
                c.put32(at(0x40), integer_bias);
                const auto score = c.i16(cell + 0x11e);
                c.set(4, biased(std::int32_t(c.r(4))));
                c.set(6, biased(score));
                c.put32(at(0x44), c.r(4));
                c.set_f(1, c.f64(doubles + 0x80));
                c.set(4, cell);
                c.set_f(0, c.f64(at(0x40)));
                c.put32(at(0x3c), c.r(6));
                c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
                c.set_f(1, c.f32(c.r(29) + 0x88));
                c.put32(at(0x38), c.r(5));
                c.set_f(3, c.f64(doubles + 0x80));
                c.set_f(2, c.f64(at(0x38)));
                c.set_f(0, fp.multiply_add(0x1022e0fc, c.f(1), c.f(31), c.f(0)));
                c.set_f(4, c.f64(doubles + 0x270));
                c.set_f(1, fp.fsub(c.f(2), c.f(3)));
                c.set_f(0, fp.multiply_add(0x1022e108, c.f(4), c.f(1), c.f(0)));
                publish(0x48);
                break;
            }
            case 0x1022e29c:
            case 0x1022e2f4:
                c.set(4, integer_bias);
                c.put32(at(0x38), c.r(4));
                c.set(5, std::int32_t(c.i16(cell + 0x11e)));
                if (target == 0x1022e29c) {
                    c.set_f(2, c.f64(doubles + 0x80));
                    c.set(4, cell);
                    c.set_f(3, c.f64(doubles + 0x280));
                    c.put32(at(0x3c), biased(std::int32_t(c.r(5))));
                    c.set_f(0, c.f64(doubles + 0xd8));
                    c.set_f(1, c.f64(at(0x38)));
                    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
                    c.set_f(0, fp.multiply_add(0x1022e2dc, c.f(3), c.f(1), c.f(0)));
                } else {
                    c.set_f(1, c.f64(doubles + 0x80));
                    c.set(4, cell);
                    c.set_f(2, c.f64(doubles + 0x108));
                    c.put32(at(0x3c), biased(std::int32_t(c.r(5))));
                    c.set_f(0, c.f64(at(0x38)));
                    c.set_f(0, fp.fsub(c.f(0), c.f(1)));
                    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
                }
                publish(0x40);
                break;
            default:
                throw std::logic_error("MATCH_PL response normalization table target");
            }
        }
        c.set(3, c.r(3) + 1);
    }
    enter(c, 0x1022e384, {actor.address});
    refresh_attributes(c, actor.address);
}
}
