#include "aot_runtime.hpp"
#include <iostream>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000;
constexpr std::uint32_t random_bytes = fixture + 0x100;
constexpr std::uint32_t start = 0x1013541c, division = 0x101355d4, end = 0x10135614;
constexpr std::size_t f30 = 16384 + 30 * 16;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::unique_ptr<State> setup(const char* directory, double bound, std::uint32_t sample) {
    auto state = std::make_unique<State>();
    auto& s = *state;
    load_program(s, directory);
    s.memory.map(fixture, 0x1000);
    s.memory.write<4>(fixture, random_bytes);
    s.memory.write<4>(fixture + 4, sample / 2);
    s.memory.write<4>(fixture + 8, random_bytes + 0x80);
    s.memory.write<2>(random_bytes + 2, sample - sample / 2);
    s.wr<4>(24 * 4, 0x107a75d8); // Original floating constant tables.
    s.wr<4>(25 * 4, 0x107a7644);
    s.wr<4>(26 * 4, fixture + 4);
    s.wr<4>(27 * 4, random_bytes);
    s.wr<4>(28 * 4, fixture + 8);
    s.wr<4>(29 * 4, fixture);
    s.wr<8>(f30, float_bits<8>(bound));
    s.pc = start;
    return state;
}
void until(State& s, std::uint32_t target) {
    for (unsigned i = 0; s.pc != target && i < 256; ++i) {
        // Generated pages execute several instructions per dispatch. Budget
        // boundaries let this fixture stop before the next game subsystem.
        s.budget = s.executed + 1;
        try { run(s); }
        catch (const std::runtime_error& error) {
            if (std::string(error.what()) != "Native instruction budget exhausted") throw;
        }
    }
    require(s.pc == target, "Day advance did not reach the expected continuation");
}

void check_optional_competition_stats(const char* directory) {
    constexpr std::uint32_t lookup = 0x10122b68, no_form = 0x10122cfc;
    constexpr std::uint32_t stats = fixture + 0x200, form = fixture + 0x800;
    const auto exercise = [&](std::uint32_t object) {
        auto s = setup(directory, 0, 0);
        s->memory.write<4>(stats + 0x5cc, form);
        s->memory.write<1>(form + 4, 10);  // The recorded form takes the existing default branch.
        s->wr<4>(12, object);
        s->wr<4>(13 * 4, 10);  // Default already assigned before the optional lookup.
        s->pc = lookup;
        const auto rng = s->memory.copy_out(fixture, 8);
        until(*s, no_form);
        require(s->r<4>(12) == (object ? form : 0), "Daily competition stats returned the wrong form");
        require(s->r<4>(13 * 4) == 10 && s->r<4>(4) == 0x700fff00,
                "Daily competition stats changed the default or stack");
        require(s->memory.copy_out(fixture, 8) == rng, "Daily competition stats consumed RNG");
    };
    exercise(0);
    exercise(stats);
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-day-advance-checks <original-segments>\n"; return 2; }
    try {
        unsigned cases = 0;
        for (double bound : {-3.0, -2.99, -2.0, -1.1, -1.0, -0.9, -0.1, -1e-30,
                             -0.0, 0.0, 1e-30, 0.1, 0.5, 0.999, 1.0, 1.75, 2.0, 100.0}) {
            for (std::uint32_t sample : {0u, 1u, 42772u, 65535u, 131070u}) {
                auto s = setup(argv[1], bound, sample);
                until(*s, end);
                const bool consumes_random = bound > -3 && bound != 0;
                double expected = bound;
                if (bound > -3) {
                    const auto divisor = std::int32_t(bound);
                    const auto remainder = divisor ? std::int32_t(sample) % divisor : std::int32_t(sample);
                    expected = bound == 0 ? 0 : divisor > 0 ? remainder : -remainder;
                }
                require(float_value<8>(s->r<8>(f30)) == expected, "Original day-advance signed remainder or fraction handling changed");
                require(s->memory.read<4>(fixture) == random_bytes + (consumes_random ? 2 : 0) &&
                        s->memory.read<4>(fixture + 4) == sample / 2,
                        "Day-advance arithmetic changed random-number consumption or the seed");
                require(s->r<4>(4) == 0x700fff00 && s->r<4>(8) == 0x108bba90,
                        "Day-advance arithmetic changed the original stack or TOC");
                ++cases;
            }
        }
        // divw (OE=0, Rc=0) does not trap or set status bits on PowerPC.
        // At this site its undefined zero-divisor quotient is multiplied by
        // zero, then overwritten. Different possible quotient bit patterns
        // must produce identical architectural registers and stack data.
        for (const auto quotient : {0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu, 0x12345678u}) {
            auto reference = setup(argv[1], 0.5, 42772);
            until(*reference, division);
            reference->wr<4>(12, quotient);
            reference->pc = division + 4;
            until(*reference, end);
            auto actual = setup(argv[1], 0.5, 42772);
            until(*actual, end);
            require(reference->registers == actual->registers &&
                    reference->memory.copy_out(0x700fff00, 0x100) == actual->memory.copy_out(0x700fff00, 0x100),
                    "Undefined quotient escaped the audited multiply-by-zero and overwrite sequence");
            ++cases;
        }
        bool rejected = false;
        try { (void)divide_signed<4>(42772, 0); }
        catch (const std::runtime_error& error) { rejected = std::string(error.what()) == "Signed division by zero in translated code"; }
        require(rejected, "Unaudited divisions by zero must still stop for investigation");
        check_optional_competition_stats(argv[1]);
        std::cout << cases << " original day-advance arithmetic, quotient-independence, and RNG checks passed\n"
                  << "Daily competition stats: missing object and present form passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
