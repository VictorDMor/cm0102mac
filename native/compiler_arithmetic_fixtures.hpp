#pragma once
#include "compiler_arithmetic.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool compiler_arithmetic_reference(State&); }
namespace cm::compiler_arithmetic_checks {
using namespace cm::semantic_io_checks;
namespace arithmetic = cm::aot::compiler_arithmetic;
constexpr std::uint32_t return_address = 0x7fff0000, relocated_toc = 0x31008000, thresholds = fixture + 0x80;
struct Entry { std::uint32_t address; void (*operation)(State&); const char* name; };
constexpr std::array entries {
    Entry{0x10773780, arithmetic::double_to_unsigned_word, "double-to-unsigned-word"},
    Entry{0x10773870, arithmetic::unsigned_quotient, "unsigned-quotient"},
    Entry{0x10773960, arithmetic::signed_quotient, "signed-quotient"},
    Entry{0x10773a90, arithmetic::unsigned_remainder, "unsigned-remainder"},
    Entry{0x10773b70, arithmetic::signed_remainder, "signed-remainder"},
    Entry{0x10773c80, arithmetic::signed_pair_to_double, "signed-pair-to-double"},
    Entry{0x10773d30, arithmetic::signed_pair_to_single, "signed-pair-to-single"},
    Entry{0x10773de0, arithmetic::double_to_signed_pair, "double-to-signed-pair"},
};
inline std::array<unsigned, entries.size()> comparisons{}, faults{}, traps{}, negatives{};
inline std::uint64_t pair(const State& s, unsigned high = 3) { return (s.r<4>(high * 4) << 32) | s.r<4>((high + 1) * 4); }
inline void pair(State& s, std::uint64_t value, unsigned high = 3) {
    s.wr<4>(high * 4, value >> 32); s.wr<4>((high + 1) * 4, value);
}
inline void floating(State& s, double value) { s.wr<8>(0x4010, std::bit_cast<std::uint64_t>(value)); }
inline std::unique_ptr<State> fixture_state(unsigned index) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x6d); s->unique.fill(0xb3);
    for (unsigned n = 0; n < 32; ++n) {
        s->wr<4>(n * 4, 0x2468a000 + n * 0x101);
        s->wr<8>(0x4000 + n * 16, std::bit_cast<std::uint64_t>(double(n) + 0.25));
    }
    for (unsigned n = 1280; n <= 1311; ++n) s->wr<1>(n, (n & 3) == 0);
    for (unsigned n = 1304; n <= 1308; ++n) s->wr<1>(n, 0);
    for (unsigned n = 0; n < 8; ++n) s->wr<1>(2304 + n, 0xa7);
    s->wr<1>(1024, 1); s->wr<1>(1027, 1);
    s->wr<4>(4, stack); s->wr<4>(8, relocated_toc);
    s->wr<4>(4128, return_address); s->wr<4>(4132, 0x31415926);
    s->pc = entries[index].address; s->executed = 37; s->budget = 20000;
    s->native_calls = 23; s->heap_end = 0x50000300; s->memory_error = -108;
    s->allocated = {{0x50000000, 64}}; s->available = {{0x50000100, 128}};
    s->memory.map(fixture, 0x100); s->memory.map(relocated_toc - 0x3b7c, 4); s->memory.map(stack - 0x100, 0x200);
    s->memory.write<4>(relocated_toc - 0x3b7c, thresholds);
    for (const auto [offset, value] : {std::pair{0u, 0.0}, {8u, 4294967296.0}, {16u, 2147483648.0}})
        s->memory.write<8>(thresholds + offset, std::bit_cast<std::uint64_t>(value));
    pair(*s, 1234567890123456); pair(*s, 12345, 5); floating(*s, 12345.75);
    return s;
}
struct Monitor final : MemoryObserver {
    Effects effects;
    std::function<void(State&, std::uint32_t, unsigned)> on_write;
    void mapped(std::uint32_t address, std::uint64_t size) override { effects.mapped(address, size); }
    void cleared(std::uint32_t address, std::uint64_t size) override { effects.cleared(address, size); }
    void read(std::uint32_t address, const std::uint8_t* data, unsigned size) override { effects.read(address, data, size); }
    void written(std::uint32_t address, const std::uint8_t* data, std::size_t size) override {
        effects.written(address, data, size);
        if (on_write) on_write(*effects.state, address, unsigned(size));
    }
};
using Mutation = std::function<void(Monitor&)>;
using Expectation = std::function<void(const State&)>;
inline std::string run(State& s, void (*operation)(State&)) {
    try {
        if (operation) operation(s);
        else require(compiler_arithmetic_reference(s), "Missing complete original arithmetic reference");
    } catch (const std::runtime_error& e) { return e.what(); }
    return {};
}
inline std::vector<Effects::Access> check(unsigned index, const State& initial, const std::string& name,
                                  const Expectation& expect = {}, const Mutation& mutate = {},
                                  bool fault = false, bool negative = false, const std::string& error_prefix = {}) {
    auto original = clone(initial), semantic = clone(initial);
    Monitor a, b; a.effects.state = original.get(); b.effects.state = semantic.get();
    if (mutate) { mutate(a); mutate(b); }
    original->memory.date_observer = &a; semantic->memory.date_observer = &b;
    const auto x = run(*original, nullptr), y = run(*semantic, entries[index].operation);
    original->memory.date_observer = semantic->memory.date_observer = nullptr;
    const auto label = std::string(entries[index].name) + "/" + name;
    require(x == y, label + " outcome expected=" + x + " actual=" + y);
    require(fault ? x.starts_with("Unmapped data access at ") :
        error_prefix.empty() ? x.empty() : x.starts_with(error_prefix), label + " unexpected outcome=" + x);
    Image(*original).compare(*semantic, label);
    if (index == 1 && x.empty()) require(original->executed - initial.executed ==
        arithmetic::unsigned_quotient_instruction_count(initial), label + " exact unsigned quotient instruction charge");
    require(a.effects.bytes == b.effects.bytes, label + " ordered access mismatch");
    require(semantic->executed == initial.executed, label + " raw component changed instruction accounting");
    if (expect) { expect(*original); expect(*semantic); }
    ++comparisons[index]; if (fault) ++faults[index]; if (!error_prefix.empty()) ++traps[index];
    if (negative) {
        semantic->wr<4>(12, semantic->r<4>(12) ^ 1);
        bool rejected = false; try { Image(*original).compare(*semantic, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " output negative accepted"); ++negatives[index];
    }
    return a.effects.log;
}
inline std::unique_ptr<State> without_byte(const State& initial, std::uint32_t omitted) {
    auto s = std::make_unique<State>(); s->registers = initial.registers; s->unique = initial.unique;
    s->pc = initial.pc; s->executed = initial.executed; s->budget = initial.budget;
    s->heap_end = initial.heap_end; s->native_calls = initial.native_calls; s->memory_error = initial.memory_error;
    s->allocated = initial.allocated; s->available = initial.available;
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
inline void fault_matrix(unsigned index, const State& initial, const std::string& label) {
    const auto log = check(index, initial, label);
    std::set<std::uint32_t> bytes;
    for (const auto& access : log) { bytes.insert(access.address); bytes.insert(access.address + std::uint32_t(access.size) - 1); }
    for (auto byte : bytes) {
        auto broken = without_byte(initial, byte);
        check(index, *broken, label + "/missing-" + hex_address(byte), {}, {}, true);
    }
}
inline std::uint64_t magnitude(std::uint64_t value) { return value >> 63 ? 0 - value : value; }
}
