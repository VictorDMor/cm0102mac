#include "utility_object_fixtures.hpp"
#include "code_fragment_adapter.hpp"
#include "application_exit.hpp"
#include <numeric>

void utility_object_transfer_checks(const std::filesystem::path&);

namespace {
using namespace cm::utility_object_checks;
namespace object = cm::aot::utility_object;
enum class Path { direct, generated, cfm };
enum class Mode { modern, global, entry, profile };
std::array<unsigned, 11> comparisons{}, direct{}, generated{}, cfm{}, negatives{};
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
struct Boundary final : UtilityObjectBoundaryObserver {
    std::uint32_t root; bool corrupt;
    std::vector<std::uint32_t> entered;
    std::vector<std::pair<std::uint32_t, UtilityObjectOutcome>> left;
    Boundary(std::uint32_t entry, bool negative = false) : root(entry), corrupt(negative) {}
    void enter(const State&, std::uint32_t entry, bool) override { entered.push_back(entry); }
    void leave(State& s, std::uint32_t entry, bool original, UtilityObjectOutcome outcome) override {
        left.emplace_back(entry, outcome);
        if (corrupt && entry == root && !original && outcome == UtilityObjectOutcome::returned) s.wr<4>(12, s.r<4>(12) ^ 1);
    }
};
std::string execute(State& s, unsigned index, Path path) {
    try {
        s.wr<4>(4128, return_sentinel); s.pc = object::entries[index];
        if (path == Path::direct) require(try_modern_utility_object(s, s.pc), "Direct utility adapter handles entry");
        else {
            if (path == Path::cfm) {
                s.memory.write<4>(fixture, s.pc); s.memory.write<4>(fixture + 4, s.r<4>(8));
                s.wr<4>(48, fixture); s.pc = 0x10781320;
            }
            run_until(s, return_sentinel);
        }
    } catch (const std::exception& e) { return std::string(e.what()) + " at PC " + hex_address(s.pc); }
    return {};
}
void compare(unsigned index, const State& seed, Path path, Mode mode, bool negative = false) {
    auto a = clone(seed), b = clone(seed);
    cm::engine_initialization_checks::NativeEnvironment environment(*a, *b);
    prepare(*a, index); prepare(*b, index);
    Image(*a).compare(*b, "Identical original utility prerequisite");
    require(a->executed == b->executed, "Identical original utility setup accounting");
    auto& old = utility_object_diagnostics(*a); auto& modern = utility_object_diagnostics(*b);
    old.original = true;
    if (mode == Mode::global) modern.original = true;
    if (mode == Mode::entry) modern.entry_original[index] = true;
    Imports oi, mi; Boundary ob(object::entries[index]), mb(object::entries[index], negative);
    Effects x, y; x.state = a.get(); y.state = b.get();
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &oi; b->code_fragment_observer = &mi;
    old.observer = &ob; modern.observer = &mb;
    const auto ae = execute(*a, index, path), be = execute(*b, index, path);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr; old.observer = modern.observer = nullptr;
    const auto label = hex_address(object::entries[index]) + " production path=" + std::to_string(unsigned(path));
    require(ae == be && ae.empty(), label + " original=" + ae + " modern=" + be);
    require(ob.entered == mb.entered && ob.left == mb.left && !modern.original_body && !modern.dispatch_escapes,
        label + " balanced per-entry boundaries and no escape");
    if (negative) {
        bool rejected = false; try { Image(*a).compare(*b, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " production ABI negative accepted"); ++negatives[index];
    } else {
        Image(*a).compare(*b, label);
        require(x.bytes == y.bytes && oi.events == mi.events, label + " ordered accesses and actual import boundaries");
    }
    require(a->executed == b->executed, label + " exact lifetime/dependency accounting");
    const bool original = mode != Mode::modern;
    require(modern.calls[index] == unsigned(!original) && modern.returns[index] == unsigned(!original) &&
        modern.original_calls[index] == unsigned(mode == Mode::entry || mode == Mode::global) &&
        modern.profile_original_calls[index] == unsigned(mode == Mode::profile) &&
        !modern.faults[index] && !modern.budget_stops[index] && !modern.transfers[index] && !modern.terminations[index],
        label + " dedicated entry evidence");
    if (mode == Mode::modern && !negative) {
        require(!std::accumulate(modern.original_calls.begin(), modern.original_calls.end(), UINT64_C(0)) &&
            !std::accumulate(modern.profile_original_calls.begin(), modern.profile_original_calls.end(), UINT64_C(0)),
            label + " default path executes semantic utility objects");
        if (path == Path::direct) ++direct[index];
        if (path == Path::generated) ++generated[index];
        if (path == Path::cfm) ++cfm[index];
    }
    ++comparisons[index];
}
struct ExitOnWrite final : MemoryObserver {
    Effects effects;
    void mapped(std::uint32_t address, std::uint64_t size) override { effects.mapped(address, size); }
    void cleared(std::uint32_t address, std::uint64_t size) override { effects.cleared(address, size); }
    void read(std::uint32_t address, const std::uint8_t* bytes, unsigned size) override { effects.read(address, bytes, size); }
    void written(std::uint32_t address, const std::uint8_t* bytes, std::size_t size) override {
        effects.written(address, bytes, size);
        throw ApplicationExit{};
    }
};
void process_exit(const State& seed) {
    auto a = clone(seed), b = clone(seed);
    cm::engine_initialization_checks::NativeEnvironment environment(*a, *b);
    prepare(*a, 5); prepare(*b, 5);
    auto& old = utility_object_diagnostics(*a); auto& modern = utility_object_diagnostics(*b);
    old.original = true;
    Boundary ob(object::entries[5]), mb(object::entries[5]); old.observer = &ob; modern.observer = &mb;
    ExitOnWrite x, y; x.effects.state = a.get(); y.effects.state = b.get();
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    for (auto* state : {a.get(), b.get()}) {
        bool exited = false;
        try { (void)execute(*state, 5, Path::direct); } catch (const ApplicationExit&) { exited = true; }
        require(exited, "Nonstandard application exit propagates from utility memory callback");
        state->memory.date_observer = nullptr;
    }
    old.observer = modern.observer = nullptr;
    Image(*a).compare(*b, "Application exit preserves the original interrupted prefix");
    require(a->executed == b->executed && x.effects.bytes == y.effects.bytes && ob.entered == mb.entered && ob.left == mb.left,
        "Application exit keeps accounting, memory effects and balanced boundaries");
    require(modern.calls[5] == 1 && modern.terminations[5] == 1 && !modern.returns[5] && !modern.faults[5] &&
        !modern.original_body && !old.original_body, "Exit is classified separately and original selection unwinds");
    std::cout << "Utility application-exit prefix and boundary classification: PASS\n";
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2 || (argc == 3 && (std::string_view(argv[2]) == "--profile" || std::string_view(argv[2]) == "--transfer")),
            "Explicit utility source/mode");
        if (argc == 3 && std::string_view(argv[2]) == "--transfer") { utility_object_transfer_checks(argv[1]); return 0; }
        const bool profile = argc == 3;
        auto seed = std::make_unique<State>(); load_program(*seed, argv[1]); seed->budget = 100000000;
        seed->memory.map(fixture, 0x1000); seed->memory.copy_in(fixture, std::vector<std::uint8_t>(0x1000, 0xa7));
        for (unsigned i = 0; i < object::entries.size(); ++i) {
            if (profile) compare(i, *seed, Path::generated, Mode::profile);
            else {
                for (auto path : {Path::direct, Path::generated, Path::cfm}) compare(i, *seed, path, Mode::modern);
                compare(i, *seed, Path::generated, Mode::global); compare(i, *seed, Path::generated, Mode::entry);
                compare(i, *seed, Path::direct, Mode::modern, true);
            }
            std::cout << hex_address(object::entries[i]) << " comparisons=" << comparisons[i] << " direct=" << direct[i]
                << " generated=" << generated[i] << " cfm=" << cfm[i] << " negatives=" << negatives[i] << '\n';
        }
        if (!profile) process_exit(*seed);
        std::cout << "Production utility comparisons=" << std::accumulate(comparisons.begin(), comparisons.end(), 0u) << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
