#include "tree_mutation_adapter.hpp"
#include "utility_object_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <set>
#include <utility>

namespace cm::aot { bool tree_mutation_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace tree = cm::aot::tree_mutation;
constexpr std::uint32_t caller = 0x7fff1000, relocated_toc = 0x32010000;
constexpr std::uint32_t root = fixture + 0x100, compare_vector = fixture + 0x800, delete_vector = fixture + 0x808;
constexpr std::uint32_t new_node = fixture + 0x900, new_item = fixture + 0x850;
std::array<unsigned, 3> comparisons{}, faults{}, negatives{};
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

struct Dependencies final : tree::DependencyDispatch {
    bool original, allocation_fails = false;
    unsigned allocations = 0;
    std::vector<std::array<std::uint32_t, 12>> calls;
    std::vector<std::uint32_t> disposed;
    std::function<void(State&, std::uint32_t)> before;
    explicit Dependencies(bool source) : original(source) {}
    void reference(State& s, [[maybe_unused]] std::uint32_t continuation) {
        bool initial = true;
        for (;;) {
            const auto child = std::find(tree::entries.begin(), tree::entries.end(), s.pc);
            if (!initial && child != tree::entries.end()) call(s, s.pc, std::uint32_t(s.r<4>(4128)));
            else if (!tree_mutation_reference(s)) call(s, s.pc, std::uint32_t(s.r<4>(4128)));
            else if (s.pc == s.r<4>(4128)) return; // This isolated body executed its own return.
            initial = false;
        }
    }
    void call(State& s, std::uint32_t target, std::uint32_t continuation) override {
        require(s.pc == target && s.r<4>(4128) == continuation && s.r<4>(2440) == s.r<4>(8), "Published tree dependency ABI");
        calls.push_back({target, continuation, std::uint32_t(s.r<4>(4)), std::uint32_t(s.r<4>(8)),
            std::uint32_t(s.r<4>(12)), std::uint32_t(s.r<4>(16)), std::uint32_t(s.r<4>(20)),
            std::uint32_t(s.r<4>(24)), std::uint32_t(s.r<4>(48)), std::uint32_t(s.r<4>(2432)),
            std::uint32_t(s.r<4>(2440)), s.pc});
        if (before) before(s, target);
        const auto child = std::find(tree::entries.begin(), tree::entries.end(), target);
        if (child != tree::entries.end()) {
            if (original) reference(s, continuation);
            else tree::invoke(s, unsigned(child - tree::entries.begin()), this);
            if (s.pc != continuation) throw std::runtime_error("Fixture tree dependency escaped to " + hex_address(s.pc));
            return;
        }
        s.step(target);
        if (target == 0x10773520) {
            require(s.r<4>(12) == 16, "Original tree node allocation size");
            const auto node = new_node + allocations++ * 16;
            s.wr<4>(12, allocation_fails ? 0 : node);
        } else if (target == 0x10781320) {
            s.memory.write<4>(s.r<4>(4) + 20, s.r<4>(8));
            if (s.r<4>(48) == compare_vector) {
                const auto first = s.memory.read<4>(s.r<4>(12)), second = s.memory.read<4>(s.r<4>(16));
                s.wr<4>(12, first - second);
            } else {
                const auto node = std::uint32_t(s.r<4>(12)); disposed.push_back(node);
                s.memory.write<4>(node + 4, 0xdeadbeef); s.wr<4>(12, 0xcafe);
            }
        } else throw std::runtime_error("Unknown tree dependency " + hex_address(target));
        s.wr<4>(0, 0x24681357); ++s.native_calls;
        s.step(target + 4); s.pc = std::uint32_t(s.r<4>(4128));
    }
};
std::unique_ptr<State> initial(unsigned index) {
    auto s = std::make_unique<State>(); s->registers.fill(0x6d); s->unique.fill(0xb3); s->native_calls = 17;
    for (auto [a, n] : std::array<std::pair<std::uint32_t, unsigned>, 3>{{
        {relocated_toc - 0x8000, 0x8000}, {stack - 4096, 8192}, {fixture, 0x1000}}}) {
        s->memory.map(a, n); s->memory.copy_in(a, std::vector<std::uint8_t>(n, 0xa7));
    }
    s->wr<4>(4, stack); s->wr<4>(8, relocated_toc); s->wr<4>(4128, caller);
    s->memory.write<4>(stack + 8, caller);
    s->memory.write<4>(fixture, root); s->memory.write<4>(fixture + 4, compare_vector);
    s->memory.write<4>(fixture + 8, delete_vector); s->memory.write<4>(fixture + 12, 31);
    for (unsigned i = 0; i < 31; ++i) {
        const auto node = root + i * 16, key = fixture + 0x500 + i * 4;
        s->memory.write<4>(node, key); s->memory.write<4>(key, 100 + i);
        s->memory.write<4>(node + 8, i * 2 + 1 < 31 ? root + (i * 2 + 1) * 16 : 0);
        s->memory.write<4>(node + 12, i * 2 + 2 < 31 ? root + (i * 2 + 2) * 16 : 0);
    }
    s->memory.write<4>(new_item, 90);
    s->wr<4>(12, fixture); s->wr<4>(16, index == 2 ? root : fixture);
    s->wr<4>(20, index == 2 ? delete_vector : new_item); s->wr<4>(24, 0x12345678);
    s->pc = tree::entries[index]; s->budget = 20000; return s;
}
using Configure = std::function<void(Monitor&, Dependencies&)>;
std::vector<Effects::Access> compare(unsigned index, const State& seed, const std::string& name,
    Configure configure = {}, std::function<void(const State&, const Dependencies&)> expectation = {}, bool negative = false) {
    auto a = clone(seed), b = clone(seed); Monitor x(*a), y(*b); Dependencies original(true), modern(false);
    if (configure) { configure(x, original); configure(y, modern); }
    std::string ae, be;
    try { original.reference(*a, caller); } catch (const std::exception& e) { ae = e.what(); }
    try { tree::invoke(*b, index, &modern); } catch (const std::exception& e) { be = e.what(); }
    const auto label = hex_address(tree::entries[index]) + " " + name;
    if (negative) {
        b->wr<4>(4128, b->r<4>(4128) ^ 4);
        bool rejected = false; try { Image(*a).compare(*b, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " own ABI negative accepted"); ++negatives[index]; return {};
    }
    if (ae != be) {
        for (auto* list : {&original.calls, &modern.calls}) {
            std::cerr << (list == &original.calls ? "Original" : "Modern") << " dependency trace:";
            for (const auto& c : *list) std::cerr << ' ' << hex_address(c[0]) << ':' << hex_address(c[1]);
            std::cerr << '\n';
        }
    }
    require(ae == be, label + " original=" + ae + " modern=" + be);
    Image(*a).compare(*b, label);
    require(a->executed == b->executed, label + " accounting old=" + std::to_string(a->executed) + " modern=" + std::to_string(b->executed));
    require(x.effects.bytes == y.effects.bytes, label + " ordered access prefixes");
    if (original.calls != modern.calls) {
        for (unsigned i = 0; i < std::min(original.calls.size(), modern.calls.size()); ++i)
            for (unsigned field = 0; field < original.calls[i].size(); ++field)
                if (original.calls[i][field] != modern.calls[i][field])
                    throw std::runtime_error(label + " call=" + std::to_string(i) + " field=" + std::to_string(field) + " old=" + hex_address(original.calls[i][field]) + " modern=" + hex_address(modern.calls[i][field]));
    }
    require(original.calls == modern.calls, label + " dependency ABI calls old=" + std::to_string(original.calls.size()) + " modern=" + std::to_string(modern.calls.size()));
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
    const auto accesses = compare(index, *seed, "normal", {}, [index](const State& s, const Dependencies& d) {
        require(s.pc == caller && s.r<4>(4) == stack && s.r<4>(8) == relocated_toc, "Independent tree caller ABI");
        if (index == 2) {
            std::vector<std::uint32_t> expected;
            const auto postorder = [&](auto&& self, unsigned node) -> void {
                if (node >= 31) return; self(self, node * 2 + 1); self(self, node * 2 + 2); expected.push_back(root + node * 16);
            };
            postorder(postorder, 0); require(d.disposed == expected, "Independent complete postorder including actual recursive depth");
            require(s.memory.read<4>(fixture) == root && s.memory.read<4>(fixture + 12) == 31, "Cleanup preserves descriptor residues");
        } else require(s.memory.read<4>(fixture + 12) == 32 && d.allocations == 1, "Insertion count and allocation");
    });
    compare(index, *seed, "own-link-negative", {}, {}, true);
    for (unsigned shift : {1u, 3u}) { auto s = clone(*seed); s->wr<4>(4, stack + shift); s->memory.write<4>(stack + shift + 8, caller); compare(index, *s, "unaligned-stack"); }
    for (auto sp : {stack - 64, stack - 80, stack - 96}) compare(index, *seed, "frame-publication", [sp](Monitor& m, Dependencies&) {
        m.after_write = [sp](State& s, std::uint32_t a) { if (a == sp) s.wr<4>(4, stack + 256); };
    });
    compare(index, *seed, "live-SP-after-link-save", [](Monitor& m, Dependencies&) {
        m.after_write = [](State& s, std::uint32_t a) { if (a == stack + 8) s.wr<4>(4, stack + 256); };
    });
    compare(index, *seed, "multiple-register-save-cursor", [](Monitor& m, Dependencies&) {
        m.after_write = [](State& s, std::uint32_t a) { if (a == stack - 28 || a == stack - 20) s.wr<4>(2432, a + 8); };
    });
    compare(index, *seed, "dependency-fault", [](Monitor&, Dependencies& d) {
        d.before = [](State& s, std::uint32_t) { s.wr<4>(0, 0x12345678); throw std::runtime_error("Fixture tree callback fault"); };
    });
    compare(index, *seed, "live-TOC", [](Monitor&, Dependencies& d) {
        d.before = [](State& s, std::uint32_t) { s.wr<4>(8, relocated_toc + 4); };
    });
    for (auto owner : {stack - 96, stack + 8, root}) {
        auto s = clone(*seed); s->wr<4>(12, owner); compare(index, *s, "owner-alias");
    }
    if (index == 2) {
        auto s = clone(*seed); s->wr<4>(16, 0); compare(index, *s, "empty-tree", {}, [](const State&, const Dependencies& d) { require(d.disposed.empty(), "Empty tree has no callback"); });
        for (unsigned i = 0; i < 15; ++i) for (unsigned field : {8u, 12u}) {
            auto s = clone(*seed); s->memory.write<4>(root + i * 16 + field, 0); compare(index, *s, "pruned-expanded-child");
        }
        compare(index, *seed, "live-right-child-after-callback", [](Monitor&, Dependencies& d) {
            d.before = [](State& s, std::uint32_t target) {
                if (target == 0x10781320 && s.r<4>(12) == root + 15 * 16) s.memory.write<4>(root + 7 * 16 + 12, 0);
            };
        }, [](const State&, const Dependencies& d) { require(d.disposed.size() == 30, "Callback mutation changes later right-child traversal"); });
        for (auto flag : {0u, 1u, 0x8000u, UINT32_MAX}) { auto s = clone(*seed); s->wr<4>(24, flag); compare(index, *s, "full-word-callback-flag"); }
    } else {
        for (auto key : {90u, 100u, 120u}) {
            auto s = clone(*seed); s->memory.write<4>(new_item, key);
            for (bool failed : {false, true}) compare(index, *s, "insert-or-replace-allocation", [failed](Monitor&, Dependencies& d) { d.allocation_fails = failed; },
                [key, failed](const State& s, const Dependencies& d) {
                    if (key == 100) require(s.memory.read<4>(fixture) == (failed ? 0 : new_node) && s.memory.read<4>(fixture + 12) == 31 && d.disposed == std::vector<std::uint32_t>{root}, "Equal replacement failure preserves original disposal/count");
                });
        }
        for (auto count : {0u, UINT32_MAX}) {
            auto s = clone(*seed); s->memory.write<4>(fixture, 0); s->memory.write<4>(fixture + 12, count);
            for (bool failed : {false, true}) compare(index, *s, "empty-insertion-failure-and-wrap", [failed](Monitor&, Dependencies& d) { d.allocation_fails = failed; },
                [count, failed](const State& s, const Dependencies&) { require(s.memory.read<4>(fixture + 12) == count + 1 && s.memory.read<4>(fixture) == (failed ? 0 : new_node), "Count wraps and increments on failed empty insertion"); });
        }
        compare(index, *seed, "comparator-mutates-root", [](Monitor&, Dependencies& d) {
            d.before = [](State& s, std::uint32_t target) { if (target == 0x10781320 && s.r<4>(48) == compare_vector) s.memory.write<4>(fixture, root + 16); };
        });
    }
    auto complete = clone(*seed); Dependencies d(false); tree::invoke(*complete, index, &d);
    for (std::uint64_t n = 0; n <= complete->executed; ++n) { auto s = clone(*seed); s->budget = n; compare(index, *s, "budget=" + std::to_string(n)); }
    const auto tiny = minimal(*seed, accesses); compare(index, *tiny, "minimal-mapping");
    std::set<std::uint32_t> bytes;
    for (const auto& access : accesses) for (std::uint64_t n = 0; n < access.size; ++n) bytes.insert(access.address + std::uint32_t(n));
    for (auto byte : bytes) { auto s = minimal(*seed, accesses, byte); compare(index, *s, "byte-hole=" + hex_address(byte)); }
}
void diagnostics() {
    State a, b; require(!a.runtime_diagnostics && !b.runtime_diagnostics, "Lazy owned tree diagnostics");
    auto& d = tree_mutation_diagnostics(a); d.original = true; d.entry_original[1] = true; d.calls[2] = 17;
    utility_object_diagnostics(a).calls[0] = 39;
    require(!tree_mutation_diagnostics(std::as_const(b)).calls[2] && !b.runtime_diagnostics, "Independent allocation-free const tree query");
    reset_tree_mutation_diagnostics(a);
    require(!d.original && !d.entry_original[1] && !d.calls[2] && utility_object_diagnostics(a).calls[0] == 39, "Tree reset preserves accepted utility diagnostics");
}
}
int main() {
    try {
        diagnostics(); for (unsigned i = 0; i < tree::entries.size(); ++i) matrix(i);
        unsigned total = 0, failed = 0;
        for (unsigned i = 0; i < tree::entries.size(); ++i) {
            require(comparisons[i] && faults[i] && negatives[i] == 1, "Dedicated tree component evidence");
            std::cout << hex_address(tree::entries[i]) << " comparisons=" << comparisons[i] << " faults=" << faults[i] << " negatives=" << negatives[i] << '\n';
            total += comparisons[i]; failed += faults[i];
        }
        std::cout << "Tree component comparisons=" << total << " faults=" << failed << " budget_stops=" << budget_stops << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
