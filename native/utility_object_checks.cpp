#include "utility_object_adapter.hpp"
#include "engine_initialization_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <set>
#include <utility>

namespace cm::aot { bool utility_object_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace object = cm::aot::utility_object;
constexpr std::uint32_t caller = 0x7fff1000, relocated_toc = 0x32010000;
constexpr std::uint32_t node1 = fixture + 0x100, node2 = fixture + 0x140, payload = fixture + 0x800;
std::array<unsigned, 11> comparisons{}, faults{}, negatives{};
unsigned budget_stops = 0;
struct Monitor final : MemoryObserver {
    State& state; Effects effects;
    std::function<void(State&, std::uint32_t)> after_write;
    explicit Monitor(State& s) : state(s) { effects.state = &s; s.memory.date_observer = this; }
    ~Monitor() { state.memory.date_observer = nullptr; }
    void mapped(std::uint32_t a, std::uint64_t n) override { effects.mapped(a, n); }
    void cleared(std::uint32_t a, std::uint64_t n) override { effects.cleared(a, n); }
    void read(std::uint32_t a, const std::uint8_t* b, unsigned n) override { effects.read(a, b, n); }
    void written(std::uint32_t a, const std::uint8_t* b, std::size_t n) override {
        effects.written(a, b, n); if (after_write) after_write(state, a);
    }
};
struct Dependencies final : object::DependencyDispatch {
    bool original, allocation_fails = false;
    std::vector<std::array<std::uint32_t, 12>> calls;
    std::function<void(State&, std::uint32_t)> before;
    explicit Dependencies(bool source) : original(source) {}
    void reference(State& state, std::uint32_t continuation) {
        bool initial = true;
        while (state.pc != continuation) {
            const auto entry = std::find(object::entries.begin(), object::entries.end(), state.pc);
            if (!initial && entry != object::entries.end()) call(state, state.pc, std::uint32_t(state.r<4>(4128)));
            else if (!utility_object_reference(state)) call(state, state.pc, std::uint32_t(state.r<4>(4128)));
            else if (state.pc == state.r<4>(4128)) return;
            initial = false;
        }
    }
    void call(State& state, std::uint32_t target, std::uint32_t continuation) override {
        require(state.pc == target && state.r<4>(4128) == continuation && state.r<4>(2440) == state.r<4>(8),
            "Published utility dependency ABI");
        calls.push_back({target, continuation, std::uint32_t(state.r<4>(4)), std::uint32_t(state.r<4>(8)),
            std::uint32_t(state.r<4>(12)), std::uint32_t(state.r<4>(16)), std::uint32_t(state.r<4>(20)),
            std::uint32_t(state.r<4>(24)), std::uint32_t(state.r<4>(28)), std::uint32_t(state.r<4>(124)),
            std::uint32_t(state.r<4>(2440)), state.pc});
        if (before) before(state, target);
        const auto child = std::find(object::entries.begin(), object::entries.end(), target);
        if (child != object::entries.end()) {
            if (original) reference(state, continuation);
            else object::invoke(state, unsigned(child - object::entries.begin()), this);
            return;
        }
        state.step(target);
        const auto argument = std::uint32_t(state.r<4>(12));
        switch (target) {
        case 0x107735f0:
            if (!allocation_fails) state.memory.write<4>(payload + 8, argument);
            state.wr<4>(12, allocation_fails ? 0 : payload); break;
        case 0x10773610: case 0x107735a0:
            if (argument) state.memory.write<4>(argument + 12, target);
            break;
        case 0x10739ef0: {
            const auto count = std::uint32_t(state.r<4>(20));
            require(count <= 64, "Fixture rejects large signed node copy");
            state.memory.copy_in(argument, state.memory.copy_out(state.r<4>(16), count)); break;
        }
        case 0x1074ea30:
            // The actual tree algorithm belongs to the existing dispatcher.
            // Its arguments and an observable callback effect are controlled here.
            state.memory.write<4>(argument + 12, 0xdeadbeef); break;
        case 0x10739890:
            state.memory.write<4>(state.r<4>(24), 0x31415926);
            state.memory.write<4>(state.r<4>(28), 0x27182818); break;
        case 0x10768810: state.wr<4>(12, 0x1234); break;
        case 0x1077b720: case 0x10769b60: case 0x1076f7d0: break;
        default: throw std::runtime_error("Unknown controlled utility dependency " + hex_address(target));
        }
        state.wr<4>(0, 0x24681357); ++state.native_calls;
        state.step(target + 4); state.pc = std::uint32_t(state.r<4>(4128));
    }
};
std::unique_ptr<State> initial(unsigned index) {
    auto state = std::make_unique<State>(); auto& s = *state;
    s.registers.fill(0x6d); s.unique.fill(0xb3); s.native_calls = 17;
    for (auto [address, length] : std::array<std::pair<std::uint32_t, unsigned>, 3>{
        {{relocated_toc - 0x8000, 0x8000}, {stack - 1024, 2048}, {fixture, 0x1000}}}) {
        s.memory.map(address, length); s.memory.copy_in(address, std::vector<std::uint8_t>(length, 0xa7));
    }
    s.wr<4>(4, stack); s.wr<4>(8, relocated_toc); s.wr<4>(4128, caller);
    s.wr<4>(12, fixture); s.wr<4>(16, 1); s.wr<4>(20, 7);
    s.memory.write<4>(fixture, payload); s.memory.write<4>(fixture + 4, node1);
    s.memory.write<4>(fixture + 8, fixture + 0x400);
    s.memory.write<4>(node1, payload); s.memory.write<4>(node1 + 8, node2);
    s.memory.write<4>(node2, payload); s.memory.write<4>(node2 + 8, 0);
    for (auto offset : {-0x3e6c, -0x7ba8, -0x7bac})
        s.memory.write<4>(relocated_toc + offset, fixture + 0x500 + unsigned(-offset & 0xff));
    if (index == 0) s.wr<4>(16, fixture + 0x600);
    if (index == 5) s.wr<4>(16, 50000);
    if (index == 7 || index == 10) s.wr<4>(16, fixture + 0x400);
    s.pc = object::entries[index]; s.budget = 10000; return state;
}
using Configure = std::function<void(Monitor&, Dependencies&)>;
std::vector<Effects::Access> compare(unsigned index, const State& seed, const std::string& name,
    Configure configure = {}, std::function<void(const State&, const Dependencies&)> expectation = {}, bool negative = false) {
    auto a = clone(seed), b = clone(seed); Monitor x(*a), y(*b); Dependencies original(true), modern(false);
    if (configure) { configure(x, original); configure(y, modern); }
    std::string ae, be;
    try { original.reference(*a, caller); } catch (const std::exception& e) { ae = e.what(); }
    try { object::invoke(*b, index, &modern); } catch (const std::exception& e) { be = e.what(); }
    const auto label = hex_address(object::entries[index]) + " " + name;
    if (negative) {
        b->wr<4>(4128, b->r<4>(4128) ^ 4);
        bool rejected = false; try { Image(*a).compare(*b, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " own ABI negative accepted"); ++negatives[index]; return {};
    }
    require(ae == be, label + " original=" + ae + " modern=" + be);
    Image(*a).compare(*b, label);
    require(a->executed == b->executed && x.effects.bytes == y.effects.bytes && original.calls == modern.calls,
        label + " exact accounting, ordered accesses or dependency ABI differs");
    if (!ae.empty()) { ++faults[index]; if (ae == "Native instruction budget exhausted") ++budget_stops; }
    else if (expectation) expectation(*b, modern);
    ++comparisons[index]; return x.effects.log;
}
std::unique_ptr<State> minimal(const State& seed, const std::vector<Effects::Access>& accesses, std::uint32_t hole = UINT32_MAX) {
    auto s = std::make_unique<State>(); s->registers = seed.registers; s->unique = seed.unique; s->pc = seed.pc;
    s->budget = seed.budget; s->native_calls = seed.native_calls; s->executed = seed.executed;
    s->heap_end = seed.heap_end; s->allocated = seed.allocated; s->available = seed.available; s->memory_error = seed.memory_error;
    std::set<std::uint32_t> bytes;
    for (const auto& access : accesses) for (std::uint64_t n = 0; n < access.size; ++n) bytes.insert(access.address + std::uint32_t(n));
    for (auto address : bytes) if (address != hole) {
        s->memory.map(address, 1); s->memory.copy_in(address, seed.memory.copy_out(address, 1));
    }
    return s;
}
void matrix(unsigned index) {
    auto seed = initial(index);
    const auto accesses = compare(index, *seed, "normal", {}, [index, padding = seed->memory.read<2>(fixture + 2)](const State& s, const Dependencies& d) {
        require(s.pc == caller && s.r<4>(4) == stack && s.r<4>(8) == relocated_toc && s.r<4>(4128) == caller,
            "Independent utility caller ABI");
        if (index == 2) require(s.memory.read<2>(fixture) == 0 && s.memory.read<2>(fixture + 2) == padding &&
            s.memory.read<4>(fixture + 4) == 0 && s.memory.read<4>(fixture + 8) == 0 && s.memory.read<4>(fixture + 12) == 0,
            "List initialization preserves halfword padding");
        if (index == 4) require(s.memory.read<2>(fixture) == 0 && s.memory.read<4>(fixture + 4) == 0 &&
            s.memory.read<4>(fixture + 8) == 0 && s.memory.read<4>(fixture + 12) == 0, "List clear resets its own fields");
        if (index == 5) require(s.memory.read<4>(fixture) == payload && s.memory.read<4>(fixture + 4) == 0 &&
            s.memory.read<4>(fixture + 8) == 50000 && d.calls.front()[4] == 50000, "Buffer storage, length and requested capacity");
        if (index == 7) require(s.memory.read<4>(fixture) == 0 && s.memory.read<4>(fixture + 4) == fixture + 0x400 &&
            s.memory.read<4>(fixture + 8) == 7 && s.memory.read<4>(fixture + 12) == 0, "Live descriptor vectors and count");
        if (index == 9) require(s.memory.read<4>(fixture + 12) == 0 && s.memory.read<4>(fixture) == payload,
            "Descriptor reset leaves the original root word untouched");
        if (index == 10) require(d.calls.size() == 1 && d.calls[0][4] == fixture && d.calls[0][5] == payload &&
            d.calls[0][6] == fixture + 0x400 && d.calls[0][7] == 7, "Descriptor cleanup callback and deleting arguments");
    });
    compare(index, *seed, "own-link-negative", {}, {}, true);
    for (unsigned shift : {1u, 3u}) { auto s = clone(*seed); s->wr<4>(4, stack + shift); compare(index, *s, "unaligned-stack"); }
    compare(index, *seed, "live-SP-after-link-save", [](Monitor& m, Dependencies&) {
        m.after_write = [](State& s, std::uint32_t address) { if (address == stack + 8) s.wr<4>(4, stack + 256); };
    });
    compare(index, *seed, "cached-frame-publication", [](Monitor& m, Dependencies&) {
        m.after_write = [](State& s, std::uint32_t address) {
            if (address == stack - 64 || address == stack - 80 || address == stack - 112) s.wr<4>(4, stack + 256);
        };
    });
    compare(index, *seed, "live-TOC-after-call", [](Monitor&, Dependencies& d) {
        d.before = [](State& s, std::uint32_t) { s.wr<4>(8, relocated_toc + 4); };
    });
    compare(index, *seed, "dependency-fault-prefix", [](Monitor&, Dependencies& d) {
        d.before = [](State& s, std::uint32_t) { s.wr<4>(0, 0x12345678); throw std::runtime_error("Fixture utility dependency fault"); };
    });
    for (auto owner : {stack - 64, stack + 8, payload}) {
        auto s = clone(*seed); s->wr<4>(12, owner);
        compare(index, *s, "owner-alias=" + hex_address(owner));
    }
    if (index == 0) for (auto source : {fixture, fixture + 4, payload}) {
        auto s = clone(*seed); s->wr<4>(16, source);
        compare(index, *s, "node-source-alias=" + hex_address(source));
    }
    if (index == 5) {
        auto s = clone(*seed); s->wr<4>(12, relocated_toc - 0x3e6c);
        compare(index, *s, "buffer-owner-aliases-live-TOC-slot");
    }
    if (index == 5) compare(index, *seed, "allocation-failure-reporting", [](Monitor&, Dependencies& d) { d.allocation_fails = true; });
    if (index == 1 || index == 3 || index == 6 || index == 8) {
        for (std::uint32_t flag : {0u, 0x7fffu, 0x8000u, 0xffffu, 0x10001u}) {
            auto s = clone(*seed); s->wr<4>(16, flag); compare(index, *s, "deleting-flag=" + std::to_string(flag));
        }
        auto s = clone(*seed); s->wr<4>(12, 0); compare(index, *s, "null-owner");
    }
    if (index == 0) for (std::uint32_t size : {0u, 1u, 0x7fffu, 0x8000u, 0xffffu, 0x10001u}) {
        auto s = clone(*seed); s->wr<4>(20, size); compare(index, *s, "signed-node-size=" + std::to_string(size));
    }
    if (index == 3 || index == 4) {
        auto s = clone(*seed); s->memory.write<4>(fixture + 4, 0); compare(index, *s, "empty-list");
    }
    if (index == 6) {
        auto s = clone(*seed); s->memory.write<4>(fixture, 0); compare(index, *s, "null-buffer-payload");
        compare(index, *seed, "live-condition-after-clear", [](Monitor& m, Dependencies&) {
            m.after_write = [](State& s, std::uint32_t address) { if (address == fixture) s.wr<1>(2304, 2); };
        });
    }
    auto complete = clone(*seed); Dependencies d(false); object::invoke(*complete, index, &d);
    for (std::uint64_t n = 0; n <= complete->executed; ++n) {
        auto s = clone(*seed); s->budget = n; compare(index, *s, "budget=" + std::to_string(n));
    }
    const auto tiny = minimal(*seed, accesses); compare(index, *tiny, "minimal-mapping");
    std::set<std::uint32_t> bytes;
    for (const auto& access : accesses) for (std::uint64_t n = 0; n < access.size; ++n) bytes.insert(access.address + std::uint32_t(n));
    for (auto byte : bytes) { auto s = minimal(*seed, accesses, byte); compare(index, *s, "byte-hole=" + hex_address(byte)); }
}
void diagnostics() {
    State a, b; require(!a.runtime_diagnostics && !b.runtime_diagnostics, "Lazy owned runtime diagnostics");
    auto& d = utility_object_diagnostics(a); d.original = true; d.entry_original[4] = true; d.calls[2] = 17;
    engine_initialization_diagnostics(a).calls[0] = 39;
    require(!utility_object_diagnostics(std::as_const(b)).calls[2] && !b.runtime_diagnostics, "Independent allocation-free const query");
    reset_utility_object_diagnostics(a);
    require(!d.original && !d.entry_original[4] && !d.calls[2] && engine_initialization_diagnostics(a).calls[0] == 39,
        "Utility reset preserves accepted initializer diagnostics");
}
}
int main() {
    try {
        diagnostics(); for (unsigned i = 0; i < object::entries.size(); ++i) matrix(i);
        unsigned total = 0, failed = 0;
        for (unsigned i = 0; i < object::entries.size(); ++i) {
            require(comparisons[i] && faults[i] && negatives[i] == 1, "Dedicated utility component evidence");
            std::cout << hex_address(object::entries[i]) << " comparisons=" << comparisons[i] << " faults=" << faults[i] << " negatives=" << negatives[i] << '\n';
            total += comparisons[i]; failed += faults[i];
        }
        std::cout << "Utility objects: comparisons=" << total << " faults=" << failed << " budget_stops=" << budget_stops << " negatives=11\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
