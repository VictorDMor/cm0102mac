#pragma once
#include "actor_construction.hpp"
#include "guest_context_transfer.hpp"
#include "floating_point.hpp"
#include <bit>
namespace cm::aot::actor_construction {
class ConstructionCall {
    State& state;
    std::uint32_t next;
    GuestNativeScope scope;
    DependencyDispatch* dependencies;
    void step() { state.step(next); next += 4; }
    void condition(std::int64_t a, std::int64_t b, bool record = false) {
        state.wr<1>(2304, (record ? state.r<1>(2304) & 0xf0 : 0) |
            (a < b ? 8 : a > b ? 4 : 2) | (state.r<1>(1024) & 1));
    }
public:
    ConstructionCall(State& s, std::uint32_t root, DependencyDispatch* d)
        : state(s), next(root), scope(s, root), dependencies(d) {}
    std::uint32_t r(unsigned n) const { return std::uint32_t(state.r<4>(n * 4)); }
    void at(std::uint32_t address) { next = address; }
    void link() { step(); state.wr<4>(0, state.r<4>(4128)); }
    void save(unsigned n) { step(); state.memory.write<4>(r(1) - (32 - n) * 4, r(n)); }
    void restore(unsigned n) { step(); state.wr<4>(n * 4, state.memory.read<4>(r(1) - (32 - n) * 4)); }
    void save_link() { step(); state.memory.write<4>(r(1) + 8, r(0)); }
    void frame(unsigned bytes) {
        step(); const auto old = r(1), current = old - bytes;
        state.memory.write<4>(current, old); state.wr<4>(4, current); scope.frame(current);
    }
    void multiple(unsigned first, bool restore = false, std::int32_t extra = 0) {
        step(); state.wr<4>(2432, r(1) - (32 - first) * 4 + std::uint32_t(extra));
        for (unsigned n = first; n < 32; ++n) {
            if (restore) state.wr<4>(n * 4, state.memory.read<4>(state.r<4>(2432)));
            else state.memory.write<4>(state.r<4>(2432), r(n));
            state.wr<4>(2432, state.r<4>(2432) + 4);
        }
    }
    void move(unsigned destination, unsigned source, bool record = false) {
        step(); const auto value = r(source); state.wr<4>(destination * 4, value);
        if (record) condition(std::bit_cast<std::int32_t>(value), 0, true);
    }
    void value(unsigned destination, std::uint32_t value) { step(); state.wr<4>(destination * 4, value); }
    void offset(unsigned destination, unsigned source, std::int32_t displacement) {
        step(); state.wr<4>(destination * 4, r(source) + std::uint32_t(displacement));
    }
    void add(unsigned destination, unsigned first, unsigned second) {
        step(); state.wr<4>(destination * 4, r(first) + r(second));
    }
    template<unsigned N = 4> void load(unsigned destination, unsigned base, std::int32_t displacement, bool sign = false) {
        step(); const auto value = state.memory.read<N>(r(base) + std::uint32_t(displacement));
        state.wr<4>(destination * 4, sign ? std::uint32_t(signed_value<N>(value)) : value);
    }
    template<unsigned N = 4> void store(unsigned base, std::int32_t displacement, unsigned source) {
        step(); state.memory.write<N>(r(base) + std::uint32_t(displacement), r(source));
    }
    void sign8(unsigned destination, unsigned source, bool record = false) {
        step(); const auto value = signed_value<1>(r(source)); state.wr<4>(destination * 4, std::uint32_t(value));
        if (record) condition(value, 0, true);
    }
    void sign16(unsigned destination, unsigned source) {
        step(); state.wr<4>(destination * 4, std::uint32_t(signed_value<2>(r(source))));
    }
    void exclusive_flags(unsigned destination, unsigned source, std::uint32_t bits) {
        step(); state.wr<4>(destination * 4, r(source) ^ bits);
    }
    void leading_zeroes(unsigned destination, unsigned source) {
        step(); state.wr<4>(destination * 4, unsigned(std::countl_zero(r(source))));
    }
    void indexed_load(unsigned destination, unsigned base, unsigned index) {
        step(); state.wr<4>(destination * 4, state.memory.read<4>(r(base) + r(index)));
    }
    void indexed_store(unsigned source, unsigned base, unsigned index) {
        step(); state.memory.write<4>(r(base) + r(index), r(source));
    }
    void floating_load(unsigned destination, unsigned base, std::int32_t displacement, bool single = false) {
        step(); const auto address = r(base) + std::uint32_t(displacement);
        const auto bits = single ? float_bits<8>(float_value<4>(state.memory.read<4>(address))) : state.memory.read<8>(address);
        state.wr<8>(0x4000 + destination * 16, bits);
    }
    void floating_store(unsigned source, unsigned base, std::int32_t displacement) {
        step(); state.memory.write<8>(r(base) + std::uint32_t(displacement), state.r<8>(0x4000 + source * 16));
    }
    void floating_integer(unsigned destination, unsigned source) {
        step(); original_fctiwz(state, 0x4000 + destination * 16, 0x4000 + source * 16);
    }
    void floating_result_status(double value, bool operand_nan, bool inexact) {
        state.wr<1>(1296, value < 0); state.wr<1>(1297, 0 < value); state.wr<1>(1298, value == 0);
        state.wr<1>(1299, std::isnan(value));
        if (inexact) state.wr<1>(1286, state.r<1>(1286) | state.r<1>(1294));
        state.wr<1>(1287, state.r<1>(1287) | operand_nan);
        std::uint8_t invalid = 0;
        for (auto offset : {1287u,1288u,1289u,1290u,1291u,1292u,1301u,1302u,1303u}) invalid |= state.r<1>(offset);
        state.wr<1>(1282, invalid);
        state.wr<1>(1280, state.r<1>(1280) | state.r<1>(1283) | state.r<1>(1284) | state.r<1>(1285) | state.r<1>(1286));
        std::uint8_t enabled = 0;
        for (auto [flag, mask] : {std::pair{1282u,1304u}, {1283u,1305u}, {1284u,1306u}, {1285u,1307u}, {1286u,1308u}})
            enabled ^= state.r<1>(flag) & state.r<1>(mask);
        state.wr<1>(1281, enabled);
    }
    void floating_difference(unsigned destination, unsigned first, unsigned second, bool divide = false) {
        step(); const auto x = float_value<8>(state.r<8>(0x4000 + first * 16));
        const auto y = float_value<8>(state.r<8>(0x4000 + second * 16));
        const auto value = divide ? x / y : x - y;
        state.wr<8>(0x4000 + destination * 16, float_bits<8>(value));
        if (divide) {
            state.wr<1>(1285, state.r<1>(1285) | (y == 0));
            state.wr<1>(1290, state.r<1>(1290) | (x == 0 && y == 0));
        }
        floating_result_status(value, std::isnan(x) || std::isnan(y), divide);
    }
    void compare(unsigned source, std::int32_t value, bool sign = true) {
        step(); condition(sign ? std::bit_cast<std::int32_t>(r(source)) : std::int64_t(r(source)), value);
    }
    void compare_registers(unsigned first, unsigned second, bool sign = true) {
        step(); condition(sign ? std::bit_cast<std::int32_t>(r(first)) : std::int64_t(r(first)),
            sign ? std::bit_cast<std::int32_t>(r(second)) : std::int64_t(r(second)));
    }
    void mask(unsigned destination, unsigned source, std::uint32_t mask, bool record = false, unsigned rotate = 0) {
        step(); const auto value = std::rotl(r(source), int(rotate)) & mask; state.wr<4>(destination * 4, value);
        if (record) condition(std::bit_cast<std::int32_t>(value), 0, true);
    }
    void flags(unsigned destination, unsigned source, std::uint32_t bits) {
        step(); state.wr<4>(destination * 4, r(source) | bits);
    }
    void multiply(unsigned destination, unsigned source, std::uint32_t factor) {
        step(); state.wr<4>(destination * 4, r(source) * factor);
    }
    void multiply_high(unsigned destination, unsigned first, unsigned second) {
        step(); const auto product = std::int64_t(std::bit_cast<std::int32_t>(r(first))) * std::bit_cast<std::int32_t>(r(second));
        state.wr<4>(destination * 4, std::uint32_t(std::bit_cast<std::uint64_t>(product) >> 32));
    }
    void shift_right(unsigned destination, unsigned source, unsigned shift) {
        step(); const auto value = r(source);
        state.wr<1>(1027, std::bit_cast<std::int32_t>(value) < 0 && (value & ((UINT32_C(1) << shift) - 1)));
        state.wr<4>(destination * 4, std::uint32_t(std::bit_cast<std::int32_t>(value) >> shift));
    }
    void subtract(unsigned destination, unsigned first, unsigned second, bool record = false) {
        step(); const auto value = r(second) - r(first); state.wr<4>(destination * 4, value);
        if (record) condition(std::bit_cast<std::int32_t>(value), 0, true);
    }
    void subtract_from(unsigned destination, unsigned source, std::uint32_t value) {
        step(); const auto input = r(source); state.wr<1>(1027, value >= input); state.wr<4>(destination * 4, value - input);
    }
    bool equal() { step(); return (state.r<1>(2304) & 2) != 0; }
    bool less() { step(); return (state.r<1>(2304) & 8) != 0; }
    bool positive() { step(); return (state.r<1>(2304) & 4) != 0; }
    bool nonpositive() { step(); return (state.r<1>(2304) & 4) == 0; }
    bool at_least() { step(); return (state.r<1>(2304) & 8) == 0; }
    void jump(std::uint32_t address) { step(); at(address); }
    void call(std::uint32_t target) {
        step(); state.wr<4>(2440, r(2)); state.wr<4>(4128, next); state.pc = target;
        if (dependencies) dependencies->call(state, target, next); else run_until(state, next);
    }
    void pop(unsigned bytes) {
        load(0, 1, bytes + 8); offset(1, 1, bytes); step(); state.wr<4>(4128, r(0));
    }
    void finish() { step(); state.pc = std::uint32_t(state.r<4>(4128)); }
    void floating_store_single(unsigned source, unsigned base, std::int32_t displacement) {
        step(); state.memory.write<4>(r(base) + std::uint32_t(displacement), float_bits<4>(float_value<8>(state.r<8>(0x4000 + source * 16))));
    }
    void floating_move(unsigned destination, unsigned source) {
        step(); state.wr<8>(0x4000 + destination * 16, state.r<8>(0x4000 + source * 16));
    }
    void floating_subtract(unsigned destination, unsigned first, unsigned second, bool single = false) {
        step(); const auto x = float_value<8>(state.r<8>(0x4000 + first * 16));
        const auto y = float_value<8>(state.r<8>(0x4000 + second * 16));
        const auto difference = x - y;
        const auto result = single ? double(float_value<4>(float_bits<4>(difference))) : difference;
        state.wr<8>(0x4000 + destination * 16, float_bits<8>(result));
        floating_result_status(result, std::isnan(x) || std::isnan(y), false);
    }
    void floating_product(unsigned destination, unsigned first, unsigned second) {
        step(); const auto x = float_value<8>(state.r<8>(0x4000 + first * 16));
        const auto y = float_value<8>(state.r<8>(0x4000 + second * 16));
        const auto result = x * y;
        state.wr<8>(0x4000 + destination * 16, float_bits<8>(result));
        floating_result_status(result, std::isnan(x) || std::isnan(y), true);
    }
    void floating_sum(unsigned destination, unsigned first, unsigned second) {
        step(); const auto x = float_value<8>(state.r<8>(0x4000 + first * 16));
        const auto y = float_value<8>(state.r<8>(0x4000 + second * 16));
        const auto result = x + y;
        state.wr<8>(0x4000 + destination * 16, float_bits<8>(result));
        floating_result_status(result, std::isnan(x) || std::isnan(y), true);
    }
    void floating_compare(unsigned first, unsigned second) {
        step(); const auto x = float_value<8>(state.r<8>(0x4000 + first * 16));
        const auto y = float_value<8>(state.r<8>(0x4000 + second * 16));
        state.wr<1>(1296, x < y); state.wr<1>(1297, y < x); state.wr<1>(1298, x == y);
        state.wr<1>(2304, (x < y ? 8 : y < x ? 4 : x == y ? 2 : 0) | (std::isnan(x) || std::isnan(y)));
    }
    void compare_greater_or_equal_bit() {
        step(); const auto condition = state.r<1>(2304);
        state.wr<1>(2304, (condition & ~2u) | ((condition & 6u) ? 2u : 0u));
    }
    void divide(unsigned destination, unsigned first, unsigned second) {
        step(); state.wr<4>(destination * 4, divide_signed<4>(r(first), r(second)));
    }
    void product(unsigned destination, unsigned first, unsigned second) {
        step(); state.wr<4>(destination * 4, r(first) * r(second));
    }
    void negate(unsigned destination, unsigned source) {
        step(); state.wr<4>(destination * 4, 0u - r(source));
    }
    void offset_record(unsigned destination, unsigned source, std::int32_t displacement) {
        step(); const auto sum = std::uint64_t(r(source)) + std::uint32_t(displacement);
        state.wr<1>(1027, sum > UINT32_MAX); state.wr<4>(destination * 4, std::uint32_t(sum));
        condition(std::bit_cast<std::int32_t>(std::uint32_t(sum)), 0, true);
    }
    void add_record(unsigned destination, unsigned first, unsigned second) {
        step(); const auto sum = r(first) + r(second); state.wr<4>(destination * 4, sum);
        condition(std::bit_cast<std::int32_t>(sum), 0, true);
    }
    void add_carry(unsigned destination, unsigned source) {
        step(); const auto sum = std::uint64_t(r(source)) + state.r<1>(1027);
        state.wr<1>(1027, sum > UINT32_MAX); state.wr<4>(destination * 4, std::uint32_t(sum));
    }
    void restore_link() { step(); state.wr<4>(4128, r(0)); }

};
}
