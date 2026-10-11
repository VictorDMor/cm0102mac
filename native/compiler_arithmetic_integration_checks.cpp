#include "compiler_arithmetic_adapter.hpp"
#include "compiler_arithmetic_fixtures.hpp"
#include "match_turbo.hpp"
#include <numeric>

namespace {
using namespace cm::compiler_arithmetic_checks;
enum class Path { native, generated, cfm };
enum class Mode { modern, entry_original, global_original, mixed, profile };
constexpr std::array<std::uint64_t, 8> bounds{20, 0, 780, 760, 771, 42, 43, 49};
unsigned cases = 0, stopping_cases = 0, mapping_cases = 0, enabled_traps = 0, rejected = 0;
std::array<unsigned, 8> native_witnesses{}, generated_witnesses{}, cfm_witnesses{}, original_witnesses{};
struct Boundaries final : CompilerArithmeticBoundaryObserver {
    struct Event { std::uint32_t entry; bool original, leaving, returned; };
    std::vector<Event> events;
    bool corrupt = false;
    void enter(const State&, std::uint32_t entry, bool original) override { events.push_back({entry, original, false, false}); }
    void leave(State& s, std::uint32_t entry, bool original, bool returned) override {
        events.push_back({entry, original, true, returned});
        if (corrupt && !original) s.wr<4>(12, s.r<4>(12) ^ 1);
    }
};
void prepare_path(State& s, unsigned index, Path path) {
    s.pc = entries[index].address;
    if (path == Path::cfm) {
        constexpr auto descriptor = fixture + 0xc0;
        s.memory.write<4>(descriptor, s.pc); s.memory.write<4>(descriptor + 4, s.r<4>(8));
        s.wr<4>(48, descriptor); s.pc = 0x10781320;
    }
}
std::string execute_path(State& s, unsigned index, Path path) {
    try {
        if (path == Path::native) invoke_compiler_arithmetic(s, entries[index].address);
        else run_until(s, return_address);
    } catch (const std::runtime_error& e) { return e.what(); }
    return {};
}
std::uint64_t total(const auto& counts) { return std::accumulate(counts.begin(), counts.end(), UINT64_C(0)); }
void clean_normal(const State& s, unsigned index, bool returned) {
    require(s.compiler_arithmetic_calls[index] == 1 && total(s.compiler_arithmetic_calls) == 1 &&
        total(s.compiler_arithmetic_original_calls) == 0 && total(s.compiler_arithmetic_budget_original_calls) == 0 &&
        total(s.compiler_arithmetic_profile_original_calls) == 0 && s.compiler_arithmetic_dispatch_escapes == 0 &&
        s.compiler_arithmetic_original_resume == 0 && s.compiler_arithmetic_returns[index] == unsigned(returned) &&
        s.compiler_arithmetic_faults[index] == unsigned(!returned), "Default normal per-entry evidence");
}
void compare_case(unsigned index, const State& initial, Path path, bool turbo, Mode mode = Mode::modern,
                  const std::string& expected_error = {}, bool negative = false, bool exact_accounting = false,
                  bool boundaries = true) {
    auto original = clone(initial), candidate = clone(initial);
    prepare_path(*original, index, path); prepare_path(*candidate, index, path);
    original->compiler_arithmetic_original = true;
    if (mode == Mode::entry_original) candidate->compiler_arithmetic_entry_original[index] = true;
    if (mode == Mode::global_original) candidate->compiler_arithmetic_original = true;
    if (mode == Mode::mixed) candidate->compiler_arithmetic_entry_original[(index + 1) % 8] = true;
    Monitor a, b; a.effects.state = original.get(); b.effects.state = candidate.get();
    original->memory.date_observer = &a; candidate->memory.date_observer = &b;
    Boundaries old_boundary, new_boundary; new_boundary.corrupt = negative;
    if (boundaries) { original->compiler_arithmetic_observer = &old_boundary; candidate->compiler_arithmetic_observer = &new_boundary; }
    set_match_turbo_enabled(false); const auto x = execute_path(*original, index, path);
    set_match_turbo_enabled(turbo); const auto y = execute_path(*candidate, index, path);
    original->memory.date_observer = candidate->memory.date_observer = nullptr;
    original->compiler_arithmetic_observer = candidate->compiler_arithmetic_observer = nullptr;
    const auto label = std::string(entries[index].name) + (turbo ? "/turbo" : "/regular") + "/path=" + std::to_string(unsigned(path));
    require(x == y && (expected_error.empty() ? x.empty() : x.starts_with(expected_error)), label + " outcome expected=" + x + " actual=" + y);
    if (negative) {
        bool detected = false; try { Image(*original).compare(*candidate, label); } catch (const std::runtime_error&) { detected = true; }
        require(detected && new_boundary.events.size() == 2, label + " individual negative accepted"); ++rejected;
    } else Image(*original).compare(*candidate, label);
    require(a.effects.bytes == b.effects.bytes, label + " ordered guest effects");
    if (exact_accounting || (index == 1 && x.empty())) require(original->executed == candidate->executed, label + " instruction charge");
    require(candidate->compiler_arithmetic_original_resume == 0 && candidate->compiler_arithmetic_dispatch_escapes == 0, label + " resume/escape");
    if (boundaries) {
        require(old_boundary.events.size() == 2 && new_boundary.events.size() == 2, label + " boundary count");
        for (const auto& event : old_boundary.events) require(event.entry == entries[index].address && event.original, label + " original boundary");
        for (unsigned n = 0; n < 2; ++n) {
            require(new_boundary.events[n].entry == entries[index].address && new_boundary.events[n].leaving == bool(n) &&
                new_boundary.events[n].returned == (n == 1 && x.empty()), label + " entry/leave outcome");
        }
    }
    if (mode == Mode::modern || mode == Mode::mixed) clean_normal(*candidate, index, x.empty());
    else if (mode == Mode::profile) {
        require(total(candidate->compiler_arithmetic_calls) == 0 && total(candidate->compiler_arithmetic_original_calls) == 0 &&
            candidate->compiler_arithmetic_profile_original_calls[index] == 1 && total(candidate->compiler_arithmetic_budget_original_calls) == 0,
            label + " profile selection");
    } else {
        require(total(candidate->compiler_arithmetic_calls) == 0 && candidate->compiler_arithmetic_original_calls[index] == 1 &&
            total(candidate->compiler_arithmetic_budget_original_calls) == 0 && total(candidate->compiler_arithmetic_profile_original_calls) == 0,
            label + " independent original selection"); ++original_witnesses[index];
    }
    if (mode == Mode::modern && x.empty() && !negative) {
        if (path == Path::native) ++native_witnesses[index];
        else if (path == Path::generated) ++generated_witnesses[index];
        else ++cfm_witnesses[index];
    }
    if (x.starts_with("Unmapped data access at ")) ++mapping_cases;
    if (x.starts_with("Original enabled floating")) ++enabled_traps;
    ++cases;
}
void roots_and_selection() {
    for (unsigned index = 0; index < 8; ++index) {
        auto s = fixture_state(index);
        for (bool turbo : {false, true}) {
            for (auto path : {Path::native, Path::generated, Path::cfm}) compare_case(index, *s, path, turbo);
            compare_case(index, *s, Path::generated, turbo, Mode::entry_original, {}, false, true);
            compare_case(index, *s, Path::cfm, turbo, Mode::global_original, {}, false, true);
            compare_case(index, *s, Path::generated, turbo, Mode::mixed);
        }
        compare_case(index, *s, Path::generated, true, Mode::modern, {}, true);
        if (index == 0 || index == 2 || index >= 5) {
            const auto missing = index == 0 ? relocated_toc - 0x3b7c : stack + 24;
            auto fault = without_byte(*s, missing);
            for (bool turbo : {false, true}) compare_case(index, *fault, Path::generated, turbo, Mode::modern, "Unmapped data access at ");
        }
    }
    auto s = fixture_state(0); s->wr<1>(1308, 1);
    compare_case(0, *s, Path::generated, true, Mode::modern, "Original enabled floating inexact exception requires a handler");
    s->wr<1>(1308, 0); s->wr<1>(1304, 1);
    s->memory.write<8>(thresholds + 8, std::bit_cast<std::uint64_t>(1.0e20));
    s->memory.write<8>(thresholds + 16, std::bit_cast<std::uint64_t>(1.0e20)); floating(*s, 4294967296.0);
    compare_case(0, *s, Path::generated, true, Mode::modern, "Original enabled floating invalid-conversion exception requires a handler");
}
void budget_case(unsigned index, const State& initial, std::uint64_t available, bool turbo, Path path) {
    auto old = clone(initial), candidate = clone(initial);
    old->budget = candidate->budget = initial.executed + available;
    old->compiler_arithmetic_original = true; prepare_path(*old, index, path); prepare_path(*candidate, index, path);
    Monitor a, b; a.effects.state = old.get(); b.effects.state = candidate.get();
    old->memory.date_observer = &a; candidate->memory.date_observer = &b;
    set_match_turbo_enabled(false); const auto x = execute_path(*old, index, path);
    set_match_turbo_enabled(turbo); const auto y = execute_path(*candidate, index, path);
    old->memory.date_observer = candidate->memory.date_observer = nullptr;
    const auto label = std::string(entries[index].name) + "/budget=" + std::to_string(available);
    require(x == y && (x.empty() || x == "Native instruction budget exhausted"), label + " stopping outcome");
    Image(*old).compare(*candidate, label); require(a.effects.bytes == b.effects.bytes, label + " ordered stopping prefix");
    const bool semantic = total(candidate->compiler_arithmetic_calls) != 0;
    if (index == 1 || !semantic) require(old->executed == candidate->executed, label + " exact stopping count");
    require(candidate->compiler_arithmetic_dispatch_escapes == 0 && total(candidate->compiler_arithmetic_original_calls) == 0 &&
        total(candidate->compiler_arithmetic_profile_original_calls) == 0, label + " separate budget diagnosis");
    if (!x.empty()) { require(!semantic, label + " partial call ran native body"); ++stopping_cases; }
    ++cases;
}
void budgets() {
    for (unsigned index = 0; index < 8; ++index) {
        auto s = fixture_state(index);
        const auto limit = index == 1 ? arithmetic::unsigned_quotient_instruction_count(*s) : bounds[index];
        for (std::uint64_t available = 0; available <= limit + 1; ++available)
            for (bool turbo : {false, true}) budget_case(index, *s, available, turbo, Path::generated);
    }
    for (const auto [n, d] : {std::pair{UINT64_C(0), UINT64_C(0)}, {UINT64_C(1), UINT64_C(0)},
            {UINT64_C(0x8000000000000000), UINT64_C(0)}, {UINT64_MAX, UINT64_C(3)}, {UINT64_C(0), UINT64_C(1)}}) {
        auto s = fixture_state(1); pair(*s, n); pair(*s, d, 5);
        const auto cost = arithmetic::unsigned_quotient_instruction_count(*s);
        for (std::uint64_t available = 0; available <= cost + 6; ++available)
            budget_case(1, *s, available, true, Path::cfm);
        for (auto executed : {UINT64_MAX, UINT64_MAX - 1, UINT64_MAX - cost}) {
            s->executed = executed; budget_case(1, *s, UINT64_MAX - executed, true, Path::generated);
        }
    }
}
void reset_checks() {
    for (unsigned index = 0; index < 8; ++index) {
        auto s = fixture_state(index); Boundaries boundary;
        s->compiler_arithmetic_original = true; s->compiler_arithmetic_entry_original.fill(true);
        s->compiler_arithmetic_calls.fill(1); s->compiler_arithmetic_original_calls.fill(2);
        s->compiler_arithmetic_returns.fill(3); s->compiler_arithmetic_faults.fill(4);
        s->compiler_arithmetic_budget_original_calls.fill(5); s->compiler_arithmetic_profile_original_calls.fill(6);
        s->compiler_arithmetic_original_resume = entries[index].address; s->compiler_arithmetic_dispatch_escapes = 7;
        s->compiler_arithmetic_observer = &boundary;
        s->exception_runtime_calls[0] = 77; s->file_controller_original = true; s->allocator_calls[0] = 99;
        const Image before(*s); reset_compiler_arithmetic_diagnostics(*s); before.compare(*s, "Reset architectural isolation");
        require(!s->compiler_arithmetic_original && !s->compiler_arithmetic_observer && !s->compiler_arithmetic_original_resume &&
            !s->compiler_arithmetic_dispatch_escapes && s->compiler_arithmetic_entry_original == std::array<bool, 8>{} &&
            total(s->compiler_arithmetic_calls) == 0 && total(s->compiler_arithmetic_original_calls) == 0 &&
            total(s->compiler_arithmetic_returns) == 0 && total(s->compiler_arithmetic_faults) == 0 &&
            total(s->compiler_arithmetic_budget_original_calls) == 0 && total(s->compiler_arithmetic_profile_original_calls) == 0 &&
            s->exception_runtime_calls[0] == 77 && s->file_controller_original && s->allocator_calls[0] == 99,
            "Independent arithmetic reset"); ++cases;
    }
}
void profiles() {
    require(match_pc_profile_enabled(), "Profile process must explicitly enable original-PC diagnostics");
    for (unsigned index = 0; index < 8; ++index) for (bool turbo : {false, true}) {
        auto s = fixture_state(index);
        s->executed = match_pc_sample_threshold(0) - 2; s->budget = UINT64_MAX;
        compare_case(index, *s, Path::generated, turbo, Mode::profile, {}, false, true);
    }
}
}
int main(int argc, char** argv) {
    try {
        const bool profile = argc == 2 && std::string_view(argv[1]) == "--profile";
        require(argc == 1 || profile, "Usage: compiler arithmetic integration checks [--profile]");
        if (profile) profiles();
        else {
            require(!match_pc_profile_enabled(), "Normal acceptance requires PC profiling disabled");
            roots_and_selection(); budgets(); reset_checks();
            for (unsigned index = 0; index < 8; ++index) {
                require(native_witnesses[index] == 2 && generated_witnesses[index] == 2 && cfm_witnesses[index] == 2 && original_witnesses[index] == 4,
                    "Missing individual direct/generated/CFM/original evidence");
                std::cout << entries[index].name << ": native=" << native_witnesses[index] << " generated=" << generated_witnesses[index]
                    << " CFM=" << cfm_witnesses[index] << " original=" << original_witnesses[index] << '\n';
            }
            require(rejected == 8 && mapping_cases == 10 && enabled_traps == 2, "Missing discriminating root/fault/trap evidence");
        }
        std::cout << cases << " production comparisons; " << stopping_cases << " exact budget stops; " << mapping_cases
            << " mapping faults; " << enabled_traps << " enabled traps; " << rejected << " rejected negatives"
            << (profile ? "; original-PC profiling retained\n" : "; all eight normal roots have zero original selections and escapes\n");
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    return 0;
}
