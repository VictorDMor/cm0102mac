#include "record_history_fixtures.hpp"
#include "code_fragment_adapter.hpp"
#include "application_exit.hpp"
#include <numeric>

void record_history_transfer_checks(const std::filesystem::path&);

namespace {
using namespace cm::record_history_checks;
namespace record = cm::aot::record_history;
enum class Path { direct, generated, cfm };
enum class Mode { modern, global, entry, profile };
std::array<unsigned, 2> comparisons{}, direct{}, generated{}, cfm{}, negatives{};
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
struct Boundary final : RecordHistoryBoundaryObserver {
    std::uint32_t root; bool corrupt, corrupt_child = false;
    std::vector<std::uint32_t> entered, pending;
    std::vector<std::pair<std::uint32_t, RecordHistoryOutcome>> left;
    Boundary(std::uint32_t entry, bool negative = false) : root(entry), corrupt(negative) {}
    void enter(const State&, std::uint32_t entry, bool) override { entered.push_back(entry); pending.push_back(entry); }
    void leave(State& s, std::uint32_t entry, bool original, RecordHistoryOutcome outcome) override {
        require(!pending.empty() && pending.back() == entry, "Nested record boundary order"); pending.pop_back();
        left.emplace_back(entry, outcome);
        if (corrupt && pending.empty() && entry == root && !original && outcome == RecordHistoryOutcome::returned) s.wr<4>(12, s.r<4>(12) ^ 1);
        if (corrupt_child && entry == record::entries[1] && !original && outcome == RecordHistoryOutcome::returned) {
            const auto owner = std::uint32_t(s.r<4>(23 * 4));
            const auto row = owner + s.memory.read<2>(owner + 0x2c58) * record::row_stride;
            const auto node = s.memory.read<4>(row + 16); s.memory.write<4>(node, s.memory.read<4>(node) ^ 1);
        }
    }
};
std::string execute(State& s, unsigned index, Path path) {
    try {
        s.wr<4>(4128, return_sentinel); s.pc = record::entries[index];
        if (path == Path::direct) require(try_modern_record_history(s, s.pc), "Direct record adapter handles entry");
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
void compare(unsigned index, const State& seed, Path path, Mode mode, bool negative = false, unsigned kind = 0,
             unsigned scenario = 0, bool child_negative = false) {
    auto a = clone(seed), b = clone(seed);
    cm::engine_initialization_checks::NativeEnvironment environment(*a, *b);
    const auto owner_a = prepare(*a, index, kind, scenario), owner_b = prepare(*b, index, kind, scenario);
    require(owner_a == owner_b, "Identical actual record owner");
    Image(*a).compare(*b, "Identical original record prerequisite");
    require(a->executed == b->executed, "Identical original record setup accounting");
    auto& old = record_history_diagnostics(*a); auto& modern = record_history_diagnostics(*b);
    old.original = true;
    if (mode == Mode::global) modern.original = true;
    if (mode == Mode::entry) modern.entry_original[index] = true;
    const auto initial_row = owner_b + record::row_stride;
    const auto initial_head = b->memory.read<4>(initial_row + 8), initial_tail = b->memory.read<4>(initial_row + 12);
    const auto head_next = initial_head ? b->memory.read<4>(initial_head + 0x1f8) : 0;
    const auto allocated_before = b->allocated;
    Imports oi, mi; Boundary ob(record::entries[index]), mb(record::entries[index], negative && !child_negative);
    mb.corrupt_child = child_negative;
    Effects x, y; x.state = a.get(); y.state = b.get();
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &oi; b->code_fragment_observer = &mi;
    old.observer = &ob; modern.observer = &mb;
    const auto ae = execute(*a, index, path), be = execute(*b, index, path);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr; old.observer = modern.observer = nullptr;
    const auto label = hex_address(record::entries[index]) + " production path=" + std::to_string(unsigned(path)) + " scenario=" + std::to_string(scenario);
    require(ae == be && ae.empty(), label + " original=" + ae + " modern=" + be);
    require(ob.pending.empty() && mb.pending.empty() && ob.entered.size() == ob.left.size() && mb.entered.size() == mb.left.size() && !modern.original_body && !modern.dispatch_escapes,
        label + " balanced per-entry boundaries and no escape");
    if (negative) {
        bool rejected = false; try { Image(*a).compare(*b, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " production ABI negative accepted"); ++negatives[index];
        if (child_negative) require(x.bytes != y.bytes && modern.calls[1] > 0, "Production parent rejects a semantic child field mutation");
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
    if (!negative && scenario == 0) {
        require(b->r<4>(12) == 1 && b->memory.read<2>(owner_b + 0x2c58) == 1, "Actual successful history return and selected row");
        const auto first = owner_b + record::row_stride;
        const auto row = first + (index == 0 && kind ? record::record_stride : 0);
        const auto node = b->memory.read<4>(row + 16);
        require(node != 0, label + " requires an actual current history node");
        require(node && b->memory.read<4>(node) == 9 && b->memory.read<4>(node + 8) == fixture + 0x40 &&
            b->memory.read<1>(node + 16) == (kind ? 0x80 : 0) && b->memory.read<4>(node + 12) == fixture + 0x60 &&
            b->memory.read<2>(row + 22) == 2 && b->memory.read<4>(row + 12) == node &&
            b->memory.read<4>(b->memory.read<4>(node + 0x1f4) + 0x1f8) == node,
            label + " kind=" + std::to_string(kind) + " independent history fields:" +
            " node=" + hex_address(node) + " id=" + std::to_string(b->memory.read<4>(node)) +
            " aux=" + hex_address(b->memory.read<4>(node + 8)) + " payload=" + hex_address(b->memory.read<4>(node + 12)) +
            " mark=" + std::to_string(b->memory.read<1>(node + 16)) + " count=" + std::to_string(b->memory.read<2>(row + 22)) +
            " tail=" + hex_address(b->memory.read<4>(row + 12)) + " previous=" + hex_address(b->memory.read<4>(node + 0x1f4)));
        if (index == 0) require(b->memory.read<4>(first + 4) == 11 && b->memory.read<4>(first + record::record_stride + 4) == 22 &&
            b->memory.read<2>(first + 22) == (kind ? 1 : 2), "Actual cache exchange restores order and updates only matching records");
    }
    if (!negative && scenario != 0) {
        const auto current = b->memory.read<4>(initial_row + 16);
        require(b->memory.read<2>(owner_b + 0x2c58) == 1, "Actual history scenario retains its selected row");
        if (scenario == 2 || scenario == 3 || scenario == 4) {
            require(b->r<4>(12) == 0 && b->allocated == allocated_before &&
                current == (scenario == 2 ? initial_tail : scenario == 3 ? initial_head : initial_tail),
                "Marked reuse/restoration and marked-head refusal return zero without allocating");
        } else {
            require(b->r<4>(12) == 1 && b->memory.read<4>(current) == 9 && b->memory.read<4>(initial_row + 12) == current,
                "Actual truncation, trimming or fallback appends the new current node");
            if (scenario == 1) require(b->memory.read<2>(initial_row + 22) == 2 &&
                b->memory.read<4>(current + 0x1f4) == initial_head && b->memory.read<4>(initial_head + 0x1f8) == current,
                "Owned forward-history disposal retains the preceding node and exact count");
            if (scenario == 5) require(b->memory.read<2>(initial_row + 22) == 101 && b->memory.read<4>(initial_row + 8) == head_next &&
                b->memory.read<4>(head_next + 0x1f4) == 0 && b->memory.read<4>(current + 0x1f4) == initial_tail,
                "Original 100 threshold trims one unmarked head then appends, retaining 101 nodes");
            if (scenario == 6) require(b->memory.read<2>(initial_row + 22) == 1 && b->memory.read<1>(current + 16) == 0x80 &&
                b->memory.read<4>(current + 20) == fixture + 0x40 && b->memory.read<4>(current + 28) == fixture + 0x60 &&
                !b->memory.read<1>(current + 24) && !b->memory.read<1>(current + 32),
                "Published-argument fallback initializes borrowed fields through the actual field setter");
        }
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
    prepare(*a, 1); prepare(*b, 1);
    auto& old = record_history_diagnostics(*a); auto& modern = record_history_diagnostics(*b);
    old.original = true;
    Boundary ob(record::entries[1]), mb(record::entries[1]); old.observer = &ob; modern.observer = &mb;
    ExitOnWrite x, y; x.effects.state = a.get(); y.effects.state = b.get();
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    for (auto* state : {a.get(), b.get()}) {
        bool exited = false;
        try { (void)execute(*state, 1, Path::direct); } catch (const ApplicationExit&) { exited = true; }
        require(exited, "Nonstandard application exit propagates from record memory callback");
        state->memory.date_observer = nullptr;
    }
    old.observer = modern.observer = nullptr;
    Image(*a).compare(*b, "Application exit preserves the original interrupted prefix");
    require(a->executed == b->executed && x.effects.bytes == y.effects.bytes && ob.entered == mb.entered && ob.left == mb.left,
        "Application exit keeps accounting, memory effects and balanced boundaries");
    require(modern.calls[1] == 1 && modern.terminations[1] == 1 && !modern.returns[1] && !modern.faults[1] &&
        !modern.original_body && !old.original_body, "Exit is classified separately and original selection unwinds");
    std::cout << "Record application-exit prefix and boundary classification: PASS\n";
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2 || (argc == 3 && (std::string_view(argv[2]) == "--profile" || std::string_view(argv[2]) == "--transfer")),
            "Explicit record source/mode");
        if (argc == 3 && std::string_view(argv[2]) == "--transfer") { record_history_transfer_checks(argv[1]); return 0; }
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
                if (i == 0) compare(i, *seed, Path::generated, Mode::modern, true, 0, 0, true);
                if (i == 1) for (unsigned scenario = 1; scenario <= 6; ++scenario)
                    compare(i, *seed, Path::generated, Mode::modern, false, 0, scenario);
            }
            std::cout << hex_address(record::entries[i]) << " comparisons=" << comparisons[i] << " direct=" << direct[i]
                << " generated=" << generated[i] << " cfm=" << cfm[i] << " negatives=" << negatives[i] << '\n';
        }
        if (!profile) process_exit(*seed);
        std::cout << "Production record comparisons=" << std::accumulate(comparisons.begin(), comparisons.end(), 0u) << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
