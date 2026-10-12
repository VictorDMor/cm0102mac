#include "match_actor.hpp"
#include "guest_context_transfer.hpp"
#include <bit>

namespace cm::aot::match_actor {
namespace {
// Ordered observation points retain the original argument homes, clobbers,
// budgets, memory faults and suspended frames around structured injury logic.
class ActorCall {
    State& state_;
    std::uint32_t next_;
    GuestNativeScope scope_;
    DependencyDispatch* dependencies_;
    void step() { state_.step(next_); next_ += 4; }
    void condition(std::int64_t left, std::int64_t right) {
        state_.wr<1>(2304, (left < right ? 8 : left > right ? 4 : 2) | (state_.r<1>(1024) & 1));
    }
public:
    ActorCall(State& s, std::uint32_t root, DependencyDispatch* d = nullptr)
        : state_(s), next_(root), scope_(s, root), dependencies_(d) {}
    std::uint32_t r(unsigned reg) const { return std::uint32_t(state_.r<4>(reg * 4)); }
    void at(std::uint32_t address) { next_ = address; }
    void link() { step(); state_.wr<4>(0, state_.r<4>(4128)); }
    void save_link() { step(); state_.memory.write<4>(r(1) + 8, r(0)); }
    void frame(unsigned bytes) {
        step(); const auto old = r(1), current = old - bytes;
        state_.memory.write<4>(current, old); state_.wr<4>(4, current); scope_.frame(current);
    }
    void multiple(unsigned first, bool restore = false) {
        step(); state_.wr<4>(2432, r(1) - (32 - first) * 4);
        for (unsigned reg = first; reg < 32; ++reg) {
            if (restore) state_.wr<4>(reg * 4, state_.memory.read<4>(state_.r<4>(2432)));
            else state_.memory.write<4>(state_.r<4>(2432), r(reg));
            state_.wr<4>(2432, state_.r<4>(2432) + 4);
        }
    }
    void move(unsigned destination, unsigned source) { step(); state_.wr<4>(destination * 4, r(source)); }
    void value(unsigned destination, std::uint32_t value) { step(); state_.wr<4>(destination * 4, value); }
    void offset(unsigned destination, unsigned source, std::int32_t value) { step(); state_.wr<4>(destination * 4, r(source) + std::uint32_t(value)); }
    void add(unsigned destination, unsigned first, unsigned second) { step(); state_.wr<4>(destination * 4, r(first) + r(second)); }
    template<unsigned N = 4> void load(unsigned destination, unsigned base, std::int32_t displacement, bool sign = false) {
        step(); const auto value = state_.memory.read<N>(r(base) + std::uint32_t(displacement));
        state_.wr<4>(destination * 4, sign ? std::uint32_t(signed_value<N>(value)) : value);
    }
    template<unsigned N = 4> void store(unsigned base, std::int32_t displacement, unsigned source) {
        step(); state_.memory.write<N>(r(base) + std::uint32_t(displacement), r(source));
    }
    void sign8(unsigned destination, unsigned source) { step(); state_.wr<4>(destination * 4, std::uint32_t(signed_value<1>(r(source)))); }
    void sign16(unsigned destination, unsigned source) { step(); state_.wr<4>(destination * 4, std::uint32_t(signed_value<2>(r(source)))); }
    void compare(unsigned reg, std::int32_t value) { step(); condition(std::bit_cast<std::int32_t>(r(reg)), value); }
    void compare_unsigned(unsigned first, unsigned second) { step(); condition(r(first), r(second)); }
    bool equal() { step(); return (state_.r<1>(2304) & 2) != 0; }
    bool at_least() { step(); return (state_.r<1>(2304) & 8) == 0; }
    void jump(std::uint32_t address) { step(); at(address); }
    void multiply(unsigned destination, unsigned source, std::uint32_t value) { step(); state_.wr<4>(destination * 4, r(source) * value); }
    void multiply_high(unsigned destination, unsigned first, unsigned second) {
        step(); const auto value = std::int64_t(std::bit_cast<std::int32_t>(r(first))) * std::bit_cast<std::int32_t>(r(second));
        state_.wr<4>(destination * 4, std::uint32_t(std::bit_cast<std::uint64_t>(value) >> 32));
    }
    void shift_right(unsigned destination, unsigned source, unsigned count) {
        step(); const auto value = r(source);
        state_.wr<1>(1027, std::bit_cast<std::int32_t>(value) < 0 && (value & ((UINT32_C(1) << count) - 1)));
        state_.wr<4>(destination * 4, std::uint32_t(std::bit_cast<std::int32_t>(value) >> count));
    }
    void rotate_mask(unsigned destination, unsigned source, unsigned rotation, std::uint32_t mask) {
        step(); state_.wr<4>(destination * 4, std::rotl(r(source), int(rotation)) & mask);
    }
    void stamp(unsigned source, unsigned base, unsigned index) { step(); state_.memory.write<2>(r(base) + r(index), r(source)); }
    void call(std::uint32_t target) {
        step(); state_.wr<4>(2440, r(2)); state_.wr<4>(4128, next_); state_.pc = target;
        if (dependencies_) dependencies_->call(state_, target, next_); else run_until(state_, next_);
    }
    void restore_link() { step(); state_.wr<4>(4128, r(0)); }
    void finish() { step(); state_.pc = std::uint32_t(state_.r<4>(4128)); }
};

void cap_withdrawal_minute(ActorCall& a, std::uint32_t start, unsigned period_end, unsigned saved_reg) {
    a.at(start); a.load<1>(0, 31, 0x21); a.sign8(0, 0); a.compare(0, period_end);
    if (!a.at_least()) { a.load<1>(saved_reg, 31, 0x21); a.sign8(saved_reg, saved_reg); a.jump(start + 0x20); }
    else { a.at(start + 0x1c); a.value(saved_reg, period_end); }
    a.at(start + 0x20); a.sign8(0, saved_reg); a.store<1>(31, 0x21, 0);
    if (period_end != 120) a.jump(0x101f1678);
}

void record_withdrawal(ActorCall& a) {
    a.at(0x101f1560); a.load<1>(3, 31, 0x23); a.offset(0, 3, 1); a.store<1>(31, 0x23, 0);
    a.load<2>(0, 30, 0x7948, true); a.value(3, 0x2e8c0000); a.offset(3, 3, -0x5d17);
    a.multiply_high(0, 3, 0); a.shift_right(0, 0, 1); a.rotate_mask(3, 0, 1, 1); a.add(0, 0, 3);
    a.sign8(0, 0); a.store<1>(31, 0x21, 0);
    a.move(3, 30); a.call(0x101e8900); a.sign16(0, 3); a.compare(0, 0x483);
    if (a.equal()) cap_withdrawal_minute(a, 0x101f1624, 105, 26);
    else if (a.at_least()) {
        a.at(0x101f15c0); a.compare(0, 0x528);
        if (a.equal()) cap_withdrawal_minute(a, 0x101f1650, 120, 25);
        else a.jump(0x101f1678);
    } else {
        a.compare(0, 0x3de);
        if (a.equal()) cap_withdrawal_minute(a, 0x101f15f8, 90, 27);
        else if (!a.at_least()) {
            a.compare(0, 0x1ef);
            if (a.equal()) cap_withdrawal_minute(a, 0x101f15cc, 45, 28);
            else a.jump(0x101f1678);
        }
    }
    a.at(0x101f1678); a.move(3, 30); a.move(4, 31); a.value(5, 0xfffffffd); a.call(0x101f1950);
}

void team_controller(ActorCall& a, std::uint32_t start) {
    a.at(start); a.load<1>(0, 31, 0x27); a.sign8(0, 0); a.multiply(3, 0, 0xd8); a.offset(3, 3, 0x7aac); a.add(3, 30, 3);
}
void stamp_team_change(ActorCall& a, std::uint32_t start) {
    a.at(start); a.load<2>(4, 30, 0x7948, true); a.load<1>(0, 31, 0x27); a.sign8(0, 0);
    a.rotate_mask(3, 0, 1, 0xfffffffe); a.offset(0, 3, 0x7944); a.stamp(4, 30, 0);
}
void reorganize_team(ActorCall& a) {
    team_controller(a, 0x101f176c); a.call(0x102104d0);
    team_controller(a, 0x101f1784); a.value(4, 0); a.call(0x1020ef70);
    a.move(3, 30); a.load<1>(4, 31, 0x27); a.call(0x101d3b60); a.compare_unsigned(31, 3);
    if (a.equal()) {
        a.move(3, 30); a.load<1>(4, 31, 0x27); a.call(0x101d3bf0); a.move(23, 3);
        team_controller(a, 0x101f17c4); a.call(0x10210430); a.move(24, 3);
        a.move(3, 30); a.load<1>(4, 31, 0x27); a.call(0x101d3c50); a.move(4, 3);
        a.load(3, 2, -0x7a6c); a.load(3, 3, 0); a.move(5, 24); a.move(6, 23); a.call(0x103b0bf0);
    }
    a.at(0x101f1804); a.move(3, 30); a.load<1>(4, 31, 0x27); a.call(0x101f1de0);
    a.move(3, 30); a.load<1>(4, 31, 0x27); a.call(0x101f1e10); stamp_team_change(a, 0x101f181c);
}

void withdraw_body(ActorCall& a) {
    a.link(); a.multiple(23); a.save_link(); a.frame(0x60); a.move(30, 3); a.move(31, 4);
    a.store<1>(1, 0x83, 5); a.value(29, 0); a.load<1>(0, 1, 0x83); a.sign8(0, 0); a.compare(0, 0);
    if (a.equal()) record_withdrawal(a);
    a.at(0x101f1688); a.move(3, 31); a.call(0x10289560); a.sign16(0, 3); a.compare(0, 1);
    if (a.equal()) a.value(29, 1);
    a.at(0x101f16a0); a.move(3, 31); a.value(4, 0xfffffffd); a.call(entries[2]);
    a.load<1>(0, 30, 0); a.sign8(0, 0); a.compare(0, 0);
    if (a.equal()) {
        a.move(3, 30); a.load<1>(4, 31, 0x27); a.call(0x101da5a0); a.sign8(0, 3); a.compare(0, 0);
        if (!a.equal()) return;
    }
    a.at(0x101f16d4); a.load<1>(0, 30, 0x1bc); a.sign8(0, 0); a.compare(0, 0);
    if (!a.equal()) return;
    team_controller(a, 0x101f16e4); a.call(entries[1]); a.sign8(0, 3); a.compare(0, 0);
    bool keep_goalkeeper_tactics = !a.equal();
    if (keep_goalkeeper_tactics) { a.sign8(0, 29); a.compare(0, 0); keep_goalkeeper_tactics = !a.equal(); }
    if (keep_goalkeeper_tactics) {
        team_controller(a, 0x101f1714); a.value(4, 1); a.call(0x1020ef70);
        a.load<1>(0, 30, 0); a.sign8(0, 0); a.compare(0, 0);
        if (a.equal()) { stamp_team_change(a, 0x101f1740); a.jump(0x101f1834); }
    } else {
        a.at(0x101f175c); a.load<1>(0, 30, 0); a.sign8(0, 0); a.compare(0, 0);
        if (a.equal()) reorganize_team(a);
    }
    a.at(0x101f1834); a.move(3, 30); a.load<1>(4, 31, 0x27); a.call(0x101d6a90); a.move(3, 30); a.call(0x101d6f90);
}
}

void withdraw_injured(State& state, DependencyDispatch* dependencies) {
    ActorCall call(state, entries[0], dependencies); withdraw_body(call);
    call.at(0x101f1848); call.load(0, 1, 0x68); call.offset(1, 1, 0x60); call.restore_link(); call.multiple(23, true); call.finish();
}
void read_position(State& state) { ActorCall call(state, entries[1]); call.load<1>(3, 3, 0x19); call.finish(); }
void write_position(State& state) { ActorCall call(state, entries[2]); call.store<1>(3, 0x19, 4); call.finish(); }
}
