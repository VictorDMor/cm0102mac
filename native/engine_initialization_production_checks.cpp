#include "engine_initialization_adapter.hpp"
#include "contract_manager_adapter.hpp"
#include "code_fragment_adapter.hpp"
#include "semantic_io_checks.hpp"
#include "engine_initialization_fixtures.hpp"
#include <numeric>

void engine_initialization_transfer_checks(const std::filesystem::path&);

namespace {
using namespace cm::semantic_io_checks;
namespace init = cm::aot::engine_initialization;
enum class Path { direct, generated, cfm };
enum class Mode { modern, global, entry, profile };
std::array<unsigned, 20> comparisons{}, direct{}, generated{}, cfm{}, negatives{};
struct Imports final : CodeFragmentBoundaryObserver {
    struct Event {
        bool leave; std::string name; decltype(State::registers) registers; std::uint32_t pc;
        bool operator==(const Event&) const = default;
    };
    std::vector<Event> events;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* n) override { events.push_back({false, n, s.registers, s.pc}); }
    void import_leave(const State& s, const char* n) override { events.push_back({true, n, s.registers, s.pc}); }
};
struct Boundary final : EngineInitializationBoundaryObserver {
    std::uint32_t root; bool corrupt;
    std::vector<std::uint32_t> entered;
    std::vector<std::pair<std::uint32_t, EngineInitializationOutcome>> left;
    Boundary(std::uint32_t entry, bool negative = false) : root(entry), corrupt(negative) {}
    void enter(const State&, std::uint32_t entry, bool) override { entered.push_back(entry); }
    void leave(State& s, std::uint32_t entry, bool original, EngineInitializationOutcome outcome) override {
        left.emplace_back(entry, outcome);
        if (corrupt && entry == root && !original && outcome == EngineInitializationOutcome::returned) s.wr<4>(12, s.r<4>(12) ^ 1);
    }
};
std::string execute(State& s, unsigned index, Path path) {
    try {
        s.wr<4>(4128, return_sentinel); s.pc = init::entries[index];
        if (path == Path::direct) require(try_modern_engine_initialization(s, s.pc), "Direct initializer adapter handles entry");
        else {
            if (path == Path::cfm) { s.wr<4>(48, fixture); s.pc = 0x10781320; }
            run_until(s, return_sentinel);
        }
    } catch (const std::exception& e) { return std::string(e.what()) + " at PC " + hex_address(s.pc); }
    return {};
}
void compare(unsigned index, const State& seed, Path path, Mode mode, bool negative = false) {
    auto a = clone(seed), b = clone(seed);
    cm::engine_initialization_checks::NativeEnvironment environment(*a, *b);
    cm::engine_initialization_checks::prepare_initializer(*a, *b, index);
    a->date_subsystem_original = b->date_subsystem_original = seed.date_subsystem_original;
    a->runtime_lifecycle_original = b->runtime_lifecycle_original = seed.runtime_lifecycle_original;
    a->cpp_object_storage_original = b->cpp_object_storage_original = seed.cpp_object_storage_original;
    auto& old = engine_initialization_diagnostics(*a); auto& modern = engine_initialization_diagnostics(*b);
    old.original = true;
    if (mode == Mode::global) modern.original = true;
    if (mode == Mode::entry) modern.entry_original[index] = true;
    Imports oi, mi; Boundary ob(init::entries[index]), mb(init::entries[index], negative);
    Effects x, y; x.state = a.get(); y.state = b.get();
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &oi; b->code_fragment_observer = &mi;
    old.observer = &ob; modern.observer = &mb;
    const auto ae = execute(*a, index, path), be = execute(*b, index, path);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr; old.observer = modern.observer = nullptr;
    const auto label = hex_address(init::entries[index]) + " production path=" + std::to_string(unsigned(path));
    require(ae == be && ae.empty(), label + " original=" + ae + " modern=" + be);
    require(ob.entered == mb.entered && ob.left == mb.left && !modern.original_body && !modern.dispatch_escapes,
        label + " balanced per-entry boundaries and no escape");
    if (negative) {
        bool rejected = false; try { Image(*a).compare(*b, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " production ABI negative accepted"); ++negatives[index];
    } else {
        Image(*a).compare(*b, label);
        require(x.bytes == y.bytes && oi.events == mi.events, label + " complete ordered guest accesses and import boundaries");
    }
    require(a->executed == b->executed, label + " exact initializer/dependency accounting");
    const bool original = mode == Mode::entry || mode == Mode::global || mode == Mode::profile;
    require(modern.calls[index] == unsigned(!original) && modern.returns[index] == unsigned(!original) &&
        modern.original_calls[index] == unsigned(mode == Mode::entry || mode == Mode::global) &&
        modern.profile_original_calls[index] == unsigned(mode == Mode::profile) &&
        !modern.faults[index] && !modern.budget_stops[index] && !modern.transfers[index], label + " dedicated entry evidence");
    if (mode == Mode::modern && !negative) {
        require(!std::accumulate(modern.original_calls.begin(), modern.original_calls.end(), UINT64_C(0)) &&
            !std::accumulate(modern.profile_original_calls.begin(), modern.profile_original_calls.end(), UINT64_C(0)),
            label + " default path executes only semantic initializers");
        if (path == Path::direct) ++direct[index];
        if (path == Path::generated) ++generated[index];
        if (path == Path::cfm) ++cfm[index];
    }
    ++comparisons[index];
}
void composed(const State& seed) {
    for (unsigned mode = 0; mode < 4; ++mode) {
        auto source = clone(seed);
        if (mode == 1) source->date_subsystem_original = true;
        if (mode == 2) source->runtime_lifecycle_original = true;
        if (mode == 3) source->cpp_object_storage_original = true;
        compare(0, *source, Path::generated, Mode::modern);
    }
    auto state = clone(seed);
    auto peer = clone(seed);
    cm::engine_initialization_checks::NativeEnvironment environment(*peer, *state);
    require(try_modern_engine_initialization(*state, init::entries[0]), "Complete initialization composition");
    const auto& d = engine_initialization_diagnostics(*state);
    for (unsigned n = 0; n < init::entries.size(); ++n) {
        if (init::entries[n] == 0x100939b0) {
            const auto& contracts = contract_manager_diagnostics(*state);
            require(contracts.date_initialization_calls == 1 && contracts.date_initialization_returns == 1 &&
                !d.calls[n] && !d.returns[n], "Contract manager owns the completed date initializer");
            continue;
        }
        require(d.calls[n] == 1 && d.returns[n] == 1 && !d.original_calls[n] && !d.faults[n],
            "Real dependency composition executes each initializer exactly once");
    }
    require(state->pc == return_sentinel && state->r<4>(4) == seed.r<4>(4), "Complete root returns to its original caller");
    std::cout << "Complete engine initialization: all 20 entries once, original dependencies retained, four mixed modes\n";
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2 || (argc == 3 && (std::string_view(argv[2]) == "--profile" || std::string_view(argv[2]) == "--transfer")), "Explicit initializer source/mode");
        if (argc == 3 && std::string_view(argv[2]) == "--transfer") { engine_initialization_transfer_checks(argv[1]); return 0; }
        const bool profile = argc == 3;
        auto seed = std::make_unique<State>(); load_program(*seed, argv[1]);
        seed->budget = 100000000; seed->memory.map(fixture, 16);
        for (unsigned i = 0; i < init::entries.size(); ++i) {
            seed->memory.write<4>(fixture, init::entries[i]); seed->memory.write<4>(fixture + 4, seed->r<4>(8));
            if (init::entries[i] == 0x100939b0) {
                // Dispatch now belongs to CONTRACT_MANAGER, including the
                // legacy adapter's dispatch-based "original" path. Its
                // independent reference belongs in that module's fixtures;
                // the complete-root case below checks real composition.
                std::cout << hex_address(init::entries[i]) << " transferred to CONTRACT_MANAGER fixtures\n";
                continue;
            }
            if (profile) compare(i, *seed, Path::generated, Mode::profile);
            else {
                for (auto path : {Path::direct, Path::generated, Path::cfm}) compare(i, *seed, path, Mode::modern);
                compare(i, *seed, Path::generated, Mode::global); compare(i, *seed, Path::generated, Mode::entry);
                compare(i, *seed, Path::direct, Mode::modern, true);
            }
            std::cout << hex_address(init::entries[i]) << " comparisons=" << comparisons[i] << " direct=" << direct[i]
                << " generated=" << generated[i] << " cfm=" << cfm[i] << " negatives=" << negatives[i] << '\n';
        }
        if (!profile) composed(*seed);
        std::cout << "Production initializer comparisons=" << std::accumulate(comparisons.begin(), comparisons.end(), 0u) << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
