#pragma once
#include "stdlib_core.hpp"
#include "code_fragment_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool stdlib_core_reference(State&); }
namespace cm::stdlib_core_checks {
using namespace cm::semantic_io_checks;
inline constexpr std::array<std::uint32_t, 4> entries{0x10775540, 0x10775550, 0x10775560, 0x10775600};
inline constexpr std::uint32_t caller = 0x7fff1000, key = fixture + 0x40,
    table = fixture + 0x100, descriptor = fixture + 0x800,
    comparator_import = 280984548, comparator_toc = 0x32008000;
inline std::array<unsigned, 4> comparisons{}, faults{}, negatives{};
inline unsigned accounting_cases = 0, budget_stops = 0, continuations = 0;
inline std::unique_ptr<State> fixture_state(unsigned index) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x6d); s->unique.fill(0xb3);
    for (unsigned n = 0; n < 32; ++n) {
        s->wr<4>(n * 4, 0x2468a000 + n * 0x101);
        s->wr<8>(0x4000 + n * 16, UINT64_C(0xfff0000100000000) + n);
    }
    for (unsigned n = 0; n < 8; ++n) s->wr<1>(2304 + n, 0xa0 | n);
    s->wr<1>(1024, 1); s->wr<1>(1027, 0xa7);
    s->wr<4>(4, stack); s->wr<4>(8, 0x31008000); s->wr<4>(4128, caller);
    s->wr<4>(4132, 0x31415926);
    s->memory.map(fixture, 0x1000); s->memory.copy_in(fixture, std::vector<std::uint8_t>(0x1000, 0xa7));
    s->memory.map(stack - 256, 512); s->memory.copy_in(stack - 256, std::vector<std::uint8_t>(512, 0x5b));
    s->pc = entries[index]; s->executed = 37; s->budget = 20000;
    s->native_calls = 23; s->heap_end = 0x50000300; s->memory_error = -108;
    s->allocated = {{0x50000000, 64}}; s->available = {{0x50000100, 128}};
    s->wr<4>(12, 0xfffffff9); s->wr<4>(16, 3);
    if (index == 3) {
        s->wr<4>(12, key); s->wr<4>(16, table); s->wr<4>(20, 9);
        s->wr<4>(24, 4); s->wr<4>(28, descriptor);
        s->memory.write<4>(key, 7);
        constexpr std::int32_t values[]{-17, -10, -3, 0, 1, 7, 11, 11, 28};
        for (unsigned n = 0; n < std::size(values); ++n) s->memory.write<4>(table + n * 4, std::uint32_t(values[n]));
        s->memory.write<4>(descriptor, comparator_import); s->memory.write<4>(descriptor + 4, comparator_toc);
    }
    return s;
}
struct Comparator final : CodeFragmentBoundaryObserver {
    struct Boundary {
        bool leave; std::string name; decltype(State::registers) registers;
        std::uint32_t pc; std::uint64_t native_calls;
        bool operator==(const Boundary&) const = default;
    };
    std::vector<Boundary> boundaries;
    std::vector<std::array<std::uint32_t, 5>> arguments;
    std::function<void(State&, unsigned)> body;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override {
        boundaries.push_back({false, name, s.registers, s.pc, s.native_calls});
    }
    void import_leave(const State& s, const char* name) override {
        boundaries.push_back({true, name, s.registers, s.pc, s.native_calls});
    }
    bool import_fixture(State& s, const char* name) override {
        require(std::string_view(name) == "CarbonLib::Gestalt", "Explicit comparator fixture import");
        arguments.push_back({std::uint32_t(s.r<4>(12)), std::uint32_t(s.r<4>(16)),
            std::uint32_t(s.r<4>(8)), std::uint32_t(s.r<4>(4)), std::uint32_t(s.r<4>(4128))});
        if (body) body(s, unsigned(arguments.size()));
        else {
            const auto a = std::bit_cast<std::int32_t>(std::uint32_t(s.memory.read<4>(s.r<4>(12))));
            const auto b = std::bit_cast<std::int32_t>(std::uint32_t(s.memory.read<4>(s.r<4>(16))));
            s.wr<4>(12, a < b ? std::uint32_t(-7) : a > b ? 5 : 0);
        }
        s.pc = std::uint32_t(s.r<4>(4128)); return true;
    }
};
struct Monitor final : MemoryObserver {
    Effects effects;
    std::function<void(State&, std::uint32_t, unsigned)> on_write;
    void mapped(std::uint32_t address, std::uint64_t size) override { effects.mapped(address, size); }
    void cleared(std::uint32_t address, std::uint64_t size) override { effects.cleared(address, size); }
    void read(std::uint32_t address, const std::uint8_t* data, unsigned size) override { effects.read(address, data, size); }
    void written(std::uint32_t address, const std::uint8_t* data, std::size_t size) override {
        effects.written(address, data, size); if (on_write) on_write(*effects.state, address, unsigned(size));
    }
};
using Mutation = std::function<void(Monitor&, Comparator&)>;
using Expectation = std::function<void(const State&, const Comparator&)>;
inline void reference_until(State& s, std::uint32_t destination) {
    while (s.pc != destination) {
        if (!stdlib_core_reference(s)) {
            const auto link = std::uint32_t(s.r<4>(4128));
            run_until(s, s.pc == link ? destination : link);
        }
    }
}
inline void semantic(State& s, unsigned index, stdlib_core::SearchExecution* policy = nullptr) {
    switch (index) {
    case 0: stdlib_core::absolute_word(s); break;
    case 1: stdlib_core::absolute_long_word(s); break;
    case 2: stdlib_core::floor_remainder(s); break;
    case 3: stdlib_core::binary_search(s, policy); break;
    }
}
inline std::string run(State& s, unsigned index, bool original, stdlib_core::SearchExecution* policy = nullptr) {
    try {
        if (original) reference_until(s, caller); else semantic(s, index, policy);
    } catch (const std::runtime_error& error) { return error.what(); }
    return {};
}
struct Evidence { std::vector<Effects::Access> log; std::uint64_t original_steps; unsigned callbacks; };
inline Evidence check(unsigned index, const State& initial, const std::string& name,
        const Expectation& expect = {}, const Mutation& mutate = {}, const std::string& expected_error = {},
        bool negative = false, bool accounting = false) {
    auto original = clone(initial), modern = clone(initial); Monitor a, b; Comparator x, y;
    a.effects.state = original.get(); b.effects.state = modern.get();
    if (mutate) { mutate(a, x); mutate(b, y); }
    original->memory.date_observer = &a; modern->memory.date_observer = &b;
    original->code_fragment_observer = &x; modern->code_fragment_observer = &y;
    stdlib_core::SearchExecution policy{0, reference_until};
    const auto ax = run(*original, index, true), bx = run(*modern, index, false, accounting ? &policy : nullptr);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    original->code_fragment_observer = modern->code_fragment_observer = nullptr;
    const auto label = hex_address(entries[index]) + "/" + name;
    require(ax == bx && (expected_error.empty() ? ax.empty() : ax.starts_with(expected_error)),
        label + " outcome original=" + ax + " modern=" + bx);
    Image(*original).compare(*modern, label);
    require(a.effects.bytes == b.effects.bytes && x.arguments == y.arguments && x.boundaries == y.boundaries,
        label + " ordered accesses, comparator arguments and import boundaries");
    if (accounting) {
        require(original->executed == modern->executed, label + " exact search instruction accounting");
        ++accounting_cases; budget_stops += !ax.empty(); continuations += unsigned(policy.budget_resumes);
    } else if (index < 3) require(modern->executed == initial.executed, label + " component accounting changed");
    if (expect && ax.empty()) { expect(*original, x); expect(*modern, y); }
    ++comparisons[index]; faults[index] += !ax.empty();
    if (negative) {
        modern->wr<4>(12, modern->r<4>(12) ^ 1);
        bool rejected = false; try { Image(*original).compare(*modern, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " individual result negative accepted"); ++negatives[index];
    }
    return {a.effects.log, original->executed - initial.executed, unsigned(x.arguments.size())};
}
inline std::unique_ptr<State> without_byte(const State& initial, std::uint32_t omitted) {
    auto s = std::make_unique<State>(); s->registers = initial.registers; s->unique = initial.unique; s->pc = initial.pc;
    s->executed = initial.executed; s->budget = initial.budget; s->native_calls = initial.native_calls;
    s->allocated = initial.allocated; s->available = initial.available; s->heap_end = initial.heap_end; s->memory_error = initial.memory_error;
    for (const auto [address, size] : initial.memory.mapped_ranges()) {
        const auto copy = [&](std::uint32_t start, std::uint64_t length) {
            if (length) { s->memory.map(start, length); s->memory.copy_in(start, initial.memory.copy_out(start, length)); }
        };
        if (omitted >= address && std::uint64_t(omitted) < std::uint64_t(address) + size) {
            copy(address, omitted - address); copy(omitted + 1, std::uint64_t(address) + size - omitted - 1);
        } else copy(address, size);
    }
    return s;
}
inline void fault_matrix(const State& initial, const std::string& name, const Mutation& mutate = {}) {
    const auto evidence = check(3, initial, name + " inventory", {}, mutate); std::set<std::uint32_t> bytes;
    for (const auto& access : evidence.log) {
        bytes.insert(access.address); bytes.insert(access.address + std::uint32_t(access.size) - 1);
    }
    for (auto byte : bytes) {
        auto s = without_byte(initial, byte);
        check(3, *s, name + "/missing-" + hex_address(byte), {}, mutate, "Unmapped data access at ");
    }
}
}
