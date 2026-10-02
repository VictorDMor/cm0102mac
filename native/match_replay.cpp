#include "match_replay.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace cm::aot {
namespace {
constexpr std::array<char, 8> magic{'C', 'M', 'M', 'A', 'T', 'C', 'H', '1'};
constexpr std::size_t chunk_size = 1 << 20;
bool captured = false;

template <typename T> void put(std::ostream& out, const T& value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    if (!out) throw std::runtime_error("Cannot write private match snapshot");
}
template <typename T> T take(std::istream& in) {
    T value{};
    in.read(reinterpret_cast<char*>(&value), sizeof(value));
    if (!in) throw std::runtime_error("Truncated private match snapshot");
    return value;
}
void transfer_bytes(std::ostream& out, const Memory& memory, std::uint32_t start,
                    std::uint64_t size) {
    for (std::uint64_t offset = 0; offset < size;) {
        const auto length = std::size_t(std::min<std::uint64_t>(size - offset, chunk_size));
        const auto bytes = memory.copy_out(start + std::uint32_t(offset), length);
        out.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
        if (!out) throw std::runtime_error("Cannot write private match memory");
        offset += length;
    }
}
void receive_bytes(std::istream& in, Memory& memory, std::uint32_t start,
                   std::uint64_t size) {
    std::vector<std::uint8_t> bytes(chunk_size);
    for (std::uint64_t offset = 0; offset < size;) {
        const auto length = std::size_t(std::min<std::uint64_t>(size - offset, chunk_size));
        in.read(reinterpret_cast<char*>(bytes.data()), std::streamsize(length));
        if (!in) throw std::runtime_error("Truncated private match memory");
        memory.copy_in(start + std::uint32_t(offset), {bytes.begin(), bytes.begin() + length});
        offset += length;
    }
}
void save(const State& s, const std::filesystem::path& path) {
    const auto ranges = s.memory.mapped_ranges();
    if (ranges.size() > std::numeric_limits<std::uint32_t>::max() ||
        s.allocated.size() > std::numeric_limits<std::uint32_t>::max() ||
        s.available.size() > std::numeric_limits<std::uint32_t>::max())
        throw std::runtime_error("Match snapshot table is too large");
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot create private match snapshot");
    out.write(magic.data(), magic.size());
    put(out, s.pc);
    put(out, s.executed);
    put(out, s.native_calls);
    put(out, s.heap_end);
    put(out, s.memory_error);
    out.write(reinterpret_cast<const char*>(s.registers.data()), s.registers.size());
    out.write(reinterpret_cast<const char*>(s.unique.data()), s.unique.size());
    for (const auto& table : {&s.allocated, &s.available}) {
        put(out, std::uint32_t(table->size()));
        for (const auto& [address, size] : *table) { put(out, address); put(out, size); }
    }
    put(out, std::uint32_t(ranges.size()));
    for (const auto& [start, size] : ranges) {
        put(out, start);
        put(out, size);
        transfer_bytes(out, s.memory, start, size);
    }
    if (!out) throw std::runtime_error("Incomplete private match snapshot");
}
}
bool match_snapshot_requested() {
    return !captured && std::getenv("CM_AOT_MATCH_SNAPSHOT") != nullptr;
}
void save_runtime_snapshot(const State& state, const std::filesystem::path& path) { save(state, path); }
void capture_match_snapshot(const State& state) {
    const char* path = std::getenv("CM_AOT_MATCH_SNAPSHOT");
    if (!path || captured) return;
    if (!std::filesystem::path(path).is_absolute())
        throw std::runtime_error("Match snapshot requires an absolute private path");
    captured = true;
    save(state, path);
}
void load_match_snapshot(State& s, const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open private match snapshot");
    if (take<std::array<char, 8>>(in) != magic)
        throw std::runtime_error("Unrecognized private match snapshot");
    s.pc = take<std::uint32_t>(in);
    s.executed = take<std::uint64_t>(in);
    s.native_calls = take<std::uint64_t>(in);
    s.heap_end = take<std::uint32_t>(in);
    s.memory_error = take<std::int16_t>(in);
    in.read(reinterpret_cast<char*>(s.registers.data()), s.registers.size());
    in.read(reinterpret_cast<char*>(s.unique.data()), s.unique.size());
    if (!in) throw std::runtime_error("Truncated private match registers");
    for (auto* table : {&s.allocated, &s.available}) {
        const auto count = take<std::uint32_t>(in);
        if (count > 1000000) throw std::runtime_error("Invalid private match allocation table");
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto address = take<std::uint32_t>(in);
            const auto size = take<std::uint32_t>(in);
            if (!table->emplace(address, size).second)
                throw std::runtime_error("Duplicate private match allocation");
        }
    }
    const auto count = take<std::uint32_t>(in);
    if (count > 1000000) throw std::runtime_error("Invalid private match range count");
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto start = take<std::uint32_t>(in);
        const auto size = take<std::uint64_t>(in);
        if (!size || size > (UINT64_C(1) << 32) - start)
            throw std::runtime_error("Invalid private match memory range");
        s.memory.map(start, size);
        receive_bytes(in, s.memory, start, size);
    }
    if (in.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("Private match snapshot has trailing data");
}
std::uint64_t match_snapshot_memory_hash(const State& s) {
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (const auto& [start, size] : s.memory.mapped_ranges()) {
        for (std::uint64_t offset = 0; offset < size;) {
            const auto length = std::size_t(std::min<std::uint64_t>(size - offset, chunk_size));
            for (const auto byte : s.memory.copy_out(start + std::uint32_t(offset), length)) {
                hash ^= byte;
                hash *= UINT64_C(1099511628211);
            }
            offset += length;
        }
    }
    return hash;
}
}
