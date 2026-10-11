#include "engine_initialization_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <set>
#include <utility>

namespace cm::aot { bool engine_initialization_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace init = cm::aot::engine_initialization;
constexpr std::uint32_t caller = 0x7fff1000, relocated_toc = 0x32010000, objects = 0x33000000;
std::array<unsigned, 20> comparisons{}, faults{}, negatives{};
unsigned budget_stops = 0;
struct Monitor final : MemoryObserver {
    Effects effects;
    State& state;
    std::function<void(State&, std::uint32_t)> on_write;
    explicit Monitor(State& s) : state(s) { effects.state = &s; s.memory.date_observer = this; }
    ~Monitor() { state.memory.date_observer = nullptr; }
    void mapped(std::uint32_t a, std::uint64_t n) override { effects.mapped(a, n); }
    void cleared(std::uint32_t a, std::uint64_t n) override { effects.cleared(a, n); }
    void read(std::uint32_t a, const std::uint8_t* bytes, unsigned n) override { effects.read(a, bytes, n); }
    void written(std::uint32_t a, const std::uint8_t* bytes, std::size_t n) override {
        effects.written(a, bytes, n); if (on_write) on_write(state, a);
    }
};
struct Dependencies final : init::DependencyDispatch {
    bool original;
    std::vector<std::array<std::uint32_t, 12>> calls;
    std::function<void(State&, std::uint32_t)> before;
    explicit Dependencies(bool reference) : original(reference) {}
    void reference(State& state, std::uint32_t continuation) {
        while (state.pc != continuation) {
            if (!engine_initialization_reference(state)) {
                constexpr std::array<std::uint32_t, 8> helpers{
                    0x10772df0, 0x10773320, 0x10715880, 0x107156f0,
                    0x10738c00, 0x1073ae10, 0x1074dec0, 0x1073c1b0};
                if (std::find(helpers.begin(), helpers.end(), state.pc) == helpers.end()) return;
                call(state, state.pc, std::uint32_t(state.r<4>(4128)));
            } else if (std::find(init::entries.begin(), init::entries.end(), state.pc) != init::entries.end()) {
                call(state, state.pc, std::uint32_t(state.r<4>(4128)));
            }
        }
    }
    void call(State& state, std::uint32_t target, std::uint32_t continuation) override {
        require(state.pc == target && state.r<4>(4128) == continuation && state.r<4>(2440) == state.r<4>(8),
            "Published dependency ABI before dispatch");
        calls.push_back({target, continuation, std::uint32_t(state.r<4>(4)), std::uint32_t(state.r<4>(8)),
            std::uint32_t(state.r<4>(12)), std::uint32_t(state.r<4>(16)), std::uint32_t(state.r<4>(20)),
            std::uint32_t(state.r<4>(24)), std::uint32_t(state.r<4>(28)), std::uint32_t(state.r<4>(124)),
            std::uint32_t(state.r<4>(2440)), state.pc});
        if (before) before(state, target);
        const auto child = std::find(init::entries.begin(), init::entries.end(), target);
        if (child != init::entries.end()) {
            if (original) reference(state, continuation);
            else init::invoke(state, unsigned(child - init::entries.begin()), this);
            return;
        }
        // Controlled external constructors make the boundary contract observable
        // independently of the production implementations of those dependencies.
        state.step(target);
        const auto object = std::uint32_t(state.r<4>(12)), record = std::uint32_t(state.r<4>(20));
        if (target == 0x10772df0) {
            state.memory.write<4>(record, state.r<4>(12));
            state.memory.write<4>(record + 4, state.r<4>(16));
            state.memory.write<4>(record + 8, 0xdecafbad);
        } else if (target == 0x10715880 || target == 0x107156f0) {
            state.memory.write<2>(object, target == 0x107156f0 ? 0xff80 : 0);
            state.memory.write<2>(object + 2, target == 0x107156f0 ? state.r<4>(24) : 0);
            state.memory.write<1>(object + 4, target == 0x107156f0 ? state.r<4>(16) : 0);
        } else {
            require(target == 0x10773320 || target == 0x10738c00 || target == 0x1073ae10 ||
                target == 0x1074dec0 || target == 0x1073c1b0, "Known external initializer dependency");
            state.memory.write<4>(object, target);
            state.memory.write<4>(object + 4, state.r<4>(16));
            state.memory.write<4>(object + 8, state.r<4>(24));
        }
        // Returned r3 is deliberately distinct for object constructors. The
        // registration facade must observe it rather than reload the global.
        if (target == 0x10738c00 || target == 0x1073ae10 || target == 0x1074dec0 || target == 0x1073c1b0)
            state.wr<4>(12, object + 4);
        state.wr<4>(0, 0x24681357); ++state.native_calls;
        state.step(target + 4); state.pc = std::uint32_t(state.r<4>(4128));
    }
};
std::unique_ptr<State> initial(unsigned index) {
    auto state = std::make_unique<State>(); auto& s = *state;
    s.registers.fill(0x6d); s.unique.fill(0xb3);
    for (auto [address, length] : std::array<std::pair<std::uint32_t, unsigned>, 3>{
        {{relocated_toc - 0x8000, 0x8000}, {stack - 1024, 2048}, {objects, 0x40000}}}) {
        s.memory.map(address, length); s.memory.copy_in(address, std::vector<std::uint8_t>(length, 0xa7));
    }
    for (unsigned offset = 0; offset < 0x8000; offset += 4)
        s.memory.write<4>(relocated_toc - 0x8000 + offset, objects + offset * 4);
    s.wr<4>(4, stack); s.wr<4>(8, relocated_toc); s.wr<4>(4128, caller);
    s.pc = init::entries[index]; s.budget = 100000; s.native_calls = 17; return state;
}
using Configure = std::function<void(Monitor&, Dependencies&)>;
std::vector<Effects::Access> compare(unsigned index, const State& seed, const std::string& name,
    Configure configure = {}, std::function<void(const State&, const Dependencies&)> expectation = {},
    bool negative = false) {
    auto a = clone(seed), b = clone(seed);
    Monitor x(*a), y(*b); Dependencies original(true), modern(false);
    if (configure) { configure(x, original); configure(y, modern); }
    std::string ae, be;
    try { original.reference(*a, caller); } catch (const std::exception& e) { ae = e.what(); }
    try { init::invoke(*b, index, &modern); } catch (const std::exception& e) { be = e.what(); }
    if (negative) b->wr<4>(4128, b->r<4>(4128) ^ 4);
    const auto label = hex_address(init::entries[index]) + " " + name;
    if (negative) {
        bool rejected = false;
        try { Image(*a).compare(*b, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " own ABI negative accepted"); ++negatives[index]; return {};
    }
    require(ae == be, label + " fault mismatch: " + ae + " / " + be);
    Image(*a).compare(*b, label);
    require(a->executed == b->executed && x.effects.bytes == y.effects.bytes && original.calls == modern.calls,
        label + " instruction accounting, ordered accesses or dependency arguments differ");
    if (!ae.empty()) { ++faults[index]; if (ae == "Native instruction budget exhausted") ++budget_stops; }
    else if (expectation) expectation(*b, modern);
    ++comparisons[index]; return x.effects.log;
}
std::unique_ptr<State> minimal(const State& seed, const std::vector<Effects::Access>& accesses,
                               std::uint32_t excluded = UINT32_MAX) {
    auto s = std::make_unique<State>(); s->registers = seed.registers; s->unique = seed.unique; s->pc = seed.pc;
    s->executed = seed.executed; s->budget = seed.budget; s->native_calls = seed.native_calls;
    s->heap_end = seed.heap_end; s->allocated = seed.allocated; s->available = seed.available; s->memory_error = seed.memory_error;
    std::set<std::uint32_t> bytes;
    for (const auto& access : accesses)
        for (std::uint64_t n = 0; n < access.size; ++n) bytes.insert(access.address + std::uint32_t(n));
    for (auto address : bytes) if (address != excluded) {
        s->memory.map(address, 1); s->memory.copy_in(address, seed.memory.copy_out(address, 1));
    }
    return s;
}
void matrix(unsigned index) {
    auto seed = initial(index);
    const auto accesses = compare(index, *seed, "normal", {}, [index](const State& s, const Dependencies& d) {
        require(s.pc == caller && s.r<4>(4) == stack && s.r<4>(4128) == caller && s.r<4>(8) == relocated_toc &&
            !d.calls.empty(), "Independent return ABI and nonempty dependency coverage");
        if (index == 3 || index == 15) require(s.r<4>(124) == 0x6d6d6d6d, "Nonvolatile owner restored");
        if (index == 0) {
            std::vector<std::uint32_t> children;
            for (const auto& c : d.calls) if (std::find(init::entries.begin(), init::entries.end(), c[0]) != init::entries.end()) children.push_back(c[0]);
            require(children == std::vector<std::uint32_t>(init::entries.begin() + 1, init::entries.end()), "Nineteen child initializers in original order");
        }
        if (index == 3) {
            const auto object = std::uint32_t(s.memory.read<4>(relocated_toc - 0x79e8));
            require(s.memory.read<2>(object) == 0xff80 && s.memory.read<2>(object + 2) == 2001 &&
                s.memory.read<1>(object + 4) == 31, "Signed-day copy, independent field publication");
        }
        if (index == 5 || index == 6) {
            require(d.calls.front()[7] == (index == 5 ? 0x78u : 0x4cu) && d.calls.front()[8] == (index == 5 ? 0xa8cu : 34u) &&
                d.calls.front()[6] == 0, "Independent array size, count and null destructor");
        }
        if (index == 9 || index == 12 || index == 17)
            require(d.calls.size() == 2 && d.calls[1][4] == d.calls[0][4] + 4, "Registration uses constructor return");
        if (index == 15)
            require(d.calls.size() == 4 && d.calls[0][5] == 50000 && d.calls[2][5] == 3072 &&
                d.calls[1][5] == d.calls[3][5], "Independent buffer requests and saved destructor owner");
    });
    compare(index, *seed, "own-link-negative", {}, {}, true);
    for (unsigned offset : {1u, 3u}) {
        auto s = clone(*seed); s->wr<4>(4, stack + offset); compare(index, *s, "unaligned-stack");
    }
    compare(index, *seed, "live-sp-after-saved-link", [](Monitor& m, Dependencies&) {
        m.on_write = [](State& s, std::uint32_t address) { if (address == stack + 8) s.wr<4>(4, stack + 256); };
    });
    compare(index, *seed, "cached-frame-update", [](Monitor& m, Dependencies&) {
        m.on_write = [](State& s, std::uint32_t address) { if (address == stack - 64 || address == stack - 80) s.wr<4>(4, stack + 256); };
    });
    compare(index, *seed, "live-toc-after-dependency", [](Monitor&, Dependencies& d) {
        d.before = [](State& s, std::uint32_t) { s.wr<4>(8, relocated_toc + 4); };
    });
    compare(index, *seed, "dependency-fault-prefix", [](Monitor&, Dependencies& d) {
        d.before = [](State& s, std::uint32_t) { s.wr<4>(0, 0x12345678); throw std::runtime_error("Fixture initializer dependency fault"); };
    });
    auto aliased = clone(*seed);
    // All TOC reads now overlap the stack frame/save area. Values remain valid
    // guest object addresses until the original stores replace them.
    aliased->wr<4>(8, stack + 0x7be0); aliased->memory.map(stack + 0x7be0 - 0x8000, 0x8000);
    for (unsigned n = 0; n < 0x8000; n += 4) aliased->memory.write<4>(stack + 0x7be0 - 0x8000 + n, objects + n * 4);
    compare(index, *aliased, "toc-stack-alias");
    // Every checkpoint is a possible finite stop, including composed children.
    auto complete = clone(*seed); Dependencies d(false); init::invoke(*complete, index, &d);
    for (std::uint64_t n = 0; n <= complete->executed; ++n) {
        auto s = clone(*seed); s->budget = n; compare(index, *s, "budget=" + std::to_string(n));
    }
    const auto tiny = minimal(*seed, accesses); compare(index, *tiny, "minimal-mapping");
    std::set<std::uint32_t> bytes;
    for (const auto& access : accesses) for (std::uint64_t n = 0; n < access.size; ++n) bytes.insert(access.address + std::uint32_t(n));
    for (auto byte : bytes) { const auto hole = minimal(*seed, accesses, byte); compare(index, *hole, "one-byte-hole=" + hex_address(byte)); }
}
void diagnostics() {
    auto a = initial(1), b = initial(1);
    require(!a->runtime_diagnostics && !b->runtime_diagnostics, "Diagnostics are lazy and owned per State");
    auto& d = engine_initialization_diagnostics(*a); d.original = true; d.entry_original[4] = true;
    d.calls[3] = 77; a->native_service_calls[0] = 39;
    require(!engine_initialization_diagnostics(std::as_const(*b)).calls[3] && !b->runtime_diagnostics,
        "Const diagnostics query is allocation-free and separate");
    reset_engine_initialization_diagnostics(*a);
    require(!d.original && !d.entry_original[4] && !d.calls[3] && a->native_service_calls[0] == 39,
        "Independent reset preserves previously delivered service diagnostics");
}
}
int main() {
    try {
        diagnostics(); for (unsigned i = 0; i < init::entries.size(); ++i) matrix(i);
        unsigned total = 0, failed = 0, negative = 0;
        for (unsigned i = 0; i < init::entries.size(); ++i) {
            require(comparisons[i] && faults[i] && negatives[i] == 1, "Dedicated per-entry component coverage");
            std::cout << hex_address(init::entries[i]) << " comparisons=" << comparisons[i] << " faults=" << faults[i] << " negatives=" << negatives[i] << '\n';
            total += comparisons[i]; failed += faults[i]; negative += negatives[i];
        }
        std::cout << "Engine initialization: comparisons=" << total << " faults=" << failed << " budget_stops=" << budget_stops << " negatives=" << negative << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
