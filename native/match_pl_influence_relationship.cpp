#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
int temperament_gap(Cpu& c, int temperament, int baseline = 25) {
    c.s.wr<1>(reg::xer_ca, std::uint32_t(temperament) <= std::uint32_t(baseline));
    return baseline - temperament;
}
}

void PlayerInfluence::relationship_query(bool opposite, bool controller, std::uint32_t target, std::uint32_t back) {
    const auto side = std::int8_t(actor.side());
    if (controller) {
        c.set(5, actor.match());
        c.set(4, c.u32(actor.address + 0x6c));
        const auto controller_side = opposite ? int(side == 0) : int(side);
        c.set(3, c.r(5) + std::uint32_t(controller_side) * 0xd8 + 0x7aac);
        c.set(3, c.u32(c.r(3) + 0x20));
    } else {
        c.set(3, actor.match());
        const bool away = opposite ? compare(c, int(side == 0)) != 0 : compare(c, side) != 0;
        c.set(3, c.u32(c.r(3) + 0x3118));
        c.set(4, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
        c.set(3, c.u32(actor.address + 0x6c));
    }
    c.call(target, back);
}

void PlayerInfluence::captain_relationship(std::uint32_t lookup_back, std::uint32_t target, std::uint32_t back) {
    keep(c, {{19, actor.side()}, {17, actor.match()}});
    compare(c, std::int8_t(c.r(19)));
    select_captain(19, 17, lookup_back);
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))) * 0x1c4);
    c.set(4, c.u32(actor.address + 0x6c));
    const auto side_offset = std::uint32_t(std::int32_t(std::int8_t(c.r(19)))) * 0x2350;
    c.set(3, c.r(17) + c.r(3) + side_offset + 0x311c);
    c.set(3, c.u32(c.r(3) + 0x6c));
    c.call(target, back);
}

void PlayerInfluence::relationship_bonus() {
    c.set_f(1, c.f32(floats + 0x3c));
    c.set_f(0, c.f32(actor.address + 0xf8));
    c.put32(at(0x38), 0x43300000);
    c.set_f(1, fp.fadds(c.f(1), c.f(0)));
    c.set_f(2, c.f64(doubles + 0x80));
    c.set_f(0, c.f32(floats + 0x3c));
    c.put_f32(actor.address + 0xf8, float(c.f(1)));
    c.set(3, c.u32(actor.address + 0x6c));
    c.set_f(3, c.f32(actor.address + 0x78));
    const auto gap = temperament_gap(c, c.i8(c.r(3) + 0x67));
    c.put32(at(0x3c), std::uint32_t(gap) ^ 0x80000000);
    c.set_f(1, c.f64(at(0x38)));
    c.set_f(1, fp.fsubs(c.f(1), c.f(2)));
    c.set_f(0, fp.fdivs(c.f(1), c.f(0)));
    c.set_f(0, fp.fadds(c.f(3), c.f(0)));
    c.put_f32(actor.address + 0x78, float(c.f(0)));
}

void PlayerInfluence::relationship_penalty(unsigned secondary_field, bool loyalty) {
    const unsigned denominator = loyalty ? 0x5c : 0x3c;
    c.set(4, c.u32(actor.address + 0x6c));
    c.set(3, 0x43300000);
    int temperament;
    if (loyalty) {
        c.set(5, c.u32(actor.address + 0x6c));
        c.set(4, c.u8(c.r(4) + 0x67));
        c.set(5, c.u8(c.r(5) + 0x65));
        c.set(4, std::int32_t(std::int8_t(c.r(4))));
        c.put32(at(0x38), c.r(3));
        c.set(5, std::int32_t(std::int8_t(c.r(5))));
        c.set_f(4, c.f64(doubles + 0x80));
        temperament = std::int32_t(c.r(4));
    } else {
        c.put32(at(0x38), c.r(3));
        c.set(3, c.u8(c.r(4) + 0x67));
        c.set_f(4, c.f64(doubles + 0x80));
        c.set(3, std::int32_t(std::int8_t(c.r(3))));
        temperament = std::int32_t(c.r(3));
    }
    const auto gap = temperament_gap(c, temperament, loyalty ? 50 : 25);
    c.set(3, gap);
    c.set_f(1, c.f32(floats + denominator));
    if (loyalty) c.set(3, c.r(3) - c.r(5));
    c.set_f(5, c.f32(actor.address + 0xf8));
    c.set(3, c.r(3) ^ 0x80000000);
    c.put32(at(0x40), 0x43300000);
    c.set_f(2, c.f64(doubles + 0x80));
    c.put32(at(0x3c), c.r(3));
    c.set_f(0, c.f32(floats + denominator));
    c.set_f(3, c.f64(at(0x38)));
    c.set_f(3, fp.fsubs(c.f(3), c.f(4)));
    c.set_f(1, fp.fdivs(c.f(3), c.f(1)));
    c.set_f(1, fp.fsubs(c.f(5), c.f(1)));
    c.put_f32(actor.address + 0xf8, float(c.f(1)));
    c.set(3, c.u32(actor.address + 0x6c));
    if (loyalty) c.set(4, c.u32(actor.address + 0x6c));
    else c.set_f(3, c.f32(actor.address + secondary_field));
    temperament = c.i8(c.r(3) + 0x67);
    if (loyalty) {
        c.set(3, c.u8(c.r(4) + 0x65));
        c.set_f(3, c.f32(actor.address + secondary_field));
        c.set(3, std::int32_t(std::int8_t(c.r(3))));
    }
    const auto second = temperament_gap(c, temperament, loyalty ? 50 : 25);
    const auto numerator = std::uint32_t(second) - (loyalty ? c.r(3) : 0);
    c.put32(at(0x44), numerator ^ 0x80000000);
    c.set_f(1, c.f64(at(0x40)));
    c.set_f(1, fp.fsubs(c.f(1), c.f(2)));
    c.set_f(0, fp.fdivs(c.f(1), c.f(0)));
    c.set_f(0, fp.fsubs(c.f(3), c.f(0)));
    c.put_f32(actor.address + secondary_field, float(c.f(0)));
}

void PlayerInfluence::contract_history() {
    const auto contract = c.r(27);
    if (compare_logical(c, contract) == 0) return;
    for (unsigned field : {0x10u, 0x14u}) {
        if (compare(c, std::int32_t(c.u32(contract + field)), -1) <= 0 ||
            compare(c, std::int32_t(c.u32(contract + 0xc))) <= 0) continue;
        c.set(5, c.u32(contract + field));
        c.set(4, 0x43300000);
        c.set(3, c.u32(contract + 0xc));
        c.set(5, c.r(5) ^ 0x80000000);
        c.put32(at(0x38), c.r(4));
        c.set(3, c.r(3) ^ 0x80000000);
        c.set_f(1, c.f64(doubles + 0x80));
        c.put32(at(0x3c), c.r(5));
        c.set_f(4, c.f32(floats + 0x80));
        c.set_f(0, c.f64(at(0x38)));
        c.put32(at(0x44), c.r(3));
        c.set_f(3, fp.fsubs(c.f(0), c.f(1)));
        c.set_f(2, c.f64(doubles + 0x80));
        c.put32(at(0x40), 0x43300000);
        c.set_f(0, c.f64(doubles + 0x230));
        c.set_f(1, c.f64(at(0x40)));
        c.set_f(3, fp.fmuls(c.f(4), c.f(3)));
        c.set_f(1, fp.fsubs(c.f(1), c.f(2)));
        c.set_f(4, fp.fdivs(c.f(3), c.f(1)));
        fp.fcmp(0, c.f(4), c.f(0));
        if (!(c.cr(0) & 4)) continue;

        c.set(3, c.u32(actor.address + 0x6c));
        c.put32(at(0x38), 0x43300000);
        const auto temperament = c.i8(c.r(3) + 0x67);
        c.set_f(2, c.f64(doubles + 0x80));
        c.set_f(0, c.f32(floats + 0x44));
        const auto gap = temperament_gap(c, temperament);
        c.set_f(3, c.f32(actor.address + 0xe4));
        c.put32(at(0x3c), std::uint32_t(gap) ^ 0x80000000);
        c.set_f(1, c.f64(at(0x38)));
        c.set_f(1, fp.fsubs(c.f(1), c.f(2)));
        c.set_f(1, fp.fmuls(c.f(4), c.f(1)));
        c.set_f(0, fp.fdivs(c.f(1), c.f(0)));
        c.set_f(0, field == 0x10 ? fp.fsubs(c.f(3), c.f(0)) : fp.fadds(c.f(3), c.f(0)));
        c.put_f32(actor.address + 0xe4, float(c.f(0)));
    }
}

void PlayerInfluence::relationship_adjustments() {
    c.set(3, actor.match());
    c.set(3, c.u32(c.r(3) + 0x3118));
    if (compare(c, c.i8(c.r(3) + 0x40)) == 0) return;
    const auto accepted = [&] { return compare(c, std::int8_t(c.r(3))) != 0; };

    relationship_query(true, false, 0x100da050, 0x1022b258);
    if (accepted()) relationship_bonus();
    else {
        relationship_query(true, false, 0x100da0b0, 0x1022b2ec);
        if (accepted()) relationship_penalty(0x78, true);
    }

    relationship_query(true, true, 0x100d9580, 0x1022b3c4);
    if (accepted()) relationship_bonus();
    else {
        relationship_query(true, true, 0x100d9520, 0x1022b450);
        if (accepted()) relationship_penalty(0x78, true);
    }

    relationship_query(false, false, 0x100da0b0, 0x1022b524);
    if (accepted()) {
        c.set_f(1, c.f32(floats + 0x3c));
        c.set_f(0, c.f32(actor.address + 0xf8));
        c.set_f(0, fp.fadds(c.f(1), c.f(0)));
        c.put_f32(actor.address + 0xf8, float(c.f(0)));
    } else {
        relationship_query(false, false, 0x100da050, 0x1022b56c);
        if (accepted()) {
            c.set(3, c.u32(actor.address + 0x6c));
            c.put32(at(0x38), 0x43300000);
            const auto temperament = c.i8(c.r(3) + 0x67);
            c.set_f(2, c.f64(doubles + 0x80));
            c.set_f(0, c.f32(floats + 0x3c));
            const auto gap = temperament_gap(c, temperament);
            c.set_f(3, c.f32(actor.address + 0xf8));
            c.put32(at(0x3c), std::uint32_t(gap) ^ 0x80000000);
            c.set_f(1, c.f64(at(0x38)));
            c.set_f(1, fp.fsubs(c.f(1), c.f(2)));
            c.set_f(0, fp.fdivs(c.f(1), c.f(0)));
            c.set_f(0, fp.fsubs(c.f(3), c.f(0)));
            c.put_f32(actor.address + 0xf8, float(c.f(0)));
        }
    }

    const auto own_bonus = [&] {
        c.set_f(2, c.f32(floats + 0x3c));
        c.set_f(0, c.f32(actor.address + 0xf8));
        c.set_f(1, c.f32(floats + 0x3c));
        c.set_f(0, fp.fadds(c.f(2), c.f(0)));
        c.put_f32(actor.address + 0xf8, float(c.f(0)));
        c.set_f(0, c.f32(actor.address + 0xe4));
        c.set_f(0, fp.fadds(c.f(1), c.f(0)));
        c.put_f32(actor.address + 0xe4, float(c.f(0)));
    };
    relationship_query(false, true, 0x100d9580, 0x1022b5d8);
    if (accepted()) relationship_penalty(0xe4, false);
    else {
        relationship_query(false, true, 0x100d9520, 0x1022b688);
        if (accepted()) own_bonus();
    }
    captain_relationship(0x1022b718, 0x100d9580, 0x1022b740);
    if (accepted()) relationship_penalty(0xe4, false);
    else {
        captain_relationship(0x1022b834, 0x100d9520, 0x1022b85c);
        if (accepted()) own_bonus();
    }
    contract_history();
}
}
