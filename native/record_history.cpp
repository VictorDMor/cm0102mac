#include "record_history.hpp"
#include "guest_context_transfer.hpp"
#include <algorithm>

namespace cm::aot::record_history {
namespace {
// Named history operations retain ordered guest checkpoints and live fields.
// Nodes, field ownership, cached records and callbacks remain in guest memory.
class HistoryCall {
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
    HistoryCall(State& s, std::uint32_t root, DependencyDispatch* d)
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
    template<unsigned N> void indexed_store(unsigned base, unsigned index, unsigned source) {
        step(); state.memory.write<N>((base ? r(base) : 0) + r(index), r(source));
    }
    void multiple(unsigned first, bool restore = false) {
        step(); state.wr<4>(2432, r(1) - (32 - first) * 4);
        for (unsigned n = first; n < 32; ++n) {
            if (restore) state.wr<4>(n * 4, state.memory.read<4>(state.r<4>(2432)));
            else state.memory.write<4>(state.r<4>(2432), r(n));
            state.wr<4>(2432, state.r<4>(2432) + 4);
        }
    }
    void multiply(unsigned destination, unsigned source, std::uint32_t factor) {
        step(); state.wr<4>(destination * 4, r(source) * factor);
    }
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
    bool positive() { step(); return (state.r<1>(2304) & 10) == 0; }
    bool nonpositive() { step(); return (state.r<1>(2304) & 10) != 0; }
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
void append_current_history(HistoryCall&);
void apply_history_to_groups(HistoryCall& a) {
    a.link(); a.multiple(22); a.move(23, 3); a.move(24, 4); a.move(25, 5); a.move(26, 6);
    a.move(27, 7); a.move(28, 8); a.move(29, 9); a.save_link(); a.value(0, 0); a.frame(96);
    a.load<2>(30, 3, 0x2c58, true); a.store<2>(3, 0x2c58, 0); a.jump(0x130);
    for (;;) {
        a.at(0x130); a.load<2>(4, 23, 0x2c58, true); a.compare(4, row_count); if (!a.less()) break;
        a.at(0x3c); a.scale(3, 4, 1); a.offset(0, 3, 0x2c38); a.indexed_load<2>(0, 23, 0, true); a.compare(0, 0);
        if (!a.equal()) {
            a.compare(24, 0, false); bool selected = a.equal();
            if (!selected) {
                a.multiply(3, 4, row_stride); a.offset(0, 3, 4); a.indexed_load(0, 23, 0);
                a.compare_registers(24, 0, false); selected = a.equal();
            }
            if (selected) {
                a.at(0x6c); a.move(3, 23); a.move(4, 25); a.move(5, 26); a.move(6, 27); a.move(7, 28); a.move(8, 29);
                a.call(entries[1]);
            }
            a.at(0x88); a.value(31, 1); a.value(22, record_stride); a.jump(0xec);
            for (;;) {
                a.at(0xec); a.load<2>(5, 23, 0x2c58, true); a.sign16(4, 31); a.scale(3, 5, 1);
                a.offset(0, 3, 0x2c38); a.indexed_load<2>(0, 23, 0, true); a.compare_registers(4, 0);
                if (!a.less()) break;
                a.at(0x94); a.multiply(0, 5, row_stride); a.value(5, record_stride); a.add(3, 23, 0); a.add(4, 3, 22); a.call(0x10745e00);
                a.compare(24, 0, false); selected = a.equal();
                if (!selected) {
                    a.load<2>(0, 23, 0x2c58, true); a.multiply(3, 0, row_stride); a.offset(0, 3, 4);
                    a.indexed_load(0, 23, 0); a.compare_registers(24, 0, false); selected = a.equal();
                }
                if (selected) {
                    a.at(0xc8); a.move(3, 23); a.move(4, 25); a.move(5, 26); a.move(6, 27); a.move(7, 28); a.move(8, 29);
                    a.call(entries[1]);
                }
                a.at(0xe4); a.offset(22, 22, record_stride); a.offset(31, 31, 1);
            }
            a.compare(0, 1);
            if (!a.nonpositive()) {
                a.multiply(0, 5, row_stride); a.value(5, record_stride); a.add(3, 23, 0); a.offset(4, 3, record_stride); a.call(0x10745e00);
            }
        }
        a.at(0x124); a.load<2>(3, 23, 0x2c58, true); a.offset(0, 3, 1); a.store<2>(23, 0x2c58, 0);
    }
    a.store<2>(23, 0x2c58, 30); a.value(3, 1); a.pop(96); a.multiple(22, true); a.finish();
}
void report_invalid_arguments(HistoryCall& a) {
    a.offset(3, 30, 0x78); a.offset(6, 1, 0x40); a.offset(7, 1, 0x38); a.value(4, 0); a.value(5, 0);
    a.call(0x10739890); a.call(0x10768810); a.move(5, 3); a.move(3, 29); a.offset(4, 30, 10);
    a.offset(6, 1, 0x40); a.offset(7, 1, 0x38); a.value(8, 0x4eb); a.call(0x1077b720);
    a.move(4, 29); a.offset(3, 30, 0x17); a.value(5, 0); a.call(0x10769b60);
    a.load(4, 2, -0x7bac); a.value(0, 0); a.value(3, 0); a.store<1>(4, 0, 0); a.jump(0x4c0);
}
// A matching marked identifier selects the last marked successor, or restores
// the previous current node when its identifier differs. It returns zero and
// never appends in either case; reads and temporary register effects stay live.
bool select_existing_marked(HistoryCall& a) {
    a.at(0xe8); a.add(3, 22, 4); a.load(5, 3, 16); a.compare(5, 0, false);
    bool search = !a.equal();
    if (search) { a.sign8(0, 25, true); search = !a.equal(); }
    if (!search) return false;
    a.load(6, 3, 8); a.jump(0x17c);
    for (;;) {
        a.at(0x17c); a.compare(6, 0, false); if (a.equal()) return false;
        a.at(0x108); a.load<1>(0, 6, 16); a.sign8(0, 0, true);
        bool match = !a.equal();
        if (match) { a.load(0, 6, 0); a.compare_registers(0, 23, false); match = a.equal(); }
        if (match) {
            a.jump(0x148);
            for (;;) {
                a.at(0x148); a.compare(6, 0, false); if (a.equal()) break;
                a.at(0x128); a.load<1>(0, 6, 16); a.sign8(0, 0, true);
                if (!a.equal()) {
                    a.load<2>(0, 22, 0x2c58, true); a.multiply(3, 0, row_stride); a.offset(0, 3, 16); a.indexed_store<4>(22, 0, 6);
                }
                a.at(0x144); a.load(6, 6, 0x1f8);
            }
            a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(4, 22, 0);
            a.load(3, 4, 16); a.load(0, 3, 0); a.compare_registers(0, 23, false);
            if (!a.equal()) a.store(4, 16, 5);
            a.at(0x170); a.value(3, 0); a.jump(0x4c0); return true;
        }
        a.at(0x178); a.load(6, 6, 0x1f8);
    }
}
void dispose_owned_fields(HistoryCall& a, unsigned loop, unsigned test, unsigned increment) {
    a.jump(test);
    for (;;) {
        a.at(test); a.sign16(0, 29); a.compare(0, field_count); if (!a.less()) break;
        a.at(loop); a.add(4, 31, 21); a.load(3, 4, 20); a.compare(3, 0, false);
        if (!a.equal()) { a.load<1>(0, 4, 24); a.sign8(0, 0, true); if (!a.equal()) a.call(0x10773610); }
        a.at(increment); a.offset(21, 21, 8); a.offset(29, 29, 1);
    }
}
void truncate_after_current(HistoryCall& a) {
    a.at(0x184); a.add(3, 22, 4); a.load(0, 3, 12); a.compare_registers(5, 0, false);
    if (a.equal()) return;
    a.store(3, 12, 5); a.value(0, 0); a.load<2>(3, 22, 0x2c58, true); a.multiply(3, 3, row_stride);
    a.add(3, 22, 3); a.load(4, 3, 16); a.load(3, 3, 12); a.load(31, 4, 0x1f8); a.store(3, 0x1f8, 0); a.jump(0x244);
    for (;;) {
        a.at(0x244); a.compare(31, 0, false); if (a.equal()) break;
        a.at(0x1bc); a.load<2>(0, 22, 0x2c58, true); a.value(29, 0); a.load(4, 31, 0x1f8); a.value(21, 0);
        a.multiply(3, 0, row_stride); a.offset(0, 3, 16); a.indexed_store<4>(22, 0, 4);
        dispose_owned_fields(a, 0x1e0, 0x208, 0x200);
        a.move(3, 31); a.call(0x107735a0); a.load<2>(0, 22, 0x2c58, true); a.multiply(3, 0, row_stride);
        a.offset(4, 3, 22); a.indexed_load<2>(3, 22, 4, true); a.offset(0, 3, -1); a.indexed_store<2>(22, 4, 0);
        a.load<2>(0, 22, 0x2c58, true); a.multiply(3, 0, row_stride); a.offset(0, 3, 16); a.indexed_load(31, 22, 0);
    }
    a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 22, 0); a.load(0, 3, 12); a.store(3, 16, 0);
}
bool trim_oldest(HistoryCall& a) {
    a.at(0x260); a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 22, 0);
    a.load<2>(0, 3, 22, true); a.compare(0, 100);
    if (a.nonpositive()) return false;
    a.load(31, 3, 8); a.load<1>(0, 31, 16); a.sign8(0, 0, true);
    if (!a.equal()) { a.value(3, 0); a.jump(0x4c0); return true; }
    a.load(0, 31, 0x1f8); a.value(29, 0); a.move(21, 29); a.store(3, 8, 0);
    a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 22, 0); a.load(3, 3, 8); a.store(3, 0x1f4, 29);
    dispose_owned_fields(a, 0x2b8, 0x2e0, 0x2d8);
    a.move(3, 31); a.call(0x107735a0); a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride);
    a.add(4, 22, 0); a.load<2>(3, 4, 22, true); a.offset(0, 3, -1); a.store<2>(4, 22, 0); return false;
}
void publish_new_history(HistoryCall& a) {
    a.at(0x30c); a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 22, 0); a.load(3, 3, 16); a.compare(3, 0, false);
    if (!a.equal()) { a.load(12, 3, 4); a.compare(12, 0, false); if (!a.equal()) { a.call(0x10781320); a.load(2, 1, 20); } }
    a.at(0x338); a.value(3, 0x2fc); a.call(0x10773520); a.move(31, 3); a.value(4, 0); a.value(5, 0x2fc); a.call(0x10778300);
    a.store<1>(31, 16, 25); a.offset(3, 31, 0x1fc); a.offset(4, 30, 0xb4);
    a.store(31, 0, 23); a.store(31, 4, 26); a.store(31, 12, 27); a.store(31, 8, 24); a.call(0x1077dbf0);
    a.sign8(0, 25, true);
    if (!a.equal()) {
        a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(4, 22, 0);
        a.load<2>(3, 4, 20, true); a.offset(0, 3, 1); a.store<2>(4, 20, 0);
    }
    a.at(0x390); a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 22, 0); a.load(3, 3, 12); a.compare(3, 0, false);
    if (!a.equal()) {
        a.store(3, 0x1f8, 31); a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride);
        a.add(3, 22, 0); a.load(0, 3, 12); a.store(31, 0x1f4, 0);
    }
    a.at(0x3c0); a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 22, 0); a.store(3, 12, 31);
    a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 22, 0); a.load(0, 3, 8); a.compare(0, 0, false);
    if (a.equal()) a.store(3, 8, 31);
    a.at(0x3ec); a.load<2>(3, 22, 0x2c58, true); a.value(5, 0); a.value(4, UINT32_MAX); a.sign8(0, 25, true);
    a.multiply(0, 3, row_stride); a.add(3, 22, 0); a.store(3, 16, 31);
    a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(6, 22, 0); a.load<2>(3, 6, 22, true); a.offset(0, 3, 1); a.store<2>(6, 22, 0);
    a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 22, 0); a.store<1>(3, 25, 5);
    a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 22, 0); a.store<2>(3, 28, 4);
    a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 22, 0); a.store(3, 32, 5);
    if (!a.equal()) { a.load<2>(0, 22, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 22, 0); a.store<1>(3, 26, 5); }
    a.at(0x464); a.load<2>(3, 22, 0x2c58, true); a.sign8(0, 28, true); a.value(4, 1); a.multiply(0, 3, row_stride); a.add(3, 22, 0); a.store<1>(3, 24, 4);
    if (!a.equal()) {
        a.value(23, 0); a.move(21, 23); a.jump(0x4ac);
        for (;;) {
            a.at(0x4ac); a.load<2>(0, 22, 0x2c18, true); a.sign16(3, 23); a.compare_registers(3, 0); if (!a.less()) break;
            a.at(0x48c); a.load(5, 22, 0x2c14); a.move(3, 22); a.sign16(4, 23); a.value(6, 0); a.indexed_load(5, 5, 21); a.call(0x1073f1e0);
            a.offset(21, 21, 4); a.offset(23, 23, 1);
        }
    }
    a.at(0x4bc); a.value(3, 1);
}
void append_current_history(HistoryCall& a) {
    a.link(); a.multiple(21); a.move(23, 4); a.move(22, 3); a.load(29, 2, -0x7ba8);
    a.move(24, 5); a.load(30, 2, -0x3e5c); a.move(25, 6); a.move(26, 7); a.move(27, 8); a.value(28, 0); a.save_link(); a.frame(144);
    a.load<2>(0, 3, 0x2c58, true); a.multiply(4, 0, row_stride); a.add(3, 22, 4); a.load(0, 3, 4); a.compare(0, 0, false);
    bool fallback = a.equal();
    if (fallback) { a.load(0, 3, 8); a.compare(0, 0, false); fallback = a.equal(); }
    if (fallback) { a.load(0, 22, 0x2c00); a.compare(0, 0, false); fallback = !a.equal(); }
    if (fallback) {
        a.load(24, 22, 0x2c04); a.move(23, 0); a.load<1>(25, 22, 0x2c08); a.value(28, 1);
        a.load(26, 22, 0x2c0c); a.load(27, 22, 0x2c10);
    }
    a.at(0x7c); a.compare(23, 0, false); bool valid = !a.equal();
    if (valid) { a.compare(24, 0, false); valid = !a.equal(); }
    if (!valid) { a.at(0x8c); report_invalid_arguments(a); }
    else if (!select_existing_marked(a)) {
        truncate_after_current(a);
        if (!trim_oldest(a)) publish_new_history(a);
    }
    a.at(0x4c0); a.pop(144); a.multiple(21, true); a.finish();
}
}
unsigned entry_index(std::uint32_t entry) {
    const auto found = std::find(entries.begin(), entries.end(), entry);
    if (found == entries.end()) throw std::runtime_error("Unknown record-history entry");
    return unsigned(found - entries.begin());
}
void invoke(State& state, unsigned index, DependencyDispatch* dependencies) {
    if (index >= entries.size()) throw std::runtime_error("Unknown record-history index");
    HistoryCall call(state, entries[index], dependencies);
    if (index == 0) apply_history_to_groups(call); else append_current_history(call);
}
}
