// Behavioral checks for the intentionally corrected MATCH_MAN substitution policy.
// Use real role accessors, the production callback routes and the game's qsort.
#include "match_man_adapter.hpp"
#include "match_man_callbacks.hpp"
#include "match_man_checks.hpp"
#include "match_replay.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>

namespace {
using namespace cm::aot;
using guest::Cpu;
constexpr std::uint32_t fixture = 0x60000000, scaled = 0x1020ea00, plain = 0x1020ee00;
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
int sign(std::int32_t value) { return (value > 0) - (value < 0); }

struct StackWrites final : MemoryObserver {
    std::uint32_t sp;
    explicit StackWrites(std::uint32_t stack) : sp(stack) {}
    void mapped(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Comparator allocated guest memory"); }
    void written(std::uint32_t at, const std::uint8_t*, std::size_t size) override {
        require(at >= sp - 0x400 && std::uint64_t(at) + size <= sp + 12, "Comparator changed records or RNG outside its stack");
    }
    void cleared(std::uint32_t at, std::uint64_t size) override { written(at, nullptr, size); }
};

struct Fixture {
    State& s;
    Cpu c;
    std::uint32_t match, stack;
    std::uint64_t comparisons = 0, permutations = 0;
    Fixture(State& state, std::uint32_t match_address) : s(state), c(state), match(match_address), stack(c.sp()) {
        s.memory.map(fixture, 0x10000);
        c.put32(match + 0x7934, fixture + 0x5000);
        // Distinct caller registers make accidental ABI clobbers visible.
        for (unsigned n = 13; n < 32; ++n) c.set(n, 0x5a000000 + n * 0x10101);
        for (unsigned n = 14; n < 32; ++n) c.set_f(n, 1000.0 + n * 3.25);
    }
    std::uint32_t player(unsigned i) const { return fixture + 0x1000 + i * 0x1c4; }
    void set(unsigned i, unsigned role, int score = 100, int position = 99, int keeper = 20, bool staff = true,
             float form = 0) {
        const auto p = player(i), data = fixture + 0x4000 + i * 0x50;
        c.put32(p + 0x1a4, match); c.put8(p + 0x27, 0);
        c.put8(p + 0x19, position == 99 ? i : position);
        c.put32(p + 0x70, staff ? data : 0); c.put8(data + 0xf, keeper);
        c.put16(p + 0x36, score); c.put32(p + 0x50, std::bit_cast<std::uint32_t>(form));
        c.put16(fixture + 0x5000 + i * 2, role);
    }
    std::int32_t compare(std::uint32_t entry, std::uint32_t a, std::uint32_t b, bool production, bool original = false) {
        c.put32(fixture, a); c.put32(fixture + 4, b);
        c.set(3, fixture); c.set(4, fixture + 4); c.set_lr(return_sentinel);
        s.pc = entry; s.budget = UINT64_MAX;
        const auto saved = s.registers;
        StackWrites writes(stack); s.memory.observer = &writes;
        try {
            if (original) match_man_run_original(s, entry);
            else if (production) run_until(s, return_sentinel);
            else match_man_run_rewrite(s, entry);
        } catch (...) { s.memory.observer = nullptr; throw; }
        s.memory.observer = nullptr;
        require(s.pc == return_sentinel && c.sp() == stack, "Comparator did not return with its caller's stack");
        for (unsigned n = 13; n < 32; ++n)
            for (unsigned byte = 0; byte < 4; ++byte)
                require(s.registers[n * 4 + byte] == saved[n * 4 + byte], "Comparator clobbered a saved GPR");
        for (unsigned n = 14; n < 32; ++n)
            for (unsigned byte = 0; byte < 8; ++byte)
                require(s.registers[guest::reg::fpr(n) + byte] == saved[guest::reg::fpr(n) + byte], "Comparator clobbered a saved FPR");
        ++comparisons;
        return std::int32_t(c.r(3));
    }
    void laws(std::uint32_t entry, bool production) {
        set(0, 4, 100); set(1, 8, 100); set(2, 0x10, 100);
        set(3, 0x20, 100, -2); set(4, 0x20, 120, -2);
        set(5, 1, 100, 99, 19); set(6, 1, 120, 99, 19);
        set(7, 1, 100); set(8, 0, 100); set(9, 0x40, 100);
        set(10, 4, 100, 99, 20, false);
        std::array<std::uint32_t, 12> players{};
        for (unsigned i = 0; i < 11; ++i) players[i] = player(i);
        std::array<std::array<int, 12>, 12> order{};
        for (unsigned i = 0; i < players.size(); ++i)
            for (unsigned j = 0; j < players.size(); ++j)
                order[i][j] = sign(compare(entry, players[i], players[j], production));
        for (unsigned i = 0; i < players.size(); ++i) {
            require(order[i][i] == 0, "Self-comparison must be zero");
            for (unsigned j = 0; j < players.size(); ++j) {
                require(order[i][j] == -order[j][i], "Comparison is not antisymmetric");
                for (unsigned k = 0; k < players.size(); ++k) {
                    if (order[i][j] < 0 && order[j][k] < 0) require(order[i][k] < 0, "Comparison is not transitive");
                    if (order[i][j] == 0) require(order[i][k] == order[j][k], "Tied players have inconsistent ordering");
                }
            }
        }
        require(order[3][5] < 0 && order[5][0] < 0 && order[0][10] < 0 && order[10][11] == 0,
                "Off-pitch, emergency keeper, ordinary and absent priorities changed");
        require(order[0][1] == (entry == scaled ? 1 : 0), "Equal ratings use the wrong positional weights");
    }
    void numerical_edges(std::uint32_t entry, bool production) {
        const std::array forms{0.f, -0.f, .5f, -.5f, 1.e30f, -1.e30f,
                              std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
                              std::numeric_limits<float>::quiet_NaN()};
        for (const auto role_a : {4, 8, 0x10, 0x20}) for (const auto role_b : {4, 8, 0x10, 0x20})
        for (auto a : forms) for (auto b : forms) {
            set(0, role_a, 100, 99, 20, true, a); set(1, role_b, 100, 99, 20, true, b);
            const auto ab = sign(compare(entry, player(0), player(1), production));
            const auto ba = sign(compare(entry, player(1), player(0), production));
            require(ab == -ba, "Converted score extremes reverse the comparison");
        }
        set(0, 0x20, 0, 99, 20, true, -1.e30f); set(1, 0x20, 0, 99, 20, true, 1.e30f);
        require(compare(entry, player(0), player(1), production) < 0, "Score subtraction overflow reverses low and high scores");
    }
    void selection(std::uint32_t entry) {
        // Six distinct scores: the weighted order differs from the raw order.
        set(0, 4, 100); set(1, 8, 101); set(2, 0x10, 102);
        set(3, 0x20, 115); set(4, 0x40, 95); set(5, 2, 80);
        const std::array<unsigned, 6> expected = entry == scaled ? std::array<unsigned, 6>{4, 5, 2, 3, 1, 0}
                                                               : std::array<unsigned, 6>{5, 4, 0, 1, 2, 3};
        std::array<unsigned, 6> order{0, 1, 2, 3, 4, 5};
        do {
            for (unsigned i = 0; i < order.size(); ++i) c.put32(fixture + 0x100 + i * 4, player(order[i]));
            c.set(3, fixture + 0x100); c.set(4, order.size()); c.set(5, 4);
            // Same original transition vectors used by automatic substitutions.
            c.set(6, c.toc(entry == scaled ? -0x63fc : -0x6400));
            require(c.u32(c.r(6)) == entry, "Wrong original substitution callback vector");
            c.set_lr(return_sentinel); s.pc = 0x10768300; s.budget = UINT64_MAX;
            run_until(s, return_sentinel);
            require(c.sp() == stack, "Guest sort lost the caller's stack");
            for (unsigned i = 0; i < expected.size(); ++i)
                if (c.u32(fixture + 0x100 + i * 4) != player(expected[i])) {
                    std::cerr << "sort " << std::hex << entry << std::dec << " permutation " << permutations << " position " << i
                              << " expected " << expected[i] << " actual " << (c.u32(fixture + 0x100 + i * 4) - player(0)) / 0x1c4 << '\n';
                    throw std::runtime_error("Substitution order depends on input permutation");
                }
            ++permutations;
        } while (std::next_permutation(order.begin(), order.end()));
    }
    void original_controls() {
        set(0, 8, 100); set(1, 8, 100);
        require(compare(scaled, player(0), player(0), true, true) == -5, "Original self-comparison control no longer exposes the defect");
        require(compare(scaled, player(0), player(1), true) == 0, "Role-8 equal players must tie at weight 1.20");
        set(1, 0x20, 122);
        require(compare(scaled, player(0), player(1), true) < 0 && compare(scaled, player(1), player(0), true) > 0,
                "Role-8 weight must be 1.20, not 1.25, on either side");
        set(0, 0x20, 110); set(1, 4, 100);
        require(compare(scaled, player(0), player(1), true, true) > 0, "Original wrong-role control no longer exposes the defect");
        require(compare(scaled, player(0), player(1), true) < 0, "Each player must use its own positional weight");
        for (const auto entry : {scaled, plain}) {
            set(0, 4, 100, -2); set(1, 4, 100, -2);
            require(compare(entry, player(0), player(1), true, true) == -1, "Original priority-tie control did not fail");
            require(compare(entry, player(0), player(1), true) == 0, "Same special priority must tie");
        }
    }
};
}

int main(int argc, char** argv) try {
    require(argc == 2, "Expected an existing match capture directory");
    require(!match_man::original_substitution_order && !match_man_declining, "Behavioral checks must use the corrected production policy");
    const std::filesystem::path capture(argv[1]);
    auto s = std::make_unique<State>(); load_match_snapshot(*s, capture / "initial.snapshot");
    std::uint32_t match = 0; std::ifstream(capture / "match.txt") >> match;
    require(match != 0, "Missing captured match identity");
    Fixture f(*s, match);
    f.original_controls();
    for (const auto entry : {scaled, plain}) {
        for (bool production : {false, true}) { f.laws(entry, production); f.numerical_edges(entry, production); }
        f.selection(entry);
    }
    std::cout << "Corrected substitution order: " << f.comparisons << " comparisons, " << f.permutations
              << " guest-sort permutations; ordering laws, selection, ABI, no record/RNG writes and original defect controls passed\n";
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
