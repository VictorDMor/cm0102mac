// A bounded detailed-match interval with MATCH_PL original/default routing.
#include "match_pl_adapter.hpp"
#include "actor_construction_adapter.hpp"
#include "match_eng_adapter.hpp"
#include "match_replay.hpp"
#include "match_turbo.hpp"
#include "code_fragment_adapter.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <string_view>
#include <chrono>
#include <iomanip>

using namespace cm::aot;
namespace {
std::uint32_t match_address = 0;
constexpr unsigned advances = 600;
bool timed = false, construction = false, response = false, obstruction = false;
unsigned pair_index = 0;
struct Inputs final : CodeFragmentBoundaryObserver {
    unsigned clocks = 0;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    bool import_fixture(State& s, const char* name) override {
        if (std::string_view(name) != "CarbonLib::Microseconds") return false;
        s.memory.write<8>(s.r<4>(12), 1000000000ULL + 1000000ULL * ++clocks);
        s.pc = s.r<4>(4128);
        return true;
    }
};
// Count saved LR writes at the eight shot and two periodic-balance incoming
// call sites. This also sees direct C++ calls which bypass dispatch probes.
struct ShotCalls final : MemoryObserver {
    State& s;
    MemoryObserver* previous;
    unsigned calls = 0, balance_calls = 0, construction_calls = 0, influence_calls = 0, obstruction_calls = 0;
    explicit ShotCalls(State& state) : s(state), previous(s.memory.date_observer) { if (!timed) s.memory.date_observer = this; }
    ~ShotCalls() { if (!timed) s.memory.date_observer = previous; }
    void mapped(std::uint32_t at, std::uint64_t size) override { if (previous) previous->mapped(at, size); }
    void cleared(std::uint32_t at, std::uint64_t size) override { if (previous) previous->cleared(at, size); }
    void read(std::uint32_t at, const std::uint8_t* bytes, unsigned size) override { if (previous) previous->read(at, bytes, size); }
    void written(std::uint32_t at, const std::uint8_t* bytes, std::size_t size) override {
        if (previous) previous->written(at, bytes, size);
        if (size != 4 || at != std::uint32_t(s.r<4>(4)) + 8) return;
        const auto back = (std::uint32_t(bytes[0]) << 24) | (std::uint32_t(bytes[1]) << 16) |
                          (std::uint32_t(bytes[2]) << 8) | bytes[3];
        const auto live_lr = std::uint32_t(s.r<4>(4128));
        if (back == 0x1022ba40 && (live_lr == back || live_lr == 0x1022746c)) {
            // The original saves the incoming LR after calling its FPR save helper.
            ++influence_calls;
            return;
        }
        if (back != live_lr) return;
        switch (back) {
        case 0x101dad40: case 0x101db1b4: case 0x102586ac:
        case 0x102787d4: case 0x10278b94: case 0x10279980: case 0x1028166c: case 0x102816a8:
            ++calls; break;
        case 0x101d93ec: case 0x101d942c: ++balance_calls; break;
        case 0x1020e9e0: ++construction_calls; break;
        case 0x101f0b70: ++obstruction_calls; break;
        }
    }
};
void interval(State& s, bool original) {
    Inputs input;
    ShotCalls shots(s);
    s.code_fragment_observer = &input;
    guest::Cpu c(s);
    const auto initial_stack = c.sp();
    s.budget = UINT64_MAX;
    if (construction) {
        // Reuse an existing captured player's staff and the actual controller.
        // The parent supplies the constructor arguments and continues afterward.
        const auto staff = c.u32(match_address + 0x311c + 8 * 0x1c4 + 0x6c);
        c.set(3, match_address + 0x7aac); c.set(4, 8); c.set(5, staff); c.set(6, 8);
        c.set_lr(return_sentinel);
        auto& diagnostics = actor_construction_diagnostics(s);
        const auto previous = diagnostics.original;
        diagnostics.original = original;
        try {
            if (original) match_pl_run_original(s, 0x1020e8f0);
            else { s.pc = 0x1020e8f0; run_until(s, return_sentinel); }
        } catch (...) { diagnostics.original = previous; throw; }
        diagnostics.original = previous;
        if (s.pc != return_sentinel || c.sp() != initial_stack || c.r(3) != 1 || shots.construction_calls != 1)
            throw std::runtime_error("Captured construction parent did not return through its constructor");
    }
    for (unsigned tick = 0; tick < advances; ++tick) {
        c.set(3, match_address); c.set(4, 1); c.set_lr(return_sentinel);
        if (original) match_pl_run_original(s, 0x101d3f60);
        else { s.pc = 0x101d3f60; run_until(s, return_sentinel); }
        if (s.pc != return_sentinel || c.sp() != initial_stack || (c.r(3) != 1 && c.r(3) != 4))
            throw std::runtime_error("Detailed match advance did not return normally");
    }
    s.code_fragment_observer = nullptr;
    // Require equal host input consumption through ordinary compared memory.
    c.put32(0x61000000, input.clocks);
    c.put32(0x61000004, shots.calls);
    c.put32(0x61000008, shots.balance_calls);
    c.put32(0x6100000c, shots.construction_calls);
    if (response) {
        // The response routine is the sole caller of influence.
        c.put32(0x61000010, shots.influence_calls);
        if (!shots.influence_calls) throw std::runtime_error("Actual-match interval did not exercise player response");
        std::cerr << (original ? "original" : "cpp") << " response and influence calls=" << shots.influence_calls << '\n';
    }
    if (obstruction) {
        c.put32(0x61000014, shots.obstruction_calls);
        if (!shots.obstruction_calls) throw std::runtime_error("Actual-match interval did not exercise player obstruction");
        std::cerr << (original ? "original" : "cpp") << " obstruction calls=" << shots.obstruction_calls << '\n';
    }
    if (!timed && !shots.balance_calls) throw std::runtime_error("Actual-match interval did not exercise periodic balance");
    if (!timed && !shots.calls) throw std::runtime_error("Actual-match interval did not exercise shot selection");
    if (!timed) std::cerr << (original ? "original" : "cpp") << " shot-selection calls=" << shots.calls << " periodic-balance calls=" << shots.balance_calls << " construction calls=" << shots.construction_calls << '\n';
}
void measured_interval(State& s, bool original) {
    const auto started = std::chrono::steady_clock::now();
    try { interval(s, original); }
    catch (const std::exception&) { throw; }
    catch (...) { throw std::runtime_error("Detailed match interval did not return normally"); }
    const auto milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    if (timed) std::cerr << "pair=" << pair_index << " route=" << (original ? "original" : "cpp")
                         << " milliseconds=" << std::fixed << std::setprecision(6) << milliseconds << '\n';
}
void original(State& s, std::uint32_t) {
    // The whole State runs the original bodies: MATCH_ENG.CPP's hook declines for it, as MATCH_PL's does.
    match_eng_original_state = &s;
    struct Restore { ~Restore() { match_eng_original_state = nullptr; } } restore;
    measured_interval(s, true);
}
void rewrite(State& s, std::uint32_t) { measured_interval(s, false); }
}
int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) return 2;
    try {
        unsigned pairs = 1;
        if (argc == 3) {
            const std::string_view option(argv[2]);
            if (option == "--construction") construction = true;
            else if (option == "--response") response = true;
            else if (option == "--obstruction") obstruction = true;
            else {
                if (!option.starts_with("--timing-pairs=")) throw std::runtime_error("Unknown MATCH_PL replay option");
                pairs = unsigned(std::stoul(std::string(option.substr(15))));
                if (!pairs || pairs > 100) throw std::runtime_error("MATCH_PL timing pairs must be in 1..100");
                timed = true;
            }
        }
        const std::filesystem::path input(argv[1]);
        std::ifstream(input / "match.txt") >> match_address;
        if (!match_address) throw std::runtime_error("Missing original match identity");
        auto state = std::make_unique<State>();
        load_match_snapshot(*state, input / "initial.snapshot");
        if (state->memory.read<4>(match_address + 0x311c + 0x1a4) != match_address)
            throw std::runtime_error("Captured match lacks its actual player records");
        state->memory.map(0x61000000, 0x1000);
        set_match_turbo_enabled(false);
        if (match_pc_profile_enabled()) throw std::runtime_error("Disable match PC profiling for the paired check");
        dispatch_probe = nullptr;
        for (pair_index = 1; pair_index <= pairs; ++pair_index) {
            const bool cpp_first = pair_index % 2 == 0;
            const auto differences = compare_routine_call(*state, 0x101d3f60,
                cpp_first ? rewrite : original, cpp_first ? original : rewrite,
                RoutineChecker::result_r3 | RoutineChecker::result_f1);
            for (const auto& line : differences) std::cerr << line << '\n';
            if (!differences.empty()) return 1;
        }
        std::cout << advances << " actual-match advances returned on both routes; memory, ABI, RNG and clock consumption match\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
