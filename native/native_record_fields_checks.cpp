#include "semantic_io_checks.hpp"
#include "match_speed.hpp"
#include "match_turbo.hpp"
#include "native_record_fields.hpp"
#include "native_drawing.hpp"
#include <random>

namespace {
using namespace cm::semantic_io_checks;
constexpr std::uint32_t base = 0x30000000;
constexpr std::array entries{0x100c5ab0u, 0x100c6150u, 0x100c63c0u, 0x100c65d0u};

std::string execute(State& state, std::uint32_t entry, bool promoted) {
    state.budget = promoted ? UINT64_MAX : UINT64_MAX - 1;
    state.pc = entry;
    try { run_until(state, return_sentinel); }
    catch (const std::exception& error) { return error.what(); }
    return {};
}

void arguments(State& state) {
    state.wr<4>(4, base + 0xf000);
    state.wr<4>(12, base + 0x1000);
    state.wr<4>(16, base + 0x2000);
    state.wr<4>(20, base + 0x3000);
    state.wr<4>(24, base + 0x4000);
    state.wr<4>(28, base + 0x5000);
    state.wr<4>(4128, return_sentinel);
}

void equivalence() {
    std::mt19937 random(5931);
    unsigned comparisons = 0, faults = 0;
    for (const auto entry : entries) {
        for (unsigned seed = 0; seed < 40; ++seed) {
            for (unsigned kind = 0; kind < 12; ++kind) {
                auto initial = std::make_unique<State>();
                initial->memory.map(base, 0x10000);
                std::vector<std::uint8_t> bytes(0x10000);
                for (auto& byte : bytes) byte = random();
                initial->memory.copy_in(base, bytes);
                for (auto& byte : initial->registers) byte = random();
                for (auto& byte : initial->unique) byte = random();
                arguments(*initial);
                initial->wr<4>(24, kind & 1 ? base + 0x4000 : 0);
                initial->wr<4>(28, kind & 2 ? base + 0x5000 : 0);
                if (kind == 4) initial->wr<4>(20, base + 0x1000);
                if (kind == 5) initial->wr<4>(16, base + 0x1001);
                if (kind == 6) initial->wr<4>(20, base + 0x2002);
                if (kind == 7) initial->wr<4>(24, base + 0x3002);
                if (kind == 8) initial->wr<4>(12, 0);
                if (kind == 9) initial->wr<4>(20, 0x80000000);
                if (kind == 10) initial->wr<4>(4, base + 2);
                if (kind == 11) initial->wr<4>(20, base + 0xff80);
                auto original = clone(*initial), promoted = clone(*initial);
                const auto expected = execute(*original, entry, false);
                const auto actual = execute(*promoted, entry, true);
                const auto label = "Record-field leaf " + hex_address(entry) + " case=" + std::to_string(kind);
                require(expected == actual, label + " fault differs: " + expected + " / " + actual);
                Image(*original).compare(*promoted, label);
                require(original->executed == promoted->executed, label + " instruction count differs");
                faults += !expected.empty();
                ++comparisons;
            }
        }
    }
    std::cout << "PASS comparisons=" << comparisons << " exact-faults=" << faults << std::endl;
}

void fallback_and_drawing() {
    auto state = std::make_unique<State>();
    state->memory.map(base, 0x10000);
    state->budget = UINT64_MAX;
    arguments(*state);
    const auto declines = [&](std::uint32_t entry, const std::string& label) {
        const Image before(*state);
        const auto count = state->executed;
        require(!try_native_record_fields(*state, entry), label + " was specialized");
        before.compare(*state, label + " fallback changed architectural state");
        require(state->executed == count, label + " fallback changed instruction count");
    };
    declines(0x100c5ab4, "Unknown entry");
    for (const std::uint64_t budget : {UINT64_C(42), UINT64_MAX - 1}) {
        state->budget = budget;
        declines(entries[0], "Finite instruction budget");
    }
    state->budget = UINT64_MAX;
    state->executed = UINT64_MAX - 100;
    declines(entries[0], "Near-overflow instruction accounting");
    state->executed = 0;
    Effects effects;
    effects.state = state.get();
    state->memory.observer = &effects;
    declines(entries[0], "General memory observer");
    state->memory.observer = nullptr;
    state->memory.date_observer = &effects;
    declines(entries[0], "Read observer");
    state->memory.date_observer = nullptr;
    std::vector<std::pair<std::uint32_t, unsigned>> reads;
    state->memory.read_trace = &reads;
    declines(entries[0], "Read tracing");
    state->memory.read_trace = nullptr;

    // Use real drawing surfaces to verify the one eligible passive observer.
    auto original = clone(*state), promoted = clone(*state);
    NativeDrawingObserver original_drawing, promoted_drawing;
    original_drawing.register_surface(*original, base + 0x3000, 8, 8, 16, 16);
    promoted_drawing.register_surface(*promoted, base + 0x3000, 8, 8, 16, 16);
    require(execute(*original, entries[0], false).empty(), "Original drawing case faulted");
    require(try_native_record_fields(*promoted, entries[0]), "Native drawing was not eligible");
    Image(*original).compare(*promoted, "Promoted drawing writes");
    require(original->executed == promoted->executed, "Drawing instruction count differs");
    require(original_drawing.snapshot(*original, base + 0x3000)->background ==
            promoted_drawing.snapshot(*promoted, base + 0x3000)->background, "Native surface pixels differ");
    original->memory.observer = promoted->memory.observer = nullptr;
}
}

int main(int argc, char** argv) {
    try {
        initialize_native_match_speed();
        if (argc == 2 && std::string(argv[1]) == "--profile") {
            require(match_pc_profile_enabled(), "Original-PC profiling must be enabled");
            State state;
            state.memory.map(base, 0x10000);
            state.budget = UINT64_MAX;
            arguments(state);
            const Image before(state);
            for (const auto entry : entries) {
                require(!try_native_record_fields(state, entry), "Profiling must retain original checkpoints");
                before.compare(state, "Original-PC profile fallback");
                require(state.executed == 0, "Profile fallback changed instruction count");
            }
            std::cout << "PASS: all four profiled entries retain the original path\n";
            return 0;
        }
        require(argc == 1 && !match_pc_profile_enabled(), "Unexpected record-field check policy");
        equivalence();
        fallback_and_drawing();
        std::cout << "PASS: complete state and fault prefixes, instruction counts, entry/budget/overflow/observer fallbacks and native drawing updates\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
