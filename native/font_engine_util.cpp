// The font engine, block `util` (UTIL, 0x10767ce0..0x10768074): the fixed-point helpers every other unit of the engine calls, a
// bubble sort of halfwords and a vector length. Pure functions of their arguments (the sort and nothing else touches memory);
// what each leaves in r4-r8 is set where the original's instructions leave it (Leftovers), because callers' indirect calls
// can read them and the checker compares them.
#include "font_engine_detail.hpp"

namespace cm::aot::font_engine {
namespace {
// |value| as the original computes it (neg of a negative value: INT_MIN stays), and the sign of a product of two operands.
constexpr std::uint32_t magnitude(std::int32_t v) { return v < 0 ? negate(std::uint32_t(v)) : std::uint32_t(v); }
constexpr std::int32_t product_sign(bool first_negative, bool second_negative) { return first_negative == second_negative ? 1 : -1; }
}

std::int32_t fix_multiply(std::int32_t a, std::int32_t b, Leftovers& left) {
    const auto ma = magnitude(a), mb = magnitude(b);
    const auto sign = product_sign(a < 0, b < 0);
    const auto a_low = low_half(ma), a_high = low_half(high_half(ma)), b_low = low_half(mb), b_high = low_half(high_half(mb));
    const auto high_high = a_high * b_high, low_low = a_low * b_low, a_low_b_high = a_low * b_high, a_high_b_low = a_high * b_low;
    const auto cross = a_low_b_high + a_high_b_low;
    const auto result = ((high_high << 16) + (cross + rlwinm(low_low, 16, 16, 31))) * std::uint32_t(sign);
    left = {.r4 = a_high_b_low, .r5 = a_high, .r6 = cross, .r7 = a_low, .r8 = std::uint32_t(sign)};
    return wrap(result);
}

std::int32_t frac_multiply(std::int32_t a, std::int32_t b, Leftovers& left) {
    const auto ma = magnitude(a), mb = magnitude(b);
    const auto sign = product_sign(a < 0, b < 0);
    const auto a_low = low_half(ma), a_high = low_half(high_half(ma)), b_low = low_half(mb), b_high = low_half(high_half(mb));
    const auto high_high = rlwinm(a_high * b_high, 26, 0, 5), low_low = rlwinm(a_low * b_low, 26, 6, 31);
    const auto cross = a_low * b_high + a_high * b_low;
    const auto result = (high_high + (rlwinm(cross, 10, 0, 21) + low_low)) * std::uint32_t(sign);
    left = {.r4 = high_high, .r5 = b_low, .r6 = a_high, .r7 = cross, .r8 = std::uint32_t(sign)};
    return wrap(result);
}

std::int32_t scale_multiply(std::int32_t value, std::int32_t scale, Leftovers& left) {
    // The scale's sign is that of its low halfword; its magnitude is taken from the whole register when that sign is negative.
    const bool scale_negative = std::int16_t(scale & 0xffff) < 0;
    const auto m = magnitude(value);
    const auto sign = product_sign(value < 0, scale_negative);
    const auto factor = low_half(scale_negative ? negate(std::uint32_t(scale)) : std::uint32_t(scale));
    const auto high = low_half(high_half(m)), low = low_half(m);
    const auto result = (rlwinm(high * factor, 2, 0, 29) + rlwinm(low * factor, 18, 14, 31)) * std::uint32_t(sign);
    // r7 and r8 are not written.
    left.r4 = high;
    left.r5 = factor;
    left.r6 = std::uint32_t(sign);
    return wrap(result);
}

std::int32_t fix_divide(std::int32_t a, std::int32_t b, Leftovers& left) {
    const auto ma = magnitude(a);
    auto divisor = std::int32_t(magnitude(b));
    const auto sign = product_sign(a < 0, b < 0);
    const auto quotient = std::uint32_t(divide_unsigned(ma, std::uint32_t(divisor)));
    auto remainder = ma - quotient * std::uint32_t(divisor);
    // The remainder (and the divisor with it) is halved until the remainder fits 16 bits; the divisor shifts as signed.
    while (remainder > 0xffff) {
        remainder >>= 1;
        divisor >>= 1;
    }
    const auto fraction = std::uint32_t(divide_unsigned(remainder << 16, std::uint32_t(divisor)));
    left.r4 = std::uint32_t(divisor);
    left.r5 = std::uint32_t(sign);
    left.r6 = quotient;
    return wrap(((quotient << 16) + fraction) * std::uint32_t(sign));
}

std::int32_t distance(std::int32_t a, std::int32_t b, Leftovers& left) {
    // |a| and |b| by the xor/subtract idiom (INT_MIN stays negative); r5 holds a's sign mask.
    const auto sign_a = std::uint32_t(a >> 31), sign_b = std::uint32_t(b >> 31);
    left.r5 = sign_a;
    const auto abs_a = wrap((sign_a ^ std::uint32_t(a)) - sign_a), abs_b = wrap((sign_b ^ std::uint32_t(b)) - sign_b);
    if (abs_a == 0) return abs_b;
    if (abs_b == 0) return abs_a;
    // The first estimate: the larger component plus half the smaller.
    auto estimate = abs_a > abs_b ? wrap(std::uint32_t(abs_a) + std::uint32_t(abs_b >> 1)) : wrap(std::uint32_t(abs_b) + std::uint32_t(abs_a >> 1));
    const auto square_over_estimate = [&](std::int32_t component) {
        const auto quotient = fix_divide(component, estimate, left);
        return fix_multiply(component, quotient, left);
    };
    // Two Newton rounds of x = (x + (a^2 + b^2) / x + 1) / 2 and a third that is the result.
    for (int round = 0; round < 2; ++round) {
        const auto from_b = square_over_estimate(abs_b);
        const auto from_a = square_over_estimate(abs_a);
        estimate = wrap(std::uint32_t(estimate) + (std::uint32_t(from_a) + std::uint32_t(from_b) + 1)) >> 1;
    }
    const auto from_b = square_over_estimate(abs_b);
    const auto from_a = square_over_estimate(abs_a);
    return wrap(std::uint32_t(estimate) + (std::uint32_t(from_a) + std::uint32_t(from_b) + 1)) >> 1;
}

void sort_shorts(Cpu& c, std::uint32_t array, std::int32_t count, Leftovers& left) {
    const auto last = std::uint32_t(count) - 1;  // r4: the number of pairs a pass compares
    left.r4 = last;
    bool swapped;
    do {
        swapped = false;
        left.r5 = 0;
        left.r8 = 0;
        for (std::uint32_t offset = 0, n = last; std::int32_t(last) > 0 && n != 0; offset += 2, --n) {
            const auto pair = array + offset;
            const auto next = std::uint32_t(std::int32_t(c.i16(pair + 2))), current = std::uint32_t(std::int32_t(c.i16(pair)));
            left.r6 = pair;
            left.r7 = next;
            if (std::int32_t(next) < std::int32_t(current)) {
                c.put16(pair, next);
                swapped = true;
                left.r8 = 1;
                c.put16(pair + 2, current);
            }
            left.r5 = offset + 2;
        }
    } while (swapped);
}
}
