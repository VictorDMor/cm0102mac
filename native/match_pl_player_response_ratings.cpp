#include "match_pl_player_response.hpp"

namespace cm::aot::match_pl {
using namespace guest;

namespace {
std::int32_t signed_high_third(std::int16_t value) {
    return std::int32_t(std::uint64_t(std::int64_t(value) * 0x55555556) >> 32);
}

std::uint32_t narrowed_third(std::int32_t high) {
    return std::uint32_t(high + (high < 0)) ^ 0x80000000;
}
}

void PlayerResponse::secondary_specialty() {
    const auto subtract = [&](int rating, int accumulated) {
        const auto half = rating / 2;
        c.set(3, std::int32_t(std::int16_t(10 - half)));
        c.s.wr<1>(reg::xer_ca, std::uint32_t(half) <= 10);
        c.put16(actor.address + 0x38, std::uint32_t(accumulated) - c.r(3));
    };
    const auto plain = [&](unsigned field) {
        c.set(3, c.u32(actor.address + 0x70));
        const auto accumulated = c.i16(actor.address + 0x38);
        c.set(4, std::int32_t(c.i8(c.r(3) + field)));
        subtract(std::int32_t(c.r(4)), accumulated);
    };
    const auto hybrid = [&](unsigned field) {
        c.set(3, c.u32(actor.address + 0x70));
        c.set(4, c.u32(actor.address + 0x70));
        const auto first = c.i8(c.r(3) + field);
        c.set(4, c.u8(c.r(4) + 0x19));
        c.set(3, std::int32_t(first));
        c.set(4, std::int32_t(std::int8_t(c.r(4))));
        const bool full = compare(c, std::int32_t(c.r(4)), first / 2) > 0;
        c.set(3, c.u32(actor.address + 0x70));
        const auto rating = c.i8(c.r(3) + (full ? 0x19 : field));
        if (!full) c.set(3, std::int32_t(rating));
        c.set(4, std::int32_t(full ? rating : rating / 2));
        subtract(std::int32_t(c.r(4)), c.i16(actor.address + 0x38));
    };
    if (role_has(0x1022c540, 0x80, true)) plain(0x18);
    else if (role_has(0x1022c580, 0x100, true)) hybrid(0x18);
    else if (role_has(0x1022c608, 0x200, true)) plain(0x19);
    else if (role_has(0x1022c648, 0x400, true)) hybrid(0x17);
    else if (role_has(0x1022c6d0, 0x800, true)) plain(0x17);
}

void PlayerResponse::publish_initial_ratings() {
    const auto floats = c.r(29);
    c.set(3, std::int32_t(c.i16(actor.address + 0x38)));
    c.put32(at(0x480), 0x43300000);
    c.set_f(2, c.f64(doubles + 0x248));
    c.put32(at(0x484), c.r(3) ^ 0x80000000);
    c.set_f(1, c.f32(actor.address + 0xf0));
    c.set_f(0, c.f64(doubles + 0x240));
    c.set_f(4, c.f64(doubles + 0x80));
    c.set_f(3, c.f64(at(0x480)));
    c.set_f(1, fp.multiply_add(0x1022c72c, c.f(2), c.f(1), c.f(0)));
    c.set_f(2, c.f32(floats + 0x84));
    c.set_f(3, fp.fsub(c.f(3), c.f(4)));
    c.set_f(0, c.f32(floats + 0x2c));
    c.set_f(1, fp.fmul(c.f(3), c.f(1)));
    c.put64(at(0x488), fp.fctiwz(1, c.f(1)));
    c.put16(actor.address + 0x38, c.u32(at(0x48c)));
    c.set_f(1, c.f32(actor.address + 0xc4));
    c.set_f(1, fp.fadds(c.f(2), c.f(1)));
    fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 4) {
        c.set_f(1, c.f32(floats + 0x84));
        c.set_f(0, c.f32(actor.address + 0xc4));
        c.set_f(0, fp.fadds(c.f(1), c.f(0)));
    } else c.set_f(0, c.f32(floats + 0x2c));

    publish_simple_rating(0xd0, 0x111, 0x478, 0x470, 0x468);

    const auto leadership = fp.fctiwz(0, c.f(0));
    c.set(3, 0x55550000);
    c.put32(at(0x458), 0x43300000);
    c.set(3, 0x55555556);
    c.set_f(4, c.f64(doubles + 0x1a8));
    c.put64(at(0x460), leadership);
    c.set_f(1, c.f64(doubles + 0x18));
    const auto first_rating = c.u32(at(0x464));
    c.set_f(2, c.f64(doubles + 0x80));
    c.put8(actor.address + 0x113, first_rating);
    c.set_f(0, c.f64(doubles + 0xc8));
    const auto penalty = c.i16(actor.address + 0x38);
    c.set_f(3, c.f32(actor.address + 0xbc));
    c.set(3, std::uint32_t(signed_high_third(penalty)));
    c.set_f(3, fp.multiply_add(0x1022c85c, c.f(4), c.f(3), c.f(1)));
    c.put32(at(0x45c), narrowed_third(std::int32_t(c.r(3))));
    c.set_f(1, c.f64(at(0x458)));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(1, fp.fadd(c.f(3), c.f(1)));
    fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 4) {
        c.set(4, 0x55550000);
        c.set(3, std::int32_t(c.i16(actor.address + 0x38)));
        c.set(4, 0x55555556);
        c.set(3, std::uint32_t(signed_high_third(std::int16_t(c.r(3)))));
        c.put32(at(0x450), 0x43300000);
        c.set_f(3, c.f64(doubles + 0x1a8));
        c.set_f(2, c.f32(actor.address + 0xbc));
        c.set_f(0, c.f64(doubles + 0x18));
        c.set_f(1, c.f64(doubles + 0x80));
        c.set_f(2, fp.multiply_add(0x1022c8b0, c.f(3), c.f(2), c.f(0)));
        c.put32(at(0x454), narrowed_third(std::int32_t(c.r(3))));
        c.set_f(0, c.f64(at(0x450)));
        c.set_f(0, fp.fsub(c.f(0), c.f(1)));
        c.set_f(0, fp.fadd(c.f(2), c.f(0)));
    } else c.set_f(0, c.f64(doubles + 0xc8));

    publish_simple_rating(0xb8, 0x114, 0x448, 0x440, 0x438);
    const auto movement = fp.fctiwz(0, c.f(0));
    c.put32(at(0x428), 0x43300000);
    c.set_f(3, c.f64(doubles + 0x80));
    c.put64(at(0x430), movement);
    c.set_f(0, c.f32(floats + 0x5c));
    const auto third_rating = c.u32(at(0x434));
    c.set_f(2, c.f64(doubles + 0xc8));
    c.put8(actor.address + 0x115, third_rating);
    const auto motivation = c.i8(actor.address + 0x17);
    c.set_f(4, c.f32(actor.address + 0xa0));
    c.put32(at(0x42c), std::uint32_t(std::int32_t(motivation)) ^ 0x80000000);
    c.set_f(1, c.f64(at(0x428)));
    c.set_f(1, fp.fsubs(c.f(1), c.f(3)));
    c.set_f(1, fp.fadds(c.f(4), c.f(1)));
    c.set_f(1, fp.fsubs(c.f(1), c.f(0)));
    c.call(0x10780fc0, 0x1022c9cc);
}

void PlayerResponse::publish_simple_rating(unsigned field, unsigned previous_field, unsigned output_slot,
    unsigned compare_slot, unsigned recalculate_slot) {
    const auto floats = c.r(29);
    const auto converted = fp.fctiwz(0, c.f(0));
    c.set(3, 0x55550000);
    c.put32(at(compare_slot), 0x43300000);
    c.set(3, 0x55555556);
    c.set_f(3, c.f32(floats + 0x84));
    c.put64(at(output_slot), converted);
    c.set_f(2, c.f64(doubles + 0x80));
    const auto previous = c.u32(at(output_slot + 4));
    c.set_f(0, c.f32(floats + 0x2c));
    c.put8(actor.address + previous_field, previous);
    const auto penalty = c.i16(actor.address + 0x38);
    c.set_f(1, c.f32(actor.address + field));
    c.set(3, std::uint32_t(signed_high_third(penalty)));
    c.set_f(3, fp.fadds(c.f(3), c.f(1)));
    c.put32(at(compare_slot + 4), narrowed_third(std::int32_t(c.r(3))));
    c.set_f(1, c.f64(at(compare_slot)));
    c.set_f(1, fp.fsubs(c.f(1), c.f(2)));
    c.set_f(1, fp.fadds(c.f(3), c.f(1)));
    fp.fcmp(0, c.f(1), c.f(0));
    if (!(c.cr(0) & 4)) {
        c.set_f(0, c.f32(floats + 0x2c));
        return;
    }
    c.set(4, 0x55550000);
    c.set(3, std::int32_t(c.i16(actor.address + 0x38)));
    c.set(4, 0x55555556);
    c.set(3, std::uint32_t(signed_high_third(std::int16_t(c.r(3)))));
    c.put32(at(recalculate_slot), 0x43300000);
    c.set_f(2, c.f32(floats + 0x84));
    c.set_f(0, c.f32(actor.address + field));
    c.set_f(1, c.f64(doubles + 0x80));
    c.set_f(2, fp.fadds(c.f(2), c.f(0)));
    c.put32(at(recalculate_slot + 4), narrowed_third(std::int32_t(c.r(3))));
    c.set_f(0, c.f64(at(recalculate_slot)));
    c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
    c.set_f(0, fp.fadds(c.f(2), c.f(0)));
}
}
