#include "record_query.hpp"
#include "guest_context_transfer.hpp"
#include <algorithm>

namespace cm::aot::record_query {
namespace {
// Named queries retain guest checkpoints, live field reloads and exact ABI.
class QueryCall {
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
    QueryCall(State& s, std::uint32_t root, DependencyDispatch* d)
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
    void add(unsigned destination, unsigned first, unsigned second) {
        step(); state.wr<4>(destination * 4, r(first) + r(second));
    }
    void sign16(unsigned destination, unsigned source, bool record = false) {
        step(); const auto value = signed_value<2>(r(source));
        state.wr<4>(destination * 4, std::uint32_t(value));
        if (record) condition(value, 0, true);
    }
    template<unsigned N = 4> void load(unsigned destination, unsigned base, std::int32_t displacement, bool sign = false) {
        step(); const auto value = state.memory.read<N>(r(base) + std::uint32_t(displacement));
        state.wr<4>(destination * 4, sign ? std::uint32_t(signed_value<N>(value)) : value);
    }
    template<unsigned N = 4> void store(unsigned base, std::int32_t displacement, unsigned source) {
        step(); state.memory.write<N>(r(base) + std::uint32_t(displacement), r(source));
    }
    template<unsigned N = 4> void indexed_load(unsigned destination, unsigned base, unsigned index, bool sign = false) {
        step(); const auto value = state.memory.read<N>((base ? r(base) : 0) + r(index));
        state.wr<4>(destination * 4, sign ? std::uint32_t(signed_value<N>(value)) : value);
    }
    void multiply(unsigned destination, unsigned source, std::uint32_t factor) {
        step(); state.wr<4>(destination * 4, r(source) * factor);
    }
    void leading_zeroes(unsigned destination, unsigned source) {
        step(); state.wr<4>(destination * 4, std::countl_zero(r(source)));
    }
    void rotate_mask(unsigned destination, unsigned source, int shift, std::uint32_t mask) {
        step(); state.wr<4>(destination * 4, std::rotl(r(source), shift) & mask);
    }
    void negate(unsigned destination, unsigned source) { step(); state.wr<4>(destination * 4, 0u - r(source)); }
    void bit_or(unsigned destination, unsigned first, unsigned second) { step(); state.wr<4>(destination * 4, r(first) | r(second)); }
    void scale(unsigned destination, unsigned source, unsigned shift) {
        step(); state.wr<4>(destination * 4, r(source) << shift);
    }
    void sign8(unsigned destination, unsigned source, bool record = false) {
        step(); const auto value = signed_value<1>(r(source));
        state.wr<4>(destination * 4, std::uint32_t(value));
        if (record) condition(value, 0, true);
    }
    void compare(unsigned source, std::int32_t value, bool sign = true) {
        step(); condition(sign ? std::bit_cast<std::int32_t>(r(source)) : std::int64_t(r(source)), value);
    }
    void compare_registers(unsigned first, unsigned second, bool sign = true) {
        step(); condition(sign ? std::bit_cast<std::int32_t>(r(first)) : std::int64_t(r(first)),
            sign ? std::bit_cast<std::int32_t>(r(second)) : std::int64_t(r(second)));
    }
    bool equal() { step(); return (state.r<1>(2304) & 2) != 0; }
    bool less() { step(); return (state.r<1>(2304) & 8) != 0; }
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
void publish_arguments(QueryCall& a) {
    a.store(3, 0x2c00, 4); a.store(3, 0x2c04, 5); a.store<1>(3, 0x2c08, 6);
    a.store(3, 0x2c0c, 7); a.store(3, 0x2c10, 8); a.store(3, 0x2c14, 9);
    a.store<2>(3, 0x2c18, 10); a.finish();
}
void count_unmarked(QueryCall& a) {
    a.value(9, 0); a.value(8, 0); a.value(6, 0); a.jump(0x40);
    for (;;) {
        a.at(0x40); a.load<2>(7, 3, 0x2c58, true); a.sign16(5, 8); a.scale(4, 7, 1);
        a.offset(0, 4, 0x2c38); a.indexed_load<2>(0, 3, 0, true); a.compare_registers(5, 0);
        if (!a.less()) break;
        a.at(0x10); a.multiply(0, 7, row_stride); a.add(4, 0, 6); a.add(4, 3, 4);
        a.load<1>(0, 4, 25); a.sign8(0, 0, true);
        if (a.equal()) { a.load<1>(0, 4, 26); a.sign8(0, 0, true); if (a.equal()) a.offset(9, 9, 1); }
        a.at(0x38); a.offset(6, 6, record_stride); a.offset(8, 8, 1);
    }
    a.move(3, 9); a.finish();
}
void report_missing_current(QueryCall& a, unsigned line) {
    a.offset(3, 31, 0x78); a.offset(6, 1, 0x40); a.offset(7, 1, 0x38); a.value(4, 0); a.value(5, 0);
    a.call(0x10739890); a.call(0x10768810); a.move(5, 3); a.move(3, 30); a.offset(4, 31, 10);
    a.offset(6, 1, 0x40); a.offset(7, 1, 0x38); a.value(8, line); a.call(0x1077b720);
    a.move(4, 30); a.offset(3, 31, 0x17); a.value(5, 0); a.call(0x10769b60);
    a.load(4, 2, -0x7bac); a.value(0, 0); a.value(3, UINT32_MAX); a.store<1>(4, 0, 0); a.call(0x1076f7d0);
}
void begin_current_query(QueryCall& a, unsigned line, bool with_option = false) {
    a.link(); a.save(31); a.load(31, 2, -0x3e5c); a.save(30); a.load(30, 2, -0x7ba8); a.save(29);
    a.move(29, with_option ? 4 : 3);
    if (with_option) { a.save(28); a.move(28, 3); }
    a.save_link(); a.frame(112); a.load<2>(0, 3, 0x2c58, true); a.multiply(0, 0, row_stride);
    a.add(3, with_option ? 28 : 29, 0); a.load(0, 3, 16); a.compare(0, 0, false);
    if (a.equal()) report_missing_current(a, line);
    a.at(with_option ? 0xa0 : 0x98);
}
void finish_current_query(QueryCall& a, bool with_option = false) {
    a.pop(112); a.restore(31); a.restore(30); a.restore(29);
    if (with_option) a.restore(28);
    a.finish();
}
void current_is_last(QueryCall& a) {
    begin_current_query(a, 0x5cc); a.load<2>(0, 29, 0x2c58, true); a.multiply(0, 0, row_stride);
    a.add(3, 29, 0); a.load(3, 3, 16); a.load(0, 3, 0x1f8); a.leading_zeroes(0, 0);
    a.rotate_mask(0, 0, 27, 0x07ffffff); a.sign8(3, 0); finish_current_query(a);
}
void current_is_first(QueryCall& a) {
    begin_current_query(a, 0x5df, true); a.load<2>(0, 28, 0x2c58, true); a.value(4, 0); a.multiply(0, 0, row_stride);
    a.add(3, 28, 0); a.load(3, 3, 16); a.load(0, 3, 0x1f4); a.compare(0, 0, false);
    bool first = a.equal();
    if (!first) {
        a.sign8(0, 29, true);
        if (a.equal()) { a.load<1>(0, 3, 16); a.sign8(0, 0, true); first = !a.equal(); }
    }
    if (first) { a.at(0xd4); a.value(4, 1); }
    a.at(0xd8); a.sign8(3, 4); finish_current_query(a, true);
}
void current_mark(QueryCall& a) {
    begin_current_query(a, 0x5f3); a.load<2>(0, 29, 0x2c58, true); a.multiply(0, 0, row_stride);
    a.add(3, 29, 0); a.load(3, 3, 16); a.load<1>(3, 3, 16); finish_current_query(a);
}
void current_has_payload(QueryCall& a) {
    begin_current_query(a, 0x606); a.load<2>(0, 29, 0x2c58, true); a.multiply(0, 0, row_stride);
    a.add(3, 29, 0); a.load(3, 3, 16); a.load(3, 3, 12); a.negate(0, 3); a.bit_or(0, 0, 3);
    a.rotate_mask(0, 0, 1, 1); a.sign8(3, 0); finish_current_query(a);
}
}
unsigned entry_index(std::uint32_t entry) {
    const auto found = std::find(entries.begin(), entries.end(), entry);
    if (found == entries.end()) throw std::runtime_error("Unknown record-query entry");
    return unsigned(found - entries.begin());
}
void invoke(State& state, unsigned index, DependencyDispatch* dependencies) {
    if (index >= entries.size()) throw std::runtime_error("Unknown record-query index");
    QueryCall call(state, entries[index], dependencies);
    switch (index) {
    case 0: publish_arguments(call); break;
    case 1: count_unmarked(call); break;
    case 2: current_is_last(call); break;
    case 3: current_is_first(call); break;
    case 4: current_mark(call); break;
    case 5: current_has_payload(call); break;
    }
}
}
