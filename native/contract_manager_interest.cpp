#include "contract_manager_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::contract_manager {
using namespace detail;

void adjust_contract_interest(Cpu& c, std::uint32_t person, std::uint32_t club, std::uint32_t contract_address) {
    Frame frame(c, 0x1007d530, 0x60, {.first_gpr = 31});
    Kept<31> contract(c, contract_address); const bool has_contract = nonzero(c, std::int32_t(contract_address));
    const auto constants = c.toc(-0x571c); c.set(7, constants); Program program(c);
    if (!has_contract || !nonzero_logical(c, club) || !nonzero_logical(c, person)) return;
    const auto attributes = c.u32(person + 0x14); c.set(8, attributes);
    if (!nonzero_logical(c, attributes)) return;
    guest::Fpu fpu(c);
    const auto raw_age = c.i16(person + 0x5c), reputation = c.i16(club + 0x1ec), ability = c.i16(attributes + 10);
    const auto age = std::int32_t(std::int8_t(raw_age)), difference = std::int32_t(ability) - reputation;
    c.set(6, extended(raw_age)); c.set(4, 0x43300000); c.set(3, extended(reputation));
    c.set(0, std::uint32_t(difference) ^ 0x80000000u); c.set(5, std::uint32_t(age) ^ 0x80000000u);
    c.put32(frame.at(0x50), 0x43300000); c.put32(frame.at(0x54), std::uint32_t(age) ^ 0x80000000u);
    const auto bias = c.f64(constants + 8), age_weight = c.f64(constants + 0x70);
    c.set_f(4, bias); c.set_f(5, age_weight);
    const auto converted_age = fpu.fsub(c.f64(frame.at(0x50)), bias); c.set_f(3, converted_age);
    c.put32(frame.at(0x4c), std::uint32_t(difference) ^ 0x80000000u);
    const auto offset = c.f64(constants + 0x78), threshold = c.f64(constants + 0x60);
    c.set_f(1, offset); c.set_f(0, threshold); c.put32(frame.at(0x48), 0x43300000);
    const auto converted_difference = fpu.fsub(c.f64(frame.at(0x48)), bias);
    const auto weighted = fpu.multiply_add(0x1007d5bc, age_weight, converted_age, converted_difference);
    const auto adjusted = fpu.frsp(fpu.fsub(weighted, offset)); c.set_f(2, adjusted);
    fpu.fcmp(0, adjusted, threshold);
    double factor;
    if (adjusted < threshold) {
        const auto slope = c.f64(constants + 0x80), baseline = c.f64(constants + 0x18);
        c.set_f(1, slope); c.set_f(0, baseline);
        factor = fpu.frsp(fpu.multiply_add(0x1007d5d8, slope, adjusted, baseline));
    } else {
        const auto divisor = c.f64(constants + 0x88), baseline = c.f64(constants + 0x18);
        c.set_f(0, divisor); c.set_f(1, baseline);
        const auto share = fpu.fdiv(adjusted, divisor); c.set_f(0, share);
        factor = fpu.frsp(fpu.fadd(baseline, share));
    }
    c.set_f(3, factor);
    const auto mood = c.i8(contract + 0x46); c.set(3, std::uint32_t(std::int32_t(mood))); c.set(0, 0x43300000);
    const auto minimum = c.f64(constants + 0x90); c.set_f(0, minimum);
    c.put32(frame.at(0x40), 0x43300000); c.set(0, std::uint32_t(std::int32_t(mood) + 5) ^ 0x80000000u);
    const auto numerator_bias = c.f64(constants + 8); c.set_f(2, numerator_bias);
    fpu.fcmp(0, minimum, factor);
    c.put32(frame.at(0x44), std::uint32_t(std::int32_t(mood) + 5) ^ 0x80000000u);
    const auto numerator = fpu.fsubs(c.f64(frame.at(0x40)), numerator_bias); c.set_f(1, numerator);
    double bounded = minimum;
    // The original ble/bge test negated GT/LT, including unordered values.
    if (!(minimum > factor)) {
        const auto maximum = c.f64(constants + 0x98); c.set_f(0, maximum); fpu.fcmp(0, maximum, factor);
        bounded = !(maximum < factor) ? factor : maximum;
    }
    const auto divisor = fpu.frsp(bounded);
    const auto quotient = fpu.fdivs(numerator, divisor);
    const auto integer = fpu.fctiwz(0, quotient); c.put64(frame.at(0x38), integer);
    auto delta_bits = c.u32(frame.at(0x3c)); c.set(0, delta_bits);
    const auto initial_delta = std::int8_t(delta_bits); c.set(3, std::uint32_t(std::int32_t(initial_delta)));
    if (compare(c, initial_delta, 10) > 0) { delta_bits = 10; c.set(0, 10); }
    const auto flags = c.u32(contract + 0x50); c.set(4, flags);
    std::uint32_t ceiling = 100; c.set(6, ceiling); c.set(3, flags & 2u);
    if (nonzero(c, std::int32_t(flags & 2u))) { ceiling = 50; c.set(6, ceiling); }
    for (const auto flag : {1u, 8u, 32u, 64u, 128u, 256u}) {
        const auto signed_ceiling = std::int8_t(ceiling); c.set(3, std::uint32_t(std::int32_t(signed_ceiling)));
        if (compare(c, signed_ceiling, -50) < 0) continue;
        const auto set = flags & flag; c.set(3, set);
        if (nonzero(c, std::int32_t(set))) { ceiling -= 50; c.set(6, ceiling); }
    }
    const auto current_raw = c.u8(contract + 0x4c);
    const auto delta = std::int8_t(delta_bits);
    c.set(4, current_raw); c.set(5, std::uint32_t(std::int32_t(delta)));
    const auto updated = std::int32_t(std::int8_t(current_raw)) + delta; c.set(3, std::uint32_t(updated));
    if (compare(c, updated, 100) > 0) { c.set(3, 100); c.put8(contract + 0x4c, 100); }
    else if (compare(c, updated, -100) < 0) { c.set(3, 0xffffff9c); c.put8(contract + 0x4c, 0xffffff9c); }
    else { c.set(3, std::uint32_t(current_raw) + std::uint32_t(std::int32_t(delta))); c.put8(contract + 0x4c, std::uint32_t(current_raw) + std::uint32_t(std::int32_t(delta))); }
    const auto current = c.i8(contract + 0x4c), limit = std::int8_t(ceiling);
    c.set(4, std::uint32_t(std::int32_t(current))); c.set(3, std::uint32_t(std::int32_t(limit)));
    if (compare(c, current, limit) > 0) c.put8(contract + 0x4c, ceiling);
    const auto locked = c.i8(contract + 0x54); c.set(3, std::uint32_t(std::int32_t(locked)));
    if (!nonzero(c, locked)) {
        c.set(4, std::uint32_t(std::int32_t(delta)));
        bool clear = false;
        if (compare(c, delta, 5) > 0) {
            const auto stored = c.i8(contract + 0x4c); c.set(3, std::uint32_t(std::int32_t(stored)));
            const auto improved = std::int32_t(delta) + stored; c.set(3, std::uint32_t(improved)); clear = compare(c, improved) > 0;
        }
        if (!clear) {
            const auto score = c.i8(contract + 0x44); c.set(3, std::uint32_t(std::int32_t(score)));
            const auto required = std::int32_t(75) - delta; c.set(0, std::uint32_t(required));
            c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(std::int32_t(delta)) <= 75u);
            clear = compare(c, score, required) >= 0;
        }
        if (clear) {
            const auto first_flags = c.u32(contract + 0x50); c.set(0, first_flags & 4u);
            if (nonzero(c, std::int32_t(first_flags & 4u))) program.call(0x1037f7d0, 0x1007d7bc, {contract, 4});
            const auto second_flags = c.u32(contract + 0x50); c.set(0, second_flags & 16u);
            if (nonzero(c, std::int32_t(second_flags & 16u))) program.call(0x1037f7d0, 0x1007d7d4, {contract, 16});
        }
    }
    program.call(0x1037f590, 0x1007d7e0, {contract, 0});
}
}
