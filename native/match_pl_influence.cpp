#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;
PlayerInfluence::PlayerInfluence(Cpu& cpu, std::uint32_t player)
    : c(cpu), frame(c, 0x10227460, 0x100, {.first_gpr = 17, .gpr_offset = -0x7c, .first_fpr = 25}),
      fp(c), actor{c, player}, floats(c.toc(-0x51e0)), doubles(c.toc(-0x51e8)) {
    keep(c, {{31, player}, {29, floats}, {28, 0}}); c.set(3, c.u32(player + 0x6c));
    keep(c, {{21, actor.side()}}); c.set(3, std::int32_t(c.i16(c.r(3) + 0x5c)));
    compare(c, std::int8_t(c.r(21)));
    keep(c, {{17, c.toc(-0x7b9c)}, {18, c.toc(-0x7a6c)}, {26, std::uint32_t(std::int32_t(std::int8_t(c.r(3))))},
        {19, c.toc(-0x7be4)}, {30, doubles}, {24, c.toc(-0x7b90)}, {23, c.toc(-0x7b8c)},
        {22, c.toc(-0x7b88)}, {25, c.toc(-0x7b84)}});
    for (unsigned reg : {31u, 30u, 29u, 28u, 27u}) c.set_f(reg, c.f32(floats));
    keep(c, {{20, actor.match()}});
}

void PlayerInfluence::select_captain(unsigned side_register, unsigned match_register, std::uint32_t back) {
    // Each lookup rereads the fixture and side-specific status/club. The second
    // lookup may select a different record after the intervening readiness call.
    const auto side = std::int8_t(c.r(side_register));
    c.set(3, c.u32(c.r(match_register) + 0x3118));
    c.set(3, c.i8(c.r(3) + (side ? 0x41 : 0x40)) == 0);
    const bool away = compare(c, side) != 0; c.set(5, std::int32_t(std::int8_t(c.r(3))));
    c.set(3, c.u32(c.r(match_register) + 0x3118)); c.set(4, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
    c.set(3, c.u32(c.r(18))); c.set(6, 0); c.call(0x103b0a00, back);
}

void PlayerInfluence::captain_effects() {
    select_captain(21, 20, 0x1022752c);
    const auto selected = std::uint32_t(std::int32_t(std::int8_t(c.r(3)))) * 0x1c4;
    c.set(3, selected); c.set(3, selected + std::uint32_t(std::int32_t(std::int8_t(c.r(21)))) * 0x2350);
    Kept<27> candidate(c, c.r(20) + c.r(3) + 0x311c);
    if (compare(c, std::int32_t(candidate)) == 0 || compare_logical(c, c.u32(candidate + 0x70)) == 0) return;
    enter(c, 0x10227564, {candidate}); readiness_rating(c, candidate);
    c.set(3, std::int32_t(std::int8_t(c.r(3)))); c.set(3, c.r(3) ^ 0x80000000);
    c.put32(at(0x78), 0x43300000); keep(c, {{20, actor.side()}}); c.put32(at(0x7c), c.r(3));
    c.set_f(3, c.f64(doubles + 0x80)); compare(c, std::int8_t(c.r(20)));
    c.set_f(2, c.f64(at(0x78))); c.set_f(1, c.f64(doubles + 0xd0)); c.set_f(2, fp.fsub(c.f(2), c.f(3)));
    c.set_f(0, c.f64(doubles + 0xf8)); keep(c, {{21, actor.match()}});
    c.set_f(1, fp.fsub(c.f(2), c.f(1))); c.set_f(30, fp.fdiv(c.f(1), c.f(0))); c.set_f(30, fp.frsp(c.f(30)));
    select_captain(20, 21, 0x10227600);
    const auto slot = std::int8_t(c.r(3)), side = std::int8_t(c.r(20));
    c.set(5, std::int32_t(side)); c.set(4, c.u32(candidate + 0x70)); c.set(6, std::uint32_t(std::int32_t(slot)) * 0x1c4);
    c.set(3, 0x43300000); c.set_f(1, c.f64(doubles + 0x80)); c.set(4, c.u8(c.r(4) + 0x39));
    c.put32(at(0x70), c.r(3)); c.set(5, std::uint32_t(std::int32_t(side)) * 0x2350); c.set(3, std::int32_t(std::int8_t(c.r(4))));
    c.put32(at(0x68), 0x43300000); const auto leadership = c.r(3) ^ 0x80000000;
    c.set_f(3, c.f64(doubles + 0x80)); c.set(4, c.r(5) + c.r(6)); c.set(3, c.r(4) + 0x311c);
    c.put32(at(0x74), leadership); c.set(3, c.r(21) + c.r(3)); c.set_f(5, c.f64(doubles + 0xc8));
    c.set(3, c.u32(c.r(3) + 0x70)); c.set_f(0, c.f64(at(0x70))); const auto ability = c.i16(c.r(3) + 4);
    c.set_f(4, fp.fsub(c.f(0), c.f(1))); c.set_f(1, c.f64(doubles + 0xb8)); c.set_f(0, c.f64(doubles + 0x88));
    c.put32(at(0x6c), std::uint32_t(std::int32_t(ability)) ^ 0x80000000); c.set_f(2, c.f64(at(0x68)));
    c.set_f(2, fp.fsub(c.f(2), c.f(3))); c.set_f(2, fp.multiply_add(0x10227674, c.f(5), c.f(4), c.f(2)));
    c.set_f(2, fp.frsp(c.f(2))); c.set_f(26, fp.fdiv(c.f(2), c.f(1))); c.set_f(26, fp.frsp(c.f(26)));
    fp.fcmp(0, c.f(0), c.f(26));
    if (c.cr(0) & 4) c.set_f(26, c.f64(doubles + 0x88));
    else { c.set_f(0, c.f64(doubles + 0x40)); fp.fcmp(0, c.f(0), c.f(26)); if (c.cr(0) & 8) c.set_f(26, c.f64(doubles + 0x40)); }
    c.set_f(26, fp.frsp(c.f(26)));
    enter(c, 0x102276b0, {candidate}); readiness_rating(c, candidate);
    c.set(3, std::int32_t(std::int8_t(c.r(3)))); c.put32(at(0x60), 0x43300000);
    const auto readiness = c.r(3) ^ 0x80000000; c.set_f(2, c.f64(doubles + 0x80)); c.set(3, candidate);
    c.put32(at(0x64), readiness); c.set_f(0, c.f64(doubles + 0x38)); c.set_f(1, c.f64(at(0x60)));
    c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(1, fp.fmuls(c.f(26), c.f(1)));
    c.set_f(31, fp.fdiv(c.f(1), c.f(0))); c.set_f(31, fp.frsp(c.f(31)));
    enter(c, 0x102276e8, {candidate}); current_role(c, candidate);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) {
        enter(c, 0x102276fc, {candidate}); readiness_rating(c, candidate);
        c.set(3, std::int32_t(std::int8_t(c.r(3)))); c.put32(at(0x58), 0x43300000);
        c.set_f(2, c.f64(doubles + 0x80)); c.put32(at(0x5c), (c.r(3) - 20) ^ 0x80000000);
        c.set_f(0, c.f64(doubles + 0x28)); c.set_f(1, c.f64(at(0x58)));
        c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(0, fp.fmul(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0)));
    } else {
        enter(c, 0x10227738, {candidate}); readiness_rating(c, candidate);
        c.set(4, c.u32(candidate + 0x70)); c.set(3, std::int32_t(std::int8_t(c.r(3)))); c.put32(at(0x50), 0x43300000);
        c.set(4, c.u8(c.r(4) + 0x44)); c.set_f(4, c.f64(doubles + 0x80));
        const auto product = std::uint32_t(std::int32_t(std::int8_t(c.r(4)))) * c.r(3);
        c.set_f(2, c.f64(doubles + 0x40)); c.set_f(1, c.f64(doubles + 0xd0)); c.set_f(0, c.f64(doubles + 0x28));
        c.put32(at(0x54), product ^ 0x80000000); c.set_f(3, c.f64(at(0x50))); c.set_f(3, fp.fsubs(c.f(3), c.f(4)));
        c.set_f(2, fp.fdiv(c.f(3), c.f(2))); c.set_f(1, fp.fsub(c.f(2), c.f(1))); c.set_f(1, fp.frsp(c.f(1)));
        c.set_f(0, fp.fmul(c.f(1), c.f(0))); c.set_f(0, fp.frsp(c.f(0)));
    }
    c.set(3, c.u32(candidate + 0x6c)); c.set_f(29, c.f(0)); c.set(3, std::int32_t(std::int8_t(c.i16(c.r(3) + 0x5c))));
    const auto age = std::int32_t(c.r(3));
    unsigned first = 0, second = 0;
    if (compare(c, age, 18) < 0) { first = 8; second = 0xc; }
    else if (compare(c, std::int8_t(age), 21) < 0) { first = 0xc; second = 0x10; }
    else if (compare(c, std::int8_t(age), 24) < 0) { first = second = 0x14; }
    if (first) {
        c.set_f(2, c.f32(floats + first)); c.set_f(1, c.f32(floats + second)); c.set_f(0, c.f32(floats + second));
        c.set_f(30, fp.fmuls(c.f(30), c.f(2))); c.set_f(29, fp.fmuls(c.f(29), c.f(1))); c.set_f(31, fp.fmuls(c.f(31), c.f(0)));
    } else if (compare(c, std::int8_t(age), 30) > 0 && compare(c, std::int8_t(age), 36) < 0) {
        c.set_f(0, c.f32(floats + 0x18)); c.set_f(30, fp.fmuls(c.f(30), c.f(0)));
    }
    c.set(5, c.u32(candidate + 0x6c)); c.set(3, c.toc(-0x7b80));
    c.set(4, c.u32(c.r(5) + 0x4c)); c.set(5, c.u32(c.r(5) + 0x50)); c.call(0x10718130, 0x10227840);
    c.set(4, c.u32(candidate + 0x6c)); const auto loyalty = c.i8(c.r(4) + 0x65);
    const auto threshold = 500u - std::uint32_t(std::int32_t(loyalty)) * 10;
    c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(loyalty)) * 10 <= 500);
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(threshold)) < 0) {
        c.set_f(0, c.f32(floats + 0x14)); c.set_f(30, fp.fmuls(c.f(30), c.f(0)));
    }
    c.set(3, c.u32(candidate + 0x6c));
    if (compare(c, c.i8(c.r(3) + 0x67), 10) >= 0) return;
    c.set(3, c.u32(candidate + 0x6c)); c.put32(at(0x50), 0x43300000);
    const auto temperament = c.i8(c.r(3) + 0x67); c.set_f(2, c.f64(doubles + 0x80)); c.set_f(0, c.f32(floats + 0x1c));
    c.set_f(3, c.f32(floats + 0x14)); c.put32(at(0x54), std::uint32_t(std::int32_t(temperament)) ^ 0x80000000);
    c.set_f(1, c.f64(at(0x50))); c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(0, fp.fdivs(c.f(1), c.f(0)));
    c.set_f(0, fp.fadds(c.f(3), c.f(0))); c.set_f(30, fp.fmuls(c.f(30), c.f(0)));
}

void PlayerInfluence::run() {
    captain_effects();
    const auto side = actor.side(); c.set(3, actor.match()); const bool away = compare(c, std::int8_t(side)) != 0;
    c.set(3, c.u32(c.r(3) + 0x3118)); c.set(3, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
    c.call(0x100c7d00, 0x102278dc);
    contract_and_morale(); fixture_pressure(); initial_ability();
    c.set(3, c.u32(actor.address + 0xfc)); c.call(0x100c7d00, 0x10228220);
    experience_penalty(); limit_ability(); publish_ability();
    const auto live_side = actor.side(); c.set(3, actor.match()); const bool live_away = compare(c, std::int8_t(live_side)) != 0;
    c.set(3, c.u32(c.r(3) + 0x3118)); c.set(3, c.u32(c.r(3) + (live_away ? 0x20 : 0x1c)));
    c.set({{4, 0}, {5, 1}, {6, 0}, {7, 0}}); c.call(0x1014c2e0, 0x102286e4);
    coaching_effects();
    const auto opponent = std::int8_t(actor.side()) == 0;
    c.set(3, actor.match()); c.set(4, opponent); c.call(0x101fee30, 0x102288bc);
    technical_attributes();
    c.set_f(1, c.f32(actor.address + 0xb8)); c.set_f(2, c.f64(doubles + 0x198)); c.call(0x10780fc0, 0x10228e18);
    positional_attribute();
    const auto opposite = std::int8_t(actor.side()) == 0;
    c.set(3, actor.match()); c.set(4, opposite); c.call(0x101fee30, 0x102293f8);
    leadership_attribute();
    c.set_f(1, c.f32(actor.address + 0xd0)); c.set_f(2, c.f64(doubles + 0x198)); c.call(0x10780fc0, 0x1022987c);
    late_ratings();
    profile_ratings();
    age_and_fixture_ratings();
    contextual_ratings();
    if (!ordinary_player()) return;
    contract_adjustments();
    relationship_adjustments();
}
}
