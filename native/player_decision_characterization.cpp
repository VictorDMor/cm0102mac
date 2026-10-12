#include "match_debugger_factory.hpp"
#include "match_replay.hpp"
#include "match_turbo.hpp"
#include "semantic_io_checks.hpp"
#include "code_fragment_adapter.hpp"
#include "process_bridge.hpp"
#include <chrono>
#include <fstream>
#include <map>
#include <set>
#include <string_view>

namespace cm::aot { bool player_decision_reference(State&); }

namespace {
using namespace cm::semantic_io_checks;
constexpr std::uint32_t entry = 0x102836a0, end = 0x10285748;
struct Call {
    std::uint32_t pc, target;
    std::array<std::uint32_t, 8> arguments;
};
struct Trace {
    std::set<std::uint32_t> instructions;
    std::vector<Call> calls;
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
            // Preserve a backing that cannot be safely enumerated or removed.
        }
    }
};
struct ClockInput final : CodeFragmentBoundaryObserver {
    std::uint64_t initial;
    unsigned calls = 0;
    struct Event {
        bool leave;
        std::string name;
        decltype(State::registers) registers;
        std::uint32_t pc;
        bool operator==(const Event&) const = default;
    };
    std::vector<Event> events;
    explicit ClockInput(std::uint64_t value) : initial(value) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& state, const char* name) override { events.push_back({false, name, state.registers, state.pc}); }
    void import_leave(const State& state, const char* name) override { events.push_back({true, name, state.registers, state.pc}); }
    bool import_fixture(State& state, const char* name) override {
        // Headless characterization ends at the native event boundary. It does
        // not supply a fictitious click or treat a pending UI path as a return.
        if (std::string_view(name) == "CarbonLib::WaitNextEvent" || std::string_view(name) == "CarbonLib::GetNextEvent")
            throw std::runtime_error("Original player decision awaits native event input");
        // Presentation consumes host time. Both executions receive the same
        // monotonic external input; football, RNG, rendering and other services
        // still execute their real production bodies.
        if (std::string_view(name) != "CarbonLib::Microseconds") return false;
        state.memory.write<8>(state.r<4>(12), initial + UINT64_C(1000000) * ++calls);
        state.pc = std::uint32_t(state.r<4>(4128));
        return true;
    }
};
enum class Scenario { baseline, dead_ball_receiver, loose_dead_ball, marker, no_marker };
struct Result { std::string error; Trace trace; std::size_t effects; std::uint32_t action; };
unsigned cases = 0, normal = 0, interrupted = 0, awaiting_input = 0, negatives = 0;
std::set<std::uint32_t> visited, targets;

Result compare(const State& input, std::uint32_t match, unsigned side, unsigned slot,
    Scenario scenario, std::uint64_t budget, bool negative, std::ostream& receipt) {
    auto generated = clone(input), reference = clone(input);
    generated->platform = reference->platform = input.platform;
    const auto actor = match + (side ? 0x546c : 0x311c) + slot * 0x1c4;
    for (auto* state : {generated.get(), reference.get()}) {
        state->wr<4>(12, actor); state->wr<4>(4128, return_sentinel); state->pc = entry;
        state->budget = state->executed + budget;
        if (scenario == Scenario::dead_ball_receiver) {
            state->memory.write<1>(match + 0x7924, 255);
            state->memory.write<4>(match + 0xdff8, actor);
            state->memory.write<1>(actor + 0x10a, 20);
        } else if (scenario == Scenario::loose_dead_ball) {
            state->memory.write<1>(match + 0x7924, 255);
            state->memory.write<4>(match + 0xdff8, 0);
        } else if (scenario == Scenario::marker) {
            const auto opponent = match + (side ? 0x311c : 0x546c) + 8 * 0x1c4;
            state->memory.write<4>(actor + 0x1b0, opponent);
        } else if (scenario == Scenario::no_marker) {
            state->memory.write<4>(actor + 0x1b0, 0);
        }
    }
    Effects x, y; x.state = generated.get(); y.state = reference.get();
    const auto clock_start = native_microseconds();
    ClockInput clock_x(clock_start), clock_y(clock_start);
    generated->code_fragment_observer = &clock_x; reference->code_fragment_observer = &clock_y;
    generated->memory.date_observer = &x; reference->memory.date_observer = &y;
    const auto execute = [](State& state, bool independent) {
        try {
            if (!independent) { run_until(state, return_sentinel); return std::string{}; }
            while (state.pc != return_sentinel) {
                if (player_decision_reference(state)) continue;
                require(state.pc < entry || state.pc >= end, "Missing independent original player decision label");
                const auto continuation = std::uint32_t(state.r<4>(4128));
                run_until(state, continuation);
            }
        } catch (const std::exception& error) { return std::string(error.what()); }
        return std::string{};
    };
    Result result;
    const auto expected = execute(*generated, false);
    { TraceScope scope(result.trace); result.error = execute(*reference, true); }
    generated->memory.date_observer = reference->memory.date_observer = nullptr;
    generated->code_fragment_observer = reference->code_fragment_observer = nullptr;
    const auto label = "original player decision case " + std::to_string(cases);
    require(expected == result.error, label + " generated=" + expected + " reference=" + result.error);
    require(!result.trace.instructions.empty(), label + " no independent original execution");
    const bool native_input = result.error == "Original player decision awaits native event input";
    require(result.error.empty() || result.error == "Native instruction budget exhausted" || native_input,
        label + " unexpected original outcome " + result.error);
    if (negative) reference->wr<4>(12, reference->r<4>(12) ^ 1);
    bool rejected = false;
    try { Image(*generated).compare(*reference, label); }
    catch (const std::runtime_error&) {
        if (!negative) {
            std::cerr << label << " PC " << hex_address(generated->pc) << '/' << hex_address(reference->pc)
                << " LR " << hex_address(std::uint32_t(generated->r<4>(4128))) << '/' << hex_address(std::uint32_t(reference->r<4>(4128)))
                << " r0 " << hex_address(std::uint32_t(generated->r<4>(0))) << '/' << hex_address(std::uint32_t(reference->r<4>(0)))
                << " r1 " << hex_address(std::uint32_t(generated->r<4>(4))) << '/' << hex_address(std::uint32_t(reference->r<4>(4)))
                << " instructions " << generated->executed - input.executed << '/' << reference->executed - input.executed
                << " effects " << x.log.size() << '/' << y.log.size() << '\n';
            const auto same_access = [](const auto& a, const auto& b) {
                return a.kind == b.kind && a.address == b.address && a.size == b.size && a.value == b.value;
            };
            const auto mismatch = std::mismatch(x.log.begin(), x.log.end(), y.log.begin(), y.log.end(), same_access);
            std::cerr << "First effect divergence index=" << mismatch.first - x.log.begin();
            if (mismatch.first != x.log.end()) std::cerr << " generated-address=" << hex_address(mismatch.first->address) << " value=" << mismatch.first->value;
            if (mismatch.second != y.log.end()) std::cerr << " reference-address=" << hex_address(mismatch.second->address) << " value=" << mismatch.second->value;
            std::cerr << "\nReference root calls:";
            for (const auto& call : result.trace.calls) std::cerr << ' ' << hex_address(call.pc) << "->" << hex_address(call.target);
            std::cerr << '\n';
            throw;
        }
        rejected = true;
    }
    require(rejected == negative, label + " full-state negative discriminant");
    if (x.bytes != y.bytes || generated->executed != reference->executed) {
        const auto same_access = [](const auto& a, const auto& b) {
            return a.kind == b.kind && a.address == b.address && a.size == b.size && a.value == b.value;
        };
        const auto mismatch = std::mismatch(x.log.begin(), x.log.end(), y.log.begin(), y.log.end(), same_access);
        std::cerr << label << " instructions " << generated->executed - input.executed << '/' << reference->executed - input.executed
            << " effects " << x.log.size() << '/' << y.log.size() << " first-index=" << mismatch.first - x.log.begin()
            << " clocks " << clock_x.calls << '/' << clock_y.calls << " imports-equal=" << (clock_x.events == clock_y.events)
            << " budget-stop=" << !result.error.empty() << '\n';
        if (mismatch.first != x.log.end()) std::cerr << "Generated access kind=" << mismatch.first->kind << " address=" << hex_address(mismatch.first->address) << " size=" << mismatch.first->size << " value=" << mismatch.first->value << '\n';
        if (mismatch.second != y.log.end()) std::cerr << "Reference access kind=" << mismatch.second->kind << " address=" << hex_address(mismatch.second->address) << " size=" << mismatch.second->size << " value=" << mismatch.second->value << '\n';
        throw std::runtime_error(label + " ordered effects and instruction accounting");
    }
    require(clock_x.calls == clock_y.calls && clock_x.events == clock_y.events, label + " complete native import boundaries and identical external clock input");
    result.effects = y.log.size();
    result.action = reference->memory.read<2>(actor + 0x19c);
    visited.insert(result.trace.instructions.begin(), result.trace.instructions.end());
    for (const auto& call : result.trace.calls) targets.insert(call.target);
    receipt << "{\"case\":" << cases << ",\"side\":" << side << ",\"slot\":" << slot
        << ",\"scenario\":" << unsigned(scenario) << ",\"budget\":" << budget << ",\"negative\":" << (negative ? "true" : "false")
        << ",\"normal\":" << (result.error.empty() ? "true" : "false") << ",\"action\":" << result.action
        << ",\"awaitingNativeInput\":" << (native_input ? "true" : "false")
        << ",\"executed\":" << reference->executed - input.executed << ",\"orderedEffects\":" << result.effects
        << ",\"clockStart\":" << clock_start << ",\"clockInterval\":1000000,\"clockSamples\":" << clock_y.calls << ",\"visited\":[";
    bool first = true;
    for (auto pc : result.trace.instructions) { if (!first) receipt << ','; first = false; receipt << pc; }
    receipt << "],\"calls\":["; first = true;
    for (const auto& call : result.trace.calls) {
        if (!first) receipt << ','; first = false;
        receipt << "{\"pc\":" << call.pc << ",\"target\":" << call.target << ",\"arguments\":[";
        for (unsigned n = 0; n < call.arguments.size(); ++n) { if (n) receipt << ','; receipt << call.arguments[n]; }
        receipt << "]}";
    }
    receipt << "]}\n";
    ++cases; normal += result.error.empty() && !negative; interrupted += result.error == "Native instruction budget exhausted";
    awaiting_input += native_input; negatives += negative;
    return result;
}
}

namespace cm::aot {
void player_decision_reference_step_seen(State&, std::uint32_t pc) {
    if (active_trace) active_trace->instructions.insert(pc);
}
void player_decision_reference_call_seen(State& state, std::uint32_t pc, std::uint32_t target) {
    if (!active_trace) return;
    Call call{pc, target, {}};
    for (unsigned n = 0; n < call.arguments.size(); ++n) call.arguments[n] = std::uint32_t(state.r<4>((n + 3) * 4));
    active_trace->calls.push_back(call);
}
}

int main(int argc, char** argv) {
    try {
        require(argc == 4 && !match_pc_profile_enabled(), "Expected original program, game and new private output directory; profiling disabled");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        require(!std::filesystem::exists(root), "Characterization requires a new private directory");
        std::filesystem::create_directories(root);
        OwnedBacking cleanup{root / "backing"};
        std::ofstream receipt(root / "cases.ndjson"); require(bool(receipt), "Cannot create private decision receipt");
        auto created = cm::aot::match_debugger::create_match({argv[1], argv[2], root / "backing"}, {677, 5677, 12345, 2001, 194});
        auto& state = *created.state;
        set_match_turbo_enabled(false);
        for (unsigned advance = 1; advance <= 3; ++advance) {
            state.wr<4>(12, created.match); state.wr<4>(16, 1); state.wr<4>(4128, return_sentinel);
            state.pc = 0x101d3f60; state.budget = state.executed + 50000000; run_until(state, return_sentinel);
            if (advance == 1) save_runtime_snapshot(state, root / "fresh-step.snapshot");
            if (advance == 2) continue;
            for (unsigned side = 0; side < 2; ++side) for (unsigned slot = 0; slot < 11; ++slot)
                compare(state, created.match, side, slot, Scenario::baseline, 50000000, false, receipt);
        }
        for (unsigned side = 0; side < 2; ++side) for (auto scenario : {Scenario::dead_ball_receiver, Scenario::loose_dead_ball, Scenario::marker, Scenario::no_marker})
            compare(state, created.match, side, 8, scenario, 50000000, false, receipt);
        for (auto limit : {0u, 1u, 8u, 19u, 32u, 64u, 128u, 256u})
            compare(state, created.match, 0, 8, Scenario::baseline, limit, false, receipt);
        compare(state, created.match, 0, 2, Scenario::baseline, 50000000, true, receipt);
        require(normal + awaiting_input == 52 && normal && awaiting_input && interrupted == 8 && negatives == 1 && cases == 61,
            "Original characterization matrix incomplete");
        require(visited.contains(entry) && visited.contains(end - 4), "Original root entry and normal return not observed");
        receipt.close(); require(bool(receipt), "Private decision receipt failed");
        created.state.reset();
        for (const auto& item : std::filesystem::recursive_directory_iterator(root / "backing"))
            require(!item.is_symlink(), "Unexpected symlink in owned characterization backing");
        std::filesystem::remove_all(root / "backing");
        std::cout << "PASS: original player decision characterization cases=" << cases << " normal=" << normal << " budgets=" << interrupted
            << " native-input-boundaries=" << awaiting_input << " negative=" << negatives << " visited-instructions=" << visited.size() << "/2090 dependency-targets=" << targets.size()
            << "; full original/generated ABI, memory, import boundaries, ordered effects and accounting agree; controlled host clock, pending UI paths are not returns; this does not prove a semantic port\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
