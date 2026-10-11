#include "compiler_arithmetic_fixtures.hpp"
#include <bit>
#include <cmath>
#include <limits>
#include <random>
#include <set>

namespace {
using namespace cm::compiler_arithmetic_checks;
void integer_cases() {
    std::vector<std::uint64_t> values{0, 1, 2, 3, 7, 15, UINT32_MAX, UINT64_MAX, UINT64_C(0x8000000000000000)};
    for (unsigned bit : {4u, 16u, 31u, 32u, 33u, 48u, 62u, 63u})
        for (auto delta : {-1, 0, 1}) values.push_back((UINT64_C(1) << bit) + delta);
    std::mt19937_64 random(0x23c0102);
    for (unsigned index = 1; index <= 4; ++index) {
        auto s = fixture_state(index);
        const auto test = [&](std::uint64_t n, std::uint64_t d, const std::string& label) {
            pair(*s, n); pair(*s, d, 5);
            const bool signed_result = index == 2 || index == 4, remainder = index >= 3;
            const auto numerator = signed_result ? magnitude(n) : n, denominator = signed_result ? magnitude(d) : d;
            std::uint64_t result;
            if (denominator) {
                result = remainder ? numerator % denominator : numerator / denominator;
            } else {
                // Characterized original zero-divisor behavior, including the
                // 65-iteration top-bit case. No host division by zero occurs.
                const auto width = 65 - unsigned(std::countl_zero(numerator));
                result = remainder ? (numerator >> 63 ? 0 : numerator) :
                    width >= 64 ? UINT64_MAX : (UINT64_C(1) << width) - 1;
            }
            if (signed_result && ((n >> 63) ^ (!remainder && (d >> 63)))) result = 0 - result;
            const Expectation expected = [result](const State& state) { require(pair(state) == result, "Independent quotient/remainder result"); };
            check(index, *s, label, expected);
        };
        for (auto n : values) for (auto d : values) test(n, d, std::to_string(n) + "/" + std::to_string(d));
        for (unsigned n = 0; n < 1024; ++n) {
            s->wr<1>(1024, n & 1); s->wr<1>(1027, (n >> 1) & 1);
            s->wr<1>(2304, (random() & 0xf0) | 7);
            const auto x = random(); auto y = (random() >> (n % 64)) | 1;
            if ((index == 2 || index == 4) && (n & 1)) y = 0 - y;
            test(x, y, "random-" + std::to_string(n));
        }
        for (const auto [input, unsigned_q, signed_q, unsigned_r, signed_r] : {
            std::array<std::uint64_t, 5>{0, 1, 1, 0, 0},
            std::array<std::uint64_t, 5>{1, 3, 3, 1, 1},
            std::array<std::uint64_t, 5>{UINT64_C(0x8000000000000000), UINT64_MAX, 1, 0, 0},
            std::array<std::uint64_t, 5>{UINT64_MAX, UINT64_MAX, UINT64_C(0xfffffffffffffffd), 0, UINT64_MAX}}) {
            pair(*s, input); pair(*s, 0, 5);
            const auto expected = index == 1 ? unsigned_q : index == 2 ? signed_q : index == 3 ? unsigned_r : signed_r;
            check(index, *s, "literal-zero-divisor-" + std::to_string(input), [expected](const State& state) {
                require(pair(state) == expected, "Independent literal zero-divisor result");
            });
        }
        pair(*s, UINT64_C(0x8000000000000000)); pair(*s, UINT64_MAX, 5);
        check(index, *s, "independent-negative", {}, {}, false, true);
        s->wr<4>(4, stack + 1); fault_matrix(index, *s, "unaligned-stack");
        pair(*s, 0); fault_matrix(index, *s, "zero-pair");
        if (index == 2) {
            pair(*s, 98765); pair(*s, 123, 5);
            check(index, *s, "live-second-sign", {}, [](Monitor& m) {
                m.effects.on_read = [](State& state, Effects& e, std::uint32_t address, unsigned) {
                    if (!e.mutated && address == state.r<4>(4) + 24) {
                        e.mutated = true; state.memory.write<4>(std::uint32_t(state.r<4>(4)) + 28, 0x80000000);
                    }
                };
            });
        }
    }
}
void floating_output_cases() {
    std::vector<std::uint64_t> values{0, 1, UINT64_MAX, UINT64_C(0x8000000000000000), UINT64_C(0x7fffffffffffffff)};
    for (unsigned bit : {1u, 23u, 24u, 25u, 31u, 32u, 52u, 53u, 54u, 62u, 63u})
        for (std::uint64_t delta : {UINT64_MAX, UINT64_C(0), UINT64_C(1), UINT64_C(511), UINT64_C(1023), UINT64_C(1024), UINT64_C(1025)}) {
            const auto value = (UINT64_C(1) << bit) + delta; values.push_back(value); values.push_back(0 - value);
        }
    std::mt19937_64 random(0x23f0102); for (unsigned n = 0; n < 1024; ++n) values.push_back(random());
    for (unsigned index : {5u, 6u}) {
        auto s = fixture_state(index);
        for (auto value : values) {
            pair(*s, value);
            const auto d = double(std::bit_cast<std::int64_t>(value));
            const auto expected = std::bit_cast<std::uint64_t>(index == 6 ? double(float(d)) : d);
            check(index, *s, std::to_string(value), [expected](const State& state) {
                require(state.r<8>(0x4010) == expected, "Independent signed-to-floating result");
            });
        }
        check(index, *s, "independent-negative", {}, {}, false, true);
        s->wr<4>(4, stack + 1); fault_matrix(index, *s, "unaligned-stack");
        pair(*s, 0); fault_matrix(index, *s, "zero-floating-pair");
        pair(*s, UINT64_C(0x1000001));
        check(index, *s, "live-assembled-double", {}, [](Monitor& m) {
            m.on_write = [&m](State& state, std::uint32_t address, unsigned) {
                if (!m.effects.mutated && address == state.r<4>(4) + 28) {
                    m.effects.mutated = true; state.memory.write<4>(address, 0x12345678);
                }
            };
        });
    }
}
void floating_input_cases() {
    std::vector<double> values{0.0, -0.0, 0.25, -0.75, 1.0, -1.0, 12345.75, -12345.75,
        std::numeric_limits<double>::denorm_min(), -std::numeric_limits<double>::denorm_min(),
        std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN(), -std::numeric_limits<double>::quiet_NaN()};
    for (double v : {2147483648.0, 4294967296.0, 9007199254740992.0, 9223372036854775808.0})
        for (double x : {std::nextafter(v, 0.0), v, std::nextafter(v, INFINITY)}) { values.push_back(x); values.push_back(-x); }
    std::mt19937_64 random(0x23d0102); for (unsigned n = 0; n < 1024; ++n) values.push_back(std::bit_cast<double>(random()));
    for (unsigned index : {0u, 7u}) {
        auto s = fixture_state(index);
        for (double value : values) {
            floating(*s, value);
            std::uint64_t expected;
            if (index == 0) expected = value < 0 ? 0 : !(value < 4294967296.0) ? UINT32_MAX : std::uint32_t(value);
            else if (!(std::abs(value) < 9223372036854775808.0)) expected = std::signbit(value) ? UINT64_C(0x8000000000000000) : UINT64_C(0x7fffffffffffffff);
            else expected = std::bit_cast<std::uint64_t>(std::int64_t(value));
            check(index, *s, std::to_string(std::bit_cast<std::uint64_t>(value)), [index, expected](const State& state) {
                require((index == 0 ? state.r<4>(12) : pair(state)) == expected, "Independent floating-to-integer result");
            });
        }
        floating(*s, 2147483648.75); check(index, *s, "independent-negative", {}, {}, false, true);
        s->wr<4>(4, stack + 1); fault_matrix(index, *s, "unaligned-stack");
        if (index == 0) {
            check(index, *s, "live-thresholds", {}, [](Monitor& m) {
                m.effects.on_read = [](State& state, Effects& e, std::uint32_t address, unsigned) {
                    if (!e.mutated && address == thresholds) {
                        e.mutated = true; state.memory.write<8>(thresholds + 16, std::bit_cast<std::uint64_t>(2147483647.0));
                    }
                };
            });
            floating(*s, 12345.75); fault_matrix(index, *s, "below-high-threshold");
            s->wr<1>(1308, 1);
            check(index, *s, "enabled-inexact", {}, {}, false, false,
                "Original enabled floating inexact exception requires a handler");
            s->wr<1>(1308, 0); s->wr<1>(1304, 1);
            // Relocated live thresholds can route an out-of-word value to the
            // existing fctiwz invalid-conversion policy.
            s->memory.write<8>(thresholds + 8, std::bit_cast<std::uint64_t>(1.0e20));
            s->memory.write<8>(thresholds + 16, std::bit_cast<std::uint64_t>(1.0e20));
            floating(*s, 4294967296.0);
            check(index, *s, "enabled-invalid", {}, {}, false, false,
                "Original enabled floating invalid-conversion exception requires a handler");
        }
    }
}
void status_and_alias_cases() {
    std::mt19937_64 random(0x23a0102);
    for (unsigned index : {0u, 5u, 6u, 7u}) {
        for (unsigned n = 0; n < 32; ++n) {
            auto s = fixture_state(index); s->wr<1>(1024, n & 1); s->wr<1>(1027, (n >> 1) & 1);
            for (unsigned byte = 1280; byte <= 1311; ++byte) s->wr<1>(byte, random() & 0xff);
            for (unsigned byte = 1304; byte <= 1308; ++byte) s->wr<1>(byte, 0);
            pair(*s, random()); floating(*s, n & 1 ? 2147483648.75 : 0.25);
            check(index, *s, "bytewise-fpscr-" + std::to_string(n));
        }
    }
    for (std::uint32_t offset : {0u, 8u, 16u}) {
        auto s = fixture_state(0);
        // The conversion scratch aliases one of the threshold words. All
        // threshold reads precede the scratch write in the original.
        s->wr<4>(4, thresholds + offset + 8); floating(*s, 2147483648.75);
        check(0, *s, "stack-threshold-alias-" + std::to_string(offset));
        fault_matrix(0, *s, "alias-fault-" + std::to_string(offset));
    }
    auto s = fixture_state(7); floating(*s, 12345.0);
    check(7, *s, "live-double-low-word", {}, [](Monitor& m) {
        m.effects.on_read = [](State& state, Effects& e, std::uint32_t address, unsigned) {
            if (!e.mutated && address == state.r<4>(4) + 24) {
                e.mutated = true; state.memory.write<4>(std::uint32_t(state.r<4>(4)) + 28, 0x12345678);
            }
        };
    });
}
}
int main() {
    try {
        integer_cases(); floating_output_cases(); floating_input_cases(); status_and_alias_cases();
        for (unsigned i = 0; i < entries.size(); ++i) {
            require(comparisons[i] > 0 && negatives[i] == 1, "Missing individual component evidence");
            std::cout << entries[i].name << ": " << comparisons[i] << " comparisons, " << faults[i] << " fault prefixes, " << traps[i] << " enabled traps, " << negatives[i] << " rejected output negative\n";
        }
        std::cout << "Raw component equivalence; production dispatch, budget and profiling evidence comes from the integration harness.\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    return 0;
}
