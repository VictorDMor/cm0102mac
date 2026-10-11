#include "record_query_adapter.hpp"
#include "utility_object_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <set>
#include <utility>
namespace cm::aot { bool record_query_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace records = cm::aot::record_query;
constexpr std::uint32_t caller = 0x7fff1000, relocated_toc = 0x32010000;
constexpr std::uint32_t control = fixture + 0x3000;
std::array<unsigned, 6> comparisons{}, faults{}, negatives{};
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
    std::vector<std::array<std::uint32_t, 12>> calls;
    std::vector<decltype(State::registers)> call_registers;
    std::function<void(State&, std::uint32_t)> before;
    explicit Dependencies(bool) {}
    void reference(State& s, std::uint32_t continuation) {
        while (s.pc != continuation) {
            if (!cm::aot::record_query_reference(s)) throw std::runtime_error("Unknown query reference PC " + hex_address(s.pc));
            if (s.pc != continuation && std::find(records::entries.begin(), records::entries.end(), s.pc) == records::entries.end()) call(s, s.pc, std::uint32_t(s.r<4>(4128)));
        }
    }
    void call(State& s, std::uint32_t target, std::uint32_t continuation) override {
        require(s.pc == target && s.r<4>(4128) == continuation && s.r<4>(2440) == s.r<4>(8), "Published query dependency ABI");
        std::array<std::uint32_t, 12> tuple{target};
        for (unsigned n = 1; n < tuple.size(); ++n) tuple[n] = std::uint32_t(s.r<4>(n <= 8 ? 8 + n * 4 : n == 9 ? 4 : n == 10 ? 8 : 4128));
        calls.push_back(tuple); call_registers.push_back(s.registers);
        if (before) before(s, target);
        s.step(target);
        if (target == 0x10739890) { s.memory.write<4>(s.r<4>(24), 0); s.memory.write<4>(s.r<4>(28), 0); }
        else if (target == 0x10768810) s.wr<4>(12, control + 0x700);
        else if (target == 0x1077b720 || target == 0x10769b60 || target == 0x1076f7d0) s.memory.write<1>(control + 0x900, 0x37);
        else throw std::runtime_error("Unknown query dependency " + hex_address(target));
        s.wr<4>(0, 0x24681357); ++s.native_calls; s.step(target + 4); s.pc = std::uint32_t(s.r<4>(4128));
    }
};
std::unique_ptr<State> initial(unsigned index) {
    auto s = std::make_unique<State>(); s->registers.fill(0x6d); s->unique.fill(0xb3); s->native_calls = 17;
    for (auto [a, n] : std::array<std::pair<std::uint32_t, unsigned>, 3>{{
        {relocated_toc - 0x8000, 0x8000}, {stack - 4096, 8192}, {fixture, 0x5000}}}) {
        s->memory.map(a, n); s->memory.copy_in(a, std::vector<std::uint8_t>(n, 0xa7));
    }
    s->wr<4>(4, stack); s->wr<4>(8, relocated_toc); s->wr<4>(4128, caller); s->memory.write<4>(stack + 8, caller);
    s->memory.write<4>(relocated_toc - 0x7bac, control + 0x800); s->memory.write<4>(relocated_toc - 0x7ba8, control + 0x900);
    s->memory.write<4>(relocated_toc - 0x3e5c, control + 0xa00);
    for (unsigned row = 0; row < records::row_count; ++row) {
        const auto base = fixture + row * records::row_stride;
        s->memory.copy_in(base, std::vector<std::uint8_t>(records::row_stride, 0));
        s->memory.write<2>(fixture + 0x2c38 + row * 2, row == 1 ? 3 : 0);
    }
    s->memory.copy_in(control + 0x200, std::vector<std::uint8_t>(0x300, 0));
    s->memory.write<4>(fixture + records::row_stride + 16, control + 0x200);
    s->memory.write<1>(fixture + records::row_stride + records::record_stride + 25, 1);
    s->memory.write<1>(fixture + records::row_stride + records::record_stride * 2 + 26, 0x80);
    s->memory.write<2>(fixture + 0x2c58, 1);
    s->wr<4>(12, fixture); s->wr<4>(16, 0); s->wr<4>(20, 0x12345678); s->wr<4>(24, 0x180);
    s->wr<4>(28, 0x87654321); s->wr<4>(32, 0xffffffff); s->wr<4>(36, 7); s->wr<4>(40, 0x18001);
    s->pc = records::entries[index]; s->budget = 3000; return s;
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
        require(s.pc == caller && s.r<4>(4) == stack && s.r<4>(8) == relocated_toc, "Independent query caller ABI");
        if (index == 0) require(s.memory.read<4>(fixture + 0x2c04) == 0x12345678 && s.memory.read<1>(fixture + 0x2c08) == 0x80 && s.memory.read<2>(fixture + 0x2c18) == 0x8001, "Published arguments retain byte/halfword truncation");
        if (index == 1) require(s.r<4>(12) == 1, "Only the record with both marks zero is counted");
        if (index >= 2) require(s.r<4>(12) == unsigned(index < 4), "Independent current-node query result");
    });
    compare(index, *seed, "own-link-negative", {}, {}, true);
    for (unsigned shift : {1u, 3u}) { auto s = clone(*seed); s->wr<4>(4, stack + shift); s->memory.write<4>(stack + shift + 8, caller); compare(index, *s, "unaligned-stack"); }
    const auto variant = [&](const std::string& name, std::function<void(State&)> setup, Configure callback = {}) {
        auto s = clone(*seed); setup(*s); compare(index, *s, name, callback);
    };
    variant("null-owner", [](State& s) { s.wr<4>(12, 0); });
    for (auto selected : {0u, 15u, 0xffffu, 0x8000u}) variant("signed-selected-row", [selected](State& s) { s.memory.write<2>(fixture + 0x2c58, selected); });
    if (index == 0) {
        variant("unaligned-owner", [](State& s) { s.wr<4>(12, fixture + 1); });
        variant("stack-aliasing-owner", [](State& s) { s.wr<4>(12, stack - 0x2c00); });
        variant("publisher-callback-redirects-owner", [](State&) {}, [](Monitor& m, Dependencies&) {
            m.after_write = [](State& s, std::uint32_t a) { if (a == fixture + 0x2c00) s.wr<4>(12, fixture + 32); };
        });
    }
    if (index == 1) {
        for (auto count : {0u, 1u, 0xffffu, 0x8000u}) variant("signed-live-row-count", [count](State& s) { s.memory.write<2>(fixture + 0x2c3a, count); });
        for (auto mark : {0u, 1u, 0x80u, 0xffu}) variant("both-signed-byte-marks", [mark](State& s) {
            s.memory.write<1>(fixture + records::row_stride + 25, mark); s.memory.write<1>(fixture + records::row_stride + 26, mark);
        });
        for (bool change_row : {false, true}) variant("mark-read-changes-live-count-or-row", [](State&) {}, [change_row](Monitor& m, Dependencies&) {
            m.after_read = [change_row](State& s, std::uint32_t a) {
                if (a == fixture + records::row_stride + 25) {
                    if (change_row) s.memory.write<2>(fixture + 0x2c58, 2);
                    else s.memory.write<2>(fixture + 0x2c3a, 0);
                }
            };
        });
    }
    if (index >= 2) {
        variant("absent-current-node", [](State& s) { s.memory.write<4>(fixture + records::row_stride + 16, 0); });
        auto absent = clone(*seed); absent->memory.write<4>(fixture + records::row_stride + 16, 0);
        compare(index, *absent, "live-reporting-TOC", [](Monitor&, Dependencies& d) { d.before = [](State& s, std::uint32_t) { s.wr<4>(8, relocated_toc + 4); }; });
        compare(index, *absent, "reporting-dependency-fault", [](Monitor&, Dependencies& d) {
            d.before = [](State& s, std::uint32_t) { s.wr<4>(0, 0x12345678); throw std::runtime_error("Fixture query reporting fault"); };
        });
        variant("validation-read-selects-a-new-row", [](State& s) {
            s.memory.copy_in(control + 0x600, std::vector<std::uint8_t>(0x300, 0));
            s.memory.write<4>(fixture + records::row_stride * 2 + 16, control + 0x600);
            s.memory.write<4>(control + 0x7f8, 1); s.memory.write<4>(control + 0x60c, 1);
            s.memory.write<1>(control + 0x610, 0x80);
        }, [](Monitor& m, Dependencies&) {
            m.after_read = [](State& s, std::uint32_t a) {
                if (a == fixture + records::row_stride + 16) s.memory.write<2>(fixture + 0x2c58, 2);
            };
        });
    }
    if (index == 2 || index == 5) {
        for (auto pointer : {0u, 1u, 0x80000000u, 0xffffffffu}) variant("zero-test-word32", [index, pointer](State& s) { s.memory.write<4>(control + 0x200 + (index == 2 ? 0x1f8 : 12), pointer); });
    }
    if (index == 3) {
        for (auto option : {0u, 1u, 0x100u, 0x80u, 0xffu}) for (auto mark : {0u, 1u, 0x80u, 0xffu})
            variant("first-node-option-and-mark", [option,mark](State& s) { s.memory.write<4>(control + 0x3f4, 1); s.memory.write<1>(control + 0x210, mark); s.wr<4>(16, option); });
    }
    if (index == 4) for (auto mark : {0u, 1u, 0x80u, 0xffu}) variant("unsigned-mark-result", [mark](State& s) { s.memory.write<1>(control + 0x210, mark); });
    std::set<std::uint32_t> bytes;
    for (const auto& access : accesses) for (unsigned n = 0; n < access.size; ++n) bytes.insert(access.address + n);
    auto small = minimal(*seed, accesses); compare(index, *small, "minimal-observed-mapping");
    for (auto hole : bytes) { auto s = minimal(*seed, accesses, hole); compare(index, *s, "mapping-hole"); }
    auto counted = clone(*small); Dependencies counter(true); counter.reference(*counted, caller);
    for (std::uint64_t budget = 0; budget < counted->executed; ++budget) { auto s = clone(*small); s->budget = budget; compare(index, *s, "exact-finite-budget"); }
}
void diagnostics() {
    State a, b; auto& d = record_query_diagnostics(a); d.original = true; d.entry_original[1] = true; d.calls[5] = 17;
    utility_object_diagnostics(a).calls[0] = 39;
    require(!record_query_diagnostics(std::as_const(b)).calls[5] && !b.runtime_diagnostics, "Allocation-free const records query");
    reset_record_query_diagnostics(a);
    require(!d.original && !d.entry_original[1] && !d.calls[5] && utility_object_diagnostics(a).calls[0] == 39, "Record reset preserves utility counters");
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
