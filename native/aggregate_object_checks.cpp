#include "aggregate_object_adapter.hpp"
#include "utility_object_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <set>
#include <utility>
namespace cm::aot { bool aggregate_object_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace aggregate = cm::aot::aggregate_object;
constexpr std::uint32_t caller = 0x7fff1000, relocated_toc = 0x32010000;
constexpr std::uint32_t control = fixture + 0x258400, descriptor = control + 0x100, items = control + 0x200;
std::array<unsigned, 6> comparisons{}, faults{}, negatives{};
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

struct Dependencies final : aggregate::DependencyDispatch {
    bool original;
    std::vector<std::array<std::uint32_t, 12>> calls;
    std::vector<decltype(State::registers)> call_registers;
    std::function<void(State&, std::uint32_t)> before;
    explicit Dependencies(bool source) : original(source) {}
    void reference(State& s, [[maybe_unused]] std::uint32_t continuation) {
        bool initial = true;
        for (;;) {
            const auto child = std::find(aggregate::entries.begin(), aggregate::entries.end(), s.pc);
            if (!initial && child != aggregate::entries.end()) call(s, s.pc, std::uint32_t(s.r<4>(4128)));
            else if (!aggregate_object_reference(s)) call(s, s.pc, std::uint32_t(s.r<4>(4128)));
            else if (s.pc == s.r<4>(4128)) return;
            initial = false;
        }
    }
    void call(State& s, std::uint32_t target, std::uint32_t continuation) override {
        require(s.pc == target && s.r<4>(4128) == continuation && s.r<4>(2440) == s.r<4>(8), "Published aggregate dependency ABI");
        calls.push_back({target, continuation, std::uint32_t(s.r<4>(4)), std::uint32_t(s.r<4>(8)),
            std::uint32_t(s.r<4>(12)), std::uint32_t(s.r<4>(16)), std::uint32_t(s.r<4>(20)),
            std::uint32_t(s.r<4>(24)), std::uint32_t(s.r<4>(48)), std::uint32_t(s.r<4>(2432)),
            std::uint32_t(s.r<4>(2440)), s.pc});
        call_registers.push_back(s.registers);
        if (before) before(s, target);
        const auto child = std::find(aggregate::entries.begin(), aggregate::entries.end(), target);
        if (child != aggregate::entries.end()) {
            if (original) reference(s, continuation);
            else aggregate::invoke(s, unsigned(child - aggregate::entries.begin()), this);
            if (s.pc != continuation) throw std::runtime_error("Fixture aggregate dependency escaped to " + hex_address(s.pc));
            return;
        }
        s.step(target);
        const auto owner = std::uint32_t(s.r<4>(12));
        if (target == 0x1073ae10) {
            require(s.r<4>(16) == 50000, "Aggregate buffer capacity");
            s.memory.write<4>(owner, control + 0x600); s.memory.write<4>(owner + 4, 0); s.memory.write<4>(owner + 8, 50000);
        } else if (target == 0x1073aee0) s.memory.write<4>(owner, 0);
        else if (target == 0x10781320) {
            s.memory.write<4>(s.r<4>(4) + 20, s.r<4>(8));
            s.memory.write<4>(owner, 0xabcdef01);
        } else if (target == 0x1073c5a0) s.memory.write<4>(owner + 0x2c28, 0x12345678);
        else if (target == 0x1073a120 || target == 0x10775170 || target == 0x1076b7e0 || target == 0x107735a0)
            s.memory.write<1>(owner + 15, 0xde);
        else if (target == 0x10745e00) s.memory.write<4>(s.r<4>(16) + 8, owner);
        else if (target == 0x1073ee30) s.memory.write<4>(owner + 8, 0);
        else if (target == 0x10718ef0) s.memory.write<1>(owner + 1, 0x42);
        else if (target == 0x10714440 || target == 0x10726a50) s.memory.write<1>(owner, 0x37);
        else if (target == 0x10773420) {
            require((s.r<4>(20) == 0x17c && s.r<4>(24) == 1200) || (s.r<4>(20) == 0xbf0 && s.r<4>(24) == 250), "Fixed aggregate array shapes");
            s.memory.write<1>(owner, 0x53);
        } else throw std::runtime_error("Unknown aggregate dependency " + hex_address(target));
        s.wr<4>(0, 0x24681357); ++s.native_calls;
        s.step(target + 4); s.pc = std::uint32_t(s.r<4>(4128));
    }
};
void table(State& s, std::uint32_t owner) {
    s.memory.write<2>(owner + 0x129da4, 2); s.memory.write<2>(owner + 0x129da6, 2);
    s.memory.write<4>(owner + 0x12a92c, descriptor);
    s.memory.write<4>(owner + 0x12a924, control + 0x400); s.memory.write<4>(owner + 0x12a928, control + 0x500);
}
std::unique_ptr<State> initial(unsigned index) {
    auto s = std::make_unique<State>(); s->registers.fill(0x6d); s->unique.fill(0xb3); s->native_calls = 17;
    for (auto [a, n] : std::array<std::pair<std::uint32_t, unsigned>, 3>{{
        {relocated_toc - 0x8000, 0x8000}, {stack - 4096, 8192}, {fixture, 0x259400}}}) {
        s->memory.map(a, n); s->memory.copy_in(a, std::vector<std::uint8_t>(n, 0xa7));
    }
    s->wr<4>(4, stack); s->wr<4>(8, relocated_toc); s->wr<4>(4128, caller); s->memory.write<4>(stack + 8, caller);
    s->memory.write<4>(relocated_toc - 0x5bf0, control); s->memory.write<4>(relocated_toc - 0x5bec, control + 4);
    s->memory.write<4>(relocated_toc - 0x5bf4, control + 8); s->memory.write<4>(relocated_toc - 0x5bf8, control + 16);
    s->memory.write<2>(control, 0); s->memory.write<4>(control + 4, control + 0x700);
    s->memory.write<4>(descriptor, items); s->memory.write<4>(descriptor + 4, 2);
    s->memory.write<4>(items, control + 0x300); s->memory.write<4>(items + 4, control + 0x310);
    table(*s, fixture); table(*s, fixture + 0x2c68); table(*s, fixture + 0x12d6b0);
    for (unsigned row = 0; row < 16; ++row) {
        s->memory.write<2>(fixture + 0x2c38 + row * 2, row < 2 ? 2 : 0);
        s->memory.write<4>(fixture + row * 0x2c0 + 16, control + 0x800);
    }
    s->memory.write<4>(control + 0x804, control + 0x900);
    s->memory.write<4>(fixture + 0x2c64, control + 0xa00);
    s->wr<4>(12, fixture); s->wr<4>(16, 1); s->wr<4>(20, 7);
    s->pc = aggregate::entries[index]; s->budget = 20000; return s;
}
using Configure = std::function<void(Monitor&, Dependencies&)>;
std::vector<Effects::Access> compare(unsigned index, const State& seed, const std::string& name,
    Configure configure = {}, std::function<void(const State&, const Dependencies&)> expectation = {}, bool negative = false) {
    auto a = clone(seed), b = clone(seed); Monitor x(*a), y(*b); Dependencies original(true), modern(false);
    if (configure) { configure(x, original); configure(y, modern); }
    std::string ae, be;
    try { original.reference(*a, caller); } catch (const std::exception& e) { ae = e.what(); }
    try { aggregate::invoke(*b, index, &modern); } catch (const std::exception& e) { be = e.what(); }
    const auto label = hex_address(aggregate::entries[index]) + " " + name;
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
    require(original.call_registers == modern.call_registers, label + " complete dependency register snapshots");
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
        require(s.pc == caller && s.r<4>(4) == stack && s.r<4>(8) == relocated_toc, "Independent aggregate caller ABI");
        if (index == 0) {
            require(s.memory.read<1>(fixture + 0x12a93f) == 1 && s.memory.read<1>(fixture + 0x12a946) == 1,
                "Independent table mode and enabled flag");
            require(s.memory.read<2>(fixture + 0x12a902) == 0xffff && s.memory.read<1>(fixture + 0x12a901) == 0,
                "Original adjacent sentinel and byte overlap");
        }
        if (index == 2) {
            require(s.r<4>(12) == fixture && s.memory.read<4>(fixture + 36) == fixture + 0x2c68, "Selected embedded table pointer");
            require(s.memory.read<4>(fixture + 0x2c34) == 50000, "Aggregate buffer capacity");
            for (unsigned row = 0; row < 16; ++row) {
                require(s.memory.read<2>(fixture + 0x2c38 + row * 2) == unsigned(row == 0), "Sixteen initialized group counts");
                for (unsigned item = 0; item < 10; ++item) require(s.memory.read<1>(fixture + 0x2580fc + row * 10 + item) == 0, "Ten status bytes per group");
            }
            require(s.memory.read<1>(fixture + 2) == 0xa7 && s.memory.read<1>(fixture + 0x2580f7) == 0xa7, "Aggregate untouched padding");
        }
        if (index == 1) {
            require(s.memory.read<4>(fixture + 0x12a92c) == 0 && s.memory.read<2>(fixture + 0x129da4) == 0 &&
                s.memory.read<2>(fixture + 0x129da6) == 0, "Table clears only its owned roots and live counts");
            std::vector<std::uint32_t> arrays;
            for (const auto& call : d.calls) if (call[0] == 0x10773420) arrays.push_back(call[7]);
            require(arrays == std::vector<std::uint32_t>{1200, 250}, "Independent array destruction order");
        }
        if (index == 4) require(s.memory.read<2>(fixture + 0x2c58) == 1, "Record cleanup retains the last nonempty row index");
        if (index == 5) require(s.memory.read<4>(fixture + 0x2c64) == 0, "Attached stream is cleared after release");
    });
    compare(index, *seed, "own-link-negative", {}, {}, true);
    for (unsigned shift : {1u, 3u}) { auto s = clone(*seed); s->wr<4>(4, stack + shift); s->memory.write<4>(stack + shift + 8, caller); compare(index, *s, "unaligned-stack"); }
    compare(index, *seed, "dependency-fault", [](Monitor&, Dependencies& d) {
        d.before = [](State& s, std::uint32_t) { s.wr<4>(0, 0x12345678); throw std::runtime_error("Fixture aggregate callback fault"); };
    });
    compare(index, *seed, "live-TOC", [](Monitor&, Dependencies& d) {
        d.before = [](State& s, std::uint32_t) { s.wr<4>(8, relocated_toc + 4); };
    });
    for (auto pointer : {0u, stack - 96, stack + 8, fixture + 1}) {
        auto s = clone(*seed); s->wr<4>(12, pointer); compare(index, *s, "null-or-alias-owner");
    }
    if (index == 0 || index == 1 || index == 3) for (auto flag : {0u, 0xffffu, 0x8000u, 0x7fffu, 0x10000u, 0x10001u, 0x12345678u}) {
        auto s = clone(*seed); s->wr<4>(16, flag); compare(index, *s, "byte-mode-or-signed-low-half-delete-flag");
    }
    if (index == 1) {
        for (auto count : {0u, 0xffffu, 0x8000u}) {
            auto s = clone(*seed); s->memory.write<2>(fixture + 0x129da4, count); s->memory.write<2>(fixture + 0x129da6, count);
            s->memory.write<4>(descriptor + 4, count == 0 ? 0 : UINT32_MAX); compare(index, *s, "signed-live-counts");
        }
        for (auto field : {0x12a92cu, 0x12a924u, 0x12a928u}) { auto s = clone(*seed); s->memory.write<4>(fixture + field, 0); compare(index, *s, "absent-owned-resource"); }
        for (auto shared_count : {1u, 0xffffu}) { auto s = clone(*seed); s->memory.write<2>(control, shared_count); compare(index, *s, "shared-signed-reference-count"); }
        compare(index, *seed, "live-loop-count-after-callback", [](Monitor&, Dependencies& d) {
            d.before = [](State& s, std::uint32_t target) {
                if (target == 0x10714440) s.memory.write<2>(fixture + 0x129da4, 1);
                if (target == 0x10775170) s.memory.write<4>(descriptor + 4, 1);
            };
        });
        compare(index, *seed, "repeated-owned-pointer-reread", [](Monitor& m, Dependencies&) {
            m.after_write = [republished = false](State& s, std::uint32_t address) mutable {
                if (!republished && address == fixture + 0x12a924 && s.pc == 0x10718cb4) {
                    republished = true; s.memory.write<4>(address, control + 0x410);
                }
            };
        }, [](const State&, const Dependencies& d) {
            unsigned repeated = 0; for (const auto& c : d.calls) repeated += c[0] == 0x1076b7e0 && c[1] == 0x10718ccc;
            require(repeated == 1, "Republished owned pointer is actually released by the second check");
        });
    }
    if (index == 2) {
        compare(index, *seed, "callback-redirected-saved-owner", [](Monitor&, Dependencies& d) {
            d.before = [](State& s, std::uint32_t target) { if (target == 0x1073ae10) s.memory.write<4>(s.r<4>(4) + 88, fixture + 4); };
        });
        compare(index, *seed, "field-callback-redirected-owner", [](Monitor& m, Dependencies&) {
            m.after_write = [](State& s, std::uint32_t address) { if (address == fixture + 0x2c64) s.memory.write<4>(s.r<4>(4) + 88, fixture + 4); };
        });
        compare(index, *seed, "live-row-iteration-register", [](Monitor& m, Dependencies&) {
            m.after_write = [](State& s, std::uint32_t address) { if (address == fixture + 0x2580fc) s.wr<4>(24, 8); };
        });
    }
    if (index == 4) {
        for (auto count : {0u, 1u, 0xffffu, 0x8000u}) {
            auto s = clone(*seed); for (unsigned row = 0; row < 16; ++row) s->memory.write<2>(fixture + 0x2c38 + row * 2, count);
            compare(index, *s, "all-sixteen-signed-group-counts");
        }
        auto s = clone(*seed); s->memory.write<4>(fixture + 16, 0); compare(index, *s, "absent-record-payload");
        s = clone(*seed); s->memory.write<4>(control + 0x804, 0); compare(index, *s, "absent-record-callback");
        compare(index, *seed, "callback-mutated-live-group-count", [](Monitor&, Dependencies& d) {
            d.before = [](State& s, std::uint32_t target) { if (target == 0x1073ee30) s.memory.write<2>(fixture + 0x2c38, 1); };
        });
    }
    if (index == 5) {
        auto s = clone(*seed); s->memory.write<4>(fixture + 0x2c64, 0); compare(index, *s, "detached-stream");
        compare(index, *seed, "flush-replaces-stream-before-release", [](Monitor&, Dependencies& d) {
            d.before = [](State& s, std::uint32_t target) { if (target == 0x1073c5a0) s.memory.write<4>(fixture + 0x2c64, control + 0xb00); };
        }, [](const State&, const Dependencies& d) {
            require(d.calls.size() == 2 && d.calls[1][4] == control + 0xb00, "Release consumes the stream republished by flush");
        });
    }
    // Each distinct accessed byte is removed in turn. This compares the exact
    // first mapping-fault prefix, rather than merely requiring both sides fail.
    std::set<std::uint32_t> bytes;
    for (const auto& access : accesses) for (unsigned n = 0; n < access.size; ++n) bytes.insert(access.address + n);
    auto small = minimal(*seed, accesses); compare(index, *small, "minimal-observed-mapping");
    for (auto hole : bytes) { auto s = minimal(*seed, accesses, hole); compare(index, *s, "mapping-hole"); }
    auto counted = clone(*small); Dependencies counter(true); counter.reference(*counted, caller);
    for (std::uint64_t budget = 0; budget < counted->executed; ++budget) {
        auto s = clone(*small); s->budget = budget; compare(index, *s, "exact-finite-budget");
    }
}
void diagnostics() {
    State a, b; auto& d = aggregate_object_diagnostics(a); d.original = true; d.entry_original[1] = true; d.calls[5] = 17;
    utility_object_diagnostics(a).calls[0] = 39;
    require(!aggregate_object_diagnostics(std::as_const(b)).calls[5] && !b.runtime_diagnostics, "Allocation-free const aggregate query");
    reset_aggregate_object_diagnostics(a);
    require(!d.original && !d.entry_original[1] && !d.calls[5] && utility_object_diagnostics(a).calls[0] == 39, "Aggregate reset preserves utility counters");
}
}
int main() {
    try {
        diagnostics(); for (unsigned i = 0; i < aggregate::entries.size(); ++i) matrix(i);
        unsigned total = 0, failed = 0;
        for (unsigned i = 0; i < aggregate::entries.size(); ++i) {
            require(comparisons[i] && faults[i] && negatives[i] == 1, "Dedicated aggregate component evidence");
            std::cout << hex_address(aggregate::entries[i]) << " comparisons=" << comparisons[i] << " faults=" << faults[i] << " negatives=" << negatives[i] << '\n';
            total += comparisons[i]; failed += faults[i];
        }
        std::cout << "Aggregate component comparisons=" << total << " faults=" << failed << " budget_stops=" << budget_stops << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
