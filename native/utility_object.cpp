#include "utility_object.hpp"
#include "guest_context_transfer.hpp"
#include <algorithm>

namespace cm::aot::utility_object {
namespace {
// Object fields and caller saves remain live guest words. Checkpoints retain
// the original budget stops and fault PCs around ordinary lifetime operations.
class LifetimeCall {
    State& state;
    std::uint32_t entry, next;
    GuestNativeScope scope;
    DependencyDispatch* dependencies;
    void step() { state.step(next); next += 4; }
    void condition(std::int64_t a, std::int64_t b, bool record = false) {
        state.wr<1>(2304, (record ? state.r<1>(2304) & 0xf0 : 0) |
            (a < b ? 8 : a > b ? 4 : 2) | (state.r<1>(1024) & 1));
    }
public:
    LifetimeCall(State& s, std::uint32_t root, DependencyDispatch* d)
        : state(s), entry(root), next(root), scope(s, root), dependencies(d) {}
    std::uint32_t r(unsigned n) const { return std::uint32_t(state.r<4>(n * 4)); }
    void at(unsigned offset) { next = entry + offset; }
    void link() { step(); state.wr<4>(0, state.r<4>(4128)); }
    void save(unsigned n) { step(); state.memory.write<4>(r(1) - (32 - n) * 4, r(n)); }
    void restore(unsigned n) { step(); state.wr<4>(n * 4, state.memory.read<4>(r(1) - (32 - n) * 4)); }
    void save_link() { step(); state.memory.write<4>(r(1) + 8, r(0)); }
    void frame(unsigned bytes) {
        step(); const auto old = r(1), current = old - bytes;
        state.memory.write<4>(current, old); state.wr<4>(4, current); scope.frame(current);
    }
    void move(unsigned destination, unsigned source, bool record = false) {
        step(); const auto value = r(source); state.wr<4>(destination * 4, value);
        if (record) condition(std::bit_cast<std::int32_t>(value), 0, true);
    }
    void value(unsigned destination, std::uint32_t value) { step(); state.wr<4>(destination * 4, value); }
    void offset(unsigned destination, unsigned source, std::int32_t displacement) {
        step(); state.wr<4>(destination * 4, r(source) + std::uint32_t(displacement));
    }
    void sign16(unsigned destination, unsigned source, bool record = false) {
        step(); const auto value = signed_value<2>(r(source));
        state.wr<4>(destination * 4, std::uint32_t(value));
        if (record) condition(value, 0, true);
    }
    template<unsigned N = 4> void load(unsigned destination, unsigned base, std::int32_t displacement) {
        step(); state.wr<4>(destination * 4, state.memory.read<N>(r(base) + std::uint32_t(displacement)));
    }
    template<unsigned N = 4> void store(unsigned base, std::int32_t displacement, unsigned source) {
        step(); state.memory.write<N>(r(base) + std::uint32_t(displacement), r(source));
    }
    void compare_zero(unsigned source) { step(); condition(r(source), 0); }
    bool equal() { step(); return (state.r<1>(2304) & 2) != 0; }
    bool positive() { step(); return (state.r<1>(2304) & 10) == 0; }
    void jump(unsigned offset) { step(); at(offset); }
    void call(std::uint32_t target) {
        step(); state.wr<4>(2440, r(2)); state.wr<4>(4128, next); state.pc = target;
        if (dependencies) dependencies->call(state, target, next); else run_until(state, next);
    }
    void pop(unsigned bytes) {
        load(0, 1, bytes + 8); offset(1, 1, bytes);
        step(); state.wr<4>(4128, r(0));
    }
    void finish() { step(); state.pc = std::uint32_t(state.r<4>(4128)); }
};
void construct_node(LifetimeCall& a) {
    a.link(); a.save(31); a.save(30); a.move(30, 5); a.sign16(31, 30);
    a.save(29); a.move(29, 4); a.save(28); a.move(28, 3); a.save_link();
    a.value(0, 0); a.frame(80); a.store(3, 12, 0); a.store(3, 8, 0);
    a.move(3, 31); a.call(0x107735f0); a.store(28, 0, 3); a.move(4, 29);
    a.move(5, 31); a.store<2>(28, 4, 30); a.load(3, 28, 0); a.call(0x10739ef0);
    a.move(3, 28); a.pop(80); a.restore(31); a.restore(30); a.restore(29); a.restore(28); a.finish();
}
void destroy_node(LifetimeCall& a) {
    a.link(); a.save(31); a.move(31, 4); a.save(30); a.move(30, 3, true); a.save_link(); a.frame(64);
    if (!a.equal()) {
        a.load(3, 30, 0); a.call(0x10773610); a.sign16(0, 31, true);
        if (a.positive()) { a.move(3, 30); a.call(0x107735a0); }
    }
    a.at(0x38); a.move(3, 30); a.pop(64); a.restore(31); a.restore(30); a.finish();
}
void construct_list(LifetimeCall& a) {
    a.value(0, 0); a.store(3, 4, 0); a.store(3, 8, 0); a.store(3, 12, 0); a.store<2>(3, 0, 0); a.finish();
}
void destroy_list(LifetimeCall& a) {
    a.link(); a.save(31); a.save(30); a.move(30, 4); a.save(29); a.move(29, 3, true); a.save_link(); a.frame(80);
    if (!a.equal()) {
        a.jump(0x38);
        while (true) {
            a.at(0x38); a.load(3, 29, 4); a.compare_zero(3);
            if (a.equal()) break;
            a.at(0x28); a.load(31, 3, 8); a.value(4, 1); a.call(0x10738ba0); a.store(29, 4, 31);
        }
        a.sign16(0, 30, true);
        if (a.positive()) { a.move(3, 29); a.call(0x107735a0); }
    }
    a.at(0x54); a.move(3, 29); a.pop(80); a.restore(31); a.restore(30); a.restore(29); a.finish();
}
void clear_list(LifetimeCall& a) {
    a.link(); a.save(31); a.save(30); a.move(30, 3); a.save_link(); a.frame(64); a.jump(0x30);
    while (true) {
        a.at(0x30); a.load(3, 30, 4); a.compare_zero(3);
        if (a.equal()) break;
        a.at(0x20); a.load(31, 3, 8); a.value(4, 1); a.call(0x10738ba0); a.store(30, 4, 31);
    }
    a.value(0, 0); a.store<2>(30, 0, 0); a.store(30, 4, 0); a.store(30, 8, 0); a.store(30, 12, 0);
    a.pop(64); a.restore(31); a.restore(30); a.finish();
}
void construct_buffer(LifetimeCall& a) {
    a.link(); a.save(31); a.load(31, 2, -0x3e6c); a.save(30); a.load(30, 2, -0x7ba8);
    a.save(29); a.move(29, 3); a.save_link(); a.value(0, 0); a.frame(112);
    a.store(3, 4, 0); a.store(3, 8, 4); a.move(3, 4); a.call(0x107735f0);
    a.store(29, 0, 3); a.load(0, 29, 0); a.compare_zero(0);
    if (a.equal()) {
        a.offset(3, 31, 0x28); a.offset(6, 1, 64); a.offset(7, 1, 56); a.value(4, 0); a.value(5, 0);
        a.call(0x10739890); a.call(0x10768810); a.move(5, 3); a.move(3, 30); a.offset(4, 31, 0x34);
        a.offset(6, 1, 64); a.offset(7, 1, 56); a.value(8, 0x232); a.call(0x1077b720);
        a.move(4, 30); a.offset(3, 31, 0x41); a.value(5, 0); a.call(0x10769b60);
        a.load(4, 2, -0x7bac); a.value(0, 0); a.value(3, UINT32_MAX); a.store<1>(4, 0, 0); a.call(0x1076f7d0);
    }
    a.at(0xa4); a.move(3, 29); a.pop(112); a.restore(31); a.restore(30); a.restore(29); a.finish();
}
void destroy_buffer(LifetimeCall& a) {
    a.link(); a.save(31); a.move(31, 4); a.save(30); a.move(30, 3, true); a.save_link(); a.frame(64);
    if (!a.equal()) {
        a.load(3, 30, 0); a.compare_zero(3);
        if (!a.equal()) a.call(0x10773610);
        a.at(0x30); a.value(3, 0); a.sign16(0, 31, true); a.store(30, 0, 3);
        if (a.positive()) { a.move(3, 30); a.call(0x107735a0); }
    }
    a.at(0x48); a.move(3, 30); a.pop(64); a.restore(31); a.restore(30); a.finish();
}
void construct_descriptor(LifetimeCall& a) {
    a.value(0, 0); a.store(3, 0, 0); a.store(3, 4, 4); a.store(3, 8, 5); a.store(3, 12, 0); a.finish();
}
void destroy_descriptor(LifetimeCall& a) {
    a.link(); a.save(31); a.move(31, 4); a.save(30); a.move(30, 3, true); a.save_link(); a.frame(64);
    if (!a.equal()) {
        a.call(0x1074dfc0); a.sign16(0, 31, true);
        if (a.positive()) { a.move(3, 30); a.call(0x107735a0); }
    }
    a.at(0x34); a.move(3, 30); a.pop(64); a.restore(31); a.restore(30); a.finish();
}
void reset_descriptor(LifetimeCall& a) {
    a.link(); a.save(31); a.move(31, 3); a.value(5, 0); a.save_link(); a.frame(64);
    a.load(4, 3, 8); a.call(0x1074e000); a.value(0, 0); a.store(31, 12, 0);
    a.pop(64); a.restore(31); a.finish();
}
void clear_descriptor(LifetimeCall& a) {
    a.link(); a.move(6, 5); a.save_link(); a.move(0, 4); a.move(5, 0); a.frame(64);
    a.load(4, 3, 0); a.call(0x1074ea30); a.pop(64); a.finish();
}
}
unsigned entry_index(std::uint32_t entry) {
    const auto found = std::find(entries.begin(), entries.end(), entry);
    if (found == entries.end()) throw std::runtime_error("Unknown utility-object entry");
    return unsigned(found - entries.begin());
}
void invoke(State& state, unsigned index, DependencyDispatch* dependencies) {
    if (index >= entries.size()) throw std::runtime_error("Unknown utility-object index");
    LifetimeCall call(state, entries[index], dependencies);
    switch (index) {
    case 0: construct_node(call); break;
    case 1: destroy_node(call); break;
    case 2: construct_list(call); break;
    case 3: destroy_list(call); break;
    case 4: clear_list(call); break;
    case 5: construct_buffer(call); break;
    case 6: destroy_buffer(call); break;
    case 7: construct_descriptor(call); break;
    case 8: destroy_descriptor(call); break;
    case 9: reset_descriptor(call); break;
    case 10: clear_descriptor(call); break;
    }
}
}
