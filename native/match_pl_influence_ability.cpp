#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void PlayerInfluence::ability_penalty() {
    c.set(3, std::int32_t(c.i16(actor.address + 0x3c))); c.set(5, c.u8(actor.address + 0x11c));
    c.set(4, 0x43300000); c.set(3, c.r(3) ^ 0x80000000); c.put32(at(0x40), 0x43300000);
    const auto tendency = std::uint32_t(std::int32_t(std::int8_t(c.r(5)))) ^ 0x80000000;
    c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x44), c.r(3)); c.set_f(3, c.f64(doubles + 0x80));
    c.set_f(0, c.f64(at(0x40))); c.put32(at(0x3c), tendency); c.set_f(1, fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, c.f64(doubles + 0x120)); c.put32(at(0x38), c.r(4)); c.set_f(2, c.f64(at(0x38)));
    fp.fcmp(0, c.f(1), c.f(0)); c.set_f(2, fp.fsubs(c.f(2), c.f(3)));
    if (c.cr(0) & 4) {
        c.set(3, std::int32_t(c.i16(actor.address + 0x3c))); c.put32(at(0x48), 0x43300000);
        c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x4c), c.r(3) ^ 0x80000000);
        c.set_f(0, c.f64(at(0x48))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    } else c.set_f(0, c.f64(doubles + 0x120));
    c.set_f(1, fp.frsp(c.f(0))); c.set_f(0, c.f32(floats + 0x38));
    c.set_f(1, fp.fmuls(c.f(2), c.f(1))); c.set_f(27, fp.fdivs(c.f(1), c.f(0)));
}

void PlayerInfluence::initial_ability() {
    c.set(4, c.u32(actor.address + 0x70)); c.set(3, 0x43300000); const auto staff = c.u32(actor.address + 0x6c);
    c.set(4, c.u8(c.r(4) + 0x21)); c.put32(at(0x50), c.r(3));
    bool ordinary = compare_logical(c, staff) == 0;
    c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x54), std::uint32_t(std::int32_t(std::int8_t(c.r(4)))) ^ 0x80000000);
    c.set_f(0, c.f64(at(0x50))); c.set_f(26, fp.fsubs(c.f(0), c.f(1)));
    if (!ordinary) {
        c.set(3, c.u32(actor.address + 0x6c)); const auto count = c.u32(c.r(19)); c.set(3, c.u32(c.r(3)));
        ordinary = compare(c, std::int32_t(c.r(3)), std::int32_t(count)) < 0;
    }
    if (ordinary) {
        c.set(4, c.u32(actor.address + 0x70)); c.set(5, 0x43300000); c.set(3, c.u32(actor.address + 0x6c));
        c.set(4, std::int32_t(c.i16(c.r(4) + 4))); c.set(6, 0x43300000); c.set(3, c.u32(c.r(3)));
        c.set(4, c.r(4) ^ 0x80000000); c.set(7, c.u8(actor.address + 0x103)); c.put32(at(0x5c), c.r(4));
        c.set(3, c.r(3) * 100); c.set(4, c.u32(c.r(17))); c.set(7, std::int32_t(std::int8_t(c.r(7))));
        c.put32(at(0x58), c.r(5)); c.set_f(1, c.f64(doubles + 0x80)); c.set(3, c.r(4) + c.r(3));
        c.set_f(0, c.f64(at(0x58))); c.set(4, c.r(7) >> 31); c.set(3, c.u8(c.r(3) + 0xa)); c.set(4, c.r(4) + c.r(7));
        c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(2, c.f64(doubles + 0x118)); c.put32(at(0x64), c.r(3));
        const auto rounded = c.r(4); c.s.wr<1>(reg::xer_ca, std::int32_t(rounded) < 0 && (rounded & 1));
        c.set(3, std::uint32_t(std::int32_t(rounded) >> 1)); c.set(3, c.r(3) ^ 0x80000000);
        c.set_f(1, c.f64(doubles + 0xf0)); c.put32(at(0x60), 0x43300000);
        c.set_f(2, fp.multiply_add(0x10227eec, c.f(2), c.f(30), c.f(0))); c.set_f(3, c.f64(doubles + 0x80));
        c.set_f(0, c.f64(at(0x60))); c.put32(at(0x54), c.r(3)); c.set_f(2, fp.fsub(c.f(2), c.f(28)));
        c.put32(at(0x50), c.r(6)); c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(1, c.f64(at(0x50)));
        c.set_f(0, fp.fsub(c.f(2), c.f(0))); c.set_f(1, fp.fsub(c.f(1), c.f(3))); c.set_f(0, fp.fadd(c.f(1), c.f(0)));
        c.put64(at(0x68), fp.fctiwz(0, c.f(0))); c.put16(actor.address + 0x3c, c.u32(at(0x6c)));
    } else {
        c.set(3, c.u32(actor.address + 0x70)); c.put32(at(0x38), 0x43300000);
        const auto ability = c.i16(c.r(3) + 4); c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles + 0x118));
        c.put32(at(0x3c), std::uint32_t(std::int32_t(ability)) ^ 0x80000000); c.set_f(0, c.f64(at(0x38)));
        c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.multiply_add(0x102280ec, c.f(2), c.f(30), c.f(0)));
        c.set_f(0, fp.fsub(c.f(0), c.f(28))); c.put64(at(0x40), fp.fctiwz(0, c.f(0)));
        c.put16(actor.address + 0x3c, c.u32(at(0x44)));
    }
    const auto side = actor.side(); c.set(3, actor.match()); const bool away = compare(c, std::int8_t(side)) != 0;
    c.set(3, c.u32(c.r(3) + 0x3118)); c.set(4, c.u32(c.r(3) + (away ? 0x20 : 0x1c))); c.set(3, c.u32(actor.address + 0x6c));
    c.call(0x100ca010, ordinary ? 0x10227f54 : 0x10228130);
    if (ordinary) {
        const auto personality = c.i8(actor.address + 0x103); c.set(7, std::int32_t(std::int8_t(c.r(3))));
        c.set(4, c.u8(actor.address + 0x17)); c.set(6, c.r(7) >> 31); c.set(3, std::int32_t(personality));
        c.set(3, c.r(3) ^ 0x80000000); c.set(5, std::int32_t(std::int8_t(c.r(4)))); c.put32(at(0x44), c.r(3));
        c.set(4, c.r(6) + c.r(7)); c.set(3, std::int32_t(std::int8_t(c.r(28)))); c.set_f(1, c.f64(doubles + 0x80));
        c.put32(at(0x40), 0x43300000);
        c.s.wr<1>(reg::xer_ca, std::int32_t(c.r(4)) < 0 && (c.r(4) & 1)); c.set(7, std::int32_t(c.r(4)) >> 1);
        c.set(4, c.r(3) ^ 0x80000000); const auto rounded = (c.r(5) >> 31) + c.r(5);
        c.set(3, 0x43300000); c.set_f(0, c.f64(at(0x40)));
        c.s.wr<1>(reg::xer_ca, std::int32_t(rounded) < 0 && (rounded & 1)); c.set(6, std::int32_t(rounded) >> 1);
        c.set(5, 0x43300000); c.set(6, (c.r(6) - 5) ^ 0x80000000);
        c.set_f(1, fp.fsubs(c.f(0), c.f(1))); c.set(8, c.r(7) ^ 0x80000000); c.set_f(0, fp.fsubs(c.f(26), c.f(28)));
        const auto target = c.i8(actor.address + 0x11b); c.set(7, 0x43300000); c.put32(at(0x4c), c.r(4));
        c.set_f(3, c.f64(doubles + 0x80)); c.set_f(0, fp.fadds(c.f(1), c.f(0))); c.put32(at(0x48), c.r(3));
        c.set(3, std::uint32_t(std::int32_t(target)) ^ 0x80000000); c.set_f(4, c.f64(doubles + 0x80));
        c.set_f(2, c.f64(at(0x48))); c.put32(at(0x7c), c.r(6)); c.set_f(1, fp.fsubs(c.f(2), c.f(3)));
        c.set_f(3, c.f64(doubles + 0x80)); c.put32(at(0x78), c.r(5)); c.set_f(2, c.f64(at(0x78)));
        c.set_f(0, fp.fadds(c.f(1), c.f(0))); c.put32(at(0x74), c.r(8)); c.set_f(2, fp.fsubs(c.f(2), c.f(3)));
        c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x70), c.r(7)); c.set_f(3, c.f64(at(0x70)));
        c.set_f(2, fp.fadds(c.f(2), c.f(0))); c.put32(at(0x3c), c.r(3)); c.set_f(3, fp.fsubs(c.f(3), c.f(4)));
        c.put32(at(0x38), 0x43300000); c.set_f(0, c.f64(at(0x38))); c.set_f(2, fp.fadds(c.f(2), c.f(3)));
        c.set_f(0, fp.fsubs(c.f(0), c.f(1))); fp.fcmp(0, c.f(2), c.f(0));
        if (c.cr(0) & 8) ability_penalty();
    } else {
        c.set(6, std::int32_t(std::int8_t(c.r(3)))); const auto target = c.i8(actor.address + 0x11b);
        c.set(5, c.r(6) >> 31); c.set(4, 0x43300000); c.set(5, c.r(5) + c.r(6)); c.set(3, std::int32_t(target));
        c.s.wr<1>(reg::xer_ca, std::int32_t(c.r(5)) < 0 && (c.r(5) & 1)); c.set(5, std::int32_t(c.r(5)) >> 1);
        c.set(5, c.r(5) ^ 0x80000000); c.set(3, c.r(3) ^ 0x80000000); c.put32(at(0x48), c.r(4));
        c.set_f(2, fp.fsubs(c.f(26), c.f(28))); c.set_f(3, c.f64(doubles + 0x80)); c.put32(at(0x4c), c.r(5));
        c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, c.f64(at(0x48))); c.put32(at(0x54), c.r(3));
        c.set_f(3, fp.fsubs(c.f(0), c.f(3))); c.put32(at(0x50), 0x43300000); c.set_f(0, c.f64(at(0x50)));
        c.set_f(2, fp.fadds(c.f(2), c.f(3))); c.set_f(0, fp.fsubs(c.f(0), c.f(1))); fp.fcmp(0, c.f(2), c.f(0));
        if (c.cr(0) & 4) ability_penalty();
    }
}
}
