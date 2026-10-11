#include "compiler_arithmetic.hpp"
#include "mac_abi.hpp"
#include <bit>

namespace cm::aot::compiler_arithmetic {
namespace {
struct IntegerAbi : MacAbi {
    std::uint64_t escapes = 0;
    explicit IntegerAbi(State& state) : MacAbi(state, escapes) {}
    std::uint64_t pair(unsigned high) const { return (std::uint64_t(r(high)) << 32) | r(high + 1); }
    void pair(unsigned high, std::uint64_t value) { put(high, std::uint32_t(value >> 32)); put(high + 1, std::uint32_t(value)); }
    void negate_pair(unsigned high) {
        const auto low = r(high + 1), upper = r(high);
        put(high + 1, 0 - low);
        const auto sum = std::uint64_t(~upper) + unsigned(low == 0);
        s.wr<1>(1027, sum >> 32); put(high, std::uint32_t(sum));
    }
    void compare_field(unsigned field, std::uint32_t value) {
        const auto signed_value = std::bit_cast<std::int32_t>(value);
        s.wr<1>(2304 + field, (signed_value < 0 ? 8 : signed_value > 0 ? 4 : 2) | (s.r<1>(1024) & 1));
    }
};

// Preserve the restoring-divider's observable register contract without
// replaying its bit loop. The final subtraction, carry and CR depend on the
// last quotient bit, even when the returned value is only the remainder.
bool divide_magnitudes(IntegerAbi& a, bool remainder_result) {
    const auto dividend = a.pair(3), divisor = a.pair(5);
    const auto dividend_zeros = unsigned(std::countl_zero(dividend));
    const auto divisor_zeros = unsigned(std::countl_zero(divisor));
    if (dividend_zeros > divisor_zeros) {
        a.put(0, dividend_zeros); a.put(9, divisor_zeros); a.put(10, 64 - dividend_zeros);
        a.s.wr<1>(1027, 1); a.compare(dividend_zeros, divisor_zeros);
        if (!remainder_result) a.pair(3, 0);
        return false;
    }
    std::uint64_t quotient, remainder, difference;
    bool subtraction_succeeded;
    if (divisor) {
        quotient = dividend / divisor; remainder = dividend % divisor;
        subtraction_succeeded = (quotient & 1) != 0;
        difference = subtraction_succeeded ? remainder : remainder - divisor;
    } else {
        // The original has no trapping divide. It makes at most 65 restoring
        // iterations and shifts in carry bits. With a top-bit dividend the
        // initial shift count is -1; word-shift masking clears both lanes and
        // the final remainder is zero. Other dividends survive unchanged.
        const auto iterations = 65 - dividend_zeros;
        quotient = iterations >= 64 ? UINT64_MAX : (UINT64_C(1) << iterations) - 1;
        remainder = dividend_zeros ? dividend : 0; difference = remainder;
        subtraction_succeeded = true;
    }
    const auto difference_high = std::uint32_t(difference >> 32);
    a.put(0, subtraction_succeeded ? 0 : std::uint32_t(difference));
    a.pair(7, remainder); a.put(9, difference_high); a.put(10, UINT32_MAX);
    a.s.wr<4>(4132, 0); a.compare(std::bit_cast<std::int32_t>(difference_high));
    const auto carry = remainder_result ? (!divisor || subtraction_succeeded) : (!divisor && !dividend_zeros);
    a.s.wr<1>(1027, carry);
    a.pair(3, remainder_result ? remainder : quotient);
    return true;
}
}
std::uint64_t unsigned_quotient_instruction_count(const State& state) {
    const auto numerator_high = std::uint32_t(state.r<4>(12)), denominator_high = std::uint32_t(state.r<4>(20));
    const auto numerator = (std::uint64_t(numerator_high) << 32) | state.r<4>(16);
    const auto denominator = (std::uint64_t(denominator_high) << 32) | state.r<4>(24);
    const auto numerator_zeros = unsigned(std::countl_zero(numerator));
    const auto denominator_zeros = unsigned(std::countl_zero(denominator));
    const auto high_word_branches = unsigned(!numerator_high) + unsigned(!denominator_high);
    if (numerator_zeros > denominator_zeros) return 14 + high_word_branches;
    const auto iterations = denominator_zeros - numerator_zeros + 1;
    // The 65th zero-divisor subtraction succeeds even though its quotient bit
    // no longer fits the returned pair. Do not count only popcount(UINT64_MAX).
    const auto successful_subtractions = denominator ? unsigned(std::popcount(numerator / denominator)) : iterations;
    return (iterations == 32 ? 33u : 35u) + high_word_branches + 8u * iterations + 3u * successful_subtractions;
}
void unsigned_quotient(State& state) { IntegerAbi a(state); divide_magnitudes(a, false); a.finish(); }
void unsigned_remainder(State& state) { IntegerAbi a(state); divide_magnitudes(a, true); a.finish(); }
void signed_quotient(State& state) {
    IntegerAbi a(state);
    a.put(9, a.r(3) & 0x80000000); a.record(a.r(9));
    if (a.r(9)) a.negate_pair(3);
    a.store(a.r(1) + 24, a.r(9), 0x10773970);
    a.put(9, a.r(5) & 0x80000000); a.record(a.r(9));
    if (a.r(9)) a.negate_pair(5);
    a.store(a.r(1) + 28, a.r(9), 0x10773984);
    if (divide_magnitudes(a, false)) {
        a.load(9, a.r(1) + 24, 0x10773a64); a.load(10, a.r(1) + 28, 0x10773a68);
        a.put(7, a.r(9) ^ a.r(10)); a.record(a.r(7));
        if (a.r(7)) { a.compare(std::bit_cast<std::int32_t>(a.r(9))); a.negate_pair(3); }
    }
    a.finish();
}
void signed_remainder(State& state) {
    IntegerAbi a(state);
    a.compare_field(7, a.r(3)); if (state.r<1>(2311) & 8) a.negate_pair(3);
    a.compare(std::bit_cast<std::int32_t>(a.r(5))); if (state.r<1>(2304) & 8) a.negate_pair(5);
    divide_magnitudes(a, true);
    if (state.r<1>(2311) & 8) a.negate_pair(3);
    a.finish();
}
}
