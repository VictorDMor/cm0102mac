#pragma once
#include "time_runtime.hpp"
#include "code_fragment_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool time_runtime_reference(State&); }
namespace cm::time_runtime_checks {
using namespace cm::semantic_io_checks;
inline constexpr std::uint32_t caller = 0x7fff1000, relocated_toc = 0x31008000,
    fields = fixture + 0x100, timestamp = fixture + 0x200, months = fixture + 0x300,
    clock_descriptor = fixture + 0x800, location_descriptor = fixture + 0x810;
inline std::array<unsigned, 8> comparisons{}, faults{}, negatives{};
inline unsigned exact_cases = 0, budget_stops = 0;
inline std::unique_ptr<State> fixture_state(unsigned index) {
    auto s = std::make_unique<State>(); s->registers.fill(0x6d); s->unique.fill(0xb3);
    for (unsigned n = 0; n < 32; ++n) {
        s->wr<4>(n * 4, 0x2468a000 + n * 0x101);
        s->wr<8>(0x4000 + n * 16, UINT64_C(0xfff0000100000000) + n);
    }
    for (unsigned n = 0; n < 8; ++n) s->wr<1>(2304 + n, 0xa0 | n);
    s->wr<1>(1024, 1); s->wr<1>(1027, 0xa7); s->wr<4>(4, stack); s->wr<4>(8, relocated_toc);
    s->wr<4>(4128, caller); s->wr<4>(4132, 0x31415926);
    for (auto [address, length] : std::array<std::pair<std::uint32_t, unsigned>, 3>{
            {{fixture, 0x1000}, {stack - 512, 1024}, {relocated_toc - 0x8000, 0x10000}}}) {
        s->memory.map(address, length); s->memory.copy_in(address, std::vector<std::uint8_t>(length, 0xa7));
    }
    s->memory.write<4>(relocated_toc - 0x5a28, months);
    s->memory.write<4>(relocated_toc - 0x3b20, fields);
    s->memory.write<4>(relocated_toc - 0x7ee4, clock_descriptor);
    s->memory.write<4>(relocated_toc - 0x7e4c, location_descriptor);
    s->memory.write<4>(clock_descriptor, 280984380); s->memory.write<4>(clock_descriptor + 4, 0x32008000);
    s->memory.write<4>(location_descriptor, 280984532); s->memory.write<4>(location_descriptor + 4, 0x33008000);
    constexpr unsigned rows[2][13]{{0,31,59,90,120,151,181,212,243,273,304,334,365},
                                  {0,31,60,91,121,152,182,213,244,274,305,335,366}};
    for (unsigned row = 0; row < 2; ++row) for (unsigned n = 0; n < 13; ++n)
        s->memory.write<2>(months + row * 26 + n * 2, rows[row][n]);
    s->memory.write<4>(timestamp, 946684800); s->pc = time_runtime::entries[index]; s->executed = 37; s->budget = 50000;
    s->native_calls = 23; s->heap_end = 0x50000300; s->memory_error = -108;
    s->allocated = {{0x50000000, 64}}; s->available = {{0x50000100, 128}};
    s->wr<4>(12, index == 1 ? 100 : index == 2 ? 946684800 : index == 3 ? fields :
        index == 4 ? timestamp : index == 5 ? timestamp : 0xdeadbeef);
    s->wr<4>(16, fields); return s;
}
struct Imports final : CodeFragmentBoundaryObserver {
    struct Boundary {
        bool leave; std::string name; decltype(State::registers) registers;
        std::uint32_t pc; std::uint64_t native_calls;
        bool operator==(const Boundary&) const = default;
    };
    std::vector<Boundary> boundaries;
    std::vector<std::array<std::uint32_t, 4>> arguments;
    std::uint32_t clock = 0x7c25b080u + 946684800u;
    std::array<std::uint32_t, 3> location{0x11223344, 0x55667788, 0x01000000};
    std::function<void(State&, std::string_view)> body;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override {
        boundaries.push_back({false, name, s.registers, s.pc, s.native_calls});
    }
    void import_leave(const State& s, const char* name) override {
        boundaries.push_back({true, name, s.registers, s.pc, s.native_calls});
    }
    bool import_fixture(State& s, const char* name) override {
        const std::string_view service(name);
        require(service == "CarbonLib::GetDateTime" || service == "CarbonLib::ReadLocation", "Explicit clock/location fixture import");
        arguments.push_back({std::uint32_t(s.r<4>(12)), std::uint32_t(s.r<4>(8)),
            std::uint32_t(s.r<4>(4)), std::uint32_t(s.r<4>(4128))});
        if (body) body(s, service);
        else if (service == "CarbonLib::GetDateTime") s.memory.write<4>(s.r<4>(12), clock);
        else for (unsigned n = 0; n < 3; ++n) s.memory.write<4>(s.r<4>(12) + n * 4, location[n]);
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
using Mutation = std::function<void(Monitor&, Imports&)>;
using Expectation = std::function<void(const State&, const Imports&)>;
inline void reference_until(State& s, std::uint32_t destination) {
    while (s.pc != destination) {
        if (!time_runtime_reference(s)) {
            const auto link = std::uint32_t(s.r<4>(4128));
            run_until(s, s.pc == link ? destination : link);
        }
    }
}
inline void resume_reference(State& s, std::uint32_t destination, std::uint32_t) { reference_until(s, destination); }
inline void modern_until(State& s, std::uint32_t destination) {
    while (s.pc != destination) {
        const auto found = std::find(time_runtime::entries.begin(), time_runtime::entries.end(), s.pc);
        if (found != time_runtime::entries.end()) {
            time_runtime::Execution policy{0, resume_reference, modern_until};
            time_runtime::invoke(s, unsigned(found - time_runtime::entries.begin()), &policy);
        } else {
            const auto link = std::uint32_t(s.r<4>(4128));
            run_until(s, s.pc == link ? destination : link);
        }
    }
}
inline std::string run(State& s, bool original) {
    try { if (original) reference_until(s, caller); else modern_until(s, caller); }
    catch (const std::runtime_error& e) { return e.what(); } return {};
}
inline std::vector<Effects::Access> check(unsigned index, const State& initial, const std::string& name,
        const Expectation& expect = {}, const Mutation& mutate = {}, const std::string& expected_error = {},
        bool negative = false, bool exact = false) {
    auto original = clone(initial), modern = clone(initial); Monitor a, b; Imports x, y;
    a.effects.state = original.get(); b.effects.state = modern.get();
    if (mutate) { mutate(a, x); mutate(b, y); }
    original->memory.date_observer = &a; modern->memory.date_observer = &b;
    original->code_fragment_observer = &x; modern->code_fragment_observer = &y;
    const auto ax = run(*original, true), bx = run(*modern, false);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    original->code_fragment_observer = modern->code_fragment_observer = nullptr;
    const auto label = hex_address(time_runtime::entries[index]) + "/" + name;
    require(ax == bx && (expected_error.empty() ? ax.empty() : ax.starts_with(expected_error)), label + " original=" + ax + " modern=" + bx);
    Image(*original).compare(*modern, label);
    require(a.effects.bytes == b.effects.bytes && x.arguments == y.arguments && x.boundaries == y.boundaries,
        label + " ordered accesses and import boundaries");
    if (ax.empty() || exact) {
        require(original->executed == modern->executed, label + " accounting expected=" + std::to_string(original->executed) +
            " actual=" + std::to_string(modern->executed)); ++exact_cases;
    }
    if (expect && ax.empty()) { expect(*original, x); expect(*modern, y); }
    ++comparisons[index]; faults[index] += !ax.empty(); budget_stops += ax == "Native instruction budget exhausted";
    if (negative) {
        modern->wr<4>(12, modern->r<4>(12) ^ 1);
        bool rejected = false; try { Image(*original).compare(*modern, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " own negative accepted"); ++negatives[index];
    }
    return a.effects.log;
}
inline std::unique_ptr<State> without_byte(const State& initial, std::uint32_t omitted) {
    auto s = std::make_unique<State>(); s->registers = initial.registers; s->unique = initial.unique;
    s->pc = initial.pc; s->executed = initial.executed; s->budget = initial.budget; s->native_calls = initial.native_calls;
    s->allocated = initial.allocated; s->available = initial.available; s->heap_end = initial.heap_end; s->memory_error = initial.memory_error;
    for (auto [address, size] : initial.memory.mapped_ranges()) {
        const auto copy = [&](std::uint32_t start, std::uint64_t length) {
            if (length) { s->memory.map(start, length); s->memory.copy_in(start, initial.memory.copy_out(start, length)); }
        };
        if (omitted >= address && std::uint64_t(omitted) < std::uint64_t(address) + size) {
            copy(address, omitted - address); copy(omitted + 1, std::uint64_t(address) + size - omitted - 1);
        } else copy(address, size);
    }
    return s;
}
inline std::unique_ptr<State> accessed_only(const State& initial, const std::vector<Effects::Access>& accesses) {
    auto s = std::make_unique<State>(); s->registers = initial.registers; s->unique = initial.unique;
    s->pc = initial.pc; s->executed = initial.executed; s->budget = initial.budget; s->native_calls = initial.native_calls;
    s->allocated = initial.allocated; s->available = initial.available; s->heap_end = initial.heap_end; s->memory_error = initial.memory_error;
    std::set<std::uint32_t> bytes;
    for (auto access : accesses) for (std::uint64_t n = 0; n < access.size; ++n) bytes.insert(access.address + std::uint32_t(n));
    for (auto it = bytes.begin(); it != bytes.end();) {
        const auto start = *it; auto end = std::uint64_t(start) + 1; ++it;
        while (it != bytes.end() && *it == end) { ++end; ++it; }
        s->memory.map(start, end - start); s->memory.copy_in(start, initial.memory.copy_out(start, end - start));
    }
    return s;
}
inline void fault_matrix(unsigned index, const State& initial, const std::string& name, const Mutation& mutation = {}) {
    const auto accesses = check(index, initial, name + "/inventory", {}, mutation); std::set<std::uint32_t> bytes;
    for (const auto& access : accesses) { bytes.insert(access.address); bytes.insert(access.address + std::uint32_t(access.size) - 1); }
    for (auto byte : bytes) { auto s = without_byte(initial, byte);
        check(index, *s, name + "/missing-" + hex_address(byte), {}, mutation, "Unmapped data access at "); }
}
}
