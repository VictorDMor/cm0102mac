#include "match_pl_movement_detail.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t PlayerMovement::high_product(std::uint32_t value, std::uint32_t multiplier) {
    return std::uint32_t((std::int64_t(std::int32_t(value)) * std::int32_t(multiplier)) >> 32);
}

std::uint32_t PlayerMovement::shift_signed(std::uint32_t word, unsigned shift) {
    c.s.wr<1>(reg::xer_ca, std::int32_t(word) < 0 && (word & ((1u << shift) - 1)));
    return std::uint32_t(std::int32_t(word) >> shift);
}

std::uint32_t PlayerMovement::toward_zero(std::uint32_t shifted) { return shifted + (shifted >> 31); }

std::uint32_t PlayerMovement::condition_deficit(std::uint32_t word) {
    const auto condition = std::uint32_t(std::int16_t(word));
    c.s.wr<1>(reg::xer_ca, condition <= 10000);
    return 10000 - condition;
}

void PlayerMovement::refill_carrier_pool(std::uint32_t back) {
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(29)));
    c.set(3, c.r(3) + 2);
    const bool wrapped = compare_logical(c, c.r(3), c.r(0)) > 0;
    c.put32(c.r(30), c.r(3));
    if (wrapped) {
        c.put32(c.r(30), c.r(28));
        c.call(0x1077b770, back);
        c.set(0, c.r(3) & 0xffff);
        c.put32(c.r(27), c.r(0));
    }
}

void PlayerMovement::store_control_pair(unsigned first, unsigned second) {
    const auto first_condition = fp.fctiwz(1, c.f(31));
    const auto second_condition = fp.fctiwz(0, c.f(30));
    c.put64(frame.at(first), first_condition);
    c.put64(frame.at(second), second_condition);
}

std::uint32_t PlayerMovement::control_interval(unsigned first, unsigned second, unsigned first_register, unsigned multiplier) {
    c.set(first_register, c.u32(frame.at(first + 4)));
    c.set(0, c.u32(frame.at(second + 4)));
    c.set(first_register, condition_deficit(c.r(first_register)));
    c.set(0, condition_deficit(c.r(0)));
    return shift_signed(high_product(c.r(first_register) * c.r(0), c.r(multiplier)), 12);
}

void PlayerMovement::sample_narrow_control() {
    const auto first = fp.fctiwz(0, c.f(31));
    c.set(3, c.u32(c.r(30)));
    const auto second = fp.fctiwz(2, c.f(30));
    const auto repeated_first = fp.fctiwz(1, c.f(31));
    c.put64(frame.at(0xbc), first);
    const auto repeated_second = fp.fctiwz(0, c.f(30));
    c.set(8, c.u16(c.r(3)));
    c.set(7, 0x14f8b589);
    c.put64(frame.at(0xc4), second);
    c.set(5, 0x14f8b589);
    c.set(3, c.u32(frame.at(0xc0)));
    c.set(0, c.u32(frame.at(0xc8)));
    c.set(4, c.u32(c.r(27)));
    c.set(3, condition_deficit(c.r(3)));
    c.set(0, condition_deficit(c.r(0)));
    c.put64(frame.at(0xcc), repeated_first);
    c.set(6, c.r(3) * c.r(0));
    c.set(0, c.r(8) + c.r(4));
    c.put64(frame.at(0xd4), repeated_second);
    c.set(4, c.u32(frame.at(0xd0)));
    c.set(3, c.u32(frame.at(0xd8)));
    const auto interval = shift_signed(high_product(c.r(6), c.r(7)), 12);
    c.set(4, condition_deficit(c.r(4)));
    c.set(7, toward_zero(interval));
    c.set(3, condition_deficit(c.r(3)));
    c.set(6, std::uint32_t(divide_signed<4>(c.r(0), c.r(7))));
    c.set(3, high_product(c.r(4) * c.r(3), c.r(5)));
    c.set(5, c.r(6) * c.r(7));
    c.set(3, shift_signed(c.r(3), 12));
    c.set(4, c.r(3) >> 31);
    c.set(21, c.r(0) - c.r(5));
    const bool positive = compare(c, std::int32_t(c.r(3) + c.r(4))) > 0;
    c.set(0, positive ? c.r(21) : 0u - c.r(21));
    c.set(21, c.r(0));
}

void PlayerMovement::sample_carrier_control() {
    c.set_f(31, c.f32(actor.address + 0x50));
    c.set(4, 0x14f8b589);
    c.set_f(30, c.f32(actor.address + 0x50));
    store_control_pair(0x10c, 0x104);
    const auto shifted = control_interval(0x10c, 0x104, 3, 4);
    c.set(3, shifted >> 31);
    c.set(0, toward_zero(shifted));
    if (compare(c, std::int32_t(c.r(0))) == 0) {
        c.set(21, 0);
        return;
    }
    refill_carrier_pool(0x1022ecc0);
    c.set(5, 0x14f8b589);
    c.set(3, 65535);
    store_control_pair(0x104, 0x10c);
    auto interval = control_interval(0x104, 0x10c, 4, 5);
    c.set(4, interval >> 31);
    c.set(0, toward_zero(interval));
    compare(c, std::int32_t(c.r(0)), 65535);
    // Each input to the deficit is a signed halfword. The product's
    // magnitude is at most 42,768 squared, and the scaled bound at most
    // 36,582. Both wide checks always continue to the narrow sampler.
    c.set(3, std::uint32_t(-65535));
    store_control_pair(0x104, 0x10c);
    interval = control_interval(0x104, 0x10c, 4, 5);
    c.set(4, interval >> 31);
    c.set(0, toward_zero(interval));
    compare(c, std::int32_t(c.r(0)), -65535);
    sample_narrow_control();
}

void PlayerMovement::accumulate_movement_bias() {
    c.set(0, std::uint32_t(std::int8_t(c.r(19))));
    const bool enabled = compare(c, std::int32_t(c.r(0))) != 0;
    c.set(4, std::uint32_t(std::int16_t(c.r(21))));
    if (!enabled) return;
    c.set_f(1, c.f64(c.r(26) + 0x40));
    c.set_f(0, c.f32(actor.address + 0x9c));
    c.set(0, std::uint32_t(c.i16(actor.address + 0x1a0)));
    c.set_f(0, fp.fadd(c.f(1), c.f(0)));
    c.put64(frame.at(0xbc), fp.fctiwz(0, c.f(0)));
    c.set(3, c.u32(frame.at(0xc0)));
    c.set(0, c.r(0) + c.r(3));
    c.put16(actor.address + 0x1a0, c.r(0));
    c.set(0, std::uint32_t(c.i16(actor.address + 0x1a0)));
    if (compare(c, std::int32_t(c.r(0)), 200) > 0) c.set(0, 200);
    else {
        c.set(0, std::uint32_t(c.i16(actor.address + 0x1a0)));
        if (compare(c, std::int32_t(c.r(0)), -200) >= 0) return;
        c.set(0, std::uint32_t(-200));
    }
    c.put16(actor.address + 0x1a0, c.r(0));
}

void PlayerMovement::sample_wide_action(std::int32_t bound, std::uint32_t back, unsigned destination) {
    c.set(3, std::uint32_t(bound) ^ 0x80000000);
    c.set(0, 0x43300000);
    c.put32(frame.at(0xc0), c.r(3));
    c.set(7, c.u32(c.r(30)));
    c.set(4, 0x43300000);
    c.put32(frame.at(0xbc), c.r(0));
    c.set_f(2, c.f64(c.r(26) + 0x80));
    c.set_f(1, c.f64(frame.at(0xbc)));
    c.set_f(0, c.f64(c.r(26)));
    c.set(5, 65535);
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set(8, c.u16(c.r(7)));
    c.set(7, c.u32(c.r(27)));
    c.set(0, std::uint32_t(bound) * 10);
    c.put32(frame.at(0xc4), c.r(4));
    c.set_f(2, fp.fdiv(c.f(1), c.f(0)));
    c.set(7, c.r(8) + c.r(7));
    c.set_f(1, c.f64(c.r(26) + 0x80));
    const auto sample = shift_signed(high_product(c.r(7), 0x80008001) + c.r(7), 15);
    c.set(6, sample >> 31);
    c.set(destination, c.r(7) - toward_zero(sample) * c.r(5));
    c.set(4, c.r(destination) ^ 0x80000000);
    c.put32(frame.at(0xc8), c.r(4));
    c.set(3, high_product(c.r(0), 0x80008001));
    c.set_f(0, c.f64(frame.at(0xc4)));
    c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    c.set(0, shift_signed(c.r(3) + c.r(0), 15));
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    c.set(3, toward_zero(c.r(0)));
    c.put64(frame.at(0xcc), fp.fctiwz(0, c.f(0)));
    c.set(destination, c.u32(frame.at(0xd0)));
    c.call(0x10226520, back);
    c.set(4, 0x66666667);
    const auto numerator = c.r(3) + 5;
    c.set(0, std::uint32_t(bound) - 1);
    c.set(3, shift_signed(high_product(numerator, c.r(4)), 2));
    c.set(4, c.r(3) >> 31);
    c.set(3, c.r(3) + c.r(4));
    c.set(destination, c.r(destination) + c.r(3));
    if (compare(c, std::int32_t(c.r(destination)), std::int32_t(c.r(0))) > 0) c.set(destination, c.r(0));
}

void PlayerMovement::sample_action_control(unsigned field, std::uint32_t reseed, std::uint32_t wide_back) {
    c.set(0, std::uint32_t(c.i16(actor.address + field)));
    c.set(4, std::uint32_t(std::int16_t(c.r(4))));
    c.set(3, std::uint32_t(c.i16(actor.address + 0x1a0)));
    c.set(0, c.r(3) + c.r(0));
    c.set(22, c.r(0) - c.r(4));
    const auto bound = std::int32_t(c.r(22));
    if (compare(c, bound) == 0) c.set(21, 0);
    else {
        refill_carrier_pool(reseed);
        c.set(3, 65535);
        bool wide = compare(c, bound, 65535) > 0;
        if (!wide) {
            c.set(3, std::uint32_t(-65535));
            wide = compare(c, bound, -65535) < 0;
        }
        if (wide) sample_wide_action(bound, wide_back);
        else {
            c.set(3, c.u32(c.r(30)));
            compare(c, bound);
            c.set(0, c.u32(c.r(27)));
            c.set(3, c.u16(c.r(3)) + c.r(0));
            const auto remainder = c.r(3) - std::uint32_t(divide_signed<4>(c.r(3), c.r(22))) * c.r(22);
            c.set(0, bound > 0 ? remainder : 0u - remainder);
            c.set(21, c.r(0));
        }
    }
    c.set(21, std::uint32_t(std::int16_t(c.r(21))));
}

void PlayerMovement::apply_carrier_surface() {
    c.set(3, actor.match());
    const int surface = c.i8(c.r(3) + 1);
    c.set(0, std::uint32_t(surface));
    unsigned offset = 0;
    std::uint32_t back = 0;
    if (compare(c, surface, 8) == 0) {
        back = 0x1022f5d0;
        offset = 2;
    } else if (surface < 8) {
        if (compare(c, surface, 3) >= 0 || compare(c, surface, 1) < 0) return;
        back = 0x1022f56c;
        offset = 1;
    } else {
        if (compare(c, surface, 32) != 0) return;
        back = 0x1022f638;
        offset = 3;
    }
    refill_carrier_pool(back);
    if (offset == 3) {
        c.set(3, c.u32(c.r(30)));
        c.set(0, c.u32(c.r(27)));
        c.set(3, std::uint32_t(std::int32_t(c.u16(c.r(3)) + c.r(0)) % 8));
    } else {
        c.set(4, c.u32(c.r(30)));
        const auto magic = offset == 1 ? 0x55555556u : 0x66666667u;
        c.set(3, magic);
        c.set(0, c.u32(c.r(27)));
        c.set(4, c.u16(c.r(4)) + c.r(0));
        const auto high = high_product(c.r(4), magic);
        const auto quotient = offset == 1 ? high : shift_signed(high, 1);
        c.set(3, c.r(4) - toward_zero(quotient) * (offset == 1 ? 3u : 5u));
    }
    c.set(0, std::uint32_t(std::int16_t(c.r(3) + offset)));
    c.set(21, c.r(21) - c.r(0));
}

std::uint32_t PlayerMovement::carrier_ball_bound() const {
    const auto first = int(std::int8_t(c.r(24)));
    const auto second = int(std::int8_t(c.r(23)));
    const auto third = int(std::int8_t(c.r(25)));
    return std::uint32_t(first * second * third * 50);
}

void PlayerMovement::sample_wide_ball_state(std::uint32_t bound) {
    c.set(6, c.u32(c.r(30)));
    c.set(0, 0x43300000);
    c.put32(frame.at(0xbc), c.r(0));
    c.set(5, 0x80008001);
    c.set(7, bound);
    c.set(4, 65535);
    c.set(9, c.u16(c.r(6)));
    c.set_f(3, c.f64(c.r(26) + 0x80));
    c.set_f(0, c.f64(c.r(26)));
    c.set(8, c.u32(c.r(27)));
    c.put32(frame.at(0xc4), c.r(0));
    c.set(6, c.r(7) ^ 0x80000000);
    c.set(0, c.r(9) + c.r(8));
    c.put32(frame.at(0xc0), c.r(6));
    c.set(8, high_product(c.r(0), c.r(5)));
    c.set(6, std::uint32_t(std::int8_t(c.r(24))));
    c.set_f(1, c.f64(c.r(26) + 0x80));
    c.set_f(2, c.f64(frame.at(0xbc)));
    c.set(5, std::uint32_t(std::int8_t(c.r(23))));
    c.set(7, std::uint32_t(std::int8_t(c.r(25))));
    c.set_f(2, fp.fsub(c.f(2), c.f(3)));
    c.set(8, shift_signed(c.r(8) + c.r(0), 15));
    c.set(3, 0x80008001);
    c.set(9, c.r(8) >> 31);
    c.set_f(2, fp.fdiv(c.f(2), c.f(0)));
    c.set(8, (c.r(8) + c.r(9)) * c.r(4));
    c.set(22, c.r(0) - c.r(8));
    c.set(0, c.r(22) ^ 0x80000000);
    c.put32(frame.at(0xc8), c.r(0));
    c.set(4, c.r(6) * c.r(5));
    c.set_f(0, c.f64(frame.at(0xc4)));
    c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    c.set(0, c.r(7) * c.r(4));
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    const auto sample = fp.fctiwz(0, c.f(0));
    c.set(0, c.r(0) * 500);
    c.put64(frame.at(0xcc), sample);
    c.set(3, high_product(c.r(0), c.r(3)));
    c.set(22, c.u32(frame.at(0xd0)));
    c.set(0, shift_signed(c.r(3) + c.r(0), 15));
    c.set(3, toward_zero(c.r(0)));
    c.call(0x10226520, 0x1022f804);

    const auto adjustment = shift_signed(high_product(c.r(3) + 5, 0x66666667), 2);
    c.set(4, adjustment);
    c.set(5, adjustment >> 31);
    c.set(3, bound);
    c.set(22, c.r(22) + toward_zero(adjustment));
    c.set(0, bound - 1);
    if (compare(c, std::int32_t(c.r(22)), std::int32_t(c.r(0))) > 0) {
        c.set(0, std::uint32_t(std::int32_t(bound) / 50));
        c.set(22, bound - 1);
    }
}

void PlayerMovement::sample_narrow_ball_state(std::uint32_t bound) {
    c.set(4, std::uint32_t(int(std::int8_t(c.r(24))) * int(std::int8_t(c.r(23)))));
    c.set(5, std::uint32_t(std::int8_t(c.r(25))));
    c.set(7, c.u32(c.r(30)));
    c.set(6, c.u32(c.r(27)));
    c.set(7, c.u16(c.r(7)) + c.r(6));
    c.set(4, c.r(5));
    c.set(6, bound);
    c.set(5, std::uint32_t(divide_signed<4>(c.r(7), bound)));
    c.set(3, c.r(5) * bound);
    c.set(22, c.r(7) - c.r(3));
    const bool positive = compare(c, std::int32_t(bound)) > 0;
    c.set(0, positive ? c.r(22) : 0u - c.r(22));
    c.set(22, c.r(0));
}

void PlayerMovement::sample_carrier_ball_state() {
    c.set(3, actor.match());
    c.set(4, actor.match());
    c.set(24, c.u8(c.r(3) + 0x791f));
    c.set(23, c.u8(c.r(4) + 0x791f));
    c.set(4, actor.match());
    c.set(25, c.u8(c.r(4) + 0x791f));
    c.set(3, std::uint32_t(std::int8_t(c.r(25))));
    const auto bound = carrier_ball_bound();
    c.set(0, bound);
    if (compare(c, std::int32_t(bound)) == 0) {
        c.set(22, 0);
        return;
    }
    refill_carrier_pool(0x1022f6d0);
    c.set(4, std::uint32_t(std::int8_t(c.r(25))));
    c.set(3, 65535);
    c.set(0, bound);
    bool wide = compare(c, std::int32_t(bound), 65535) > 0;
    if (!wide) {
        c.set(3, std::uint32_t(-65535));
        wide = compare(c, std::int32_t(bound), -65535) < 0;
    }
    if (wide) sample_wide_ball_state(bound);
    else sample_narrow_ball_state(bound);
}

void PlayerMovement::sample_carrier_surface_condition() {
    c.set(0, std::uint32_t(c.i16(actor.address + 0x2a)));
    c.set(3, 0x66666667);
    c.set(4, std::uint32_t(std::int16_t(c.r(22))));
    c.set(0, high_product(c.r(0), c.r(3)));
    c.set(21, c.r(21) - c.r(4));
    c.set(0, shift_signed(c.r(0), 2));
    c.set(3, toward_zero(c.r(0)));
    c.set(23, c.r(3) - 30);
    c.s.wr<1>(reg::xer_ca, c.r(3) >= 30);
    const auto bound = std::int32_t(c.r(23));
    if (compare(c, bound) == 0) {
        c.set(22, 0);
        return;
    }

    refill_carrier_pool(0x1022f91c);
    // The signed halfword divided by ten and reduced by thirty is narrow.
    c.set(3, 65535);
    compare(c, bound, 65535);
    c.set(3, std::uint32_t(-65535));
    compare(c, bound, -65535);
    c.set(3, c.u32(c.r(30)));
    const bool positive = compare(c, bound) > 0;
    c.set(0, c.u32(c.r(27)));
    c.set(3, c.u16(c.r(3)) + c.r(0));
    const auto remainder = c.r(3) - std::uint32_t(divide_signed<4>(c.r(3), c.r(23))) * c.r(23);
    c.set(0, positive ? remainder : 0u - remainder);
    c.set(22, c.r(0));
}

void PlayerMovement::query_carrier_neighbour() {
    c.set(0, frame.at(0x43));
    c.set(3, std::uint32_t(std::int16_t(c.r(22))));
    c.put32(frame.at(0x38), c.r(0));
    c.set({{0, 0}, {4, actor.address}});
    c.set(21, c.r(21) + c.r(3));
    c.put32(frame.at(0x3c), 0);
    c.set({{6, std::uint32_t(std::int8_t(c.r(18)))}, {7, frame.at(0x45)}, {8, frame.at(0x44)},
           {3, actor.match()}, {9, frame.at(0x48)}, {5, actor.side()}, {10, frame.at(0x46)}});
    c.call(0x101eaf60, 0x1022fa94);
    c.set(22, c.r(3));
    if (compare(c, std::int32_t(c.r(22))) == 0) return;
    c.set({{7, c.u8(frame.at(0x46))}, {4, actor.address}, {3, actor.match()}, {5, c.r(18)}, {6, c.r(22)}});
    c.set({{7, std::uint32_t(std::int8_t(c.r(7)))}, {8, 0}});
    c.call(0x101f5350, 0x1022fabc);
}

bool PlayerMovement::carrier_commentary_allowed() {
    for (int kind : {18, 17, 16, 15}) {
        c.set(0, std::uint32_t(c.i16(actor.address + 0x19c)));
        if (compare(c, std::int32_t(c.r(0)), kind) == 0) return false;
    }
    c.set(3, actor.address);
    c.call(0x10289560, 0x1022faf4);
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0)), 1) == 0) return false;
    refill_carrier_pool(0x1022fb20);
    c.set(4, c.u32(c.r(30)));
    c.set(3, 0x66666667);
    c.set(0, c.u32(c.r(27)));
    c.set(4, c.u16(c.r(4)) + c.r(0));
    const auto shifted = shift_signed(high_product(c.r(4), c.r(3)), 3);
    c.set(3, shifted >> 31);
    c.set(0, c.r(4) - toward_zero(shifted) * 20);
    return compare(c, std::int32_t(c.r(0))) == 0;
}

void PlayerMovement::commentary_strength(std::uint32_t back, unsigned spill, unsigned destination) {
    c.set(3, actor.address);
    c.call(0x10245160, back);
    c.set_f(2, c.f32(c.r(31) + 0x8c));
    c.set(3, 0x66666667);
    c.set_f(0, c.f64(c.r(26) + 0xc0));
    c.set_f(1, fp.fmuls(c.f(2), c.f(1)));
    c.set_f(0, fp.fadd(c.f(0), c.f(1)));
    c.put64(frame.at(spill), fp.fctiwz(0, c.f(0)));
    c.set(0, std::uint32_t(std::int8_t(c.u32(frame.at(spill + 4)))));
    const auto shifted = shift_signed(high_product(c.r(0), c.r(3)), 1);
    c.set(3, shifted >> 31);
    c.set(0, toward_zero(shifted));
    c.set(destination, c.r(0) * 5);
}

void PlayerMovement::narrate_carrier_position(unsigned message, std::uint32_t back) {
    c.set({{3, actor.match()}, {10, std::uint32_t(std::int8_t(c.r(23)))}, {7, actor.side()}, {4, message}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{8, UINT32_MAX}, {6, c.u8(actor.address + 0x2f)}, {9, UINT32_MAX}, {5, actor.match()}});
    c.call(0x10201180, back);
}

bool PlayerMovement::carrier_flank(bool left) {
    c.set(0, signed_side());
    if (compare(c, std::int32_t(c.r(0))) == 0) {
        c.set(0, std::uint32_t(actor.column()));
        const int order = compare(c, std::int32_t(c.r(0)), left ? 2 : 6);
        if (left ? order < 0 : order > 0) return true;
    }
    c.set(0, actor.side());
    if (compare(c, std::int32_t(c.r(0)), 1) != 0) return false;
    c.set(0, std::uint32_t(actor.column()));
    const int order = compare(c, std::int32_t(c.r(0)), left ? 6 : 2);
    return left ? order > 0 : order < 0;
}

bool PlayerMovement::carrier_forward() {
    c.set(0, actor.side());
    c.set(23, std::uint32_t(std::int8_t(c.r(3))));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    if (compare(c, std::int32_t(c.r(0))) == 0) {
        c.set(0, std::uint32_t(actor.row()));
        if (compare(c, std::int32_t(c.r(0)), 7) > 0) return true;
    }
    c.set(0, actor.side());
    if (compare(c, std::int32_t(c.r(0)), 1) != 0) return false;
    c.set(0, std::uint32_t(actor.row()));
    return compare(c, std::int32_t(c.r(0)), 4) < 0;
}

bool PlayerMovement::carrier_backward() {
    c.set(0, actor.side());
    if (compare(c, std::int32_t(c.r(0)), 1) == 0) {
        c.set(0, std::uint32_t(actor.row()));
        if (compare(c, std::int32_t(c.r(0)), 7) > 0) return true;
    }
    c.set(0, signed_side());
    if (compare(c, std::int32_t(c.r(0))) != 0) return false;
    c.set(0, std::uint32_t(actor.row()));
    return compare(c, std::int32_t(c.r(0)), 4) < 0;
}

void PlayerMovement::narrate_carrier_centre() {
    c.set(0, c.u8(actor.address + 0x105));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))) - 4);
    c.call(0x10775540, 0x1022ff4c);
    if (compare(c, std::int32_t(c.r(3)), 2) >= 0) return;
    c.set(0, c.u8(actor.address + 0x106));
    if (compare(c, std::int32_t(c.r(0)), 5) != 0) {
        c.set(0, c.u8(actor.address + 0x106));
        if (compare(c, std::int32_t(c.r(0)), 6) != 0) return;
    }
    refill_carrier_pool(0x1022ff8c);
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(27)));
    c.set(3, c.u16(c.r(3)));
    const auto word = c.r(3) + c.r(0);
    c.set(3, word >> 31);
    c.set(0, std::uint32_t(std::int32_t(word) % 2));
    if (compare(c, std::int32_t(c.r(0))) != 0) narrate_carrier_position(0x215e, 0x1022ffe0);
}

void PlayerMovement::narrate_carrier_zones() {
    commentary_strength(0x1022fb64, 0xbc, 0);
    if (compare(c, std::int32_t(c.r(0)), 1) < 0) c.set(3, 1);
    else commentary_strength(0x1022fbb8, 0xc4, 3);
    if (carrier_forward()) {
        if (carrier_flank(true)) narrate_carrier_position(0x2160, 0x1022fc90);
        else if (carrier_flank(false)) narrate_carrier_position(0x2162, 0x1022fcf4);
        else {
            c.set(0, signed_side());
            c.set(3, actor.address);
            c.set(0, c.r(0) == 0);
            c.set(4, c.r(0));
            c.call(0x10239d10, 0x1022fd14);
            c.set(0, std::uint32_t(std::int8_t(c.r(3))));
            if (compare(c, std::int32_t(c.r(0))) == 0) narrate_carrier_position(0x2161, 0x1022fd44);
        }
    } else if (carrier_backward()) {
        if (carrier_flank(true)) narrate_carrier_position(0x215a, 0x1022fde0);
        else if (carrier_flank(false)) narrate_carrier_position(0x215c, 0x1022fe44);
        else narrate_carrier_position(0x215b, 0x1022fe70);
    } else if (carrier_flank(true)) narrate_carrier_position(0x215d, 0x1022fed4);
    else if (carrier_flank(false)) narrate_carrier_position(0x215f, 0x1022ff38);
    else narrate_carrier_centre();
}

void PlayerMovement::sample_carrier() {
    sample_carrier_control();
    accumulate_movement_bias();
    const auto kind = std::int32_t(std::int16_t(c.r(18)));
    c.set(0, std::uint32_t(kind));
    if (compare(c, kind, 15) == 0) sample_action_control(0x13c, 0x1022f0c0, 0x1022f198);
    else {
        c.set(0, std::uint32_t(kind));
        if (compare(c, kind, 18) == 0) sample_action_control(0x142, 0x1022f250, 0x1022f328);
        else sample_action_control(0x140, 0x1022f3d4, 0x1022f4ac);
    }
}

void PlayerMovement::sample_carrier_environment() {
    apply_carrier_surface();
    sample_carrier_ball_state();
    sample_carrier_surface_condition();
}

void PlayerMovement::prepare_carrier_interaction() {
    query_carrier_neighbour();
    if (carrier_commentary_allowed()) narrate_carrier_zones();
}
}
