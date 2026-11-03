#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
std::uint32_t divide_ten(Cpu& c, std::uint32_t value) {
    const auto high = std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * 0x66666667) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 3));
    const auto shifted = high >> 2; c.set(3, unsigned(shifted < 0));
    return std::uint32_t(shifted) + c.r(3);
}
}

void PlayerInfluence::contract_and_morale() {
    if (compare(c, std::int8_t(c.r(3))) == 0) {
        const auto side = actor.side(); c.set(3, actor.match()); const bool away = compare(c, std::int8_t(side)) != 0;
        c.set(3, c.u32(c.r(3) + 0x3118)); c.set(5, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
        c.set(3, c.toc(-0x7b7c)); c.set(4, c.u32(actor.address + 0x6c)); c.set(3, c.u32(c.r(3)));
        c.call(0x10082f90, 0x10227918); keep(c, {{27, c.r(3)}});
    } else keep(c, {{27, 0}});
    c.set(4, actor.side()); c.set(3, 0x43300000); const auto confidence = c.i16(actor.address + 0x34);
    c.set(4, std::int32_t(std::int8_t(c.r(4)))); c.set(5, actor.match()); c.set(4, c.r(4) * 2);
    c.put32(at(0x50), c.r(3)); c.set(3, c.r(5) + c.r(4)); c.set_f(2, c.f64(doubles + 0x80));
    c.set(3, std::int32_t(c.i16(c.r(3) + 0x1e8))); const bool confident = compare(c, confidence, 10) > 0;
    c.set_f(0, c.f64(doubles + 0x100)); c.set_f(3, c.f64(doubles + 0x10));
    c.put32(at(0x54), (c.r(3) - 100) ^ 0x80000000); c.set_f(1, c.f64(at(0x50)));
    c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(0, fp.fdiv(c.f(1), c.f(0)));
    c.set_f(0, fp.fadd(c.f(3), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.set_f(30, fp.fmuls(c.f(30), c.f(0)));
    const auto temperament_adjustment = [&](unsigned field, bool deficit) {
        c.set(4, c.u32(actor.address + 0x6c)); c.set(3, c.u32(actor.address + 0x6c));
        c.set(4, c.u8(c.r(4) + 0x66)); const auto determination = c.i8(c.r(3) + 0x64);
        c.set(3, std::int32_t(std::int8_t(c.r(4)))); c.set(4, std::int32_t(c.i16(actor.address + field)));
        const bool composure = compare(c, std::int32_t(c.r(3)), determination) > 0;
        const auto amount = deficit ? 10u - c.r(4) : 0u - c.r(4);
        if (deficit) c.s.wr<1>(reg::xer_ca, c.r(4) <= 10);
        c.set(4, amount); c.set(3, c.u32(actor.address + 0x6c));
        c.set(3, std::int32_t(c.i8(c.r(3) + (composure ? 0x66 : 0x64)))); c.set(5, 0x66666667);
        const auto scaled = amount * (c.r(3) - 10);
        const auto adjustment = divide_ten(c, scaled);
        keep(c, {{28, c.r(28) + std::uint32_t(std::int32_t(std::int8_t(adjustment)))}});
    };
    if (confident) {
        const auto value = c.i16(actor.address + 0x34); const auto shifted = value >> 2;
        const bool carry = value < 0 && (std::uint32_t(value) & 3);
        const auto rounded = std::uint32_t(shifted) + unsigned(carry);
        c.s.wr<1>(reg::xer_ca, carry && rounded == 0);
        keep(c, {{28, c.r(28) + std::uint32_t(std::int32_t(std::int8_t(rounded)))}});
    } else temperament_adjustment(0x34, true);
    if (compare(c, c.i16(actor.address + 0x32)) < 0) temperament_adjustment(0x32, false);
    else {
        const auto pressure_adjustment = [&] {
            c.set(3, c.u32(actor.address + 0x6c)); c.set(4, 0x66666667);
            const auto pressure = std::uint32_t(std::int32_t(c.i16(actor.address + 0x32)));
            c.set(3, std::int32_t(c.i8(c.r(3) + 0x67))); c.set(3, c.r(3) - 10);
            return divide_ten(c, c.r(3) * pressure);
        };
        auto adjustment = pressure_adjustment();
        if (compare(c, std::int32_t(adjustment), 10) > 0) adjustment = 10;
        else adjustment = pressure_adjustment(); // The original repeats live reads.
        keep(c, {{28, c.r(28) + std::uint32_t(std::int32_t(std::int8_t(adjustment)))}});
    }
    enter(c, 0x10227af8, {actor.address}); current_role(c, actor.address);
    const bool keeper = compare(c, std::int16_t(c.r(3)), 1) == 0;
    const auto morale = std::int8_t(c.r(28)); const auto low = keeper ? -5 : -10, high = keeper ? 15 : 10;
    if (compare(c, morale, low) < 0) c.set(3, low);
    else { c.set(3, high); if (compare(c, morale, high) <= 0) c.set(3, std::int32_t(morale)); }
    keep(c, {{28, std::uint32_t(std::int32_t(std::int8_t(c.r(3))))}});
}

void PlayerInfluence::fixture_pressure() {
    const auto side = std::int8_t(actor.side()); c.set(3, actor.match()); c.set(3, c.r(3) + std::uint32_t(std::int32_t(side)));
    if (compare(c, c.i8(c.r(3) + 0x1c6)) == 0) return;
    c.set(3, actor.match()); bool special = compare_logical(c, c.u32(c.r(3) + 0x3118)) != 0;
    if (special) {
        c.set(3, actor.match()); c.set(3, c.u32(c.r(3) + 0x3118)); special = compare(c, c.u8(c.r(3) + 0x3a), 8) == 0;
        if (!special) { c.set(3, actor.match()); c.set(3, c.u32(c.r(3) + 0x3118)); special = compare(c, c.u8(c.r(3) + 0x3a), 10) == 0; }
    }
    if (!special) {
        c.set(3, actor.match()); c.set_f(0, c.f32(floats + 0x24));
        bool increase = compare(c, c.i8(c.r(3))) == 0;
        if (increase) {
            c.set(3, actor.match()); c.set(3, c.u32(c.r(3) + 0x3118)); c.set(3, c.u32(c.r(3) + 0x14));
            increase = compare(c, c.i16(c.r(3) + 0x18), 12) > 0;
        }
        if (increase) {
            c.set(3, actor.match()); c.put32(at(0x50), 0x43300000); c.set(3, c.u32(c.r(3) + 0x3118));
            c.set_f(3, c.f64(doubles + 0x80)); c.set(3, c.u32(c.r(3) + 0x14)); c.set_f(1, c.f32(floats + 0x28));
            c.set(3, std::int32_t(c.i16(c.r(3) + 0x18))); c.put32(at(0x54), (c.r(3) - 10) ^ 0x80000000);
            c.set_f(2, c.f64(at(0x50))); c.set_f(2, fp.fsubs(c.f(2), c.f(3)));
            c.set_f(1, fp.fdivs(c.f(2), c.f(1))); c.set_f(0, fp.fadds(c.f(0), c.f(1)));
        }
    }
    // The two fixture kinds have the same integer staging and stack slots,
    // but retain distinct FPR layouts and fused-operation sites.
    c.set(3, c.u32(actor.address + 0x6c)); c.set(5, actor.side()); c.set(4, 0x43300000);
    c.set(3, c.u8(c.r(3) + 0x67)); c.set(7, 0x43300000); c.set(8, c.u32(actor.address + 0x70));
    c.set(5, std::int32_t(std::int8_t(c.r(5)))); c.set(3, std::int32_t(std::int8_t(c.r(3)))); c.set(6, actor.match());
    c.set(3, c.r(3) - 10); c.set(8, c.u8(c.r(8) + 0x2c)); c.set(3, c.r(3) ^ 0x80000000); c.set(5, c.r(6) + c.r(5));
    c.put32(at(0x64), c.r(3)); c.set(6, std::int32_t(std::int8_t(c.r(8)))); c.set(3, c.u8(c.r(5) + 0x1c6));
    c.set(5, c.r(6) - 10); c.put32(at(0x60), 0x43300000); c.set(5, c.r(5) ^ 0x80000000);
    const auto pressure = std::uint32_t(std::int32_t(std::int8_t(c.r(3)))) ^ 0x80000000;
    const auto f = unsigned(!special);
    c.set_f(2 + f, c.f64(doubles + 0x80)); c.set_f(1 + f, c.f64(at(0x60))); c.set_f(f, c.f64(doubles + 0x48));
    c.set_f(1 + f, fp.fsubs(c.f(1 + f), c.f(2 + f))); c.put32(at(0x5c), pressure);
    c.set_f(2 + f, c.f64(doubles + 0x80)); c.put32(at(0x58), c.r(4));
    c.set_f(f, fp.fdiv(c.f(1 + f), c.f(f))); c.set_f(3 + f, c.f64(doubles + 0xb8));
    c.set_f(1 + f, c.f64(at(0x58))); c.set_f(4 + f, c.f32(floats + 0x20)); c.set_f(5 + f, c.f64(doubles + (special ? 0x108 : 0xc0)));
    c.put32(at(0x54), c.r(5)); c.set_f(1 + f, fp.fsub(c.f(1 + f), c.f(2 + f))); c.put32(at(0x50), c.r(7));
    c.set_f(7 + f, c.f64(doubles + 0x80)); c.set_f(6 + f, c.f64(at(0x50))); c.set_f(1 + f, fp.fadd(c.f(3 + f), c.f(1 + f)));
    c.set_f(2 + f, c.f64(doubles + 0x110)); c.set_f(3 + f, fp.fsubs(c.f(6 + f), c.f(7 + f))); c.set_f(6 + f, c.f64(doubles + 0x28));
    c.set_f(1 + f, fp.frsp(c.f(1 + f))); c.set_f(1 + f, fp.fmuls(c.f(4 + f), c.f(1 + f)));
    c.set_f(1 + f, fp.fadd(c.f(5 + f), c.f(1 + f))); c.set_f(f, fp.fsub(c.f(1 + f), c.f(f)));
    c.set_f(f, fp.multiply_add(special ? 0x10227c70 : 0x10227da4, c.f(3 + f), c.f(2 + f), c.f(f)));
    const auto adjusted = fp.multiply_add(special ? 0x10227c74 : 0x10227da8, c.f(30), c.f(6 + f), c.f(f));
    if (special) c.set_f(28, fp.frsp(adjusted));
    else { c.set_f(1, fp.frsp(adjusted)); c.set_f(28, fp.fdivs(c.f(1), c.f(0))); }
    c.set(4, actor.side()); c.set(5, actor.match()); c.set(3, actor.address);
    c.set(4, std::int32_t(std::int8_t(c.r(4)))); c.put32(at(0x50), 0x43300000); c.set(4, c.r(5) + c.r(4));
    c.set_f(2, c.f64(doubles + 0x80)); const auto raw_pressure = c.i8(c.r(4) + 0x1c6);
    c.set_f(3, c.f64(doubles + 0xc0)); c.set(4, std::int32_t(raw_pressure)); c.set_f(0, c.f32(floats + 0x2c));
    const auto rounded = std::uint32_t(std::int32_t(raw_pressure)) + unsigned(raw_pressure < 0);
    c.s.wr<1>(reg::xer_ca, std::int32_t(rounded) < 0 && (rounded & 1));
    c.put32(at(0x54), std::uint32_t(std::int32_t(rounded) >> 1) ^ 0x80000000); c.set_f(1, c.f64(at(0x50)));
    c.set_f(1, fp.fsub(c.f(1), c.f(2))); c.set_f(1, fp.fadd(c.f(3), c.f(1))); c.set_f(1, fp.frsp(c.f(1)));
    c.set_f(28, fp.fadds(c.f(28), c.f(1))); c.set_f(28, fp.fmuls(c.f(28), c.f(0)));
    enter(c, 0x10227e14, {actor.address}); current_role(c, actor.address);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) { c.set_f(0, c.f32(floats + 0x30)); c.set_f(28, fp.fdivs(c.f(28), c.f(0))); }
    c.set_f(0, c.f64(doubles + 0x10)); fp.fcmp(0, c.f(28), c.f(0));
    if (c.cr(0) & 8) c.set_f(28, c.f32(floats + 0x34));
}
}
