#include "match_engine.hpp"
#include "match_speed.hpp"
#include "guest_context_transfer.hpp"
#include "floating_point.hpp"
#include <bit>

namespace cm::aot::match_engine {
namespace {
// Ordered checkpoints keep the guest ABI and interrupted prefixes observable.
// Structured operations own advancement, presentation and fixture lifetimes.
class MatchAdvanceCall {
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
    MatchAdvanceCall(State& s, std::uint32_t root, DependencyDispatch* d)
        : state(s), next(root), scope(s, root), dependencies(d) {}
    std::uint32_t r(unsigned n) const { return std::uint32_t(state.r<4>(n * 4)); }
    void presentation_speed(unsigned n) { state.wr<4>(n * 4, match_simulation_speed(r(n))); }
    void at(std::uint32_t address) { next = address; }
    void link() { step(); state.wr<4>(0, state.r<4>(4128)); }
    void save(unsigned n) { step(); state.memory.write<4>(r(1) - (32 - n) * 4, r(n)); }
    void restore(unsigned n) { step(); state.wr<4>(n * 4, state.memory.read<4>(r(1) - (32 - n) * 4)); }
    void save_link() { step(); state.memory.write<4>(r(1) + 8, r(0)); }
    void frame(unsigned bytes) {
        step(); const auto old = r(1), current = old - bytes;
        state.memory.write<4>(current, old); state.wr<4>(4, current); scope.frame(current);
    }
    void multiple(unsigned first, bool restore = false) {
        step(); state.wr<4>(2432, r(1) - (32 - first) * 4);
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
    void player_probability() {
        step(); const auto factor = float_value<8>(state.r<8>(0x4020));
        const auto attribute = float_value<8>(state.r<8>(0x4010));
        const auto baseline = float_value<8>(state.r<8>(0x4000));
        // The accepted export rounds the multiply and subtraction separately,
        // then rounds to single before negating. Host FMA changes this contract.
        const auto product = factor * attribute;
        const auto difference = product - baseline;
        const auto result = -float_value<4>(float_bits<4>(difference));
        state.wr<8>(0x4000, float_bits<8>(result));
        floating_result_status(result, std::isnan(factor) || std::isnan(attribute) || std::isnan(baseline), true);
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
    bool positive() { step(); return (state.r<1>(2304) & 10) == 0; }
    bool nonpositive() { step(); return (state.r<1>(2304) & 10) != 0; }
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
};

void report_background_allocation_failure(MatchAdvanceCall& a) {
    a.at(0x101cd7c0); a.move(3, 31); a.offset(6, 1, 0x40); a.offset(7, 1, 0x38); a.value(4, 0); a.value(5, 0);
    a.call(0x10739890); a.call(0x10768810); a.move(5, 3); a.move(3, 30); a.offset(4, 31, 0xe);
    a.offset(6, 1, 0x40); a.offset(7, 1, 0x38); a.value(8, 0x704); a.call(0x1077b720);
    a.move(4, 30); a.offset(3, 31, 0x1b); a.value(5, 0); a.call(0x10769b60);
    a.load(4, 2, -0x7bac); a.value(0, 0); a.value(3, 0); a.store<1>(4, 0, 0); a.jump(0x101cd85c);
}

bool background_rate_gate(MatchAdvanceCall& a, bool six_ticks) {
    a.at(six_ticks ? 0x101cd668 : 0x101cd624); a.sign8(0, 4, true);
    if (!a.equal()) return false;
    a.load(3, 6, 0); a.load(0, 3, 4); a.mask(0, 0, six_ticks ? 0x80000 : 0x40000, true);
    if (a.equal()) return false;
    a.value(3, six_ticks ? 0x2aab0000 : 0x55550000); a.load(5, 2, -0x3918);
    a.offset(0, 3, six_ticks ? -0x5555 : 0x5556); a.multiply_high(3, 0, 5);
    a.mask(0, 3, 1, false, 1); a.add(0, 3, 0); a.multiply(0, 0, six_ticks ? 6 : 3); a.subtract(0, 0, 5, true);
    if (a.nonpositive()) return false;
    a.value(3, 0); a.jump(0x101cd85c); return true;
}

void count_completed_background(MatchAdvanceCall& a, std::uint32_t address) {
    a.at(address); a.load<1>(0, 29, 0x68); a.sign8(0, 0); a.compare(0, 2);
    bool complete = a.equal();
    if (!complete) { a.compare(0, 8); complete = a.equal(); }
    if (complete) {
        a.at(address == 0x101cd700 ? 0x101cd718 : 0x101cd764);
        a.load(3, 28, 0x28); a.offset(0, 3, 1); a.store(28, 0x28, 0);
    }
}

void advance_background_body(MatchAdvanceCall& a) {
    a.link(); a.save(31); a.load(6, 2, -0x7ac8); a.save(30); a.load(30, 2, -0x7ba8);
    a.save(29); a.load(31, 2, -0x52f0); a.save(28); a.move(28, 3); a.save_link(); a.frame(112);
    a.load<1>(0, 2, -0x391a); a.sign8(0, 0, true);
    if (a.equal()) { a.value(3, 0); a.value(0, 1); a.store(2, -0x3918, 3); a.store<1>(2, -0x391a, 0); }
    a.at(0x101cd608); a.load(3, 2, -0x3918); a.offset(0, 3, 1); a.compare(0, 100); a.store(2, -0x3918, 0);
    if (!a.nonpositive()) { a.value(0, 0); a.store(2, -0x3918, 0); }
    if (background_rate_gate(a, false) || background_rate_gate(a, true)) return;
    a.at(0x101cd6ac); a.load(0, 28, 0x30); a.compare(0, 0);
    if (!a.at_least()) { a.value(3, 0); a.jump(0x101cd85c); return; }
    a.at(0x101cd6c0); a.multiply(0, 0, 0x6c); a.load(3, 28, 0x1c); a.add(29, 3, 0); a.load(3, 29, 0x58); a.compare(3, 0, false);
    if (a.equal()) { a.value(3, 0); a.jump(0x101cd85c); return; }
    a.at(0x101cd6e0); a.sign8(0, 4, true);
    if (a.equal()) {
        a.at(0x101cd740); a.value(4, 1); a.call(entries[2]); a.store<1>(29, 0x68, 3);
        count_completed_background(a, 0x101cd74c);
    } else {
        a.jump(0x101cd724);
        for (;;) {
            a.at(0x101cd724); a.load<1>(0, 29, 0x68); a.sign8(0, 0); a.compare(0, 2);
            if (a.equal()) break;
            a.compare(0, 8); if (a.equal()) { a.jump(0x101cd770); break; }
            a.at(0x101cd6f0); a.load(3, 29, 0x58); a.value(4, UINT32_MAX); a.call(entries[2]); a.store<1>(29, 0x68, 3);
            count_completed_background(a, 0x101cd700);
        }
    }
    a.at(0x101cd770); a.load<1>(0, 29, 0x68); a.sign8(0, 0); a.compare(0, 2);
    bool complete = a.equal(); if (!complete) { a.compare(0, 8); complete = a.equal(); }
    if (!complete) { a.at(0x101cd858); a.value(3, 0); return; }
    a.at(0x101cd788); a.value(3, 0xdfc); a.call(0x10773520); a.move(28, 3, true);
    if (!a.equal()) {
        a.load(4, 2, -0x7acc); a.offset(3, 28, 0x120); a.value(5, 0); a.value(6, 0x664); a.value(7, 2); a.call(0x10773320);
    }
    a.at(0x101cd7b0); a.store(29, 0x60, 28); a.load(5, 29, 0x60); a.compare(5, 0, false);
    if (a.equal()) { report_background_allocation_failure(a); return; }
    a.at(0x101cd81c); a.load(3, 2, -0x7b4c); a.load(4, 29, 0x58); a.load(3, 3, 0); a.call(0x1029e9e0);
    a.load(3, 29, 0x58); a.value(4, 1); a.call(0x101d3e50); a.value(0, 0); a.load(3, 2, -0x7b20);
    a.store(29, 0x58, 0); a.offset(4, 29, 4); a.load(3, 3, 0); a.call(0x10430550); a.value(3, 1); a.jump(0x101cd85c);
}

void publish_pending_commentary(MatchAdvanceCall& a, std::uint32_t address) {
    a.at(address); a.load(3, 29, 0x79f4); a.load<2>(4, 26, 0, true); a.call(0x101ff180);
    a.sign8(0, 3); a.compare(0, 2);
    if (a.equal()) { a.value(0, 1); a.store<1>(30, 0x55, 0); }
    a.at(address + 0x20); a.value(0, 8); a.offset(3, 30, 0x28); a.store<1>(30, 0, 0); a.value(4, 0); a.value(5, 16);
    a.store(30, 4, 26); a.call(0x10778300); a.load<2>(3, 31, 0x2c58, true); a.value(4, 1); a.value(0, 0);
    a.add(3, 30, 3); a.store<1>(3, 0x28, 4); a.store<1>(30, 0x4a, 4); a.store(30, 0x4c, 0);
    a.call(0x10746a30); a.store(30, 0x50, 3);
}

void update_watched_event_flags(MatchAdvanceCall& a) {
    a.at(0x101ccfd4); a.value(3, 0); a.store<1>(30, 0, 3); a.load<1>(0, 28, 0x68); a.compare(0, 2);
    if (a.equal()) { a.load<1>(0, 30, 0); a.flags(0, 0, 0x40); a.store<1>(30, 0, 0); a.store(30, 0x4c, 3); }
    a.at(0x101ccff8); a.load<1>(0, 28, 0x68); a.compare(0, 8);
    if (a.equal()) { a.load<1>(3, 30, 0); a.value(0, 0); a.flags(3, 3, 0x40); a.store<1>(30, 0, 3); a.store(30, 0x4c, 0); }
    a.at(0x101cd018); a.load<1>(0, 28, 0x68); a.compare(0, 4);
    if (a.equal()) { a.load<1>(3, 30, 0); a.value(0, 0); a.flags(3, 3, 0x20); a.store<1>(30, 0, 3); a.store(30, 0x4c, 0); }
    a.at(0x101cd038); a.offset(4, 29, 0x10000); a.load(3, 28, 0x60); a.load<1>(4, 4, -0x1fc3); a.load<1>(0, 3, 0x120);
    a.sign8(3, 4); a.compare_registers(3, 0);
    if (!a.nonpositive()) { a.load<1>(3, 30, 0); a.value(0, 0); a.flags(3, 3, 1); a.store<1>(30, 0, 3); a.store(30, 0x4c, 0); }
    a.at(0x101cd068); a.offset(4, 29, 0x10000); a.load(3, 28, 0x60); a.load<1>(4, 4, -0x1f8d); a.load<1>(0, 3, 0x784);
    a.sign8(3, 4); a.compare_registers(3, 0);
    if (!a.nonpositive()) { a.load<1>(3, 30, 0); a.value(0, 0); a.flags(3, 3, 2); a.store<1>(30, 0, 3); a.store(30, 0x4c, 0); }
    a.at(0x101cd098); a.load<1>(0, 30, 0x55); a.sign8(0, 0, true);
    if (!a.equal()) { a.load<1>(3, 30, 0); a.value(0, 0); a.flags(3, 3, 0x10); a.store<1>(30, 0, 3); a.store(30, 0x4c, 0); }
    a.at(0x101cd0b8); a.load<1>(0, 29, 0x7928); a.sign8(0, 0, true);
    if (!a.equal()) { a.load<1>(3, 30, 0); a.value(0, 0); a.flags(3, 3, 0x80); a.store<1>(30, 0, 3); a.store(30, 0x4c, 0); }
    a.at(0x101cd0d8); a.value(3, 0x2e8c0000); a.load<2>(0, 29, 0x7948, true); a.offset(4, 3, -0x5d17);
    a.load(3, 28, 0x60); a.multiply_high(4, 4, 0); a.load<2>(0, 3, 0x4e, true); a.shift_right(3, 4, 1);
    a.mask(4, 3, 1, false, 1); a.add(3, 3, 4); a.compare_registers(3, 0);
    if (!a.nonpositive()) {
        a.load<1>(0, 30, 0); a.flags(0, 0, 4); a.store<1>(30, 0, 0); a.load(3, 28, 0x60);
        a.load<2>(0, 3, 0x52, true); a.subtract_from(0, 0, 11); a.multiply(0, 0, 50); a.store(30, 0x4c, 0);
    }
}

void select_priority_fixture(MatchAdvanceCall& a) {
    a.at(0x101cd17c); a.load(3, 27, 0x38); a.load(3, 3, 0x60); a.compare(3, 0, false);
    bool replace = a.equal();
    if (!replace) {
        a.load<2>(3, 3, 0x50, true); a.value(4, 1); a.call(0x101ce350); a.load(5, 28, 0x60); a.sign8(29, 3);
        a.value(4, 1); a.load<2>(3, 5, 0x50, true); a.call(0x101ce350); a.sign8(0, 3); a.compare_registers(0, 29);
        replace = a.positive();
        if (!replace) {
            a.load(3, 27, 0x38); a.load(4, 28, 0x60); a.load(3, 3, 0x60); a.load<2>(4, 4, 0x4e, true);
            a.load<2>(0, 3, 0x4e, true); a.compare_registers(4, 0); replace = !a.nonpositive();
        }
    }
    if (replace) { a.at(0x101cd1d4); a.store(27, 0x38, 28); }
    a.at(0x101cd1d8); a.value(3, 1); a.jump(0x101cd23c);
}

void service_background_while_waiting(MatchAdvanceCall& a) {
    a.at(0x101cd1e0); a.move(3, 27); a.value(4, 0); a.call(entries[1]); a.sign8(0, 3, true);
    if (!a.equal()) { a.move(3, 27); a.call(0x101cd350); a.jump(0x101cd238); }
    a.at(0x101cd238); a.value(3, 0);
}

void advance_watched_body(MatchAdvanceCall& a) {
    a.link(); a.multiple(26); a.move(28, 4, true); a.load(31, 2, -0x7bb0); a.move(27, 3); a.save_link(); a.frame(80);
    if (a.equal()) {
        a.value(4, 0); a.call(entries[1]); a.sign8(0, 3, true);
        if (!a.equal()) { a.move(3, 27); a.call(0x101cd350); }
        a.at(0x101ccde8); a.value(3, 0); a.jump(0x101cd23c); return;
    }
    a.at(0x101ccdf0); a.load(30, 28, 0x5c); a.load(29, 28, 0x58); a.load<1>(4, 30, 0); a.sign8(0, 4, true);
    if (!a.equal()) {
        a.load<1>(3, 30, 0x4a); a.load<1>(0, 30, 0x48); a.sign8(3, 3); a.sign8(0, 0); a.compare_registers(3, 0);
        if (!a.equal()) {
            a.at(0x101cd200); a.load<2>(0, 31, 0x2c58, true); a.add(3, 30, 0); a.load<1>(0, 3, 0x28); a.sign8(0, 0, true);
            if (!a.equal()) { a.at(0x101cd238); a.value(3, 0); return; }
            a.value(0, 1); a.store<1>(3, 0x28, 0); a.load<1>(3, 30, 0x4a); a.offset(0, 3, 1); a.store<1>(30, 0x4a, 0);
            a.call(0x10746a30); a.store(30, 0x50, 3); a.value(3, 1); a.jump(0x101cd23c); return;
        }
    }
    a.at(0x101cce1c); a.load<1>(0, 30, 0x4b); a.sign8(0, 0, true);
    if (!a.equal()) { service_background_while_waiting(a); return; }
    a.mask(0, 4, 8, true);
    if (!a.equal()) {
        a.load(3, 29, 0x79f4); a.offset(4, 30, 8); a.call(0x102036f0); a.sign8(0, 3, true);
        if (a.equal()) { service_background_while_waiting(a); return; }
    }
    a.at(0x101cce44); a.call(0x10746a30); a.load(4, 29, 0x79f4); a.load(5, 30, 0x50); a.load<1>(0, 4, 0x4070);
    a.presentation_speed(0);
    a.compare(0, 20, false);
    if (a.equal()) { a.value(0, 0); a.jump(0x101cce68); }
    else { a.at(0x101cce64); a.load(0, 30, 0x4c); }
    a.at(0x101cce68); a.add(0, 5, 0); a.compare_registers(3, 0, false);
    if (a.less()) { service_background_while_waiting(a); return; }
    a.load<1>(0, 30, 0); a.mask(0, 0, 8, true);
    if (!a.equal()) {
        a.load(3, 29, 0x79f4); a.value(4, 0); a.call(0x10201ef0); a.move(26, 3, true);
        if (!a.equal()) {
            publish_pending_commentary(a, 0x101cce94); a.value(3, 1); a.jump(0x101cd23c); return;
        }
    }
    a.at(0x101ccefc); a.load<1>(0, 30, 0x54); a.sign8(0, 0, true);
    if (!a.equal()) {
        a.value(0, 0); a.move(3, 29); a.store<1>(30, 0x55, 0); a.value(4, 1); a.call(entries[2]); a.store<1>(28, 0x68, 3);
        a.load<1>(0, 28, 0x68); a.sign8(0, 0); a.compare(0, 2);
        bool complete = a.equal(); if (!complete) { a.compare(0, 8); complete = a.equal(); }
        if (complete) { a.at(0x101ccf38); a.load(3, 27, 0x28); a.offset(0, 3, 1); a.store(27, 0x28, 0); }
        a.at(0x101ccf44); a.load(3, 29, 0x79f4); a.load<1>(0, 3, 5); a.sign8(0, 0, true);
        if (!a.equal()) {
            a.value(4, 0); a.call(0x10201ef0); a.move(26, 3, true);
            if (!a.equal()) {
                publish_pending_commentary(a, 0x101ccf64); a.value(0, 0); a.value(3, 1); a.store<1>(30, 0x54, 0);
                a.jump(0x101cd23c); return;
            }
        }
    }
    update_watched_event_flags(a); a.at(0x101cd124); a.value(0, 1); a.store<1>(30, 0x54, 0);
    a.load<1>(0, 30, 0); a.sign8(0, 0, true);
    if (a.equal()) { a.at(0x101cd238); a.value(3, 0); return; }
    a.offset(3, 30, 0x28); a.value(4, 0); a.value(5, 16); a.call(0x10778300); a.load<2>(0, 31, 0x2c58, true);
    a.value(4, 1); a.add(3, 30, 0); a.store<1>(3, 0x28, 4); a.store<1>(30, 0x4a, 4); a.call(0x10746a30);
    a.store(30, 0x50, 3); a.move(4, 29); a.load(3, 2, -0x7b4c); a.value(6, 1); a.load(5, 28, 0x60); a.load(3, 3, 0); a.call(0x1029f920);
    select_priority_fixture(a);
}

// The completion paths deliberately retain their distinct register and access
// order. Their shared business operation is to publish eligible club reports.
void finish_already_completed_match(MatchAdvanceCall& a) {
    a.at(0x101d3fa4); a.move(3, 31); a.call(0x101db340); a.move(3, 31); a.call(0x101f7100); a.move(3, 31); a.call(0x101f84e0);
    a.load(3, 31, 0x3118); a.load<1>(0, 3, 0x40); a.sign8(3, 0); a.compare(3, 0);
    if (a.equal()) {
        a.load(4, 31, 0x3118); a.load(3, 4, 0x1c); a.value(4, 1); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x1014c2e0);
        a.sign8(3, 3); a.compare(3, 0);
        if (!a.equal()) {
            a.load(3, 27, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x1c); a.load(4, 4, 0x90); a.call(0x101481d0);
            a.sign8(3, 3); a.compare(3, 0);
            if (a.equal()) {
                a.load(3, 28, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x1c); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x103afff0);
            }
        }
    }
    a.at(0x101d4034); a.load(4, 31, 0x3118); a.load<1>(0, 4, 0x41); a.sign8(0, 0); a.compare(0, 0);
    if (a.equal()) {
        a.load(3, 31, 0x3118); a.load(3, 3, 0x20); a.value(4, 1); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x1014c2e0);
        a.sign8(3, 3); a.compare(3, 0);
        if (!a.equal()) {
            a.load(3, 27, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x20); a.load(4, 4, 0x90); a.call(0x101481d0);
            a.sign8(3, 3); a.compare(3, 0);
            if (a.equal()) {
                a.load(3, 28, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x20); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x103afff0);
            }
        }
    }
    a.at(0x101d40ac); a.move(3, 31); a.call(0x101fd900); a.value(3, 2); a.jump(0x101d5644);
}

bool advance_early_stopped_phase(MatchAdvanceCall& a) {
    a.at(0x101d4108); a.move(3, 31); a.value(4, 4); a.call(0x101da820); a.load<1>(0, 31, 0x7929); a.sign8(4, 0); a.compare(4, 0);
    if (!a.equal()) { a.at(0x101d4240); a.offset(20, 20, -1); a.jump(0x101d5634); return false; }
    a.move(3, 31); a.call(0x101f84e0); a.load(3, 31, 0x3118); a.load<1>(3, 3, 0x40); a.sign8(3, 3); a.compare(3, 0);
    if (a.equal()) {
        a.load(3, 31, 0x3118); a.load(3, 3, 0x1c); a.value(4, 1); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x1014c2e0);
        a.sign8(3, 3); a.compare(3, 0);
        if (!a.equal()) {
            a.load(3, 27, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x1c); a.load(4, 4, 0x90); a.call(0x101481d0);
            a.move(4, 3); a.sign8(3, 4); a.compare(3, 0);
            if (a.equal()) {
                a.load(3, 28, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x1c); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x103afff0); a.move(4, 3);
            }
        }
    }
    a.at(0x101d41ac); a.load(3, 31, 0x3118); a.load<1>(5, 3, 0x41); a.sign8(3, 5); a.compare(3, 0);
    if (a.equal()) {
        a.load(4, 31, 0x3118); a.load(3, 4, 0x20); a.value(4, 1); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x1014c2e0);
        a.sign8(0, 3); a.compare(0, 0);
        if (!a.equal()) {
            a.load(3, 27, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x20); a.load(4, 4, 0x90); a.call(0x101481d0);
            a.sign8(0, 3); a.compare(0, 0);
            if (a.equal()) {
                a.load(3, 28, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x20); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x103afff0);
            }
        }
    }
    a.at(0x101d4224); a.move(3, 31); a.load<2>(4, 31, 0x794c, true); a.call(0x101f2400); a.move(3, 31); a.call(0x101fd900); a.value(3, 2); a.jump(0x101d5644); return true;
}

bool advance_late_stopped_phase(MatchAdvanceCall& a) {
    a.at(0x101d4248); a.move(3, 31); a.value(4, 3); a.call(0x101da820); a.load<1>(0, 31, 0x7929); a.sign8(0, 0); a.compare(0, 0);
    if (!a.equal()) { a.at(0x101d436c); a.offset(20, 20, -1); a.jump(0x101d5634); return false; }
    a.move(3, 31); a.call(0x101f84e0); a.load(3, 31, 0x3118); a.load<1>(0, 3, 0x40); a.sign8(0, 0); a.compare(0, 0);
    if (a.equal()) {
        a.load(3, 31, 0x3118); a.load(3, 3, 0x1c); a.value(4, 1); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x1014c2e0);
        a.sign8(0, 3); a.compare(0, 0);
        if (!a.equal()) {
            a.load(3, 27, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x1c); a.load(4, 4, 0x90); a.call(0x101481d0);
            a.sign8(0, 3); a.compare(0, 0);
            if (a.equal()) {
                a.load(3, 28, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x1c); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x103afff0);
            }
        }
    }
    a.at(0x101d42e4); a.load(4, 31, 0x3118); a.load<1>(3, 4, 0x41); a.sign8(0, 3); a.compare(0, 0);
    if (a.equal()) {
        a.load(4, 31, 0x3118); a.load(3, 4, 0x20); a.value(4, 1); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x1014c2e0);
        a.sign8(3, 3); a.compare(3, 0);
        if (!a.equal()) {
            a.load(3, 27, 0); a.load(4, 31, 0x3118); a.load(5, 4, 0x20); a.load(4, 5, 0x90); a.call(0x101481d0);
            a.sign8(3, 3); a.compare(3, 0);
            if (a.equal()) {
                a.load(3, 28, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x20); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x103afff0);
            }
        }
    }
    a.at(0x101d435c); a.move(3, 31); a.call(0x101fd900); a.value(3, 2); a.jump(0x101d5644); return true;
}

bool announce_pending_restart(MatchAdvanceCall& a) {
    a.at(0x101d4374); a.load<1>(0, 31, 0x1e3); a.sign8(7, 0); a.compare(7, 0);
    if (!a.equal()) return false;
    a.load<1>(3, 31, 0x7928); a.sign8(4, 3); a.compare(4, 8); if (!a.equal()) return false;
    a.load<1>(3, 31, 0x1c1); a.sign8(0, 3); a.compare(0, 0); if (!a.equal()) return false;
    a.load<2>(3, 31, 0x7948, true); a.compare(3, 1);
    if (!a.positive()) {
        a.move(3, 31); a.call(0x101d5870); a.value(4, 0x212f); a.move(5, 31); a.load<1>(6, 31, 0x7924); a.value(7, 0); a.call(0x101d5810); a.jump(0x101d43f4);
    } else {
        a.at(0x101d43d0); a.move(3, 31); a.call(0x101d5870); a.move(7, 3); a.move(3, 7);
        a.value(4, 0x1f74); a.move(5, 31); a.load<1>(6, 31, 0x7924); a.value(7, 0); a.call(0x101d5810);
    }
    a.at(0x101d43f4); a.value(3, 1); a.store<1>(31, 0x1e3, 3); a.jump(0x101d5634); return true;
}

bool select_pending_restart_player(MatchAdvanceCall& a) {
    a.at(0x101d4400); a.load<1>(4, 31, 0x7928); a.sign8(4, 4); a.compare(4, 0); if (a.equal()) return false;
    a.load<1>(3, 31, 0x792a); a.sign8(4, 3); a.compare(4, 0); if (!a.equal()) return false;
    a.load<1>(3, 31, 0x1c1); a.sign8(0, 3); a.compare(0, 0); if (!a.equal()) return false;
    a.value(16, 0);
    for (;;) {
        a.at(0x101d4434); a.value(3, 2); a.call(0x101d3c90); a.sign8(18, 3); a.value(3, 20); a.call(0x101d3c90); a.sign8(17, 3);
        a.offset(16, 16, 1); a.sign8(0, 18); a.multiply(3, 0, 0x2350); a.sign8(7, 17); a.multiply(0, 7, 0x1c4);
        a.add(3, 3, 0); a.offset(3, 3, 0x311c); a.add(3, 31, 3); a.call(0x101d3b50); a.sign8(3, 3); a.compare(3, 0);
        if (a.at_least()) break;
        a.sign8(0, 16); a.compare(0, 20); if (a.at_least()) break;
        a.sign8(0, 18); a.multiply(7, 0, 0x2350); a.sign8(3, 17); a.multiply(4, 3, 0x1c4); a.add(4, 7, 4);
        a.offset(3, 4, 0x311c); a.add(3, 31, 3); a.value(4, 1); a.value(5, 0); a.call(0x10286650); a.sign8(4, 3); a.compare(4, 0);
        if (!a.equal()) break;
    }
    a.at(0x101d44bc); a.sign8(3, 16); a.compare(3, 20); if (a.at_least()) return false;
    a.sign8(0, 18); a.multiply(4, 0, 0x2350); a.sign8(0, 17); a.multiply(3, 0, 0x1c4); a.add(3, 4, 3);
    a.offset(3, 3, 0x311c); a.add(3, 31, 3); a.call(0x101d3b50); a.move(4, 3); a.sign8(0, 4); a.compare(0, 0); if (a.less()) return false;
    a.move(3, 31); a.sign8(0, 18); a.multiply(5, 0, 0x2350); a.sign8(0, 17); a.multiply(0, 0, 0x1c4); a.add(4, 5, 0);
    a.offset(4, 4, 0x311c); a.add(4, 31, 4); a.call(0x101f75a0); a.load<1>(3, 31, 0x792a); a.sign8(3, 3); a.compare(3, 0);
    return !a.equal();
}

bool service_pending_incident(MatchAdvanceCall& a) {
    a.at(0x101d452c); a.load<1>(0, 31, 0x792a); a.sign8(3, 0); a.compare(3, 2);
    if (a.equal()) { a.value(0, 0); a.store<1>(31, 0x792a, 0); }
    a.at(0x101d4544); a.move(3, 31); a.call(0x101d6980); a.sign8(0, 3); a.compare(0, -1);
    if (a.nonpositive()) return false;
    a.load<1>(3, 31, 0x7928); a.sign8(3, 3); a.compare(3, 0);
    if (!a.equal()) {
        a.move(3, 31); a.call(0x101d5880); a.sign8(3, 3); a.compare(3, 0); if (a.equal()) return false;
        a.value(3, 5); a.call(0x101d3c90); a.compare(3, 0); if (!a.equal()) return false;
        a.move(3, 31); a.call(0x101d5980); a.sign8(0, 3); a.compare(0, 0); if (!a.equal()) return true;
        a.jump(0x101d4600); return false;
    }
    a.at(0x101d45a4); a.value(3, 7); a.call(0x101d3c90); a.move(0, 3); a.compare(0, 0); if (!a.equal()) return false;
    a.move(3, 31); a.call(0x101d6980); a.move(15, 3); a.move(3, 31); a.call(0x101d5870); a.move(0, 3); a.move(3, 0);
    a.value(4, 0x20ef); a.move(5, 31); a.move(6, 15); a.value(7, 0); a.call(0x101d5810); a.move(3, 31); a.call(0x101d5980);
    a.move(0, 3); a.sign8(0, 0); a.compare(0, 0); return !a.equal();
}

void prepare_stopped_ball_owner(MatchAdvanceCall& a) {
    a.at(0x101d4600); a.load<1>(0, 31, 0x1c1); a.sign8(3, 0); a.compare(3, 0);
    if (a.equal()) { a.load<1>(0, 31, 0x7928); a.sign8(3, 0); a.compare(3, 0); if (!a.equal()) { a.move(3, 31); a.call(0x101e6c50); } }
    a.at(0x101d4628); a.load<1>(0, 31, 0x1c1); a.sign8(0, 0); a.compare(0, 0); if (!a.equal()) return;
    a.load<1>(3, 31, 0x7928); a.sign8(0, 3); a.compare(0, 0); if (a.equal()) return;
    a.offset(3, 31, 0x10000); a.load(3, 3, -0x2000); a.compare(3, 0, false);
    if (!a.equal()) {
        a.offset(3, 31, 0x10000); a.load(3, 3, -0x2000); a.call(0x101d3b50); a.sign8(0, 3); a.compare(0, 0); if (a.at_least()) return;
    }
    a.at(0x101d4670); a.move(3, 31); a.load<1>(4, 31, 0x7924); a.load<1>(5, 31, 0x7928); a.value(6, 0); a.call(0x101f3a30);
    a.offset(4, 31, 0x10000); a.store(4, -0x2000, 3); a.offset(3, 31, 0x10000); a.load(3, 3, -0x2000);
    a.load<1>(4, 31, 0x791d); a.load<1>(5, 31, 0x791e); a.call(0x102374f0); a.offset(3, 31, 0x10000); a.load(3, 3, -0x2000); a.value(4, 200); a.call(0x101d57d0);
    a.offset(5, 31, 0x10000); a.load(3, 5, -0x2000); a.call(0x1022e3a0); a.load<1>(0, 31, 0x7924); a.sign8(0, 0); a.compare(0, 0);
    if (a.equal()) { a.value(0, 10); a.store(1, 0x40, 0); a.jump(0x101d46e0); }
    else { a.at(0x101d46d8); a.value(0, 1); a.store(1, 0x40, 0); }
    a.at(0x101d46e0); a.offset(3, 31, 0x10000); a.load(3, 3, -0x2000); a.load<1>(4, 31, 0x791d); a.load<1>(5, 31, 0x791e);
    a.value(6, 4); a.load(0, 1, 0x40); a.sign8(7, 0); a.call(0x10237830);
}

bool abort_at_configured_time(MatchAdvanceCall& a) {
    a.at(0x101d472c); a.offset(3, 31, 0x10000); a.load<1>(0, 3, -0x1f48); a.sign8(0, 0); a.compare(0, -1); if (a.equal()) return false;
    a.offset(3, 31, 0x10000); a.load<1>(0, 3, -0x1f48); a.sign8(3, 0); a.multiply(0, 3, 11); a.sign16(3, 0);
    a.load<2>(0, 31, 0x7948, true); a.compare_registers(3, 0); if (a.at_least()) return false;
    a.value(0, 0xfd); a.offset(3, 31, 0x10000); a.store<1>(3, -0x1fc3, 0); a.value(4, 0xfd); a.offset(3, 31, 0x10000); a.store<1>(3, -0x1f8d, 4);
    a.value(4, std::uint32_t(-3)); a.load(3, 31, 0x3118); a.store<1>(3, 0x43, 4); a.value(0, 0); a.store<1>(31, 0x7928, 0);
    a.load(3, 31, 0x3118); a.load<1>(15, 3, 0x4f); a.mask(3, 15, 3, false, 30); a.mask(0, 3, 4); a.compare(0, 0);
    if (!a.equal()) {
        a.load(3, 31, 0x79f4); a.value(4, 0x217a); a.move(5, 31); a.value(6, 0); a.value(7, 0); a.call(0x101d5810); a.jump(0x101d480c);
    } else {
        a.at(0x101d47c0); a.load(3, 31, 0x3118); a.load<1>(0, 3, 0x4f); a.mask(3, 0, 3, false, 30); a.mask(0, 3, 16); a.compare(0, 0);
        if (!a.equal()) {
            a.load(3, 31, 0x79f4); a.value(4, 0x2179); a.move(5, 31); a.value(6, 0); a.value(7, 0); a.call(0x101d5810); a.jump(0x101d480c);
        } else { a.at(0x101d47f4); a.load(3, 31, 0x79f4); a.value(4, 0x217b); a.move(5, 31); a.value(6, 0); a.value(7, 0); a.call(0x101d5810); }
    }
    a.at(0x101d480c); a.value(3, 8); a.jump(0x101d5644); return true;
}

void finish_regular_match_period(MatchAdvanceCall& a) {
    a.at(0x101d49d0); a.move(3, 31); a.call(0x101f84e0); a.load(4, 31, 0x3118); a.load<1>(3, 4, 0x40); a.sign8(3, 3); a.compare(3, 0);
    if (a.equal()) {
        a.load(4, 31, 0x3118); a.load(3, 4, 0x1c); a.value(4, 1); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x1014c2e0);
        a.sign8(0, 3); a.compare(0, 0);
        if (!a.equal()) {
            a.load(3, 27, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x1c); a.load(4, 4, 0x90); a.call(0x101481d0); a.sign8(0, 3); a.compare(0, 0);
            if (a.equal()) { a.load(3, 28, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x1c); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x103afff0); }
        }
    }
    a.at(0x101d4a50); a.load(3, 31, 0x3118); a.load<1>(3, 3, 0x41); a.sign8(3, 3); a.compare(3, 0);
    if (a.equal()) {
        a.load(3, 31, 0x3118); a.load(3, 3, 0x20); a.value(4, 1); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x1014c2e0);
        a.sign8(4, 3); a.compare(4, 0);
        if (!a.equal()) {
            a.load(3, 27, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x20); a.load(4, 4, 0x90); a.call(0x101481d0); a.sign8(4, 3); a.compare(4, 0);
            if (a.equal()) { a.load(3, 28, 0); a.load(4, 31, 0x3118); a.load(4, 4, 0x20); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x103afff0); }
        }
    }
    a.at(0x101d4ac8); a.move(3, 31); a.call(0x101fd900); a.value(3, 2); a.jump(0x101d5644);
}

void publish_period_score(MatchAdvanceCall& a) {
    a.at(0x101d4898); a.move(3, 31); a.call(0x101f7100); a.load(3, 31, 0x3118); a.load<1>(0, 3, 0x4b); a.sign8(3, 0); a.compare(3, -1);
    if (!a.equal()) {
        a.load(3, 31, 0x3118); a.load<1>(3, 3, 0x47); a.sign8(0, 3); a.compare(0, -1);
        if (!a.equal()) {
            a.load(4, 31, 0x3118); a.load<1>(0, 4, 0x4b); a.load(3, 31, 0x3118); a.load<1>(3, 3, 0x47); a.add(0, 0, 3); a.store<1>(4, 0x4b, 0);
            a.load(4, 31, 0x3118); a.load<1>(0, 4, 0x4c); a.load(3, 31, 0x3118); a.load<1>(3, 3, 0x48); a.add(3, 0, 3); a.store<1>(4, 0x4c, 3); a.jump(0x101d492c);
        } else {
            a.at(0x101d48fc); a.load(3, 31, 0x3118); a.load<1>(5, 3, 0x4b); a.load(4, 31, 0x3118); a.load<1>(0, 4, 0x43); a.add(0, 5, 0); a.store<1>(3, 0x4b, 0);
            a.load(4, 31, 0x3118); a.load<1>(5, 4, 0x4c); a.load(3, 31, 0x3118); a.load<1>(0, 3, 0x44); a.add(3, 5, 0); a.store<1>(4, 0x4c, 3);
        }
    }
    a.at(0x101d492c); a.load(4, 31, 0x3118); a.load(3, 4, 0); a.compare(3, -2);
    if (!a.equal()) { a.load(3, 28, 0); a.load(4, 31, 0x3118); a.call(0x103af130); }
    a.at(0x101d4948); a.load<1>(4, 31, 0x7929); a.sign8(4, 4); a.compare(4, 0);
    if (!a.equal()) {
        a.load<2>(4, 31, 0x7948, true); a.value(3, 0x88890000); a.offset(3, 3, -0x7777); a.multiply_high(3, 3, 4); a.add(0, 3, 4); a.shift_right(0, 0, 4);
        a.mask(4, 0, 1, false, 1); a.add(3, 0, 4); a.multiply(3, 3, 30); a.sign16(3, 3); a.store<2>(31, 0x7948, 3);
        a.load<1>(3, 31, 0x7929); a.sign8(4, 3); a.compare(4, 1);
        if (a.equal()) { a.load(3, 31, 0x79f4); a.value(4, 0x2003); a.move(5, 31); a.value(6, 0); a.value(7, 0); a.call(0x101d5810); a.jump(0x101d49c8); }
        else { a.at(0x101d49b0); a.load(3, 31, 0x79f4); a.value(4, 0x2002); a.move(5, 31); a.value(6, 0); a.value(7, 0); a.call(0x101d5810); }
        a.at(0x101d49c8); a.value(3, 4); a.jump(0x101d5644);
    } else finish_regular_match_period(a);
}

void report_invalid_period(MatchAdvanceCall& a) {
    a.at(0x101d4ad8); a.offset(3, 25, 0x45); a.value(4, 0); a.value(5, 0); a.offset(6, 1, 0x4d); a.offset(7, 1, 0x48); a.call(0x10739890);
    a.call(0x10768810); a.move(5, 3); a.move(3, 24); a.offset(4, 25, 0x53); a.offset(6, 1, 0x4d); a.offset(7, 1, 0x48); a.value(8, 0x735); a.call(0x1077b720);
    a.offset(3, 25, 0x60); a.move(4, 24); a.value(5, 0); a.call(0x10769b60); a.value(4, 0); a.store<1>(19, 0, 4); a.value(3, UINT32_MAX); a.call(0x1076f7d0);
}

bool advance_match_clock_and_period(MatchAdvanceCall& a) {
    a.at(0x101d4700); a.load<2>(15, 31, 0x794a, true); a.offset(5, 15, 1); a.store<2>(31, 0x794a, 5); a.load<2>(3, 31, 0x794a, true); a.compare(3, 1);
    if (!a.at_least()) { a.load<1>(0, 31, 0x1c1); a.sign8(3, 0); a.compare(3, 0); if (a.equal()) return false; }
    a.at(0x101d4728); a.offset(20, 20, -1); if (abort_at_configured_time(a)) return true;
    a.at(0x101d4814); a.move(3, 31); a.call(0x101d57c0); a.sign8(0, 3); a.compare(0, 0);
    if (!a.equal()) { a.move(3, 31); a.value(4, 0x3de); a.call(0x101f2400); }
    a.at(0x101d4834); a.load<2>(3, 31, 0x7948, true); a.offset(0, 3, 1); a.store<2>(31, 0x7948, 0); a.value(3, 0); a.store<2>(31, 0x794a, 3);
    a.move(3, 31); a.call(0x101d95a0); a.sign8(3, 3); a.compare(3, 2);
    if (a.equal()) { a.at(0x101d4888); a.value(3, 4); a.jump(0x101d5644); return true; }
    if (!a.at_least()) {
        a.compare(3, 0); if (a.equal()) { a.at(0x101d4b30); a.move(3, 31); a.call(0x101d9460); return false; }
        if (a.at_least()) { a.at(0x101d4880); a.value(3, 4); a.jump(0x101d5644); return true; }
        a.jump(0x101d4ad8);
    } else {
        a.at(0x101d4870); a.compare(3, 4); if (a.equal()) { publish_period_score(a); return true; }
        if (!a.at_least()) { a.jump(0x101d4890); a.at(0x101d4890); a.value(3, 4); a.jump(0x101d5644); return true; }
    }
    report_invalid_period(a); a.at(0x101d4b30); a.move(3, 31); a.call(0x101d9460); return false;
}

bool start_restart_after_clock(MatchAdvanceCall& a) {
    a.at(0x101d4b38); a.load<1>(3, 31, 0x7928); a.sign8(0, 3); a.compare(0, 0); if (!a.equal()) return false;
    a.move(3, 31); a.call(0x101d6da0); a.load<1>(4, 31, 0x7928); a.sign8(4, 4); a.compare(4, 0); if (a.equal()) return false;
    a.value(3, 10); a.call(0x101d3c90); a.sign8(0, 3); a.store<1>(1, 0x3a, 0); a.load<2>(4, 31, 0x7948, true);
    a.load<1>(3, 1, 0x3a); a.sign8(0, 3); a.add(0, 4, 0); a.store<2>(31, 0x7948, 0); a.load<1>(3, 1, 0x3a); a.sign8(0, 3);
    a.floating_load(1, 29, 0x60); a.exclusive_flags(0, 0, 0x80000000); a.store(1, 0x78, 0); a.value(3, 0x43300000); a.store(1, 0x74, 3);
    a.floating_load(0, 1, 0x74); a.floating_difference(1, 0, 1); a.floating_load(0, 29, 0x78); a.floating_difference(0, 1, 0, true);
    a.floating_integer(0, 0); a.floating_store(0, 1, 0x6c); a.load(3, 1, 0x70); a.call(0x101d3c90); a.sign8(0, 3);
    a.load<1>(4, 31, 0x792d); a.add(4, 4, 0); a.store<1>(31, 0x792d, 4); a.jump(0x101d5634); return true;
}

void initialize_tick_team_targets(MatchAdvanceCall& a) {
    a.at(0x101d4bd4); a.offset(3, 31, 0x10000); a.load(0, 3, -0x1fd8); a.offset(4, 31, 0x10000); a.load(3, 4, -0x2000); a.compare_registers(0, 3, false);
    if (!a.equal()) {
        a.offset(4, 31, 0x10000); a.load(3, 4, -0x2000); a.value(0, 0); a.compare_registers(3, 0, false);
        if (!a.equal()) { a.value(4, 0); a.offset(3, 31, 0x10000); a.store(3, -0x1fd8, 4); a.value(3, 0); a.offset(4, 31, 0x10000); a.store(4, -0x1fdc, 3); }
    }
    a.at(0x101d4c18); a.move(3, 31); a.call(0x101d57c0); a.move(5, 3); a.move(3, 31); a.load<1>(4, 31, 0x7924); a.call(0x101f4900);
    a.move(3, 31); a.call(0x101d93b0); a.move(3, 31); a.call(0x101e4130); a.move(3, 31); a.call(0x101f0f20); a.move(3, 31); a.call(0x101d9080);
    a.value(3, 0); a.store(31, 0x7c68, 3); a.move(3, 31); a.value(4, 0); a.load<1>(5, 31, 0x791d); a.load<1>(6, 31, 0x791e); a.call(0x101eea30);
    a.sign8(4, 3); a.leading_zeroes(0, 4); a.mask(3, 0, 0x07ffffff, false, 27); a.sign8(7, 3); a.move(3, 31); a.value(4, 0);
    a.load<1>(5, 31, 0x791d); a.load<1>(6, 31, 0x791e); a.call(0x101e32e0); a.offset(4, 31, 0x10000); a.store(4, -0x1ffc, 3);
    a.move(3, 31); a.value(4, 1); a.load<1>(5, 31, 0x791d); a.load<1>(6, 31, 0x791e); a.call(0x101eea30); a.sign8(0, 3);
    a.leading_zeroes(3, 0); a.mask(3, 3, 0x07ffffff, false, 27); a.sign8(7, 3); a.move(3, 31); a.value(4, 1);
    a.load<1>(5, 31, 0x791d); a.load<1>(6, 31, 0x791e); a.call(0x101e32e0); a.offset(4, 31, 0x10000); a.store(4, -0x1ff8, 3);
    a.move(3, 31); a.value(4, 0); a.call(0x101f1e10); a.move(4, 3); a.move(3, 31); a.load<1>(5, 31, 0x791d); a.load<1>(6, 31, 0x791e); a.call(0x101e31a0); a.store<1>(31, 0x7922, 3);
    a.move(3, 31); a.value(4, 1); a.call(0x101f1e10); a.move(4, 3); a.move(3, 31); a.load<1>(5, 31, 0x791d); a.load<1>(6, 31, 0x791e); a.call(0x101e31a0); a.store<1>(31, 0x7923, 3);
    a.load<1>(0, 31, 0x791d); a.store<1>(1, 0x3d, 0); a.load<1>(3, 31, 0x791e); a.store<1>(1, 0x3c, 3); a.value(3, UINT32_MAX); a.store<1>(31, 2, 3); a.value(0, UINT32_MAX); a.store<1>(31, 3, 0);
    a.move(3, 31); a.value(4, 0); a.call(0x101d5760); a.sign8(7, 3); a.compare(7, 0);
    if (!a.equal()) { a.move(3, 31); a.value(4, 0); a.call(0x101d8f20); }
    a.at(0x101d4d64); a.move(3, 31); a.value(4, 1); a.call(0x101d5760); a.move(0, 3); a.sign8(3, 0); a.compare(3, 0);
    if (!a.equal()) { a.move(3, 31); a.value(4, 1); a.call(0x101d8f20); }
}

void reset_live_actor_queue(MatchAdvanceCall& a) {
    a.at(0x101d4d8c); a.value(22, 0); a.jump(0x101d4ddc);
    for (;;) {
        a.at(0x101d4ddc); a.sign8(4, 22); a.load<1>(3, 31, 0x792e); a.sign8(0, 3); a.compare_registers(4, 0);
        if (!a.less()) break;
        a.at(0x101d4d94); a.sign8(3, 22); a.mask(3, 3, 0xfffffffc, false, 2); a.offset(0, 3, 0x79f8); a.indexed_load(3, 31, 0); a.move(4, 22); a.call(0x101d5750);
        a.sign8(0, 22); a.mask(7, 0, 0xfffffffc, false, 2); a.offset(3, 7, 0x79f8); a.indexed_load(3, 31, 3); a.call(0x101d5740);
        a.sign8(4, 22); a.mask(4, 4, 0xfffffffc, false, 2); a.offset(3, 4, 0x79f8); a.indexed_load(3, 31, 3); a.value(4, 0); a.call(0x101d5730); a.offset(22, 22, 1);
    }
}

void assign_live_actor_partner(MatchAdvanceCall& a, std::uint32_t address) {
    a.at(address); a.move(3, 26); a.move(4, 21); a.call(0x101d5730); a.move(3, 21); a.move(4, 26); a.call(0x101d5700);
    if (address != 0x101d5034) a.jump(0x101d504c);
}

void choose_live_actor_partner(MatchAdvanceCall& a) {
    a.at(0x101d4df8); a.sign8(0, 22); a.mask(3, 0, 0xfffffffc, false, 2); a.offset(3, 3, 0x79f8); a.indexed_load(26, 31, 3);
    a.move(3, 26); a.call(0x101d3b50); a.move(4, 3); a.load<1>(0, 26, 0x27); a.sign8(0, 0); a.multiply(5, 0, 0x18e0);
    a.offset(3, 5, 0x7c6c); a.add(3, 31, 3); a.call(0x101149f0); a.move(0, 3); a.sign8(0, 0); a.mask(4, 0, 1); a.compare(4, 0); if (!a.equal()) return;
    a.move(3, 26); a.call(0x10285ef0); a.move(21, 3); a.compare(21, 0, false);
    if (!a.equal()) { assign_live_actor_partner(a, 0x101d4e54); return; }
    a.at(0x101d4e70); a.move(3, 26); a.call(0x10289560); a.sign16(0, 3); a.compare(0, 1); if (a.equal()) return;
    a.offset(3, 31, 0x10000); a.load(0, 3, -0x2000); a.compare_registers(26, 0, false); if (a.equal()) return;
    a.load<1>(3, 26, 0x27); a.sign8(0, 3); a.multiply(3, 0, 0x18e0); a.offset(3, 3, 0x7c6c); a.add(3, 31, 3); a.call(0x101d56f0);
    a.mask(3, 3, 0x4000); a.compare(3, 0);
    if (!a.equal()) {
        a.move(3, 31); a.move(4, 26); a.call(0x101d70d0); a.move(21, 3); a.compare(21, 0, false);
        if (!a.equal()) assign_live_actor_partner(a, 0x101d4ed0);
        return;
    }
    a.at(0x101d4eec); a.load<1>(3, 31, 0x7924); a.sign8(3, 3); a.load<1>(0, 26, 0x27); a.sign8(0, 0); a.compare_registers(3, 0);
    if (a.equal()) {
        a.move(3, 31); a.call(0x101d56e0); a.move(15, 3); a.move(3, 31); a.load<1>(0, 26, 0x27); a.sign8(0, 0);
        a.leading_zeroes(0, 0); a.mask(0, 0, 0x07ffffff, false, 27); a.sign8(4, 0); a.move(5, 15); a.call(0x101ee9f0); a.sign8(0, 3); a.compare(0, 0); if (a.equal()) return;
    }
    a.at(0x101d4f3c); a.move(3, 26); a.call(0x10289560); a.sign16(0, 3); a.mask(0, 0, 4); a.compare(0, 0);
    bool check_player = !a.equal();
    if (!check_player) { a.move(3, 26); a.call(0x10289560); a.sign16(0, 3); a.mask(3, 0, 2); a.compare(3, 0); check_player = !a.equal(); }
    if (check_player) {
        a.at(0x101d4f6c); a.move(3, 26); a.call(0x10283620); a.sign8(3, 3); a.compare(3, 0);
        if (!a.equal()) {
            a.at(0x101d501c); a.move(3, 31); a.move(4, 26); a.call(0x101d70d0); a.move(21, 3); a.compare(21, 0, false);
            if (!a.equal()) assign_live_actor_partner(a, 0x101d5034);
            return;
        }
    }
    a.at(0x101d4f80); a.move(3, 26); a.call(0x10289560); a.sign16(0, 3); a.mask(3, 0, 8); a.compare(3, 0);
    bool team_target = !a.equal();
    if (!team_target) { a.move(3, 26); a.call(0x10289560); a.move(4, 3); a.sign16(3, 4); a.mask(3, 3, 16); a.compare(3, 0); team_target = !a.equal(); }
    bool pair = false;
    if (team_target) {
        a.at(0x101d4fb4); a.move(3, 31); a.call(0x101d56e0); a.move(5, 3); a.move(3, 31); a.load<1>(0, 26, 0x27); a.sign8(0, 0);
        a.leading_zeroes(0, 0); a.mask(0, 0, 0x07ffffff, false, 27); a.sign8(4, 0); a.call(0x101ee9f0); a.sign8(0, 3); a.compare(0, 0); pair = !a.equal();
    }
    if (!pair) {
        a.at(0x101d4fe8); a.move(3, 31); a.call(0x101d56e0); a.move(15, 3); a.move(3, 31); a.call(0x101d56d0); a.move(5, 3);
        a.move(3, 31); a.load<1>(4, 26, 0x27); a.move(6, 15); a.call(0x101eea30); a.sign8(0, 3); a.compare(0, 0); pair = !a.equal();
    }
    if (pair) {
        a.at(0x101d501c); a.move(3, 31); a.move(4, 26); a.call(0x101d70d0); a.move(21, 3); a.compare(21, 0, false);
        if (!a.equal()) assign_live_actor_partner(a, 0x101d5034);
    }
}

void pair_live_actor_queue(MatchAdvanceCall& a) {
    a.at(0x101d4df0); a.value(22, 0); a.jump(0x101d5050);
    for (;;) {
        a.at(0x101d5050); a.sign8(3, 22); a.load<1>(0, 31, 0x792e); a.sign8(0, 0); a.compare_registers(3, 0); if (!a.less()) break;
        choose_live_actor_partner(a); a.at(0x101d504c); a.offset(22, 22, 1);
    }
}

void prepare_live_actor_action(MatchAdvanceCall& a) {
    a.at(0x101d50b0); a.load<1>(0, 1, 0x3b); a.sign8(3, 0); a.compare(3, 0);
    bool execute_action = a.equal();
    if (!execute_action) {
        a.value(3, 10); a.call(0x101d3c90); a.move(15, 3); a.mask(3, 23, 0xfffffffc, false, 2); a.offset(0, 3, 0x79f8);
        a.indexed_load(3, 31, 0); a.call(0x101d56b0); a.sign8(0, 3); a.compare_registers(0, 15); execute_action = !a.nonpositive();
    }
    if (execute_action) { a.at(0x101d50e8); a.mask(3, 23, 0xfffffffc, false, 2); a.offset(0, 3, 0x79f8); a.indexed_load(3, 31, 0); a.call(0x102775e0); }
    a.at(0x101d50f8); a.mask(3, 23, 0xfffffffc, false, 2); a.offset(0, 3, 0x79f8); a.indexed_load(3, 31, 0); a.call(0x101d56a0); a.sign16(0, 3); a.compare(0, 17);
    if (a.equal()) {
        a.floating_load(2, 30, 0x20, true); a.mask(3, 23, 0xfffffffc, false, 2); a.offset(3, 3, 0x79f8); a.indexed_load(3, 31, 3);
        a.floating_load(1, 3, 0x74, true); a.floating_load(0, 30, 0x1c, true); a.player_probability(); a.floating_integer(0, 0); a.floating_store(0, 1, 0x6c);
        a.load(3, 1, 0x70); a.call(0x101d3c90); a.compare(3, 0);
        if (a.equal()) { a.value(0, 1); a.store<1>(1, 0x3b, 0); a.jump(0x101d51ac); }
    } else {
        a.at(0x101d5154); a.mask(3, 23, 0xfffffffc, false, 2); a.offset(3, 3, 0x79f8); a.indexed_load(3, 31, 3); a.call(0x101d56a0); a.sign16(0, 3); a.compare(0, 18);
        if (a.equal()) {
            a.floating_load(2, 30, 0x20, true); a.mask(4, 23, 0xfffffffc, false, 2); a.offset(0, 4, 0x79f8); a.indexed_load(3, 31, 0);
            a.floating_load(1, 3, 0xc4, true); a.floating_load(0, 30, 0x1c, true); a.player_probability(); a.floating_integer(0, 0); a.floating_store(0, 1, 0x6c);
            a.load(3, 1, 0x70); a.call(0x101d3c90); a.compare(3, 0); if (a.equal()) { a.value(0, 1); a.store<1>(1, 0x3b, 0); }
        }
    }
}

bool account_action_interruption(MatchAdvanceCall& a) {
    a.at(0x101d51ac); a.load<1>(4, 31, 0x7928); a.sign8(0, 4); a.compare(0, 0);
    bool stopped = !a.equal();
    if (!stopped) { a.load<1>(3, 31, 0x1c1); a.sign8(3, 3); a.compare(3, 0); stopped = !a.equal(); }
    if (!stopped) return false;
    a.at(0x101d51cc); a.value(3, 10); a.call(0x101d3c90); a.sign8(3, 3); a.store<1>(1, 0x39, 3); a.load<2>(5, 31, 0x7948, true);
    a.load<1>(4, 1, 0x39); a.sign8(0, 4); a.add(0, 5, 0); a.store<2>(31, 0x7948, 0); a.load<1>(4, 1, 0x39); a.sign8(5, 4);
    a.floating_load(1, 29, 0x60); a.exclusive_flags(3, 5, 0x80000000); a.store(1, 0x70, 3); a.value(0, 0x43300000); a.store(1, 0x6c, 0);
    a.floating_load(0, 1, 0x6c); a.floating_difference(1, 0, 1); a.floating_load(0, 29, 0x78); a.floating_difference(0, 1, 0, true);
    a.floating_integer(0, 0); a.floating_store(0, 1, 0x74); a.load(3, 1, 0x78); a.call(0x101d3c90); a.sign8(4, 3);
    a.load<1>(3, 31, 0x792d); a.add(3, 3, 4); a.store<1>(31, 0x792d, 3); a.load<1>(4, 31, 0x792b); a.sign8(4, 4); a.compare(4, 0);
    if (!a.equal()) { a.move(3, 31); a.call(0x101f6c60); a.jump(0x101d5634); }
    return true;
}

void service_action_timer_and_ball(MatchAdvanceCall& a) {
    a.at(0x101d5258); a.load<1>(3, 31, 0x1de); a.sign8(3, 3); a.compare(3, 0);
    if (!a.equal()) {
        a.load<1>(4, 31, 0x1de); a.offset(3, 4, -1); a.store<1>(31, 0x1de, 3); a.load<1>(4, 31, 0x1de); a.sign8(0, 4); a.compare(0, 0);
        if (a.equal()) {
            a.load<1>(0, 31, 0x791f); a.sign8(3, 0); a.compare(3, 4);
            if (a.equal()) {
                a.load(3, 31, 0x3104); a.compare(3, 0, false);
                if (a.equal()) { a.move(3, 31); a.load<1>(4, 31, 0x1df); a.load<1>(5, 31, 0x1e0); a.call(0x101ec890); a.jump(0x101d53fc); return; }
            }
        }
    }
    a.at(0x101d52b4); a.load<1>(4, 31, 0x1de); a.sign8(4, 4); a.compare(4, 0); if (!a.equal()) return;
    a.load(3, 31, 0x3104); a.compare(3, 0, false); if (a.equal()) return;
    a.load(3, 31, 0x3104); a.call(0x101d5690); a.sign8(4, 3); a.load<1>(3, 31, 0x791d); a.sign8(0, 3); a.compare_registers(0, 4);
    bool at_ball = a.equal();
    if (at_ball) { a.load(3, 31, 0x3104); a.call(0x101d5680); a.move(4, 3); a.sign8(4, 4); a.load<1>(3, 31, 0x791e); a.sign8(0, 3); a.compare_registers(0, 4); at_ball = a.equal(); }
    if (at_ball) {
        a.load(3, 31, 0x3104); a.load<2>(4, 31, 0x794e, true); a.load<1>(5, 31, 0x1e1); a.load<1>(6, 31, 0x1e2); a.call(0x1023cd20); a.jump(0x101d53fc); return;
    }
    a.at(0x101d5324); a.load(4, 31, 0x3104); a.offset(3, 31, 0x10000); a.load(3, 3, -0x2008); a.compare_registers(4, 3, false); if (!a.equal()) return;
    a.load(3, 31, 0x3104); a.load<1>(3, 3, 0x2c); a.sign8(3, 3); a.compare(3, 0); if (!a.equal()) return;
    a.load(3, 31, 0x3104); a.call(0x101d5670); a.sign8(4, 3); a.compare(4, 0); if (a.equal()) return;
    a.load(3, 31, 0x3104); a.call(0x101d5660); a.move(4, 3); a.sign8(4, 4); a.compare(4, 0); if (a.equal()) return;
    a.load(3, 31, 0x3104); a.store(1, 0x44, 3); a.value(4, 1); a.store<1>(31, 0x1de, 4); a.load(3, 31, 0x3104); a.value(4, 1); a.call(0x10281af0);
    a.value(4, 0); a.store<1>(31, 0x1de, 4); a.load(3, 31, 0x3104); a.load(5, 1, 0x44); a.compare_registers(3, 5, false); if (!a.equal()) return;
    a.load(3, 31, 0x3104); a.call(0x101d5690); a.sign8(4, 3); a.load<1>(3, 31, 0x791d); a.sign8(0, 3); a.compare_registers(0, 4); if (!a.equal()) return;
    a.load(3, 31, 0x3104); a.call(0x101d5680); a.move(4, 3); a.sign8(4, 4); a.load<1>(3, 31, 0x791e); a.sign8(0, 3); a.compare_registers(0, 4); if (!a.equal()) return;
    a.load(3, 31, 0x3104); a.load<2>(4, 31, 0x794e, true); a.load<1>(5, 31, 0x1e1); a.load<1>(6, 31, 0x1e2); a.call(0x1023cd20);
}

bool account_ball_interruption(MatchAdvanceCall& a) {
    a.at(0x101d53fc); a.load<1>(4, 31, 0x7928); a.sign8(3, 4); a.compare(3, 0);
    bool stopped = !a.equal();
    if (!stopped) { a.load<1>(0, 31, 0x1c1); a.sign8(0, 0); a.compare(0, 0); stopped = !a.equal(); }
    if (!stopped) return false;
    a.at(0x101d541c); a.value(3, 10); a.call(0x101d3c90); a.sign8(0, 3); a.store<1>(1, 0x38, 0); a.load<2>(0, 31, 0x7948, true);
    a.load<1>(3, 1, 0x38); a.sign8(3, 3); a.add(0, 0, 3); a.store<2>(31, 0x7948, 0); a.load<1>(4, 1, 0x38); a.sign8(4, 4);
    a.floating_load(1, 29, 0x60); a.exclusive_flags(3, 4, 0x80000000); a.store(1, 0x70, 3); a.value(0, 0x43300000); a.store(1, 0x6c, 0);
    a.floating_load(0, 1, 0x6c); a.floating_difference(1, 0, 1); a.floating_load(0, 29, 0x78); a.floating_difference(0, 1, 0, true);
    a.floating_integer(0, 0); a.floating_store(0, 1, 0x74); a.load(3, 1, 0x78); a.call(0x101d3c90); a.move(4, 3); a.sign8(3, 4);
    a.load<1>(4, 31, 0x792d); a.add(3, 4, 3); a.store<1>(31, 0x792d, 3); a.jump(0x101d5634); return true;
}

void repeat_actor_if_requested(MatchAdvanceCall& a) {
    a.at(0x101d54a0); a.mask(4, 23, 0xfffffffc, false, 2); a.offset(3, 4, 0x79f8); a.indexed_load(3, 31, 3); a.call(0x101d56a0); a.sign16(4, 3); a.compare(4, 0x104);
    bool repeat = a.equal();
    if (!repeat) { a.mask(5, 23, 0xfffffffc, false, 2); a.offset(3, 5, 0x79f8); a.indexed_load(3, 31, 3); a.call(0x101d56a0); a.sign16(4, 3); a.compare(4, 0x76); repeat = a.equal(); }
    if (!repeat) return;
    a.at(0x101d54d8); a.compare(23, 22); if (a.at_least()) return;
    a.mask(3, 23, 0xfffffffc, false, 2); a.offset(0, 3, 0x79f8); a.indexed_load(7, 31, 0); a.load<1>(3, 31, 0x792e); a.move(4, 3);
    a.offset(3, 3, 1); a.store<1>(31, 0x792e, 3); a.sign8(0, 4); a.mask(3, 0, 0xfffffffc, false, 2); a.offset(3, 3, 0x79f8); a.indexed_store(7, 31, 3);
}

void refresh_tick_team_targets(MatchAdvanceCall& a) {
    a.at(0x101d550c); a.move(3, 31); a.call(0x101e4130); a.load<1>(7, 1, 0x3d); a.sign8(3, 7); a.load<1>(4, 31, 0x791d); a.sign8(4, 4); a.compare_registers(3, 4);
    if (a.equal()) { a.load<1>(3, 1, 0x3c); a.sign8(4, 3); a.load<1>(3, 31, 0x791e); a.sign8(0, 3); a.compare_registers(4, 0); if (a.equal()) return; }
    a.at(0x101d5544); a.move(3, 31); a.value(4, 0); a.load<1>(5, 31, 0x791d); a.load<1>(6, 31, 0x791e); a.call(0x101eea30); a.sign8(3, 3);
    a.leading_zeroes(3, 3); a.mask(0, 3, 0x07ffffff, false, 27); a.sign8(7, 0); a.move(3, 31); a.value(4, 0); a.load<1>(5, 31, 0x791d); a.load<1>(6, 31, 0x791e); a.call(0x101e32e0);
    a.move(0, 3); a.offset(3, 31, 0x10000); a.store(3, -0x1ffc, 0); a.move(3, 31); a.value(4, 1); a.load<1>(5, 31, 0x791d); a.load<1>(6, 31, 0x791e); a.call(0x101eea30);
    a.sign8(3, 3); a.leading_zeroes(0, 3); a.mask(0, 0, 0x07ffffff, false, 27); a.sign8(7, 0); a.move(3, 31); a.value(4, 1); a.load<1>(5, 31, 0x791d); a.load<1>(6, 31, 0x791e); a.call(0x101e32e0);
    a.offset(4, 31, 0x10000); a.store(4, -0x1ff8, 3); a.move(3, 31); a.value(4, 0); a.call(0x101f1e10); a.move(4, 3); a.move(3, 31);
    a.load<1>(5, 31, 0x791d); a.load<1>(6, 31, 0x791e); a.call(0x101e31a0); a.store<1>(31, 0x7922, 3); a.move(3, 31); a.value(4, 1); a.call(0x101f1e10); a.move(4, 3); a.move(3, 31);
    a.load<1>(5, 31, 0x791d); a.load<1>(6, 31, 0x791e); a.call(0x101e31a0); a.store<1>(31, 0x7923, 3); a.load<1>(0, 31, 0x791d); a.store<1>(1, 0x3d, 0); a.load<1>(4, 31, 0x791e); a.store<1>(1, 0x3c, 4);
}

void execute_live_actor_queue(MatchAdvanceCall& a) {
    a.at(0x101d5064); a.value(0, 0); a.store<1>(1, 0x3b, 0); a.value(23, 0); a.jump(0x101d5624);
    for (;;) {
        // Read the current count on every iteration: actions can append actors.
        a.at(0x101d5624); a.load<1>(0, 31, 0x792e); a.sign8(3, 0); a.compare_registers(23, 3); if (!a.less()) return;
        a.at(0x101d5074); a.mask(3, 23, 0xfffffffc, false, 2); a.offset(0, 3, 0x79f8); a.indexed_load(3, 31, 0); a.call(0x101d56c0); a.compare(3, 0, false);
        bool active_side = a.equal();
        if (!active_side) {
            a.load<1>(0, 31, 0x7924); a.sign8(0, 0); a.mask(3, 23, 0xfffffffc, false, 2); a.offset(4, 3, 0x79f8); a.indexed_load(3, 31, 4);
            a.load<1>(4, 3, 0x27); a.sign8(3, 4); a.compare_registers(0, 3); active_side = a.equal();
        }
        if (active_side) {
            prepare_live_actor_action(a); if (account_action_interruption(a)) return;
            service_action_timer_and_ball(a); if (account_ball_interruption(a)) return;
            a.at(0x101d5494); a.load(0, 31, 0x7c68); a.compare(0, 0, false); if (!a.equal()) return;
            repeat_actor_if_requested(a); refresh_tick_team_targets(a);
        }
        a.at(0x101d5620); a.offset(23, 23, 1);
    }
}

bool advance_active_match_tick(MatchAdvanceCall& a) {
    if (announce_pending_restart(a) || select_pending_restart_player(a) || service_pending_incident(a)) return false;
    prepare_stopped_ball_owner(a); if (advance_match_clock_and_period(a)) return true;
    if (start_restart_after_clock(a)) return false;
    initialize_tick_team_targets(a); reset_live_actor_queue(a); pair_live_actor_queue(a); execute_live_actor_queue(a); return false;
}

void advance_shared_body(MatchAdvanceCall& a) {
    a.link(); a.multiple(15); a.save_link(); a.frame(208); a.move(31, 3); a.move(20, 4);
    a.load(27, 2, -0x7a54); a.load(28, 2, -0x7a6c); a.load(19, 2, -0x7bac); a.load(24, 2, -0x7ba8);
    a.load(25, 2, -0x52cc); a.load(30, 2, -0x52c8); a.load(29, 2, -0x52c4); a.load<1>(4, 31, 0); a.sign8(5, 4); a.compare(5, 0);
    if (!a.equal()) { finish_already_completed_match(a); return; }
    for (;;) {
        a.at(0x101d5634); a.sign8(4, 20); a.compare(4, 0);
        if (a.equal()) { a.value(3, 1); return; }
        a.at(0x101d40bc); a.load<1>(0, 31, 0x792a); a.sign8(4, 0); a.compare(4, 1);
        if (a.equal()) {
            a.move(3, 31); a.value(4, 0); a.call(0x101f75a0); a.offset(20, 20, -1); a.jump(0x101d5634); continue;
        }
        a.at(0x101d40e0); a.load<1>(4, 31, 0x7929); a.sign8(3, 4); a.compare(3, 4);
        if (!a.at_least()) {
            a.compare(3, 1);
            if (a.at_least()) { if (advance_early_stopped_phase(a)) return; }
            else { a.jump(0x101d4374); if (advance_active_match_tick(a)) return; }
        } else {
            a.at(0x101d40fc); a.compare(3, 7);
            if (a.at_least()) { if (advance_active_match_tick(a)) return; }
            else { a.jump(0x101d4248); if (advance_late_stopped_phase(a)) return; }
        }
    }
}
}

void advance_watched(State& state, DependencyDispatch* dependencies) {
    MatchAdvanceCall call(state, entries[0], dependencies); advance_watched_body(call);
    call.at(0x101cd23c); call.pop(80); call.multiple(26, true); call.finish();
}
void advance_background(State& state, DependencyDispatch* dependencies) {
    MatchAdvanceCall call(state, entries[1], dependencies); advance_background_body(call);
    call.at(0x101cd85c); call.pop(112); call.restore(31); call.restore(30); call.restore(29); call.restore(28); call.finish();
}
void advance_shared(State& state, DependencyDispatch* dependencies) {
    MatchAdvanceCall call(state, entries[2], dependencies); advance_shared_body(call);
    call.at(0x101d5644); call.pop(208); call.multiple(15, true); call.finish();
}
}
