#include "match_debugger_factory.hpp"
#include "match_replay.hpp"
#include "match_turbo.hpp"
#include "semantic_io_checks.hpp"
#include "code_fragment_adapter.hpp"
#include "player_decision_adapter.hpp"
#include "process_bridge.hpp"
#include <chrono>
#include <fstream>
#include <set>
#include <string_view>

namespace cm::aot { bool player_movement_reference(State&); }

namespace {
using namespace cm::semantic_io_checks;
constexpr std::uint32_t entry = 0x1022e7e0, end = 0x10235da4;
constexpr std::string_view pending_input = "Original movement awaits native event input";
struct Call {
    std::uint32_t pc, target;
    std::array<std::uint32_t, 8> arguments;
};
struct Jump { std::uint32_t pc, target, table, byte_index; };
struct Trace {
    std::set<std::uint32_t> instructions;
    std::vector<std::uint32_t> ordered;
    std::vector<Call> calls;
    std::vector<Jump> jumps;
};
thread_local Trace* active_trace = nullptr;
struct TraceScope {
    Trace* previous;
    explicit TraceScope(Trace& trace) : previous(active_trace) { active_trace = &trace; }
    ~TraceScope() { active_trace = previous; }
};
struct OwnedBacking {
    std::filesystem::path directory;
    ~OwnedBacking() noexcept {
        try {
            std::error_code error;
            if (!std::filesystem::exists(directory, error) || error || std::filesystem::is_symlink(directory, error) || error) return;
            for (const auto& item : std::filesystem::recursive_directory_iterator(directory, error)) {
                if (error || item.is_symlink(error) || error) return;
            }
            std::filesystem::remove_all(directory, error);
        } catch (const std::filesystem::filesystem_error&) {
            // Retain any backing whose ownership cannot be safely checked.
        }
    }
};
struct ClockInput final : CodeFragmentBoundaryObserver {
    struct Event {
        bool leave;
        std::string name;
        decltype(State::registers) registers;
        std::uint32_t pc;
        bool operator==(const Event&) const = default;
    };
    std::uint64_t initial;
    unsigned calls = 0;
    std::vector<Event> events;
    explicit ClockInput(std::uint64_t value) : initial(value) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& state, const char* name) override { events.push_back({false, name, state.registers, state.pc}); }
    void import_leave(const State& state, const char* name) override { events.push_back({true, name, state.registers, state.pc}); }
    bool import_fixture(State& state, const char* name) override {
        if (std::string_view(name) == "CarbonLib::WaitNextEvent" || std::string_view(name) == "CarbonLib::GetNextEvent")
            throw std::runtime_error(std::string(pending_input));
        if (std::string_view(name) != "CarbonLib::Microseconds") return false;
        // Identical explicit host input; football/RNG/dependencies execute.
        state.memory.write<8>(state.r<4>(12), initial + UINT64_C(1000000) * ++calls);
        state.pc = std::uint32_t(state.r<4>(4128));
        return true;
    }
};
unsigned cases = 0, normal = 0, budgets = 0, pending = 0, negatives = 0;
unsigned computed_cases = 0, computed_returns = 0, computed_budgets = 0, computed_negatives = 0;
std::set<std::uint32_t> visited, normal_visited, targets;
std::set<std::pair<std::uint32_t, std::uint32_t>> computed_destinations;

std::string execute(State& state, bool independent) {
    try {
        if (!independent) { run_until(state, return_sentinel); return {}; }
        while (state.pc != return_sentinel) {
            if (player_movement_reference(state)) continue;
            require(state.pc < entry || state.pc >= end, "Missing original movement label");
            // External dependencies (including native imports) run normally.
            // Recursive movement reached through such a dependency is generated
            // production code; root probes describe this invocation only.
            const auto continuation = std::uint32_t(state.r<4>(4128));
            run_until(state, continuation);
        }
    } catch (const std::exception& error) { return error.what(); }
    return {};
}

void compare(const State& input, unsigned advance, unsigned side, unsigned slot,
    std::string_view scenario, std::ostream& receipt, std::uint64_t limit = 50000000, bool negative = false) {
    auto generated = clone(input), reference = clone(input);
    for (auto* state : {generated.get(), reference.get()}) {
        state->platform = input.platform; state->pc = entry; state->wr<4>(4128, return_sentinel);
        state->budget = state->executed + limit;
    }
    const auto clock_start = native_microseconds();
    ClockInput clock_x(clock_start), clock_y(clock_start);
    generated->code_fragment_observer = &clock_x; reference->code_fragment_observer = &clock_y;
    Effects x, y; x.state = generated.get(); y.state = reference.get();
    generated->memory.date_observer = &x; reference->memory.date_observer = &y;
    Trace trace;
    const auto expected = execute(*generated, false);
    std::string actual;
    { TraceScope scope(trace); actual = execute(*reference, true); }
    generated->memory.date_observer = reference->memory.date_observer = nullptr;
    generated->code_fragment_observer = reference->code_fragment_observer = nullptr;
    const auto label = "original movement case " + std::to_string(cases) + " " + std::string(scenario);
    require(expected == actual, label + " generated=" + expected + " reference=" + actual);
    require(!trace.instructions.empty(), label + " no original root probe");
    require(limit != 50000000 ? actual == "Native instruction budget exhausted" : actual.empty() || actual == pending_input,
        label + " unexpected original outcome " + actual);
    require(x.bytes == y.bytes && generated->executed == reference->executed,
        label + " ordered memory effects or accounting differ");
    require(clock_x.calls == clock_y.calls && clock_x.events == clock_y.events,
        label + " native import ABI or explicit host inputs differ");
    if (negative) reference->wr<4>(12, reference->r<4>(12) ^ 1);
    bool rejected = false;
    try { Image(*generated).compare(*reference, label); }
    catch (const std::runtime_error&) { if (!negative) throw; rejected = true; }
    require(rejected == negative, label + " full-state negative discriminant");
    visited.insert(trace.instructions.begin(), trace.instructions.end());
    if (actual.empty() && !negative) normal_visited.insert(trace.instructions.begin(), trace.instructions.end());
    for (const auto& call : trace.calls) targets.insert(call.target);
    for (const auto& jump : trace.jumps) computed_destinations.emplace(jump.pc, jump.target);
    receipt << "{\"kind\":\"whole-body\",\"case\":" << cases << ",\"advance\":" << advance << ",\"side\":" << side << ",\"slot\":" << slot
        << ",\"scenario\":\"" << scenario << "\",\"negative\":" << (negative ? "true" : "false")
        << ",\"normal\":" << (actual.empty() ? "true" : "false") << ",\"pendingInput\":" << (actual == pending_input ? "true" : "false")
        << ",\"budget\":" << limit << ",\"executed\":" << reference->executed - input.executed << ",\"orderedEffects\":" << y.log.size()
        << ",\"clockStart\":" << clock_start << ",\"clockInterval\":1000000,\"clockSamples\":" << clock_y.calls << ",\"arguments\":[";
    for (unsigned n = 3; n <= 10; ++n) { if (n != 3) receipt << ','; receipt << input.r<4>(n * 4); }
    receipt << "],\"visited\":[";
    bool first = true;
    for (auto pc : trace.instructions) { if (!first) receipt << ','; first = false; receipt << pc; }
    receipt << "],\"calls\":["; first = true;
    for (const auto& call : trace.calls) {
        if (!first) receipt << ','; first = false;
        receipt << "{\"pc\":" << call.pc << ",\"target\":" << call.target << ",\"arguments\":[";
        for (unsigned n = 0; n < call.arguments.size(); ++n) { if (n) receipt << ','; receipt << call.arguments[n]; }
        receipt << "]}";
    }
    receipt << "],\"computedJumps\":["; first = true;
    for (const auto& jump : trace.jumps) {
        if (!first) receipt << ','; first = false;
        receipt << "{\"pc\":" << jump.pc << ",\"target\":" << jump.target << ",\"table\":" << jump.table << ",\"byteIndex\":" << jump.byte_index << '}';
    }
    receipt << "]}\n";
    ++cases; normal += actual.empty() && !negative; budgets += actual == "Native instruction budget exhausted";
    pending += actual == pending_input; negatives += negative;
}

struct Capture final : PlayerDecisionBoundaryObserver {
    std::unique_ptr<State> input;
    std::uint32_t call_site = 0;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool, PlayerDecisionOutcome) override {}
    void before_dependency(State& state, std::uint32_t pc, std::uint32_t target) override {
        if (target != entry || input) return;
        input = clone(state); input->platform = state.platform; call_site = pc;
        // Complete the already-characterized caller's call ABI on the private
        // copy. The live match and its decision continuation are untouched.
        input->step(pc); input->wr<4>(2440, input->r<4>(8));
        input->wr<4>(4128, return_sentinel); input->pc = entry;
    }
};

void computed_prefix(const State& original, std::uint32_t site, std::int32_t toc_offset,
    unsigned index, std::uint32_t destination, bool mutation, std::ostream& receipt,
    unsigned limit = 5, bool negative = false) {
    State input;
    // Relocate both TOC and table; the indexed word is read live by the five
    // original instructions. This proves a prefix, not a whole-match outcome.
    input.memory.map(fixture, 0x20000); input.wr<4>(8, fixture + 0x10000); input.wr<4>(0, index);
    const auto original_table = std::uint32_t(original.memory.read<4>(original.r<4>(8) + toc_offset));
    const auto table = fixture + 0x100;
    input.memory.write<4>(fixture + 0x10000 + toc_offset, table);
    input.memory.write<4>(table + 4 * index, mutation ? destination : original.memory.read<4>(original_table + 4 * index));
    input.pc = site - 16; input.budget = limit;
    auto generated = clone(input), reference = clone(input);
    Effects x, y; x.state = generated.get(); y.state = reference.get();
    generated->memory.date_observer = &x; reference->memory.date_observer = &y;
    Trace trace;
    const auto run = [](State& state, bool independent) {
        try { require(independent ? player_movement_reference(state) : dispatch_page(state), "Missing original computed prefix"); }
        catch (const std::exception& error) { return std::string(error.what()); }
        return std::string{};
    };
    const auto expected = run(*generated, false);
    std::string actual; { TraceScope scope(trace); actual = run(*reference, true); }
    generated->memory.date_observer = reference->memory.date_observer = nullptr;
    require(expected == actual && (limit == 5 ? actual.empty() : actual == "Native instruction budget exhausted"), "Computed prefix outcome differs");
    // The original counter includes the attempted instruction that crosses a
    // budget boundary. A complete five-instruction prefix returns at five;
    // an interrupted prefix reports limit + 1, including a zero-budget stop.
    require(x.bytes == y.bytes && generated->executed == reference->executed && reference->executed == (limit == 5 ? 5 : limit + 1),
        "Computed prefix ordered effects/accounting differ");
    if (limit == 5) {
        require(reference->pc == destination && reference->r<4>(4132) == destination && trace.jumps.size() == 1 &&
            trace.jumps[0].target == destination && trace.jumps[0].byte_index == index * 4,
            "Computed prefix did not retain the live table destination/CTR");
    }
    if (negative) reference->wr<4>(4132, reference->r<4>(4132) ^ 1);
    bool rejected = false;
    try { Image(*generated).compare(*reference, "original computed prefix"); }
    catch (const std::runtime_error&) { if (!negative) throw; rejected = true; }
    require(rejected == negative, "Computed prefix negative discriminant");
    receipt << "{\"kind\":\"computed-prefix\",\"site\":" << site << ",\"index\":" << index << ",\"target\":" << destination
        << ",\"relocated\":true,\"mutatedTable\":" << (mutation ? "true" : "false") << ",\"budget\":" << limit
        << ",\"negative\":" << (negative ? "true" : "false") << ",\"executed\":" << reference->executed << ",\"orderedEffects\":" << y.log.size() << "}\n";
    ++computed_cases; computed_returns += actual.empty() && !negative; computed_budgets += !actual.empty(); computed_negatives += negative;
}
}

namespace cm::aot {
void player_movement_reference_step_seen(State&, std::uint32_t pc) {
    if (active_trace) { active_trace->instructions.insert(pc); active_trace->ordered.push_back(pc); }
}
void player_movement_reference_call_seen(State& state, std::uint32_t pc, std::uint32_t target) {
    if (!active_trace) return;
    Call call{pc, target, {}};
    for (unsigned n = 0; n < call.arguments.size(); ++n) call.arguments[n] = std::uint32_t(state.r<4>((n + 3) * 4));
    active_trace->calls.push_back(call);
}
void player_movement_reference_jump_seen(State& state, std::uint32_t pc, std::uint32_t target) {
    if (active_trace) active_trace->jumps.push_back({pc, target, std::uint32_t(state.r<4>(12)), std::uint32_t(state.r<4>(0))});
}
}

int main(int argc, char** argv) {
    try {
        require(argc == 4 && !match_pc_profile_enabled(), "Expected original program, game and private output directory; profiling disabled");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        require(!std::filesystem::exists(root), "Characterization requires a new private directory");
        std::filesystem::create_directories(root);
        OwnedBacking cleanup{root / "backing"};
        std::ofstream receipt(root / "cases.ndjson"); require(bool(receipt), "Cannot create private movement receipt");
        receipt << std::unitbuf;
        std::cout << "Preparing fresh actual movement input\n" << std::flush;
        auto created = cm::aot::match_debugger::create_match({argv[1], argv[2], root / "backing"}, {677, 5677, 12345, 2001, 194},
            [](std::string_view stage) { std::cout << stage << '\n' << std::flush; });
        auto& state = *created.state;
        set_match_turbo_enabled(false);
        unsigned captures = 0;
        bool saved = false;
        for (unsigned advance = 1; advance <= 3; ++advance) {
            state.wr<4>(12, created.match); state.wr<4>(16, 1); state.wr<4>(4128, return_sentinel);
            state.pc = 0x101d3f60; state.budget = state.executed + 50000000; run_until(state, return_sentinel);
            if (advance == 2) continue;
            for (unsigned side = 0; side < 2; ++side) {
              unsigned side_captures = 0;
              for (unsigned slot : {0u, 2u, 8u, 1u, 3u, 4u, 5u, 6u, 7u, 9u, 10u}) {
                auto decision = clone(state); decision->platform = state.platform;
                decision->wr<4>(12, created.match + 0x311c + side * 0x2350 + slot * 0x1c4);
                decision->wr<4>(4128, return_sentinel); decision->pc = 0x102836a0; decision->budget = decision->executed + 50000000;
                Capture capture; player_decision_diagnostics(*decision).observer = &capture;
                ClockInput clock(native_microseconds()); decision->code_fragment_observer = &clock;
                try { run_until(*decision, return_sentinel); }
                catch (const std::runtime_error& error) { require(error.what() == pending_input, "Unexpected decision capture outcome " + std::string(error.what())); }
                player_decision_diagnostics(*decision).observer = nullptr; decision->code_fragment_observer = nullptr;
                if (!capture.input) {
                    std::cout << "Decision used another actual action: advance=" << advance << " side=" << side << " actor=" << slot << '\n' << std::flush;
                    continue;
                }
                ++captures; ++side_captures;
                std::cout << "Captured movement: advance=" << advance << " side=" << side << " actor=" << slot << '\n' << std::flush;
                if (!saved) { save_runtime_snapshot(*capture.input, root / "fresh-movement.snapshot"); saved = true; }
                compare(*capture.input, advance, side, slot, "actual-decision-call", receipt);
                if (advance == 3 && side == 0 && side_captures == 3) {
                    for (unsigned flag : {0u, 15u, 256u, 0xffffffffu}) {
                        auto variant = clone(*capture.input); variant->platform = capture.input->platform;
                        variant->wr<4>(16, variant->memory.read<1>(variant->r<4>(12) + 0x105));
                        variant->wr<4>(20, variant->memory.read<1>(variant->r<4>(12) + 0x106)); variant->wr<4>(24, flag);
                        compare(*variant, advance, side, slot, "same-coordinate-mode", receipt);
                    }
                    for (unsigned limit : {0u, 1u, 8u, 24u, 64u, 128u, 256u, 512u})
                        compare(*capture.input, advance, side, slot, "budget", receipt, limit);
                    compare(*capture.input, advance, side, slot, "return-negative", receipt, 50000000, true);
                }
                if (side_captures == 3) break;
              }
              require(side_captures == 3, "Fewer than three actual movement callers were observed for this side");
            }
        }
        for (const auto& table : {std::array<std::int32_t, 3>{0x1023041c, -0x51f4, 47}, {0x10233b18, -0x51f8, 47}, {0x10233d98, -0x51fc, 11}}) {
            const auto site = std::uint32_t(table[0]);
            const auto address = std::uint32_t(state.memory.read<4>(state.r<4>(8) + table[1]));
            for (unsigned index = 0; index < unsigned(table[2]); ++index)
                computed_prefix(state, site, table[1], index, std::uint32_t(state.memory.read<4>(address + index * 4)), false, receipt);
            // The reference must dispatch a mutated live word, including an
            // exterior destination, rather than silently forcing a known case.
            computed_prefix(state, site, table[1], 0, entry, true, receipt);
            computed_prefix(state, site, table[1], 0, 0x12345678, true, receipt);
            for (unsigned limit = 0; limit < 5; ++limit)
                computed_prefix(state, site, table[1], 0, std::uint32_t(state.memory.read<4>(address)), false, receipt, limit);
            computed_prefix(state, site, table[1], 0, std::uint32_t(state.memory.read<4>(address)), false, receipt, 5, true);
        }
        require(captures == 12 && cases == 25 && normal + pending == 16 && budgets == 8 && negatives == 1,
            "Whole movement characterization matrix incomplete");
        require(normal_visited.contains(entry) && normal_visited.contains(end - 4), "Whole original root entry/return not observed normally");
        require(computed_cases == 129 && computed_returns == 111 && computed_budgets == 15 && computed_negatives == 3,
            "Original computed-prefix matrix incomplete");
        receipt.close(); require(bool(receipt), "Private movement receipt failed");
        created.state.reset();
        std::cout << "PASS: whole original movement cases=" << cases << " normal=" << normal << " budgets=" << budgets << " pending-input=" << pending
            << " negative=" << negatives << " normal-sites=" << normal_visited.size() << "/7537 all-observed-sites=" << visited.size()
            << " dependencies=" << targets.size() << " whole-body-computed-destinations=" << computed_destinations.size()
            << "; computed-prefix cases=" << computed_cases << " normal=" << computed_returns << " budgets=" << computed_budgets << " negatives=" << computed_negatives
            << "; full architecture, memory, ordered effects, native import ABI and accounting agree; this is original characterization, not a semantic port\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
