// A bounded detailed-match interval with MATCH_PL original/default routing.
#include "match_pl_adapter.hpp"
#include "match_replay.hpp"
#include "match_turbo.hpp"
#include "code_fragment_adapter.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <string_view>

using namespace cm::aot;
namespace {
std::uint32_t match_address = 0;
constexpr unsigned advances = 600;
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
void interval(State& s, bool original) {
    Inputs input;
    s.code_fragment_observer = &input;
    guest::Cpu c(s);
    const auto initial_stack = c.sp();
    s.budget = UINT64_MAX;
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
}
void original(State& s, std::uint32_t) { interval(s, true); }
void rewrite(State& s, std::uint32_t) { interval(s, false); }
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
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
        const auto differences = compare_routine_call(*state, 0x101d3f60, original, rewrite,
            RoutineChecker::result_r3 | RoutineChecker::result_f1);
        for (const auto& line : differences) std::cerr << line << '\n';
        if (!differences.empty()) return 1;
        std::cout << advances << " actual-match advances returned on both routes; memory, ABI, RNG and clock consumption match\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
