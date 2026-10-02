#include "aot_runtime.hpp"
#include "match_turbo.hpp"
#include <iostream>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000, random_bytes = fixture + 0x100;
constexpr std::uint32_t start = 0x102474c0, division = 0x1024773c;
constexpr std::uint32_t remainder_end = 0x10247768, quotient_dead = 0x10247948;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void check_sparse_profile_schedule() {
    constexpr std::uint64_t stride = 1048583, start = 100;
    require(match_pc_sample_threshold(start) == start + stride,
            "Sparse profile did not initialize one stride ahead");
    require(match_pc_sample_threshold(start + stride - 1) == start + stride,
            "Sparse profile postponed a still-future sample on page entry");
    require(record_match_pc_sample(0x10773870) == start + 2 * stride,
            "Sparse profile did not advance after a selected-page sample");
    const auto reentry = start + 10 * stride;
    require(match_pc_sample_threshold(reentry) == reentry + stride &&
            match_pc_sample_threshold(reentry + 1) == reentry + stride,
            "Cold-page work caused a sparse-profile catch-up burst");
    require(match_pc_sample_threshold(reentry + stride) == reentry + 2 * stride,
            "Exactly due reentry did not reset the missed sample");
    require(match_pc_sample_threshold(UINT64_MAX - stride) == UINT64_MAX,
            "Sparse profile overflowed at its exact final stride");
    require(record_match_pc_sample(0x10773874) == UINT64_MAX &&
            match_pc_sample_threshold(UINT64_MAX) == UINT64_MAX &&
            record_match_pc_sample(0x10773878) == UINT64_MAX,
            "Sparse profile wrapped its saturated threshold");
}
std::unique_ptr<State> setup(const char* directory, double factor, double denominator,
                             std::uint32_t sample, bool second_draw = false, unsigned event = 11) {
    auto state = std::make_unique<State>();
    auto& s = *state;
    load_program(s, directory);
    s.memory.map(fixture, 0x1000);
    s.memory.write<4>(fixture, random_bytes);
    s.memory.write<4>(fixture + 4, sample / 2);
    s.memory.write<4>(fixture + 8, random_bytes + 0x80);
    s.memory.write<2>(random_bytes + 2, sample - sample / 2);
    s.memory.write<2>(random_bytes + 4, sample - sample / 2);
    s.wr<4>(17 * 4, 0x107e5544); // Supplied double and float constant tables.
    s.wr<4>(18 * 4, fixture + 4);
    s.wr<4>(19 * 4, random_bytes);
    s.wr<4>(20 * 4, fixture + 8);
    s.wr<4>(21 * 4, fixture);
    s.wr<4>(22 * 4, 0x107e53d8);
    s.wr<4>(24 * 4, event);
    s.wr<4>(25 * 4, second_draw ? 0 : fixture);
    s.wr<8>(16384 + 30 * 16, float_bits<8>(denominator));
    s.wr<8>(16384 + 31 * 16, float_bits<8>(factor));
    s.pc = start;
    return state;
}
void until(State& s, std::uint32_t target) {
    for (unsigned i = 0; s.pc != target && i < 512; ++i) {
        s.budget = s.executed + 1;
        try { run(s); }
        catch (const std::runtime_error& error) {
            if (std::string(error.what()) != "Native instruction budget exhausted") throw;
        }
    }
    require(s.pc == target, "Original match arithmetic missed the expected continuation");
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-match-arithmetic-checks <original-segments>\n"; return 2; }
    try {
        check_sparse_profile_schedule();
        // The match hot pages select a specialized path only for the native
        // app's unlimited instruction budget. Even that path must report the
        // precise original PC and count if the 64-bit counter is exhausted.
        set_match_turbo_enabled(true);
        for (const auto address : {0x1071a000u, 0x1073c000u, 0x10773000u}) {
            auto state = std::make_unique<State>();
            state->pc = address;
            state->budget = UINT64_MAX;
            state->executed = UINT64_MAX;
            bool exhausted = false;
            try { dispatch_page(*state); }
            catch (const std::runtime_error& error) {
                exhausted = std::string(error.what()) == "Native instruction budget exhausted";
            }
            require(exhausted && state->pc == address && state->executed == 0,
                    "Unlimited match accounting lost its exact failure state");
            state->pc = address;
            state->budget = 0;
            state->executed = 0;
            exhausted = false;
            try { dispatch_page(*state); }
            catch (const std::runtime_error& error) {
                exhausted = std::string(error.what()) == "Native instruction budget exhausted";
            }
            require(exhausted && state->pc == address && state->executed == 1,
                    "Finite match accounting lost its exact failure state");
        }
        set_match_turbo_enabled(false);
        unsigned cases = 0;
        for (double factor : {-2.0, -1.0, -0.75, -0.5, -0.1, 0.0, 0.1, 0.5, 0.75, 1.0, 2.0, 5.0}) {
            for (double denominator : {1.0, 8.0, 100.0}) {
                for (std::uint32_t sample : {0u, 1u, 29654u, 65535u, 131070u}) {
                    auto s = setup(argv[1], factor, denominator, sample);
                    until(*s, remainder_end);
                    float bound = 4.0f;
                    for (int i = 0; i < 4; ++i) bound = float(double(bound) * factor);
                    bound = float(double(bound) / denominator);
                    const auto divisor = std::int32_t(bound);
                    const auto remainder = divisor ? std::int32_t(sample) % divisor : std::int32_t(sample);
                    const auto expected = bound == 0 ? 0 : divisor > 0 ? remainder : -remainder;
                    require(s->r<4>(13 * 4) == std::uint32_t(expected), "Original match signed remainder changed");
                    require(s->memory.read<4>(fixture) == random_bytes + (bound == 0 ? 0 : 2) &&
                            s->memory.read<4>(fixture + 4) == sample / 2,
                            "Original match random draw or seed changed");
                    require(s->r<4>(4) == 0x700fff00 && s->r<4>(8) == 0x108bba90,
                            "Original match stack or TOC changed");
                    ++cases;
                }
            }
        }
        // Follow every branch after the annihilating multiply until r3 is
        // overwritten: optional second draw, zero short remainder, and all
        // three paths through the later event adjustment. Compare the complete
        // register file and touched memory for different undefined quotients.
        for (double factor : {0.5, 2.0}) {
          const double denominator = factor == 0.5 ? 1 : 128;
          for (std::uint32_t sample : {0u, 1u, 29654u, 65536u, 131070u}) {
            for (bool second_draw : {false, true}) {
                for (unsigned event : {11u, 61u, 92u}) {
                    for (auto quotient : {0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu, 0x12345678u}) {
                        auto reference = setup(argv[1], factor, denominator, sample, second_draw, event);
                        until(*reference, division);
                        require(reference->r<4>(16) == 0, "Fixture must reach the observed zero divisor");
                        reference->wr<4>(12, quotient);
                        reference->pc = division + 4;
                        until(*reference, quotient_dead);
                        auto actual = setup(argv[1], factor, denominator, sample, second_draw, event);
                        until(*actual, quotient_dead);
                        require(reference->registers == actual->registers &&
                                reference->memory.copy_out(0x700fff00, 0x100) == actual->memory.copy_out(0x700fff00, 0x100) &&
                                reference->memory.copy_out(fixture, 0x1000) == actual->memory.copy_out(fixture, 0x1000),
                                "Undefined match quotient escaped into registers, stack, or random state");
                        ++cases;
                    }
                }
            }
          }
        }
        std::cout << cases << " original match arithmetic and branch-sensitive quotient-independence checks passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
