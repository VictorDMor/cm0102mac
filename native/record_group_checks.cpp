#include "record_group_adapter.hpp"
#include "utility_object_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <set>
#include <utility>
namespace cm::aot { bool record_group_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace records = cm::aot::record_group;
constexpr std::uint32_t caller = 0x7fff1000, relocated_toc = 0x32010000;
constexpr std::uint32_t control = fixture + 0x258400;
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

struct Dependencies final : records::DependencyDispatch {
    bool original;
    bool allocation_failure = false;
    std::vector<std::array<std::uint32_t, 12>> calls;
    std::vector<decltype(State::registers)> call_registers;
    std::function<void(State&, std::uint32_t)> before;
    explicit Dependencies(bool source) : original(source) {}
    void reference(State& s, [[maybe_unused]] std::uint32_t continuation) {
        bool initial = true;
        for (;;) {
            const auto child = std::find(records::entries.begin(), records::entries.end(), s.pc);
            if (!initial && child != records::entries.end()) call(s, s.pc, std::uint32_t(s.r<4>(4128)));
            else if (!record_group_reference(s)) call(s, s.pc, std::uint32_t(s.r<4>(4128)));
            else if (s.pc == s.r<4>(4128)) return;
            initial = false;
        }
    }
    void call(State& s, std::uint32_t target, std::uint32_t continuation) override {
        require(s.pc == target && s.r<4>(4128) == continuation && s.r<4>(2440) == s.r<4>(8), "Published records dependency ABI");
        calls.push_back({target, continuation, std::uint32_t(s.r<4>(4)), std::uint32_t(s.r<4>(8)),
            std::uint32_t(s.r<4>(12)), std::uint32_t(s.r<4>(16)), std::uint32_t(s.r<4>(20)),
            std::uint32_t(s.r<4>(24)), std::uint32_t(s.r<4>(48)), std::uint32_t(s.r<4>(2432)),
            std::uint32_t(s.r<4>(2440)), s.pc});
        call_registers.push_back(s.registers);
        if (before) before(s, target);
        const auto child = std::find(records::entries.begin(), records::entries.end(), target);
        if (child != records::entries.end()) {
            if (original) reference(s, continuation);
            else records::invoke(s, unsigned(child - records::entries.begin()), this);
            if (s.pc != continuation) throw std::runtime_error("Fixture records dependency escaped to " + hex_address(s.pc));
            return;
        }
        s.step(target);
        const auto owner = std::uint32_t(s.r<4>(12));
        if (target == 0x10773610 || target == 0x107735a0) s.memory.write<1>(owner + 15, 0xde);
        else if (target == 0x107735f0) s.wr<4>(12, allocation_failure ? 0 : control + 0x1400);
        else if (target == 0x1077dbd0) {
            unsigned length = 0; while (s.memory.read<1>(owner + length)) ++length;
            s.wr<4>(12, length);
        } else if (target == 0x10739ef0 || target == 0x10745e00) {
            std::vector<std::uint8_t> bytes;
            for (std::uint32_t n = 0; n < s.r<4>(20); ++n) bytes.push_back(s.memory.read<1>(s.r<4>(16) + n));
            s.memory.copy_in(owner, bytes);
        } else if (target == 0x10778300) {
            s.memory.copy_in(owner, std::vector<std::uint8_t>(s.r<4>(20), std::uint8_t(s.r<4>(16))));
        } else if (target == 0x1077dbf0) {
            const auto source = std::uint32_t(s.r<4>(16)); unsigned length = 0;
            while (s.memory.read<1>(source + length)) ++length;
            s.memory.copy_in(owner, s.memory.copy_out(source, length + 1));
        } else if (target == 0x10781320) {
            s.memory.write<4>(s.r<4>(4) + 20, s.r<4>(8));
            s.memory.write<1>(control + 0x1f00, 0x91);
        } else if (target == 0x10739890) {
            s.memory.write<4>(s.r<4>(24), 0); s.memory.write<4>(s.r<4>(28), 0);
        } else if (target == 0x10768810) s.wr<4>(12, control + 0x1700);
        else if (target == 0x1077b720 || target == 0x10769b60 || target == 0x1076f7d0 ||
                 target == 0x10718f30 || target == 0x1073e4b0 || target == 0x1073a4c0) {
            s.memory.write<1>(control + 0x1f00, 0x37);
        } else throw std::runtime_error("Unknown record dependency " + hex_address(target));
        s.wr<4>(0, 0x24681357); ++s.native_calls;
        s.step(target + 4); s.pc = std::uint32_t(s.r<4>(4128));
    }
};
std::unique_ptr<State> initial(unsigned index) {
    auto s = std::make_unique<State>(); s->registers.fill(0x6d); s->unique.fill(0xb3); s->native_calls = 17;
    for (auto [a, n] : std::array<std::pair<std::uint32_t, unsigned>, 3>{{
        {relocated_toc - 0x8000, 0x8000}, {stack - 4096, 8192}, {fixture, 0x25b000}}}) {
        s->memory.map(a, n); s->memory.copy_in(a, std::vector<std::uint8_t>(n, 0xa7));
    }
    s->wr<4>(4, stack); s->wr<4>(8, relocated_toc); s->wr<4>(4128, caller); s->memory.write<4>(stack + 8, caller);
    s->memory.write<4>(relocated_toc - 0x7bb0, fixture);
    s->memory.write<4>(relocated_toc - 0x7bac, control + 0x1800);
    s->memory.write<4>(relocated_toc - 0x7ba8, control + 0x1900);
    s->memory.write<4>(relocated_toc - 0x3e5c, control + 0x1a00);
    for (unsigned row = 0; row < records::row_count; ++row) {
        const auto base = fixture + row * records::row_stride;
        s->memory.copy_in(base, std::vector<std::uint8_t>(records::row_stride, 0));
        s->memory.write<2>(fixture + 0x2c38 + row * 2, row == 1 ? 2 : 0);
        s->memory.write<2>(base, row == 1 ? 1 : 0);
        s->memory.write<4>(base + 4, 11); s->memory.write<4>(base + records::record_stride + 4, 22);
        s->memory.write<4>(base + 36, fixture + 0x2c68);
    }
    const auto row = fixture + records::row_stride;
    s->memory.write<4>(row + 8, control + 0x200); s->memory.write<4>(row + 16, control + 0x200); s->memory.write<2>(row + 22, 1);
    s->memory.copy_in(control + 0x200, std::vector<std::uint8_t>(0x400, 0));
    s->memory.copy_in(control + 0x3fc, {'l','a','b','e','l',0});
    s->memory.copy_in(control + 0x600, std::vector<std::uint8_t>(0x400, 0));
    s->memory.copy_in(control + 0x7fc, {'s','e','c','o','n','d',0});
    s->memory.write<4>(row + records::record_stride + 8, control + 0x600);
    s->memory.write<4>(row + records::record_stride + 16, control + 0x600);
    s->memory.write<2>(row + records::record_stride + 22, 1);
    s->memory.write<4>(control + 0x624, control + 0x1200);
    s->memory.copy_in(control + 0x1200, {'n','e','w',0});
    s->memory.write<4>(control + 0x224, control + 0x1200);
    s->memory.write<2>(fixture + 0x2c58, 1); s->memory.write<1>(fixture + 0x2c5e, 1); s->memory.write<4>(fixture + 0x2c64, 0);
    s->memory.write<4>(fixture + 0x2581b0, 0); s->memory.write<4>(fixture + 0x2581b4, 0);
    s->memory.write<4>(fixture + 0x2c2c, control + 0x1600); s->memory.write<4>(fixture + 0x2c30, 0); s->memory.write<4>(fixture + 0x2c34, 16);
    s->wr<4>(12, fixture); s->wr<4>(16, index == 8 ? 11 : 2); s->wr<4>(20, index == 8 ? 22 : control + 0x1200);
    s->wr<4>(24, 0); s->wr<4>(28, 0);
    if (index == 1) s->wr<4>(16, control + 0x1200);
    if (index == 4) { s->wr<4>(16, 0); s->wr<4>(20, 2); s->wr<4>(24, control + 0x1200); }
    if (index == 6 || index == 7) s->wr<4>(12, 1);
    if (index == 9) { s->wr<4>(16, 0); s->wr<4>(20, 0); }
    if (index == 10) { s->wr<4>(16, 11); s->wr<4>(20, 1); }
    s->pc = records::entries[index]; s->budget = 30000; return s;
}
using Configure = std::function<void(Monitor&, Dependencies&)>;
std::vector<Effects::Access> compare(unsigned index, const State& seed, const std::string& name,
    Configure configure = {}, std::function<void(const State&, const Dependencies&)> expectation = {}, bool negative = false) {
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
    if (name == "normal") require(ae.empty(), label + " must complete normally: " + ae);
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
    const auto accesses = compare(index, *seed, "normal", {}, [index](const State& s, const Dependencies&) {
        require(s.pc == caller && s.r<4>(4) == stack && s.r<4>(8) == relocated_toc, "Independent record caller ABI");
        if (index == 0) require(s.r<4>(12) == control + 0x3fc, "Current label uses the live record");
        if (index == 3) require(s.r<4>(12) == control + 0x1200, "Current field pointer");
        if (index == 2) require(!s.memory.read<4>(fixture + records::row_stride + 8), "Released chain header");
        if (index == 8) require(s.r<4>(12) == 1 && s.memory.read<4>(fixture + records::row_stride + 4) == 22 &&
            s.memory.read<4>(fixture + records::row_stride + records::record_stride + 4) == 11, "Both identifiers swapped");
    });
    compare(index, *seed, "own-link-negative", {}, {}, true);
    for (unsigned shift : {1u, 3u}) { auto s = clone(*seed); s->wr<4>(4, stack + shift); s->memory.write<4>(stack + shift + 8, caller); compare(index, *s, "unaligned-stack"); }
    compare(index, *seed, "dependency-fault", [](Monitor&, Dependencies& d) {
        d.before = [](State& s, std::uint32_t) { s.wr<4>(0, 0x12345678); throw std::runtime_error("Fixture record callback fault"); };
    });
    compare(index, *seed, "live-TOC", [](Monitor&, Dependencies& d) { d.before = [](State& s, std::uint32_t) { s.wr<4>(8, relocated_toc + 4); }; });
    for (auto selected : {0u, 15u, 0xffffu, 0x8000u}) {
        auto s = clone(*seed); s->memory.write<2>(fixture + 0x2c58, selected); compare(index, *s, "signed-selected-row");
    }
    const auto variant = [&](const std::string& name, std::function<void(State&)> setup, Configure callback = {}) {
        auto s = clone(*seed); setup(*s); compare(index, *s, name, callback);
    };
    if (index <= 1 || index == 3 || index == 5) {
        variant("absent-current-record", [](State& s) { s.memory.write<4>(fixture + records::row_stride + 16, 0); });
    }
    if (index == 1) variant("null-label-source", [](State& s) { s.wr<4>(16, 0); });
    if (index == 2) {
        variant("empty-chain", [](State& s) { s.memory.write<4>(fixture + records::row_stride + 8, 0); });
        variant("two-node-chain", [](State& s) { s.memory.write<4>(control + 0x3f8, control + 0x600); });
        for (auto owned : {0u, 1u, 0x80u, 0xffu}) variant("signed-byte-owned-field", [owned](State& s) { s.memory.write<1>(control + 0x228, owned); });
        variant("release-callback-redirects-selected-row", [](State&) {}, [](Monitor&, Dependencies& d) {
            d.before = [](State& s, std::uint32_t target) { if (target == 0x107735a0) s.memory.write<2>(fixture + 0x2c58, 2); };
        });
    }
    if (index == 3 || index == 5) {
        for (auto field : {0u, 59u, 60u, 0xffffu, 0xffffffffu, 0x7fffffffu})
            variant("full-signed-field-index", [field](State& s) { s.wr<4>(16, field); });
    }
    if (index == 4) {
        for (auto filter : {0u, 11u, 22u, 33u}) variant("identifier-filter", [filter](State& s) { s.wr<4>(16, filter); });
        for (auto count : {0u, 1u, 0xffffu}) variant("live-signed-row-count", [count](State& s) { s.memory.write<2>(fixture + 0x2c3a, count); });
        variant("bulk-clone-owned-field", [](State& s) { s.wr<4>(28, 1); });
    }
    if (index == 5) {
        for (auto copy : {0u, 1u, 0x100u, 0x80u, 0xffu}) variant("low-byte-copy-flag", [copy](State& s) { s.wr<4>(24, copy); });
        variant("null-new-field", [](State& s) { s.wr<4>(20, 0); s.wr<4>(24, 1); });
        variant("replace-owned-field", [](State& s) { s.memory.write<1>(control + 0x228, 0xff); s.wr<4>(24, 1); });
        variant("copy-allocation-fails", [](State& s) { s.wr<4>(24, 1); }, [](Monitor&, Dependencies& d) { d.allocation_failure = true; });
        variant("release-republishes-current-record", [](State& s) { s.memory.write<1>(control + 0x228, 1); }, [](Monitor&, Dependencies& d) {
            d.before = [](State& s, std::uint32_t target) { if (target == 0x10773610) s.memory.write<4>(fixture + records::row_stride + 16, control + 0x600); };
        });
    }
    if (index == 6) {
        variant("attached-stream-with-no-pending-flush", [](State& s) { s.memory.write<4>(fixture + 0x2c64, control); s.memory.write<1>(fixture + 0x2c5e, 0); });
        variant("matching-group-insertion-shifts-records", [](State& s) { s.memory.write<2>(fixture + 0x2c38, 1); s.memory.write<2>(fixture, 2); });
        variant("matching-group-full", [](State& s) { s.memory.write<2>(fixture + 0x2c38, 1); s.memory.write<2>(fixture, 2); s.memory.write<2>(fixture + 0x2c3a, 16); });
        variant("all-sixteen-groups-full", [](State& s) {
            s.wr<4>(12, 31); for (unsigned row = 0; row < records::row_count; ++row) {
                s.memory.write<2>(fixture + 0x2c38 + row * 2, 1); s.memory.write<2>(fixture + row * records::row_stride, row);
            }
        });
        for (auto type : {0u, 0xffffu, 0x8000u, 0x10000u}) variant("signed-low-half-group-type", [type](State& s) { s.wr<4>(12, type); });
        variant("live-insertion-callback", [](State& s) { s.memory.write<4>(fixture + 0x2581b0, control + 0x1e00); });
    }
    if (index == 7) {
        variant("no-pending-removal", [](State& s) { s.memory.write<1>(fixture + 0x2c5e, 0); });
        variant("no-matching-group-type", [](State& s) { s.wr<4>(12, 31); });
    }
    if (index == 8) {
        variant("missing-identifier", [](State& s) { s.wr<4>(16, 33); });
        variant("same-identifiers", [](State& s) { s.wr<4>(20, 11); });
        variant("last-duplicate-match", [](State& s) {
            s.memory.write<2>(fixture + 0x2c3a, 3); s.memory.write<4>(fixture + records::row_stride + records::record_stride * 2 + 4, 11);
        });
        variant("first-store-redirects-the-second-swap", [](State&) {}, [](Monitor& m, Dependencies&) {
            m.after_write = [](State& s, std::uint32_t address) { if (address == fixture + records::row_stride + 4) s.memory.write<2>(fixture + 0x2c58, 2); };
        });
    }
    if (index == 9) {
        variant("insertion-extra-field", [](State& s) { s.wr<4>(24, 0x12345678); });
        variant("insertion-callback-vectors", [](State& s) { s.wr<4>(16, control + 0x1e00); s.wr<4>(20, control + 0x1e10); });
    }
    if (index == 10) {
        variant("search-first-record", [](State& s) { s.wr<4>(20, 0xffff); });
        variant("search-later-record", [](State& s) { s.wr<4>(16, 22); s.wr<4>(20, 0xffffffff); });
        variant("search-missing-record", [](State& s) { s.wr<4>(16, 33); s.wr<4>(20, 0xffffffff); });
        variant("last-record-in-nonzero-group", [](State& s) { s.memory.write<2>(fixture + 0x2c3a, 1); });
        variant("last-record-in-first-group", [](State& s) { s.wr<4>(20, 0); s.memory.write<2>(fixture + 0x2c38, 1); });
        variant("stream-marker-buffer-full", [](State& s) { s.memory.write<2>(fixture + 0x2c3a, 1); s.memory.write<4>(fixture + 0x2c34, 0); });
        variant("release-callback-vectors", [](State& s) {
            s.memory.write<4>(control + 0x204, control + 0x1e00); s.memory.write<4>(fixture + 0x2581b4, control + 0x1e10);
        });
    }
    // Missing-byte and budget matrices cover the successful bounded fixture's
    // exact effects; additional branch fixtures are characterized separately.
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
    State a, b; auto& d = record_group_diagnostics(a); d.original = true; d.entry_original[1] = true; d.calls[10] = 17;
    utility_object_diagnostics(a).calls[0] = 39;
    require(!record_group_diagnostics(std::as_const(b)).calls[10] && !b.runtime_diagnostics, "Allocation-free const records query");
    reset_record_group_diagnostics(a);
    require(!d.original && !d.entry_original[1] && !d.calls[10] && utility_object_diagnostics(a).calls[0] == 39, "Record reset preserves utility counters");
}
}
int main() {
    try {
        diagnostics(); for (unsigned i = 0; i < records::entries.size(); ++i) {
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
