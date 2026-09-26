#pragma once

// Support for C++ produced at build time from the original PowerPC program.
// No instruction decoder, interpreter, JIT, executable guest mapping, or CPU
// emulator is linked into the resulting ARM64 program.
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace cm::aot {
class NativePlatform;
constexpr std::uint32_t return_sentinel = 0xfffffff0;

template <unsigned N> constexpr std::uint64_t mask() {
    static_assert(N > 0 && N <= 8);
    if constexpr (N == 8) return UINT64_MAX;
    else return (UINT64_C(1) << (N * 8)) - 1;
}
template <unsigned N> std::uint64_t read_be(const std::uint8_t* p) {
    static_assert(N > 0 && N <= 8);
    std::uint64_t value = 0;
    for (unsigned i = 0; i < N; ++i) value = (value << 8) | p[i];
    return value;
}
template <unsigned N> void write_be(std::uint8_t* p, std::uint64_t value) {
    static_assert(N > 0 && N <= 8);
    for (unsigned i = N; i > 0; --i) { p[i - 1] = value & 255; value >>= 8; }
}
template <unsigned N> std::int64_t signed_value(std::uint64_t value) {
    value &= mask<N>();
    if constexpr (N < 8) {
        if (value & (UINT64_C(1) << (8 * N - 1))) value |= ~mask<N>();
    }
    return std::bit_cast<std::int64_t>(value);
}
template <unsigned N> std::uint64_t shift_left(std::uint64_t a, std::uint64_t b) {
    return b >= N * 8 ? 0 : (a << b) & mask<N>();
}
template <unsigned N> std::uint64_t shift_right(std::uint64_t a, std::uint64_t b) {
    return b >= N * 8 ? 0 : (a & mask<N>()) >> b;
}
template <unsigned N> std::uint64_t shift_signed(std::uint64_t a, std::uint64_t b) {
    auto v = signed_value<N>(a);
    return std::uint64_t(b >= N * 8 ? (v < 0 ? -1 : 0) : v >> b) & mask<N>();
}
template <unsigned N> std::uint64_t carry(std::uint64_t a, std::uint64_t b) {
    return (a & mask<N>()) > mask<N>() - (b & mask<N>());
}
template <unsigned N> std::uint64_t leading_zeros(std::uint64_t a) {
    return std::countl_zero(a & mask<N>()) - (64 - N * 8);
}
inline std::uint64_t divide_unsigned(std::uint64_t a, std::uint64_t b) {
    if (!b) throw std::runtime_error("Integer division by zero in translated code");
    return a / b;
}
template <unsigned N> std::uint64_t divide_signed(std::uint64_t a, std::uint64_t b) {
    auto x = signed_value<N>(a), y = signed_value<N>(b);
    if (!y) throw std::runtime_error("Signed division by zero in translated code");
    if (x == std::numeric_limits<std::int64_t>::min() && y == -1)
        throw std::runtime_error("Signed division overflow in translated code");
    return std::uint64_t(x / y) & mask<N>();
}
template <unsigned N> auto float_value(std::uint64_t bits) {
    static_assert(N == 4 || N == 8);
    if constexpr (N == 4) return std::bit_cast<float>(std::uint32_t(bits));
    else return std::bit_cast<double>(bits);
}
template <unsigned N, typename T> std::uint64_t float_bits(T value) {
    static_assert(N == 4 || N == 8);
    if constexpr (N == 4) return std::bit_cast<std::uint32_t>(float(value));
    else return std::bit_cast<std::uint64_t>(double(value));
}
template <unsigned N, typename T> std::uint64_t float_truncate(T value) {
    // Exceptional conversions need a verified PowerPC policy, not host UB.
    const auto rounded = std::trunc(static_cast<long double>(value));
    const auto bound = std::ldexp(1.0L, N * 8 - 1);
    if (!std::isfinite(rounded) || rounded < -bound || rounded >= bound)
        throw std::runtime_error("Unimplemented exceptional float-to-int conversion");
    return std::uint64_t(std::int64_t(rounded)) & mask<N>();
}

class Memory {
public:
    Memory();
    ~Memory();
    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;
    void map(std::uint32_t start, std::uint64_t size);
    void load(std::uint32_t start, const std::filesystem::path& file, std::uint64_t size);
    void copy_in(std::uint32_t start, const std::vector<std::uint8_t>& bytes);
    void clear(std::uint32_t start, std::uint64_t size);
    std::vector<std::uint8_t> copy_out(std::uint32_t start, std::size_t size) const;
    template <unsigned N> std::uint64_t read(std::uint64_t address) const {
        check(address, N);
        return read_be<N>(data_ + address);
    }
    template <unsigned N> void write(std::uint64_t address, std::uint64_t value) {
        check(address, N);
        write_be<N>(data_ + address, value);
    }
private:
    void check(std::uint64_t address, std::uint64_t size) const;
    std::uint8_t* data_;
    struct Range { std::uint64_t start, end; };
    std::vector<Range> ranges_;
};

struct State {
    Memory memory;
    std::array<std::uint8_t, 0x4300> registers{};
    std::array<std::uint8_t, 0x42000> unique{};
    std::uint32_t pc = 0;
    std::uint64_t executed = 0, budget = 10000000;
    std::uint64_t native_calls = 0;
    // The original ABI stores pointers in 32-bit data fields. Allocations live
    // in reserved, non-executable data pages, independently of ARM64 pointers.
    std::map<std::uint32_t, std::uint32_t> allocated;
    std::map<std::uint32_t, std::uint32_t> available;
    std::uint32_t heap_end = 0x20000000;
    std::int16_t memory_error = 0;
    // Keep platform implementation details out of all generated units. Future
    // filesystem, window, event, and sound changes do not alter this ABI again.
    std::shared_ptr<NativePlatform> platform;
    template <unsigned N> std::uint64_t r(std::size_t offset) const {
        if (offset + N > registers.size()) throw std::out_of_range("Register offset");
        return read_be<N>(registers.data() + offset);
    }
    template <unsigned N> void wr(std::size_t offset, std::uint64_t value) {
        if (offset + N > registers.size()) throw std::out_of_range("Register offset");
        write_be<N>(registers.data() + offset, value);
    }
    template <unsigned N> std::uint64_t u(std::size_t offset) const {
        if (offset + N > unique.size()) throw std::out_of_range("Temporary offset");
        return read_be<N>(unique.data() + offset);
    }
    template <unsigned N> void wu(std::size_t offset, std::uint64_t value) {
        if (offset + N > unique.size()) throw std::out_of_range("Temporary offset");
        write_be<N>(unique.data() + offset, value);
    }
    void step(std::uint32_t address) {
        pc = address;
        if (executed++ >= budget) throw std::runtime_error("Native instruction budget exhausted");
    }
};

// Definitions are emitted by the build-time compiler for selected original code.
bool dispatch_page(State& state);
const char* import_name(std::uint32_t address);
void load_program(State& state, const std::filesystem::path& directory);
void run(State& state);
bool bridge_import(State& state, const std::string& name);
std::uint32_t allocate(State& state, std::uint32_t size, bool clear);
void deallocate(State& state, std::uint32_t address);
std::string hex_address(std::uint32_t value);
}
