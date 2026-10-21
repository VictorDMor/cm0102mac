#pragma once
#include "aot_runtime.hpp"
#include <algorithm>
#include <functional>
#include <iostream>

using namespace cm::aot;
namespace cm::semantic_io_checks {
constexpr std::uint32_t fixture=0x30000000, stack=0x700fff00, toc=0x108bba90;
constexpr std::uint32_t left=fixture, right=fixture+0x100, destination=fixture+0x200;
inline void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
struct Effects final : MemoryObserver {
    std::vector<std::uint8_t> bytes;
    struct Access { unsigned kind; std::uint32_t address; std::uint64_t size, value; };
    std::vector<Access> log;
    std::vector<std::pair<std::uint32_t, std::uint64_t>> accesses;
    State* state = nullptr;
    std::function<void(State&, Effects&, std::uint32_t, unsigned)> on_read;
    bool mutated = false;
    std::vector<std::array<std::uint32_t,5>> callbacks;
    void number(std::uint64_t value, unsigned width) {
        for (unsigned n = width; n; --n) bytes.push_back(std::uint8_t(value >> ((n - 1) * 8)));
    }
    void event(unsigned kind, std::uint32_t address, std::uint64_t size, const std::uint8_t* data = nullptr) {
        number(kind, 1); number(address, 4); number(size, 8); accesses.emplace_back(address, size);
        std::uint64_t value = 0;
        if (data) {
            bytes.insert(bytes.end(), data, data + size);
            for (unsigned i = 0; i < std::min(size, std::uint64_t(8)); ++i) value = (value << 8) | data[i];
        }
        log.push_back({kind,address,size,value});
    }
    void mapped(std::uint32_t address, std::uint64_t size) override { event(1, address, size); }
    void written(std::uint32_t address, const std::uint8_t* data, std::size_t size) override { event(2, address, size, data); }
    void cleared(std::uint32_t address, std::uint64_t size) override { event(3, address, size); }
    void read(std::uint32_t address, const std::uint8_t* data, unsigned size) override {
        event(4, address, size, data);
        if (state->pc==0x10781320) callbacks.push_back({std::uint32_t(state->r<4>(12)),std::uint32_t(state->r<4>(16)),std::uint32_t(state->r<4>(20)),std::uint32_t(state->r<4>(48)),std::uint32_t(state->r<4>(4128))});
        if (on_read) on_read(*state, *this, address, size);
    }
};
// Instruction accounting and process-local hook diagnostics are intentionally
// excluded. Every architectural byte, including generated temporary residues,
// and every mapped memory byte is compared at returns and faults.
struct Image {
    decltype(State::registers) registers;
    decltype(State::unique) unique;
    std::uint32_t pc, heap_end;
    std::uint64_t native_calls;
    std::int16_t memory_error;
    decltype(State::allocated) allocated, available;
    std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> memory;
    explicit Image(const State& s) : registers(s.registers), unique(s.unique), pc(s.pc), heap_end(s.heap_end),
        native_calls(s.native_calls), memory_error(s.memory_error), allocated(s.allocated), available(s.available) {
        for (const auto& [address, size] : s.memory.mapped_ranges()) memory.emplace_back(address, s.memory.copy_out(address, size));
    }
    // At a fault, generated code reports the first instruction of the faulting
    // run as its PC (block accounting); `faulted` leaves the PC out.
    void compare(const State& s, const std::string& label, bool faulted = false) const {
        if (registers != s.registers) {
            const auto p = std::mismatch(registers.begin(), registers.end(), s.registers.begin());
            throw std::runtime_error(label + " register byte=" + std::to_string(p.first - registers.begin()) +
                " expected=" + std::to_string(*p.first) + " actual=" + std::to_string(*p.second));
        }
        if (unique != s.unique) {
            const auto p = std::mismatch(unique.begin(), unique.end(), s.unique.begin());
            throw std::runtime_error(label + " temporary byte=" + std::to_string(p.first - unique.begin()) +
                " expected=" + std::to_string(*p.first) + " actual=" + std::to_string(*p.second));
        }
        require(faulted || pc == s.pc, label + " PC expected=" + hex_address(pc) + " actual=" + hex_address(s.pc));
        require(heap_end == s.heap_end && native_calls == s.native_calls && memory_error == s.memory_error &&
                allocated == s.allocated && available == s.available, label + " allocator/import mismatch");
        const auto ranges = s.memory.mapped_ranges();
        require(ranges.size() == memory.size(), label + " mapping count mismatch");
        for (unsigned i = 0; i < memory.size(); ++i) {
            const auto& [address, bytes] = memory[i];
            require(ranges[i].first == address && ranges[i].second == bytes.size(), label + " mapping mismatch");
            const auto actual = s.memory.copy_out(address, bytes.size());
            if (bytes != actual) {
                const auto p = std::mismatch(bytes.begin(), bytes.end(), actual.begin());
                throw std::runtime_error(label + " memory mismatch at " + hex_address(address + std::uint32_t(p.first - bytes.begin())));
            }
        }
    }
};
inline std::unique_ptr<State> clone(const State& s) {
    auto t = std::make_unique<State>();
    t->registers = s.registers; t->unique = s.unique; t->pc = s.pc;
    t->executed = s.executed; t->budget = s.budget; t->native_calls = s.native_calls;
    t->allocated = s.allocated; t->available = s.available; t->heap_end = s.heap_end; t->memory_error = s.memory_error;
    for (const auto& [address, size] : s.memory.mapped_ranges()) {
        t->memory.map(address, size); t->memory.copy_in(address, s.memory.copy_out(address, size));
    }
    return t;
}
}
