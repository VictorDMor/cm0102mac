#pragma once
#include "register_context.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool register_context_reference(State&); }
namespace cm::register_context_checks {
using namespace cm::semantic_io_checks;
enum class Kind { save, restore, capture, jump };
struct Entry { std::uint32_t address; Kind kind; unsigned first, instructions; };
inline constexpr auto entries = [] {
    std::array<Entry, 30> result{};
    for (unsigned n = 0; n < 14; ++n) {
        result[n] = {0x107737e0 + n * 4, Kind::save, n + 18, 15 - n};
        result[n + 14] = {0x10773830 + n * 4, Kind::restore, n + 18, 15 - n};
    }
    result[28] = {0x10773eb0, Kind::capture, 0, 29};
    result[29] = {0x10773f30, Kind::jump, 0, 32};
    return result;
}();
inline constexpr std::uint32_t incoming_link = 0x7fff1000, restored_link = 0x7fff2000,
    restored_stack = 0x71010003, restored_toc = 0x32008000, buffer = fixture + 0x100;
inline std::array<unsigned, 30> comparisons{}, faults{}, negatives{};
inline std::unique_ptr<State> fixture_state(unsigned index) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x6d); s->unique.fill(0xb3);
    for (unsigned n = 0; n < 32; ++n) {
        s->wr<4>(n * 4, 0x2468a000 + n * 0x101);
        s->wr<8>(0x4000 + n * 16, UINT64_C(0xa580100000000000) + n * UINT64_C(0x100000001));
    }
    for (unsigned n = 0; n < 8; ++n) s->wr<1>(2304 + n, 0xa0 | n);
    for (unsigned n = 0; n < 32; ++n) s->wr<1>(1280 + n, 0xa4 | (n & 1));
    s->wr<4>(4, stack); s->wr<4>(8, 0x31008000); s->wr<4>(12, buffer); s->wr<4>(16, 0);
    s->wr<4>(4128, incoming_link); s->wr<4>(4132, 0x31415926);
    s->memory.map(fixture, 0x800); s->memory.map(stack - 256, 512);
    std::vector<std::uint8_t> data(0x800, 0xa7); s->memory.copy_in(fixture, data);
    s->memory.write<4>(buffer, restored_link); s->memory.write<4>(buffer + 4, 0x12345678);
    s->memory.write<4>(buffer + 8, restored_stack); s->memory.write<4>(buffer + 12, restored_toc);
    for (unsigned n = 13; n < 32; ++n) s->memory.write<4>(buffer + 20 + (n - 13) * 4, 0xcafe0000 + n);
    for (unsigned n = 14; n < 32; ++n) {
        s->memory.write<8>(buffer + 96 + (n - 14) * 8, UINT64_C(0xff80112233440000) + n);
        s->memory.write<8>(stack - (32 - n) * 8, UINT64_C(0x7ff0445566770000) + n);
    }
    s->memory.write<8>(buffer + 240, UINT64_C(0xdeadbeefff8100f3));
    s->pc = entries[index].address; s->executed = 37; s->budget = 20000;
    s->native_calls = 23; s->heap_end = 0x50000300; s->memory_error = -108;
    s->allocated = {{0x50000000, 64}}; s->available = {{0x50000100, 128}};
    return s;
}
inline void semantic(State& s, unsigned index) {
    switch (entries[index].kind) {
    case Kind::save: register_context::save_floating_suffix(s, entries[index].first); break;
    case Kind::restore: register_context::restore_floating_suffix(s, entries[index].first); break;
    case Kind::capture: register_context::capture_nonlocal_context(s); break;
    case Kind::jump: register_context::restore_nonlocal_context(s); break;
    }
}
struct Monitor final : MemoryObserver {
    Effects effects;
    std::function<void(State&, std::uint32_t, unsigned)> on_write;
    void mapped(std::uint32_t address, std::uint64_t size) override { effects.mapped(address, size); }
    void cleared(std::uint32_t address, std::uint64_t size) override { effects.cleared(address, size); }
    void read(std::uint32_t address, const std::uint8_t* bytes, unsigned size) override { effects.read(address, bytes, size); }
    void written(std::uint32_t address, const std::uint8_t* bytes, std::size_t size) override {
        effects.written(address, bytes, size);
        if (on_write) on_write(*effects.state, address, unsigned(size));
    }
};
using Mutation = std::function<void(Monitor&)>;
using Expectation = std::function<void(const State&)>;
inline std::string run(State& s, unsigned index, bool original) {
    try {
        if (original) require(register_context_reference(s), "Missing complete original register-context entry");
        else semantic(s, index);
    } catch (const std::runtime_error& e) { return e.what(); }
    return {};
}
inline std::vector<Effects::Access> check(unsigned index, const State& initial, const std::string& name,
        bool fault = false, bool negative = false, const Mutation& mutate = {}, const Expectation& expect = {}) {
    auto original = clone(initial), modern = clone(initial); Monitor a, b;
    a.effects.state = original.get(); b.effects.state = modern.get();
    if (mutate) { mutate(a); mutate(b); }
    original->memory.date_observer = &a; modern->memory.date_observer = &b;
    const auto x = run(*original, index, true), y = run(*modern, index, false);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    const auto label = hex_address(entries[index].address) + "/" + name;
    require(x == y && (fault ? x.starts_with("Unmapped data access at ") : x.empty()), label + " outcome original=" + x + " semantic=" + y);
    Image(*original).compare(*modern, label); require(a.effects.bytes == b.effects.bytes, label + " ordered access prefix");
    require(modern->executed == initial.executed, label + " raw component accounting changed");
    if (!fault) {
        const auto count = original->executed - initial.executed;
        require(count <= entries[index].instructions && count >= entries[index].instructions - 2 * unsigned(index == 29),
            label + " original complete-body bound count=" + std::to_string(count));
        if (expect) { expect(*original); expect(*modern); }
    }
    ++comparisons[index]; faults[index] += fault;
    if (negative) {
        if (index < 14) modern->memory.write<8>(std::uint32_t(initial.r<4>(4)) - (32 - entries[index].first) * 8,
            modern->memory.read<8>(std::uint32_t(initial.r<4>(4)) - (32 - entries[index].first) * 8) ^ 1);
        else if (index < 28) modern->wr<8>(0x4000 + entries[index].first * 16, modern->r<8>(0x4000 + entries[index].first * 16) ^ 1);
        else if (index == 28) modern->memory.write<4>(buffer, modern->memory.read<4>(buffer) ^ 1);
        else modern->wr<4>(12, modern->r<4>(12) ^ 1);
        bool rejected = false; try { Image(*original).compare(*modern, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " individual output negative accepted"); ++negatives[index];
    }
    return a.effects.log;
}
inline std::unique_ptr<State> without_byte(const State& initial, std::uint32_t omitted) {
    auto s = std::make_unique<State>();
    s->registers = initial.registers; s->unique = initial.unique; s->pc = initial.pc;
    s->executed = initial.executed; s->budget = initial.budget; s->native_calls = initial.native_calls;
    s->allocated = initial.allocated; s->available = initial.available;
    s->heap_end = initial.heap_end; s->memory_error = initial.memory_error;
    for (const auto [address, size] : initial.memory.mapped_ranges()) {
        const auto copy = [&](std::uint32_t start, std::uint64_t length) {
            if (length) { s->memory.map(start, length); s->memory.copy_in(start, initial.memory.copy_out(start, length)); }
        };
        if (omitted >= address && omitted < address + size) {
            copy(address, omitted - address); copy(omitted + 1, address + size - omitted - 1);
        } else copy(address, size);
    }
    return s;
}
inline void fault_matrix(unsigned index, const State& initial) {
    const auto log = check(index, initial, "fault inventory"); std::set<std::uint32_t> bytes;
    for (const auto& access : log) { bytes.insert(access.address); bytes.insert(access.address + std::uint32_t(access.size) - 1); }
    for (auto byte : bytes) { auto s = without_byte(initial, byte); check(index, *s, "missing-" + hex_address(byte), true); }
}
}
