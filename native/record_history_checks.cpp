#include "record_history_adapter.hpp"
#include "utility_object_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <set>
#include <utility>
namespace cm::aot { bool record_history_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace records = cm::aot::record_history;
constexpr std::uint32_t caller = 0x7fff1000, relocated_toc = 0x32010000;
constexpr std::uint32_t control = fixture + 0x4000;
std::array<unsigned, 2> comparisons{}, faults{}, negatives{};
unsigned budget_stops = 0;
struct Monitor final : MemoryObserver {
    State& state; Effects effects;
    std::function<void(State&, std::uint32_t)> after_write;
    std::function<void(State&, std::uint32_t)> after_read;
    explicit Monitor(State& s) : state(s) { effects.state = &s; s.memory.date_observer = this; }
    ~Monitor() { state.memory.date_observer = nullptr; }
    void mapped(std::uint32_t a, std::uint64_t n) override { effects.mapped(a, n); }
    void cleared(std::uint32_t a, std::uint64_t n) override { effects.cleared(a, n); }
    void read(std::uint32_t a, const std::uint8_t* b, unsigned n) override {
        effects.read(a, b, n); if (after_read) after_read(state, a);
    }
    void written(std::uint32_t a, const std::uint8_t* b, std::size_t n) override {
        effects.written(a, b, n); if (after_write) after_write(state, a);
    }
};

struct Dependencies final : records::DependencyDispatch {
    bool original, allocation_failure = false, corrupt_child = false;
    unsigned allocations = 0;
    std::vector<std::array<std::uint32_t, 12>> calls;
    std::vector<decltype(State::registers)> call_registers;
    std::function<void(State&, std::uint32_t)> before;
    explicit Dependencies(bool source) : original(source) {}
    void reference(State& s, [[maybe_unused]] std::uint32_t continuation) {
        bool initial = true;
        for (;;) {
            const auto child = std::find(records::entries.begin(), records::entries.end(), s.pc);
            if (!initial && child != records::entries.end()) call(s, s.pc, std::uint32_t(s.r<4>(4128)));
            else if (!cm::aot::record_history_reference(s)) call(s, s.pc, std::uint32_t(s.r<4>(4128)));
            else if (s.pc == s.r<4>(4128)) return;
            initial = false;
        }
    }
    void call(State& s, std::uint32_t target, std::uint32_t continuation) override {
        require(s.pc == target && s.r<4>(4128) == continuation && s.r<4>(2440) == s.r<4>(8), "Published history dependency ABI");
        std::array<std::uint32_t, 12> tuple{target};
        for (unsigned n = 1; n < tuple.size(); ++n) tuple[n] = std::uint32_t(s.r<4>(n <= 8 ? 8 + n * 4 : n == 9 ? 4 : n == 10 ? 8 : 4128));
        calls.push_back(tuple); call_registers.push_back(s.registers);
        if (before) before(s, target);
        const auto owner = std::uint32_t(s.r<4>(12));
        if (target == records::entries[1]) {
            if (original) reference(s, continuation); else records::invoke(s, 1, this);
            require(s.pc == continuation, "History child returns to its original caller");
            if (corrupt_child && !original) {
                const auto row = owner + s.memory.read<2>(owner + 0x2c58) * records::row_stride;
                const auto node = s.memory.read<4>(row + 16);
                s.memory.write<4>(node, s.memory.read<4>(node) ^ 1);
            }
            return;
        }
        s.step(target);
        if (target == 0x10773520) s.wr<4>(12, allocation_failure ? 0 : control + 0x3000 + allocations++ * 0x300);
        else if (target == 0x10773610 || target == 0x107735a0) s.memory.write<1>(owner + 15, 0xde);
        else if (target == 0x10745e00) {
            // The actual dependency exchanges bytes; it is not a copy helper.
            // Production tests retain its full generated implementation.
            const auto other = std::uint32_t(s.r<4>(16)), count = std::uint32_t(s.r<4>(20));
            for (std::uint32_t n = 0; n < count; ++n) {
                const auto left = s.memory.read<1>(owner + n), right = s.memory.read<1>(other + n);
                s.memory.write<1>(owner + n, right); s.memory.write<1>(other + n, left);
            }
        } else if (target == 0x10778300) s.memory.copy_in(owner, std::vector<std::uint8_t>(s.r<4>(20), std::uint8_t(s.r<4>(16))));
        else if (target == 0x1077dbf0) {
            const auto source = std::uint32_t(s.r<4>(16)); unsigned length = 0;
            while (s.memory.read<1>(source + length)) ++length;
            s.memory.copy_in(owner, s.memory.copy_out(source, length + 1));
        } else if (target == 0x10781320) {
            s.memory.write<4>(s.r<4>(4) + 20, s.r<4>(8)); s.memory.write<1>(control + 0x2900, 0x91);
        } else if (target == 0x1073f1e0) {
            const auto row = owner + s.memory.read<2>(owner + 0x2c58) * records::row_stride;
            const auto node = s.memory.read<4>(row + 16) + s.r<4>(16) * 8;
            s.memory.write<4>(node + 20, s.r<4>(20)); s.memory.write<1>(node + 24, 0);
        } else if (target == 0x10739890) {
            s.memory.write<4>(s.r<4>(24), 0); s.memory.write<4>(s.r<4>(28), 0);
        } else if (target == 0x10768810) s.wr<4>(12, control + 0x2700);
        else if (target == 0x1077b720 || target == 0x10769b60) s.memory.write<1>(control + 0x2900, 0x37);
        else throw std::runtime_error("Unknown history dependency " + hex_address(target));
        s.wr<4>(0, 0x24681357); ++s.native_calls; s.step(target + 4); s.pc = std::uint32_t(s.r<4>(4128));
    }
};
std::unique_ptr<State> initial(unsigned index) {
    auto s = std::make_unique<State>(); s->registers.fill(0x6d); s->unique.fill(0xb3); s->native_calls = 17;
    for (auto [a, n] : std::array<std::pair<std::uint32_t, unsigned>, 3>{{
        {relocated_toc - 0x8000, 0x8000}, {stack - 4096, 8192}, {fixture, 0x14000}}}) {
        s->memory.map(a, n); s->memory.copy_in(a, std::vector<std::uint8_t>(n, 0xa7));
    }
    s->wr<4>(4, stack); s->wr<4>(8, relocated_toc); s->wr<4>(4128, caller); s->memory.write<4>(stack + 8, caller);
    s->memory.write<4>(relocated_toc - 0x7bac, control + 0x2800); s->memory.write<4>(relocated_toc - 0x7ba8, control + 0x2900);
    s->memory.write<4>(relocated_toc - 0x3e5c, control + 0x2a00);
    s->memory.copy_in(control + 0x2ab4, {'d','e','f','a','u','l','t',0});
    for (unsigned row = 0; row < records::row_count; ++row) {
        s->memory.copy_in(fixture + row * records::row_stride, std::vector<std::uint8_t>(records::row_stride, 0));
        s->memory.write<2>(fixture + 0x2c38 + row * 2, row == 1 ? 1 : 0);
    }
    for (unsigned n = 0; n < 4; ++n) {
        const auto node = control + 0x200 + n * 0x400;
        s->memory.copy_in(node, std::vector<std::uint8_t>(0x300, 0)); s->memory.write<4>(node, n + 1);
    }
    const auto row = fixture + records::row_stride;
    for (unsigned n = 0; n < 2; ++n) {
        const auto rec = row + n * records::record_stride, node = control + 0x200 + n * 0x400;
        s->memory.write<4>(rec + 4, n ? 22 : 11);
        for (unsigned field : {8u, 12u, 16u}) s->memory.write<4>(rec + field, node);
        s->memory.write<2>(rec + 22, 1);
    }
    s->memory.write<4>(fixture + 0x2c00, 0); s->memory.write<4>(fixture + 0x2c14, control + 0x2000);
    s->memory.write<2>(fixture + 0x2c18, 2); s->memory.write<2>(fixture + 0x2c58, 1);
    s->memory.write<4>(control + 0x2000, control + 0x2400); s->memory.write<4>(control + 0x2004, control + 0x2410);
    s->wr<4>(12, fixture);
    if (index == 0) { s->wr<4>(16, 0); s->wr<4>(20, 9); s->wr<4>(24, control + 0x2400); s->wr<4>(28, 0); s->wr<4>(32, 0); s->wr<4>(36, 0x87654321); }
    else { s->wr<4>(16, 9); s->wr<4>(20, control + 0x2400); s->wr<4>(24, 0); s->wr<4>(28, 0); s->wr<4>(32, 0x87654321); }
    s->pc = records::entries[index]; s->budget = 10000; return s;
}
using Configure = std::function<void(Monitor&, Dependencies&)>;
std::vector<Effects::Access> compare(unsigned index, const State& seed, const std::string& name,
    Configure configure = {}, std::function<void(const State&, const Dependencies&)> expectation = {}, bool negative = false, bool expect_normal = false) {
    auto a = clone(seed), b = clone(seed); Monitor x(*a), y(*b); Dependencies original(true), modern(false);
    if (configure) { configure(x, original); configure(y, modern); }
    std::string ae, be;
    try { original.reference(*a, caller); } catch (const std::exception& e) { ae = e.what(); }
    try { records::invoke(*b, index, &modern); } catch (const std::exception& e) { be = e.what(); }
    const auto label = hex_address(records::entries[index]) + " " + name;
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
    if (name == "normal" || name == "unaligned-stack" || name == "minimal-observed-mapping" || expect_normal) require(ae.empty(), label + " must complete normally: " + ae);
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
    const auto row = fixture + records::row_stride, first = control + 0x200, second = control + 0x600;
    const auto accesses = compare(index, *seed, "normal", {}, [](const State& s, const Dependencies& d) {
        require(s.pc == caller && s.r<4>(4) == stack && s.r<4>(8) == relocated_toc, "Independent history caller ABI");
        require(s.r<4>(12) == 1 && s.memory.read<2>(fixture + 0x2c58) == 1, "Original successful result and selected-row restoration");
        const auto node = s.memory.read<4>(row + 16);
        require(d.allocations == 1 && node == control + 0x3000 && s.memory.read<4>(node) == 9 &&
            s.memory.read<4>(node + 8) == control + 0x2400 && s.memory.read<4>(node + 12) == 0x87654321 &&
            s.memory.read<4>(node + 0x1f4) == control + 0x200 && s.memory.read<4>(control + 0x3f8) == node &&
            s.memory.read<2>(row + 22) == 2 && s.memory.read<1>(row + 24) == 1 &&
            s.memory.read<2>(row + 28) == 0xffff, "Independent new-node fields, links, count and cache flags");
    });
    compare(index, *seed, "own-link-negative", {}, {}, true);
    for (unsigned shift : {1u, 3u}) { auto s = clone(*seed); s->wr<4>(4, stack + shift); s->memory.write<4>(stack + shift + 8, caller); compare(index, *s, "unaligned-stack"); }
    compare(index, *seed, "dependency-fault", [](Monitor&, Dependencies& d) {
        d.before = [](State& s, std::uint32_t) { s.wr<4>(0, 0x12345678); throw std::runtime_error("Fixture history dependency fault"); };
    });
    const auto variant = [&](const std::string& name, std::function<void(State&)> setup, Configure callback = {}, bool expect_normal = true) {
        auto s = clone(*seed); setup(*s); compare(index, *s, name, callback, {}, false, expect_normal);
    };
    variant("null-owner", [](State& s) { s.wr<4>(12, 0); }, {}, false);
    for (auto selected : {0u, 15u, 0xffffu, 0x8000u}) variant("signed-selected-row", [selected](State& s) { s.memory.write<2>(fixture + 0x2c58, selected); }, {}, index == 0 || selected < 16);
    if (index == 0) {
        for (auto filter : {0u, 11u, 22u, 33u}) {
            auto two = clone(*seed); two->wr<4>(16, filter); two->memory.write<2>(fixture + 0x2c3a, 2);
            compare(index, *two, "identifier-filter-two-cached-records", {}, [filter](const State& s, const Dependencies& d) {
                require(s.memory.read<4>(row + 4) == 11 && s.memory.read<4>(row + records::record_stride + 4) == 22,
                    "Cache exchange restores the original record order");
                require(s.memory.read<2>(row + 22) == (filter == 0 || filter == 11 ? 2 : 1) &&
                    s.memory.read<2>(row + records::record_stride + 22) == (filter == 0 || filter == 22 ? 2 : 1) &&
                    d.allocations == (filter == 0 ? 2u : filter == 33 ? 0u : 1u),
                    "Only matching cached records retain their appended history and counts");
            }, false, true);
        }
        for (auto count : {0u, 1u, 0xffffu, 0x8000u}) variant("live-signed-cache-count", [count](State& s) { s.memory.write<2>(fixture + 0x2c3a, count); });
        variant("child-allocation-redirects-row", [](State&) {}, [](Monitor&, Dependencies& d) {
            d.before = [](State& s, std::uint32_t target) { if (target == 0x10773520) s.memory.write<2>(fixture + 0x2c58, 2); };
        });
    } else {
        for (auto value : {0u, 1u, 0x100u, 0x80u, 0xffu}) variant("low-byte-new-mark", [value](State& s) { s.wr<4>(24, value); });
        variant("invalid-zero-identifier", [](State& s) { s.wr<4>(16, 0); });
        variant("invalid-zero-auxiliary", [](State& s) { s.wr<4>(20, 0); });
        variant("live-reporting-TOC", [](State& s) { s.wr<4>(16, 0); }, [](Monitor&, Dependencies& d) {
            d.before = [](State& s, std::uint32_t) { s.wr<4>(8, relocated_toc + 4); };
        }, false);
        variant("allocation-failure-prefix", [](State&) {}, [](Monitor&, Dependencies& d) { d.allocation_failure = true; }, false);
        variant("old-current-CFM-callback", [](State& s) { s.memory.write<4>(first + 4, control + 0x2600); });
        variant("callback-redirects-current-row", [](State& s) { s.memory.write<4>(first + 4, control + 0x2600); }, [](Monitor&, Dependencies& d) {
            d.before = [](State& s, std::uint32_t target) { if (target == 0x10781320) s.memory.write<2>(fixture + 0x2c58, 2); };
        });
        for (auto count : {100u, 101u, 0xffffu, 0x8000u}) for (auto mark : {0u, 0x80u})
            variant("signed-history-limit-and-marked-head", [count,mark](State& s) {
                s.memory.write<2>(row + 22, count); s.memory.write<4>(first + 0x1f8, second);
                s.memory.write<4>(second + 0x1f4, first); s.memory.write<4>(row + 12, second); s.memory.write<4>(row + 16, second);
                s.memory.write<1>(first + 16, mark);
            });
        for (auto owned : {0u, 1u, 0x80u, 0xffu}) variant("truncate-forward-owned-fields", [owned](State& s) {
            s.memory.write<4>(row + 12, second); s.memory.write<2>(row + 22, 2); s.memory.write<4>(first + 0x1f8, second);
            s.memory.write<4>(second + 0x1f4, first); s.memory.write<4>(second + 20, control + 0x2400);
            s.memory.write<1>(second + 24, owned); s.memory.write<4>(second + 20 + 59 * 8, control + 0x2410); s.memory.write<1>(second + 24 + 59 * 8, owned);
        });
        for (auto last_id : {9u, 10u}) variant("matching-mark-selects-last-or-restores", [last_id](State& s) {
            s.wr<4>(24, 1); s.memory.write<1>(first + 16, 1); s.memory.write<4>(first, 9);
            s.memory.write<4>(first + 0x1f8, second); s.memory.write<4>(second + 0x1f4, first);
            s.memory.write<1>(second + 16, 0x80); s.memory.write<4>(second, last_id); s.memory.write<4>(row + 12, second);
        });
        variant("marked-no-match-appends", [](State& s) { s.wr<4>(24, 1); s.memory.write<1>(first + 16, 0x80); });
        for (auto count : {0u, 2u, 0xffffu}) variant("published-argument-fallback-and-borrowed-fields", [count](State& s) {
            for (unsigned field : {4u,8u,12u,16u}) s.memory.write<4>(row + field, 0);
            s.memory.write<2>(row + 22, 0); s.memory.write<4>(fixture + 0x2c00, 17);
            s.memory.write<4>(fixture + 0x2c04, control + 0x2400); s.memory.write<1>(fixture + 0x2c08, 0x80);
            s.memory.write<4>(fixture + 0x2c0c, 0); s.memory.write<4>(fixture + 0x2c10, 0x87654321); s.memory.write<2>(fixture + 0x2c18, count);
        });
        variant("mark-read-changes-selected-row", [](State& s) { s.wr<4>(24, 1); s.memory.write<1>(first + 16, 1); s.memory.write<4>(first, 9); }, [](Monitor& m, Dependencies&) {
            m.after_read = [](State& s, std::uint32_t address) { if (address == first + 16) s.memory.write<2>(fixture + 0x2c58, 2); };
        });
    }
    std::set<std::uint32_t> bytes;
    for (const auto& access : accesses) for (unsigned n = 0; n < access.size; ++n) bytes.insert(access.address + n);
    auto small = minimal(*seed, accesses); compare(index, *small, "minimal-observed-mapping");
    for (auto hole : bytes) { auto s = minimal(*seed, accesses, hole); compare(index, *s, "mapping-hole"); }
    auto counted = clone(*small); Dependencies counter(true); counter.reference(*counted, caller);
    for (std::uint64_t budget = 0; budget < counted->executed; ++budget) { auto s = clone(*small); s->budget = budget; compare(index, *s, "exact-finite-budget"); }
}
void child_negative() {
    auto seed = initial(0), a = clone(*seed), b = clone(*seed);
    Monitor x(*a), y(*b); Dependencies old(true), modern(false); modern.corrupt_child = true;
    old.reference(*a, caller); records::invoke(*b, 0, &modern);
    bool rejected = false; try { Image(*a).compare(*b, "Corrupted history child"); } catch (const std::runtime_error&) { rejected = true; }
    require(rejected && x.effects.bytes != y.effects.bytes, "Parent equivalence rejects a child field mutation");
}
void diagnostics() {
    State a, b; auto& d = record_history_diagnostics(a); d.original = true; d.entry_original[1] = true; d.calls[1] = 17;
    utility_object_diagnostics(a).calls[0] = 39;
    require(!record_history_diagnostics(std::as_const(b)).calls[1] && !b.runtime_diagnostics, "Allocation-free const records query");
    reset_record_history_diagnostics(a);
    require(!d.original && !d.entry_original[1] && !d.calls[1] && utility_object_diagnostics(a).calls[0] == 39, "Record reset preserves utility counters");
}
}
int main() {
    try {
        diagnostics(); child_negative(); for (unsigned i = 0; i < records::entries.size(); ++i) {
            try { matrix(i); } catch (const std::exception& e) { throw std::runtime_error(hex_address(records::entries[i]) + " matrix: " + e.what()); }
        }
        unsigned total = 0, failed = 0;
        for (unsigned i = 0; i < records::entries.size(); ++i) {
            require(comparisons[i] && faults[i] && negatives[i] == 1, "Dedicated records component evidence");
            std::cout << hex_address(records::entries[i]) << " comparisons=" << comparisons[i] << " faults=" << faults[i] << " negatives=" << negatives[i] << '\n';
            total += comparisons[i]; failed += faults[i];
        }
        std::cout << "Record component comparisons=" << total << " faults=" << failed << " budget_stops=" << budget_stops << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
