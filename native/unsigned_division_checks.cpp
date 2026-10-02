#include "unsigned_division.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>

using namespace cm::aot;
namespace {
constexpr std::uint32_t entry = 0x10773870, continuation = 0x13579bdc;
constexpr std::uint32_t memory_base = 0x30000003;
std::uint64_t checks = 0;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void prepare(State& state, std::uint64_t dividend, std::uint64_t divisor,
             std::uint64_t executed = 17, std::uint64_t budget = UINT64_MAX) {
    // Nonzero sentinels cover every register, including unused high bytes,
    // floating-point registers, XER, CR fields, LR, CTR, and private ABI slots.
    for (std::size_t i = 0; i < state.registers.size(); ++i)
        state.registers[i] = std::uint8_t((i * 37 + dividend + divisor) ^ (i >> 4));
    state.unique.fill(0xa5);
    state.wr<4>(12, dividend >> 32);
    state.wr<4>(16, dividend);
    state.wr<4>(20, divisor >> 32);
    state.wr<4>(24, divisor);
    state.wr<4>(4128, continuation);
    state.pc = entry;
    state.executed = executed;
    state.budget = budget;
    state.native_calls = 23;
    state.allocated = {{0x50000000, 64}};
    state.available = {{0x50000100, 128}};
    state.heap_end = 0x50000300;
    state.memory_error = -108;
}

bool original(State& state) {
    try {
        require(dispatch_page(state), "Original division page was not dispatched");
        require(state.pc == continuation, "Original division missed its LR continuation");
        return false;
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()) != "Native instruction budget exhausted") throw;
        return true;
    }
}

void compare(const State& actual, const State& expected, bool compare_temporaries,
             std::uint64_t dividend, std::uint64_t divisor) {
    std::ostringstream context;
    context << " for " << dividend << " / " << divisor << " budget=" << actual.budget;
    if (actual.registers != expected.registers) {
        const auto mismatch = std::mismatch(actual.registers.begin(), actual.registers.end(),
                                            expected.registers.begin());
        context << " register byte=" << (mismatch.first - actual.registers.begin())
                << " actual=" << unsigned(*mismatch.first)
                << " expected=" << unsigned(*mismatch.second);
        throw std::runtime_error("Architectural register mismatch" + context.str());
    }
    require(actual.pc == expected.pc && actual.executed == expected.executed &&
            actual.budget == expected.budget && actual.native_calls == expected.native_calls &&
            actual.allocated == expected.allocated && actual.available == expected.available &&
            actual.heap_end == expected.heap_end && actual.memory_error == expected.memory_error &&
            actual.platform == expected.platform, "State metadata mismatch" + context.str());
    require(actual.memory.mapped_ranges() == expected.memory.mapped_ranges(),
            "Mapped memory ranges changed" + context.str());
    for (const auto& [address, size] : expected.memory.mapped_ranges())
        require(actual.memory.copy_out(address, size) == expected.memory.copy_out(address, size),
                "Mapped memory contents changed" + context.str());
    if (compare_temporaries)
        require(actual.unique == expected.unique, "Fallback temporary mismatch" + context.str());
    ++checks;
}

std::uint64_t check_complete(State& actual, State& expected, std::uint64_t dividend,
                             std::uint64_t divisor) {
    prepare(actual, dividend, divisor);
    prepare(expected, dividend, divisor);
    require(try_fast_unsigned_division(actual), "Nonzero division was not optimized");
    require(!original(expected), "Original division exhausted an unlimited budget");
    compare(actual, expected, false, dividend, divisor);
    require(((actual.r<4>(12) << 32) | actual.r<4>(16)) == dividend / divisor,
            "Original and optimized result differ from unsigned arithmetic");
    return actual.executed - 17;
}
}

int main() {
    try {
        // Turbo must stay off for the oracle even after the generator gains
        // the specialization. The original page executes its audited P-code.
        set_match_turbo_enabled(false);
        auto actual = std::make_unique<State>();
        auto expected = std::make_unique<State>();
        for (auto* state : {actual.get(), expected.get()}) {
            state->memory.map(memory_base, 257);
            for (unsigned i = 0; i < 257; ++i)
                state->memory.write<1>(memory_base + i, (i * 13) ^ 0xd7);
        }
        std::vector<std::uint64_t> values{0, UINT64_MAX};
        for (unsigned bit = 0; bit < 64; ++bit) {
            const auto power = UINT64_C(1) << bit;
            values.insert(values.end(), {power - 1, power, power + 1});
        }
        std::sort(values.begin(), values.end());
        values.erase(std::unique(values.begin(), values.end()), values.end());
        for (const auto dividend : values)
            for (const auto divisor : values)
                if (divisor) check_complete(*actual, *expected, dividend, divisor);
        std::mt19937_64 random(0x10773870);
        for (unsigned i = 0; i < 4096; ++i) {
            const auto dividend = random();
            const auto divisor = (random() >> (i % 64)) | 1;
            check_complete(*actual, *expected, dividend, divisor);
        }

        // Every stopping instruction for representative early return, both
        // normalization paths, 32/64 iterations, alternating quotient bits,
        // sign boundaries, and zero divisors. Declining must be side-effect free.
        const std::pair<std::uint64_t, std::uint64_t> budget_cases[] = {
            {0, 1}, {1, UINT64_MAX}, {7, 9}, {9, 7}, {UINT64_MAX, 1},
            {UINT64_MAX, UINT64_C(1) << 32}, {UINT64_MAX, UINT64_C(1) << 33},
            {UINT64_MAX, 3}, {UINT64_MAX, UINT64_MAX},
            {UINT64_C(1) << 63, (UINT64_C(1) << 63) + 1},
            {0, 0}, {UINT64_MAX, 0}};
        for (const auto& [dividend, divisor] : budget_cases) {
            const auto count = divisor ? check_complete(*actual, *expected, dividend, divisor) : 800;
            for (std::uint64_t available = 0; available <= count + 1; ++available) {
                prepare(*actual, dividend, divisor, 17, 17 + available);
                prepare(*expected, dividend, divisor, 17, 17 + available);
                const bool optimized = try_fast_unsigned_division(*actual);
                if (!optimized) compare(*actual, *expected, true, dividend, divisor);
                const bool actual_stopped = optimized ? false : original(*actual);
                const bool expected_stopped = original(*expected);
                require(actual_stopped == expected_stopped, "Budget exception behavior changed");
                require(optimized == (divisor && available >= count), "Budget selection changed");
                compare(*actual, *expected, !optimized, dividend, divisor);
            }
            for (const auto executed : {UINT64_MAX, UINT64_MAX - 1, UINT64_MAX - count}) {
                prepare(*actual, dividend, divisor, executed);
                prepare(*expected, dividend, divisor, executed);
                const bool optimized = try_fast_unsigned_division(*actual);
                if (!optimized) compare(*actual, *expected, true, dividend, divisor);
                const bool actual_stopped = optimized ? false : original(*actual);
                require(actual_stopped == original(*expected), "Counter overflow behavior changed");
                compare(*actual, *expected, !optimized, dividend, divisor);
            }
        }
        prepare(*actual, 9, 3, 23, 22);
        prepare(*expected, 9, 3, 23, 22);
        require(!try_fast_unsigned_division(*actual), "Already exceeded budget was optimized");
        compare(*actual, *expected, true, 9, 3);
        for (const auto address : {entry - 4, entry + 4, 0u}) {
            prepare(*actual, 9, 3);
            prepare(*expected, 9, 3);
            actual->pc = expected->pc = address;
            require(!try_fast_unsigned_division(*actual), "Interior or unrelated entry was optimized");
            compare(*actual, *expected, true, 9, 3);
        }
        std::cout << checks << " unsigned division oracle, state, and exact-budget checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
