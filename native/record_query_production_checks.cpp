#include "record_query_fixtures.hpp"
#include "code_fragment_adapter.hpp"
#include "application_exit.hpp"
#include <numeric>

void record_query_transfer_checks(const std::filesystem::path&);

namespace {
using namespace cm::record_query_checks;
namespace record = cm::aot::record_query;
enum class Path { direct, generated, cfm };
enum class Mode { modern, global, entry, profile };
std::array<unsigned, 6> comparisons{}, direct{}, generated{}, cfm{}, negatives{};
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
struct Boundary final : RecordQueryBoundaryObserver {
    std::uint32_t root; bool corrupt;
    std::vector<std::uint32_t> entered, pending;
    std::vector<std::pair<std::uint32_t, RecordQueryOutcome>> left;
    Boundary(std::uint32_t entry, bool negative = false) : root(entry), corrupt(negative) {}
    void enter(const State&, std::uint32_t entry, bool) override { entered.push_back(entry); pending.push_back(entry); }
    void leave(State& s, std::uint32_t entry, bool original, RecordQueryOutcome outcome) override {
        require(!pending.empty() && pending.back() == entry, "Nested record boundary order"); pending.pop_back();
        left.emplace_back(entry, outcome);
        if (corrupt && pending.empty() && entry == root && !original && outcome == RecordQueryOutcome::returned) s.wr<4>(12, s.r<4>(12) ^ 1);
    }
};
std::string execute(State& s, unsigned index, Path path) {
    try {
        s.wr<4>(4128, return_sentinel); s.pc = record::entries[index];
        if (path == Path::direct) require(try_modern_record_query(s, s.pc), "Direct record adapter handles entry");
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
void compare(unsigned index, const State& seed, Path path, Mode mode, bool negative = false, unsigned kind = 0) {
    auto a = clone(seed), b = clone(seed);
    cm::engine_initialization_checks::NativeEnvironment environment(*a, *b);
    const auto owner_a = prepare(*a, index, kind), owner_b = prepare(*b, index, kind);
    require(owner_a == owner_b, "Identical actual record owner");
    Image(*a).compare(*b, "Identical original record prerequisite");
    require(a->executed == b->executed, "Identical original record setup accounting");
    auto& old = record_query_diagnostics(*a); auto& modern = record_query_diagnostics(*b);
    old.original = true;
    if (mode == Mode::global) modern.original = true;
    if (mode == Mode::entry) modern.entry_original[index] = true;
    Imports oi, mi; Boundary ob(record::entries[index]), mb(record::entries[index], negative);
    Effects x, y; x.state = a.get(); y.state = b.get();
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &oi; b->code_fragment_observer = &mi;
    old.observer = &ob; modern.observer = &mb;
    const auto ae = execute(*a, index, path), be = execute(*b, index, path);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr; old.observer = modern.observer = nullptr;
    const auto label = hex_address(record::entries[index]) + " production path=" + std::to_string(unsigned(path));
    require(ae == be && ae.empty(), label + " original=" + ae + " modern=" + be);
    require(ob.pending.empty() && mb.pending.empty() && ob.entered.size() == ob.left.size() && mb.entered.size() == mb.left.size() && !modern.original_body && !modern.dispatch_escapes,
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
    require((original ? !modern.calls[index] && !modern.returns[index] : modern.calls[index] > 0 && modern.calls[index] == modern.returns[index]) &&
        modern.original_calls[index] == unsigned(mode == Mode::entry || mode == Mode::global) &&
        modern.profile_original_calls[index] == unsigned(mode == Mode::profile) &&
        !modern.faults[index] && !modern.budget_stops[index] && !modern.transfers[index] && !modern.terminations[index],
        label + " dedicated entry evidence");
    if (mode == Mode::modern && !negative) {
        require(!std::accumulate(modern.original_calls.begin(), modern.original_calls.end(), UINT64_C(0)) &&
            !std::accumulate(modern.profile_original_calls.begin(), modern.profile_original_calls.end(), UINT64_C(0)),
            label + " default path executes semantic record objects");
        if (path == Path::direct) ++direct[index];
        if (path == Path::generated) ++generated[index];
        if (path == Path::cfm) ++cfm[index];
    }
    if (!negative && index == 0) require(b->r<4>(12) == owner_b &&
        b->memory.read<4>(owner_b + 0x2c00) == 0x87654321 && b->memory.read<4>(owner_b + 0x2c04) == 0x12345678 &&
        b->memory.read<1>(owner_b + 0x2c08) == 0x80 && b->memory.read<4>(owner_b + 0x2c0c) == fixture + 0x40 &&
        b->memory.read<4>(owner_b + 0x2c10) == UINT32_MAX && b->memory.read<4>(owner_b + 0x2c14) == 7 &&
        b->memory.read<2>(owner_b + 0x2c18) == 0x8001, "Actual argument publication and narrowing");
    if (!negative && index != 0) {
        const std::array<unsigned, 6> expected{0, 2 - kind, !kind, !kind, kind ? 0x80u : 0u, kind};
        require(b->r<4>(12) == expected[index], "Independent actual current-record query result");
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
    prepare(*a, 2); prepare(*b, 2);
    auto& old = record_query_diagnostics(*a); auto& modern = record_query_diagnostics(*b);
    old.original = true;
    Boundary ob(record::entries[2]), mb(record::entries[2]); old.observer = &ob; modern.observer = &mb;
    ExitOnWrite x, y; x.effects.state = a.get(); y.effects.state = b.get();
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    for (auto* state : {a.get(), b.get()}) {
        bool exited = false;
        try { (void)execute(*state, 2, Path::direct); } catch (const ApplicationExit&) { exited = true; }
        require(exited, "Nonstandard application exit propagates from record memory callback");
        state->memory.date_observer = nullptr;
    }
    old.observer = modern.observer = nullptr;
    Image(*a).compare(*b, "Application exit preserves the original interrupted prefix");
    require(a->executed == b->executed && x.effects.bytes == y.effects.bytes && ob.entered == mb.entered && ob.left == mb.left,
        "Application exit keeps accounting, memory effects and balanced boundaries");
    require(modern.calls[2] == 1 && modern.terminations[2] == 1 && !modern.returns[2] && !modern.faults[2] &&
        !modern.original_body && !old.original_body, "Exit is classified separately and original selection unwinds");
    std::cout << "Record application-exit prefix and boundary classification: PASS\n";
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2 || (argc == 3 && (std::string_view(argv[2]) == "--profile" || std::string_view(argv[2]) == "--transfer")),
            "Explicit record source/mode");
        if (argc == 3 && std::string_view(argv[2]) == "--transfer") { record_query_transfer_checks(argv[1]); return 0; }
        const bool profile = argc == 3;
        auto seed = std::make_unique<State>(); load_program(*seed, argv[1]); seed->budget = 100000000;
        seed->memory.map(fixture, 0x1000); seed->memory.copy_in(fixture, std::vector<std::uint8_t>(0x1000, 0xa7));
        for (unsigned i = 0; i < record::entries.size(); ++i) {
            if (profile) compare(i, *seed, Path::generated, Mode::profile);
            else {
                for (auto path : {Path::direct, Path::generated, Path::cfm})
                    for (unsigned kind = 0; kind < 2; ++kind) compare(i, *seed, path, Mode::modern, false, kind);
                compare(i, *seed, Path::generated, Mode::global); compare(i, *seed, Path::generated, Mode::entry);
                compare(i, *seed, Path::direct, Mode::modern, true);
            }
            std::cout << hex_address(record::entries[i]) << " comparisons=" << comparisons[i] << " direct=" << direct[i]
                << " generated=" << generated[i] << " cfm=" << cfm[i] << " negatives=" << negatives[i] << '\n';
        }
        if (!profile) process_exit(*seed);
        std::cout << "Production record comparisons=" << std::accumulate(comparisons.begin(), comparisons.end(), 0u) << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
