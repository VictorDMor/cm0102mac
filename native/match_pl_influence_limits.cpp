#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;
void PlayerInfluence::experience_penalty() {
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        enter(c, 0x10228230, {actor.address}); current_role(c, actor.address);
        if (compare(c, std::int16_t(c.r(3)), 1) != 0) { c.set_f(0, c.f32(floats + 0x3c)); c.set_f(27, fp.fadds(c.f(0), c.f(27))); }
    }
    c.set(3, c.u8(actor.address + 0x17)); c.set(4, c.u8(actor.address + 0x11b)); c.set(5, 0x43300000);
    c.set(3, std::int32_t(std::int8_t(c.r(3)))); c.set(6, std::int32_t(c.i16(actor.address + 0x3c)));
    c.set(4, std::int32_t(std::int8_t(c.r(4)))); c.put32(at(0x40), 0x43300000);
    const auto adjustment = c.r(4) * (c.r(3) - 10); c.set(3, c.r(6) ^ 0x80000000);
    c.set_f(2, c.f64(doubles + 0x80)); c.set_f(0, c.f64(doubles + 0x120)); c.put32(at(0x3c), c.r(3));
    c.set_f(3, c.f64(doubles + 0x80)); c.put32(at(0x38), c.r(5)); c.set_f(1, c.f64(at(0x38)));
    c.put32(at(0x44), adjustment ^ 0x80000000); c.set_f(3, fp.fsubs(c.f(1), c.f(3))); c.set_f(1, c.f64(at(0x40)));
    c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(0, fp.fdiv(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0)));
    c.set_f(0, fp.fadds(c.f(3), c.f(0))); c.put64(at(0x48), fp.fctiwz(0, c.f(0))); c.put16(actor.address + 0x3c, c.u32(at(0x4c)));
    c.set(3, actor.match()); if (compare_logical(c, c.u32(c.r(3) + 0x3118)) == 0) return;
    c.set(3, actor.match()); c.set(3, c.u32(c.r(3) + 0x3118)); if (compare_logical(c, c.u32(c.r(3) + 0x14)) == 0) return;
    c.set(3, actor.match()); c.set(3, c.u32(c.r(3) + 0x3118)); if (compare(c, c.u8(c.r(3) + 0x3a), 6) == 0) return;
    c.set(3, actor.match()); c.set(3, c.u32(c.r(3) + 0x3118)); c.set(3, c.u32(c.r(3) + 0x14));
    if (compare(c, c.i16(c.r(3) + 0x18)) == 0) return;
    if (compare_logical(c, c.u32(actor.address + 0x6c)) != 0) {
        c.set(3, c.u32(actor.address + 0x6c)); const auto count = c.u32(c.r(19)); c.set(3, c.u32(c.r(3)));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(count)) >= 0) return;
    }
    c.set(3, c.u32(actor.address + 0x6c)); if (compare(c, std::int8_t(c.i16(c.r(3) + 0x5c)), 24) >= 0) return;
    c.set(3, actor.match()); c.set(4, c.u32(actor.address + 0x6c)); c.set(5, c.u32(c.r(3) + 0x3118));
    c.set(3, c.u32(actor.address + 0x70)); c.set(5, c.u32(c.r(5) + 0x14)); c.set(4, c.u8(c.r(4) + 0x66));
    c.set(5, std::int32_t(c.i16(c.r(5) + 0x18))); const auto reputation = c.i8(c.r(3) + 0xf);
    c.set(3, std::int32_t(std::int8_t(c.r(4)))); c.set(4, std::int32_t(std::int16_t(c.r(5)))); c.set(4, c.r(4) * 500);
    const bool capped_reputation = compare(c, reputation, 20) == 0;
    c.set(3, c.r(3) * 150); const auto competition = std::uint32_t(std::int32_t(std::int16_t(c.r(4))));
    c.set(3, std::int32_t(std::int16_t(c.r(3))));
    if (!capped_reputation) {
        c.set(4, c.u32(actor.address + 0x6c)); c.set(4, std::int32_t(c.i16(c.r(4) + 0x5c))); c.set(4, std::int32_t(std::int8_t(c.r(4))));
        if (compare(c, std::int32_t(c.r(4)), 21) > 0) c.set(3, std::int32_t(std::int16_t(c.r(3) + 750)));
        else {
            c.set(4, c.u32(actor.address + 0x6c)); c.set(4, std::int32_t(c.i16(c.r(4) + 0x5c))); c.set(4, std::int32_t(std::int8_t(c.r(4))));
            if (compare(c, std::int32_t(c.r(4)), 18) > 0) c.set(3, std::int32_t(std::int16_t(c.r(3) + 250)));
        }
    }
    c.set(5, std::int32_t(std::int16_t(c.r(3)))); c.set(4, competition); c.set(4, c.r(4) - c.r(5));
    if (compare(c, std::int32_t(c.r(4)), 4500) <= 0) return;
    c.set(6, c.u32(actor.address + 0x70)); c.set(5, std::int32_t(std::int16_t(c.r(3)))); c.set(4, competition);
    c.set(6, std::int32_t(c.i16(c.r(6) + 0xc))); c.set(4, c.r(4) - c.r(5));
    if (compare(c, std::int32_t(c.r(6)), std::int32_t(c.r(4))) >= 0) return;
    c.set(4, c.u32(actor.address + 0x70)); c.set(5, competition); c.set(6, std::int32_t(std::int16_t(c.r(3))));
    c.set(3, actor.address); const auto shortfall = std::uint32_t(std::int32_t(c.i16(c.r(4) + 0xc))) - c.r(5);
    keep(c, {{20, c.r(6) + shortfall}});
    enter(c, 0x1022840c, {actor.address}); current_role(c, actor.address);
    const bool keeper = compare(c, std::int16_t(c.r(3)), 1) == 0;
    unsigned offset;
    if (compare(c, std::int32_t(c.r(20)), 2500) > 0) offset = keeper ? 0x3c : 0x44;
    else if (keeper) offset = 0x40;
    else offset = compare(c, std::int32_t(c.r(20)), 1250) > 0 ? 0x3c : 0x40;
    c.set_f(0, c.f32(floats + offset)); c.set_f(27, fp.fadds(c.f(0), c.f(27)));
}

void PlayerInfluence::limit_ability() {
    enter(c, 0x10228470, {actor.address}); current_role(c, actor.address);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) {
        const auto minimum = [&](bool initial) {
            c.set(3, c.u32(actor.address + 0x70)); c.put32(at(0x38), 0x43300000);
            const auto ability = c.i16(c.r(3) + 4); if (initial) c.set(3, std::int32_t(ability));
            c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles + 0x30));
            c.put32(at(0x3c), std::uint32_t(std::int32_t(ability)) ^ 0x80000000);
            if (initial) c.set(4, std::int32_t(c.i16(actor.address + 0x3c)));
            c.set_f(0, c.f64(at(0x38))); c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
            c.put64(at(0x40), fp.fctiwz(0, c.f(0))); return c.u32(at(0x44));
        };
        const auto floor = minimum(true);
        if (compare(c, std::int32_t(c.r(4)), std::int16_t(floor)) < 0) c.put16(actor.address + 0x3c, minimum(false));
        bool real_player = compare_logical(c, c.u32(actor.address + 0x6c)) == 0;
        if (!real_player) {
            c.set(3, c.u32(actor.address + 0x6c)); const auto count = c.u32(c.r(19)); c.set(3, c.u32(c.r(3)));
            real_player = compare(c, std::int32_t(c.r(3)), std::int32_t(count)) < 0;
        }
        if (real_player && compare(c, c.i16(actor.address + 0x3c), 8) < 0) c.put16(actor.address + 0x3c, 8);
    }
    enter(c, 0x1022853c, {actor.address}); current_role(c, actor.address);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) {
        c.set(3, c.u32(actor.address + 0x70)); c.set(4, std::int32_t(c.i16(actor.address + 0x3c))); c.set(3, std::int32_t(c.i16(c.r(3) + 6)));
        if (compare(c, std::int32_t(c.r(4)), std::int16_t(c.r(3) + 10)) > 0) {
            c.set(3, c.u32(actor.address + 0x70)); c.set(3, std::int32_t(c.i16(c.r(3) + 6))); c.put16(actor.address + 0x3c, c.r(3) + 10);
        }
    } else {
        c.set(3, c.u32(actor.address + 0x70)); c.set(4, std::int32_t(c.i16(actor.address + 0x3c)));
        const auto ceiling = c.i16(c.r(3) + 6); c.set(3, std::int32_t(std::int16_t(c.r(4))));
        if (compare(c, std::int32_t(c.r(3)), ceiling) > 0) {
            c.set(3, c.u32(actor.address + 0x70)); c.put16(actor.address + 0x3c, std::uint32_t(std::int32_t(c.i16(c.r(3) + 6))));
        }
    }
}

void PlayerInfluence::publish_ability() {
    c.set(5, std::int32_t(c.i16(actor.address + 0x3c))); c.set(4, 0x43300000); c.set(3, 150);
    c.set(5, c.r(5) ^ 0x80000000); c.put32(at(0x38), c.r(4)); c.set_f(2, c.f64(doubles + 0x80));
    const bool veteran = compare(c, std::int8_t(c.r(26)), 36) > 0;
    c.put32(at(0x3c), c.r(5)); c.set_f(3, c.f64(doubles + 0x28)); c.set_f(1, c.f64(at(0x38)));
    c.set_f(0, c.f64(doubles + 0x128)); c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(5, c.f64(doubles + 0x130)); c.set_f(4, c.f64(doubles + 0xc8));
    c.set_f(0, fp.multiply_add(0x102285dc, c.f(3), c.f(1), c.f(0))); c.put64(at(0x40), fp.fctiwz(0, c.f(0)));
    c.put16(actor.address + 0x3c, c.u32(at(0x44))); c.set(4, std::int32_t(c.i16(actor.address + 0x3c)));
    const auto comparison = c.r(4) ^ c.r(3);
    c.s.wr<1>(reg::xer_ca, std::int32_t(comparison) < 0 && (comparison & 1)); c.set(3, std::int32_t(comparison) >> 1);
    keep(c, {{28, (c.r(3) - (comparison & c.r(4))) >> 31}});
    c.set(3, c.u32(actor.address + 0x70));
    if (veteran) {
        c.put32(at(0x48), 0x43300000); const auto endurance = c.i8(c.r(3) + 0x1e);
        c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles + 0x30));
        c.put32(at(0x4c), std::uint32_t(std::int32_t(endurance)) ^ 0x80000000); c.set_f(0, c.f64(at(0x48)));
        c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(3, fp.fmul(c.f(2), c.f(0))); c.set_f(3, fp.frsp(c.f(3)));
    } else {
        c.put32(at(0x50), 0x43300000); const auto endurance = c.i8(c.r(3) + 0x1e); c.set_f(1, c.f64(doubles + 0x80));
        c.put32(at(0x54), std::uint32_t(std::int32_t(endurance)) ^ 0x80000000); c.set_f(0, c.f64(at(0x50)));
        c.set_f(3, fp.fsubs(c.f(0), c.f(1)));
    }
    c.set(3, std::int32_t(c.i16(actor.address + 0x3c))); c.put32(at(0x58), 0x43300000);
    c.set_f(2, c.f64(doubles + 0x80)); c.put32(at(0x5c), c.r(3) ^ 0x80000000);
    c.set_f(0, c.f64(doubles + 0xb8)); c.set_f(1, c.f64(at(0x58))); c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(1, fp.multiply_add(0x10228694, c.f(4), c.f(3), c.f(1)));
    c.set_f(1, fp.multiply_add(0x10228698, c.f(5), c.f(27), c.f(1))); c.set_f(1, fp.frsp(c.f(1)));
    c.set_f(0, fp.fdiv(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.put_f32(actor.address + 0x80, float(c.f(0)));
}
}
