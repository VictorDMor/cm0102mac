// FINANCE.CPP, block value: the budget for one spending kind (0x100f4790).
#include "finance_value.hpp"
#include <stdexcept>

namespace cm::aot::finance {
using guest::Frame;
using namespace value_detail;

namespace {
// The kind's final range: above the upper limit it is the limit, below the lower limit the lower.
std::int32_t clamp_to_kind(std::int32_t value, std::int8_t kind) {
    struct Range { std::int32_t low, high; };
    Range range{125, 1000};
    switch (kind) {
        case 8: range = {250, 7500}; break;
        case 5: range = {1750, 35000}; break;
        case 6: case 7: range = {500, 20000}; break;
        case 10: range = {125, 1500}; break;
        case 9: range = {125, 2000}; break;
        default: break;
    }
    if (value > range.high) return range.high;
    return value < range.low ? range.low : value;
}
}

std::uint32_t club_budget_for_kind(Cpu& c, std::uint32_t account, std::int8_t kind) {
    constexpr std::uint32_t magic_slot = 0x38;  // the int-to-double idiom's words in the frame
    Frame frame(c, 0x100f4790, 0x70, {.first_gpr = 26});
    Numbers n(c);
    const auto club = club_record(c, c.u32(account + account::club));
    if (c.u8(club + 0x23d) == 3) return 0;
    const std::int32_t rating = std::int16_t(c.i16(club + club::reputation) / 50);
    std::int32_t value = kind_ceiling(kind);
    c.set({{26, account}, {27, club}, {28, std::uint32_t(rating)}, {29, std::uint32_t(value)},
           {30, std::uint32_t(std::int32_t(kind))}, {31, n.pool}});
    if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100f485c, {club})) != 0) return std::uint32_t(value);

    const auto league = c.u32(club + 0x2c);
    if (league == 0) {
        value = n.truncate(n.fpu.fmul(n.constant(0x60), n.widen(value)));
    } else {
        const auto level = level_of(c, league);
        const auto strength_byte = c.u8(league + 0x11f);
        const auto strength = std::int32_t(std::int8_t(strength_byte));
        {
            const auto strength_f = n.widen(strength);
            const auto level_f = n.widen(level * 20);
            const auto share = n.fpu.fdiv(strength_f, level_f);
            value = n.truncate(n.fpu.fmul(n.widen(value), share));
        }
        if (level == 1) {
            const auto id = c.u32(league + 0x68);
            // Leagues whose clubs spend more as the club's reputation grows: a pool factor (the ratio of
            // the rating to the league's strength) scales the part of the budget above a threshold.
            const auto ramp = [&](double factor, std::int32_t threshold, std::uint32_t added, std::uint32_t site,
                                  std::int32_t divisor_of_rating) {
                const auto limit = divisor_of_rating * 10;
                if (rating > limit) {
                    const auto numerator = n.widen(rating);
                    const auto denominator = n.widen(divisor_of_rating < 1 ? 1 : divisor_of_rating);
                    const auto quotient = n.fpu.fdiv(numerator, denominator);
                    factor = n.fpu.fmul(factor, n.fpu.fdiv(quotient, n.constant(0x80)));
                }
                if (value > threshold) {
                    n.fpu.fcmp(0, factor, n.constant(pool_one));
                    if (factor > n.constant(pool_one)) factor = n.constant(pool_one);
                    const auto above = n.widen(value - threshold);
                    value = n.truncate(n.fpu.multiply_add(site, above, factor, n.constant(added)));
                }
            };
            if (is_any_competition(c, id, {cell_c, cell_b, cell_a})) {
                if (c.i16(club + club::reputation) > 0x1c52) value = n.scale(value, 0xb8);
            } else if (is_competition(c, id, -0x7a24)) {
                // unchanged
            } else if (is_competition(c, id, -0x7a20)) {
                if (value > 0x6d6) {
                    const auto above = n.widen(value - 0x6d6);
                    value = n.truncate(n.fpu.multiply_add(0x100f49cc, above, n.constant(0x70), n.constant(0x150)));
                }
            } else if (is_any_competition(c, id, {cell_d, cell_e, -0x7a1c, -0x7a28})) {
                ramp(n.constant(0x70), 0x9c4, 0x158, 0x100f4ac8, strength);
            } else if (is_any_competition(c, id, {-0x7514, -0x750c, -0x7510, -0x79fc})) {
                ramp(n.constant(0x70), 0x4e2, 0x160, 0x100f4bc4, strength);
            } else {
                ramp(n.constant(0x98), 0x2ee, 0x168, 0x100f4c74, strength);
            }
        }
        if (const auto second = c.u32(club + 0x30); second != 0) {
            // The run at 0x100f4c90: the stack words the original leaves for later readers.
            const std::int32_t other = c.i16(second + 0x18);
            put_magic_pair(c, frame.at(magic_slot), value);
            c.put32(frame.at(0x44), std::uint32_t(other) ^ 0x80000000u);
            c.put32(frame.at(0x40), 0x43300000);
            const auto product = n.fpu.fmuls(n.fpu.fsubs(magic_word(value), n.constant(pool_magic)),
                                             n.fpu.fsubs(magic_word(other), n.constant(pool_magic)));
            const auto bits = n.fpu.fctiwz(0, n.fpu.fmul(n.constant(0x118), product));
            c.put64(frame.at(0x48), bits);
            value = std::int32_t(std::uint32_t(bits));
        }
    }
    c.set(29, std::uint32_t(value));
    const auto band = std::int8_t(invoke_unit(c, 0x100f55b0, 0x100f4d18, {account}));
    if (band < 0) {
        if (band == -2) value = n.scale(value, 0x70);
        else if (band > -3) value = n.scale(value, 0x100);
    } else if (band == 2) {
        value = std::int32_t(std::uint32_t(value) << 1);
    } else if (band == 1) {
        // The run at 0x100f4d4c.
        put_magic_pair(c, frame.at(magic_slot), value);
        const auto bits = n.fpu.fctiwz(0, n.fpu.fmul(n.constant(0x00), n.widen(value)));
        c.put64(frame.at(0x40), bits);
        value = std::int32_t(std::uint32_t(bits));
    }
    if (c.i32(account + 0x14) < value) value = c.i32(account + 0x14);
    value = clamp_to_kind(value, kind);
    // 0x100f4ee8 compares with 250000, but every kind's range ends at 35000 or below, so the cap never acts.
    if (value > 250000) throw std::logic_error("budget above 250000 after the kind's range");
    return std::uint32_t(value);
}
}
