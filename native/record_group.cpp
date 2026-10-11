#include "record_group.hpp"
#include "guest_context_transfer.hpp"
#include <algorithm>

namespace cm::aot::record_group {
namespace {
// Named record operations retain guest checkpoints for ABI, ordered faults
// and budget prefixes. All record links and callback fields remain live.
class RecordCall {
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
    RecordCall(State& s, std::uint32_t root, DependencyDispatch* d)
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
    void store_half_update(unsigned base, std::int32_t displacement, unsigned source) {
        step(); const auto address = r(base) + std::uint32_t(displacement);
        state.memory.write<2>(address, r(source)); state.wr<4>(base * 4, address);
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
void current_label(RecordCall& a) {
    a.load<2>(0, 3, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 3, 0); a.load(3, 3, 0x10); a.compare(3, 0, false);
    if (!a.equal()) { a.at(0x24); a.offset(3, 3, 0x1fc); a.finish(); }
    else { a.load(3, 2, -0x3e5c); a.offset(3, 3, 0xb6); a.finish(); }
}
void set_current_label(RecordCall& a) {
    a.link(); a.save_link(); a.frame(64); a.load<2>(0, 3, 0x2c58, true); a.multiply(0, 0, row_stride);
    a.add(3, 3, 0); a.load(3, 3, 0x10); a.compare(3, 0, false);
    if (!a.equal()) {
        a.compare(4, 0, false);
        if (!a.equal()) { a.offset(3, 3, 0x1fc); a.call(0x1077dbf0); a.jump(0x40); }
        else { a.at(0x38); a.value(0, 0); a.store<1>(3, 0x1fc, 0); }
    }
    a.at(0x40); a.pop(64); a.finish();
}
void release_current_chain(RecordCall& a) {
    a.link(); a.save(31); a.move(31, 3); a.save(30); a.save(29); a.save(28); a.save_link(); a.frame(80);
    a.load<2>(0, 3, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 31, 0); a.load(28, 3, 8); a.jump(0xbc);
    for (;;) {
        a.at(0xbc); a.compare(28, 0, false); if (a.equal()) break;
        a.at(0x34); a.load<2>(0, 31, 0x2c58, true); a.value(29, 0); a.load(4, 28, 0x1f8); a.value(30, 0);
        a.multiply(3, 0, row_stride); a.offset(0, 3, 0x10); a.indexed_store<4>(31, 0, 4); a.jump(0x80);
        for (;;) {
            a.at(0x80); a.sign16(0, 29); a.compare(0, field_count); if (!a.less()) break;
            a.at(0x58); a.add(4, 28, 30); a.load(3, 4, 0x14); a.compare(3, 0, false);
            if (!a.equal()) {
                a.load<1>(0, 4, 0x18); a.sign8(0, 0, true);
                if (!a.equal()) a.call(0x10773610);
            }
            a.at(0x78); a.offset(30, 30, 8); a.offset(29, 29, 1);
        }
        a.move(3, 28); a.call(0x107735a0); a.load<2>(0, 31, 0x2c58, true); a.multiply(3, 0, row_stride);
        a.offset(4, 3, 0x16); a.indexed_load<2>(3, 31, 4); a.offset(0, 3, -1); a.indexed_store<2>(31, 4, 0);
        a.load<2>(0, 31, 0x2c58, true); a.multiply(3, 0, row_stride); a.offset(0, 3, 0x10); a.indexed_load(28, 31, 0);
    }
    a.load<2>(0, 31, 0x2c58, true); a.value(4, 0); a.multiply(0, 0, row_stride); a.add(3, 31, 0); a.store(3, 8, 4);
    for (auto field : {12, 16, 22, 20}) {
        a.load<2>(0, 31, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 31, 0);
        if (field >= 20) a.store<2>(3, field, 4); else a.store(3, field, 4);
    }
    a.pop(80); a.restore(31); a.restore(30); a.restore(29); a.restore(28); a.finish();
}
// Original reporting dependencies, guest temporary slots, diagnostic line and
// caller register effects remain observable on invalid record/field paths.
void report_field_error(RecordCall& a, int first_slot, int second_slot, unsigned line) {
    a.offset(3, 31, 0x78); a.offset(6, 1, first_slot); a.offset(7, 1, second_slot); a.value(4, 0); a.value(5, 0);
    a.call(0x10739890); a.call(0x10768810); a.move(5, 3); a.move(3, 30); a.offset(4, 31, 10);
    a.offset(6, 1, first_slot); a.offset(7, 1, second_slot); a.value(8, line); a.call(0x1077b720);
    a.move(4, 30); a.offset(3, 31, 0x17); a.value(5, 0); a.call(0x10769b60);
}
void get_current_field(RecordCall& a) {
    a.link(); a.multiple(27); a.move(27, 3); a.load(29, 2, -0x7bac); a.load(30, 2, -0x7ba8);
    a.move(28, 4); a.load(31, 2, -0x3e5c); a.save_link(); a.frame(160);
    a.load<2>(0, 3, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 27, 0); a.load(0, 3, 0x10); a.compare(0, 0, false);
    if (a.equal()) {
        report_field_error(a, 0x68, 0x40, 0x687); a.value(0, 0); a.value(3, UINT32_MAX); a.store<1>(29, 0, 0); a.call(0x1076f7d0);
    }
    a.at(0x94); a.compare(28, 0);
    bool invalid = a.less();
    if (!invalid) { a.compare(28, 59); invalid = !a.nonpositive(); }
    if (invalid) {
        a.at(0xa4); report_field_error(a, 0x48, 0x38, 0x68d); a.value(0, 0); a.value(3, 0); a.store<1>(29, 0, 0); a.jump(0x118);
    } else {
        a.at(0xfc); a.load<2>(3, 27, 0x2c58, true); a.scale(0, 28, 3); a.multiply(3, 3, row_stride);
        a.add(3, 27, 3); a.load(3, 3, 0x10); a.add(3, 3, 0); a.load(3, 3, 0x14);
    }
    a.at(0x118); a.pop(160); a.multiple(27, true); a.finish();
}
void set_fields_in_groups(RecordCall& a) {
    a.link(); a.multiple(24); a.move(26, 3); a.move(27, 4); a.move(28, 5); a.move(29, 6); a.move(30, 7);
    a.save_link(); a.value(0, 0); a.frame(96); a.load<2>(31, 3, 0x2c58, true); a.store<2>(3, 0x2c58, 0); a.jump(0x118);
    for (;;) {
        a.at(0x118); a.load<2>(4, 26, 0x2c58, true); a.compare(4, row_count); if (!a.less()) break;
        a.at(0x34); a.scale(3, 4, 1); a.offset(0, 3, 0x2c38); a.indexed_load<2>(0, 26, 0, true); a.compare(0, 0);
        if (!a.equal()) {
            a.compare(27, 0, false); bool selected = a.equal();
            if (!selected) {
                a.multiply(3, 4, row_stride); a.offset(0, 3, 4); a.indexed_load(0, 26, 0); a.compare_registers(27, 0, false); selected = a.equal();
            }
            if (selected) {
                a.at(0x64); a.move(3, 26); a.move(4, 28); a.move(5, 29); a.move(6, 30); a.call(0x1073f1e0);
            }
            a.at(0x78); a.value(24, 1); a.value(25, record_stride); a.jump(0xd4);
            for (;;) {
                a.at(0xd4); a.load<2>(5, 26, 0x2c58, true); a.sign16(4, 24); a.scale(3, 5, 1); a.offset(0, 3, 0x2c38);
                a.indexed_load<2>(0, 26, 0, true); a.compare_registers(4, 0); if (!a.less()) break;
                a.at(0x84); a.multiply(0, 5, row_stride); a.value(5, record_stride); a.add(3, 26, 0); a.add(4, 3, 25); a.call(0x10745e00);
                a.compare(27, 0, false); selected = a.equal();
                if (!selected) {
                    a.load<2>(0, 26, 0x2c58, true); a.multiply(3, 0, row_stride); a.offset(0, 3, 4);
                    a.indexed_load(0, 26, 0); a.compare_registers(27, 0, false); selected = a.equal();
                }
                if (selected) { a.at(0xb8); a.move(3, 26); a.move(4, 28); a.move(5, 29); a.move(6, 30); a.call(0x1073f1e0); }
                a.at(0xcc); a.offset(25, 25, record_stride); a.offset(24, 24, 1);
            }
            a.compare(0, 1);
            if (!a.nonpositive()) {
                a.multiply(0, 5, row_stride); a.value(5, record_stride); a.add(3, 26, 0); a.offset(4, 3, record_stride); a.call(0x10745e00);
            }
        }
        a.at(0x10c); a.load<2>(3, 26, 0x2c58, true); a.offset(0, 3, 1); a.store<2>(26, 0x2c58, 0);
    }
    a.store<2>(26, 0x2c58, 31); a.pop(96); a.multiple(24, true); a.finish();
}
void set_current_field(RecordCall& a) {
    a.link(); a.multiple(25); a.move(25, 3); a.load(29, 2, -0x7bac); a.load(30, 2, -0x7ba8);
    a.move(28, 4); a.load(31, 2, -0x3e5c); a.move(26, 5); a.move(27, 6); a.save_link(); a.frame(208);
    a.load<2>(0, 3, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 25, 0); a.load(0, 3, 0x10); a.compare(0, 0, false);
    if (a.equal()) { report_field_error(a, 0x90, 0x48, 0x6d8); a.value(0, 0); a.value(3, UINT32_MAX); a.store<1>(29, 0, 0); a.call(0x1076f7d0); }
    a.at(0x9c); a.compare(28, 0); bool invalid = a.less();
    if (!invalid) { a.compare(28, 59); invalid = !a.nonpositive(); }
    if (invalid) {
        a.at(0xac); report_field_error(a, 0x70, 0x40, 0x6de); a.value(0, 0); a.store<1>(29, 0, 0); a.jump(0x258);
    } else {
        a.at(0x100); a.load<2>(0, 25, 0x2c58, true); a.scale(28, 28, 3); a.multiply(0, 0, row_stride);
        a.add(3, 25, 0); a.load(0, 3, 0x10); a.add(3, 0, 28); a.load<1>(0, 3, 0x18); a.sign8(0, 0, true);
        if (!a.equal()) {
            a.load(3, 3, 0x14); a.call(0x10773610); a.load<2>(0, 25, 0x2c58, true); a.value(4, 0);
            a.multiply(0, 0, row_stride); a.add(3, 25, 0); a.load(0, 3, 0x10); a.add(3, 0, 28); a.store<1>(3, 0x18, 4);
        }
        a.at(0x148); a.compare(26, 0, false); bool copy = !a.equal();
        if (copy) { a.sign8(0, 27, true); copy = !a.equal(); }
        if (copy) {
            a.move(3, 26); a.call(0x1077dbd0); a.offset(27, 3, 1); a.move(3, 27); a.call(0x107735f0);
            a.load<2>(0, 25, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(4, 25, 0); a.load(0, 4, 0x10); a.add(4, 0, 28); a.store(4, 0x14, 3);
            a.load<2>(0, 25, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 25, 0); a.load(0, 3, 0x10); a.add(3, 0, 28); a.load(3, 3, 0x14); a.compare(3, 0, false);
            if (a.equal()) {
                report_field_error(a, 0x50, 0x38, 0x6f5); a.value(0, 0); a.store<1>(29, 0, 0); a.jump(0x258);
            } else {
                a.at(0x1f8); a.move(4, 26); a.move(5, 27); a.call(0x10739ef0); a.load<2>(0, 25, 0x2c58, true); a.value(4, 1);
                a.multiply(0, 0, row_stride); a.add(3, 25, 0); a.load(0, 3, 0x10); a.add(3, 0, 28); a.store<1>(3, 0x18, 4); a.jump(0x258);
            }
        } else {
            a.at(0x224); a.load<2>(0, 25, 0x2c58, true); a.value(4, 0); a.multiply(0, 0, row_stride);
            a.add(3, 25, 0); a.load(0, 3, 0x10); a.add(3, 0, 28); a.store(3, 0x14, 26);
            a.load<2>(0, 25, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(3, 25, 0); a.load(0, 3, 0x10); a.add(3, 0, 28); a.store<1>(3, 0x18, 4);
        }
    }
    a.at(0x258); a.pop(208); a.multiple(25, true); a.finish();
}
void insert_group_record(RecordCall& a) {
    a.link(); a.multiple(26); a.load(31, 2, -0x7bb0); a.move(29, 3); a.save_link(); a.frame(80);
    a.load(0, 31, 0x2c64); a.compare(0, 0, false); bool blocked = false;
    if (!a.equal()) { a.load<1>(0, 31, 0x2c5e); a.sign8(0, 0, true); blocked = a.equal(); }
    if (!blocked) {
        a.at(0x30); a.move(3, 31); a.sign16(4, 29); a.value(30, 0); a.value(5, 0); a.jump(0xd0);
        for (;;) {
            a.at(0xd0); a.sign16(0, 30); a.compare(0, row_count); if (!a.less()) break;
            a.at(0x48); a.offset(0, 5, 0x2c38); a.indexed_load<2>(0, 31, 0, true); a.compare(0, 0);
            if (a.equal()) { a.at(0xdc); break; }
            a.load<2>(0, 3, 0, true); a.compare_registers(4, 0);
            if (a.equal()) {
                a.sign16(4, 30); a.scale(0, 4, 1); a.add(3, 31, 0); a.load<2>(3, 3, 0x2c38, true); a.compare(3, row_count);
                if (!a.less()) { blocked = true; a.at(0x1b8); break; }
                a.multiply(0, 4, row_stride); a.move(26, 3); a.multiply(28, 3, record_stride); a.add(27, 31, 0); a.jump(0xb4);
                for (;;) {
                    a.at(0xb4); a.sign16(0, 26, true); if (!a.positive()) break;
                    a.at(0x90); a.sign16(4, 26); a.add(3, 27, 28); a.offset(0, 4, -1); a.value(5, record_stride);
                    a.multiply(0, 0, record_stride); a.add(4, 27, 0); a.call(0x10745e00); a.offset(28, 28, -record_stride); a.offset(26, 26, -1);
                }
                a.jump(0xdc); break;
            }
            a.at(0xc4); a.offset(5, 5, 2); a.offset(3, 3, row_stride); a.offset(30, 30, 1);
        }
        if (!blocked) {
            a.at(0xdc); a.sign16(6, 30); a.compare(6, row_count);
            if (a.less()) {
                a.scale(0, 6, 1); a.value(4, 0); a.add(5, 31, 0); a.sign16(0, 29, true); a.load<2>(3, 5, 0x2c38, true);
                a.multiply(6, 6, row_stride); a.value(0, 1); a.offset(3, 3, 1); a.store<2>(5, 0x2c38, 3); a.add(3, 31, 6); a.offset(7, 3, 24);
                a.store<2>(3, 0, 29);
                for (auto field : {4, 8, 12, 16}) a.store(3, field, 4);
                a.store<2>(3, 20, 4); a.store<2>(3, 22, 4); a.store<1>(3, 24, 0);
                if (a.equal()) { a.offset(4, 31, 0x2c68); a.jump(0x148); }
                else { a.at(0x140); a.offset(4, 31, 0x130000); a.offset(4, 4, -0x2950); }
                a.at(0x148); a.add(6, 31, 6); a.value(5, 0); a.store(6, 36, 4); a.value(0, UINT32_MAX);
                a.offset(27, 31, 0x2c58); a.offset(4, 31, 0x260000); a.store<1>(6, 25, 5); a.offset(8, 6, 25);
                a.store<2>(6, 28, 0); a.store(6, 32, 5); a.store(6, 40, 0);
                a.load<2>(28, 31, 0x2c58, true); a.store<2>(31, 0x2c58, 30); a.load(12, 4, -0x7e50); a.compare(12, 0, false);
                if (!a.equal()) {
                    a.move(3, 29); a.call(0x10781320); a.load(2, 1, 20); a.store<2>(31, 0x2c5a, 30); a.jump(0x1b4);
                } else {
                    a.at(0x19c); a.store<1>(7, 0, 5); a.value(0, 1); a.offset(4, 3, record_stride); a.value(5, record_stride);
                    a.store<1>(8, 0, 0); a.call(0x10745e00);
                }
                a.at(0x1b4); a.store<2>(27, 0, 28);
            }
        }
    }
    a.at(0x1b8); a.pop(80); a.multiple(26, true); a.finish();
}
void remove_groups_of_type(RecordCall& a) {
    a.link(); a.multiple(24); a.load(29, 2, -0x7bb0); a.save_link(); a.frame(96);
    a.load<1>(0, 29, 0x2c5e); a.sign8(0, 0, true);
    if (!a.equal()) {
        a.value(28, 0); a.sign16(30, 3); a.move(27, 28); a.value(24, 0); a.move(31, 28); a.jump(0x7c);
        for (;;) {
            a.at(0x7c); a.sign16(0, 24); a.compare(0, row_count); if (!a.less()) break;
            a.at(0x38); a.add(26, 29, 28); a.load<2>(0, 26, 0, true); a.compare_registers(30, 0);
            if (a.equal()) {
                a.offset(25, 27, 0x2c38); a.jump(0x60);
                for (;;) {
                    a.at(0x60); a.indexed_load<2>(0, 29, 25, true); a.compare(0, 0); if (!a.positive()) break;
                    a.at(0x50); a.load(4, 26, 4); a.move(3, 29); a.move(5, 24); a.call(0x1073f900);
                }
                a.indexed_store<2>(29, 25, 31);
            }
            a.at(0x70); a.offset(28, 28, row_stride); a.offset(27, 27, 2); a.offset(24, 24, 1);
        }
    }
    a.at(0x88); a.pop(96); a.multiple(24, true); a.finish();
}
void swap_record_identifiers(RecordCall& a) {
    a.value(11, UINT32_MAX); a.value(10, UINT32_MAX); a.value(12, 0); a.value(8, 0); a.jump(0x48);
    for (;;) {
        a.at(0x48); a.load<2>(9, 3, 0x2c58, true); a.sign16(7, 12); a.scale(6, 9, 1); a.offset(0, 6, 0x2c38);
        a.indexed_load<2>(0, 3, 0, true); a.compare_registers(7, 0); if (!a.less()) break;
        a.at(0x18); a.multiply(0, 9, row_stride); a.add(6, 0, 8); a.offset(0, 6, 4); a.indexed_load(0, 3, 0);
        a.compare_registers(0, 4, false); if (a.equal()) a.move(10, 12);
        a.at(0x34); a.compare_registers(0, 5, false); if (a.equal()) a.move(11, 12);
        a.at(0x40); a.offset(8, 8, record_stride); a.offset(12, 12, 1);
    }
    a.sign16(4, 10); a.compare(4, -1); bool found = !a.nonpositive();
    if (found) { a.sign16(5, 11); a.compare(5, -1); found = !a.nonpositive(); }
    if (found) {
        a.multiply(0, 9, row_stride); a.add(6, 3, 0); a.multiply(4, 4, record_stride); a.offset(6, 6, 4); a.multiply(5, 5, record_stride);
        a.indexed_load(7, 6, 4); a.indexed_load(0, 6, 5); a.indexed_store<4>(6, 4, 0);
        a.load<2>(0, 3, 0x2c58, true); a.multiply(0, 0, row_stride); a.add(0, 3, 0); a.value(3, 1); a.add(4, 0, 5); a.store(4, 4, 7); a.finish();
    } else { a.at(0xb8); a.value(3, 0); a.finish(); }
}
void insert_with_callbacks(RecordCall& a) {
    a.link(); a.save(31); a.move(31, 6, true); a.save(30); a.move(30, 4); a.save(29); a.move(29, 3); a.save_link(); a.frame(80);
    if (!a.equal()) {
        a.offset(3, 29, 0x260000); a.value(0, 0); a.store(3, -0x7e4c, 5); a.store(3, -0x7e50, 0);
        a.load<2>(0, 29, 0x2c58, true); a.multiply(0, 0, row_stride); a.indexed_load<2>(3, 29, 0); a.call(0x1073f530);
        a.load<2>(0, 29, 0x2c58, true); a.offset(3, 29, 0x260000); a.multiply(0, 0, row_stride);
        a.add(4, 29, 0); a.store(4, 0x30, 31); a.store(3, -0x7e50, 30); a.jump(0x80);
    } else {
        a.at(0x64); a.offset(3, 29, 0x260000); a.store(3, -0x7e50, 30); a.store(3, -0x7e4c, 5);
        a.load<2>(0, 29, 0x2c58, true); a.multiply(0, 0, row_stride); a.indexed_load<2>(3, 29, 0); a.call(0x1073f530);
    }
    a.at(0x80); a.pop(80); a.restore(31); a.restore(30); a.restore(29); a.finish();
}
void remove_group_record(RecordCall& a) {
    a.link(); a.multiple(21); a.load(29, 2, -0x7bb0); a.move(23, 3); a.load(30, 2, -0x7ba8); a.load(31, 2, -0x3e5c);
    a.save_link(); a.sign16(0, 5); a.compare(0, -1); a.frame(144);
    bool found = true;
    if (!a.equal()) { a.move(25, 5); a.value(26, 0); a.jump(0xa0); }
    else {
        a.at(0x38); a.value(6, 0); a.value(25, 0); a.move(7, 6); a.jump(0x90);
        for (;;) {
            a.at(0x90); a.sign16(0, 25); a.compare(0, row_count); if (!a.less()) { a.jump(0x304); found = false; break; }
            a.at(0x48); a.offset(0, 6, 0x2c38); a.indexed_load<2>(3, 23, 0, true); a.offset(0, 3, -1);
            a.sign16(26, 0); a.multiply(5, 26, record_stride); a.jump(0x7c);
            bool matched = false;
            for (;;) {
                a.at(0x7c); a.sign16(0, 26, true); if (a.less()) break;
                a.at(0x60); a.add(3, 7, 5); a.offset(0, 3, 4); a.indexed_load(0, 23, 0); a.compare_registers(0, 4, false);
                if (a.equal()) { a.at(0xa0); matched = true; break; }
                a.offset(5, 5, -record_stride); a.offset(26, 26, -1);
            }
            if (matched) break;
            a.offset(6, 6, 2); a.offset(7, 7, row_stride); a.offset(25, 25, 1);
        }
    }
    if (found) {
        a.at(0xa0); a.load<2>(3, 23, 0x2c58, true); a.sign16(0, 25); a.compare_registers(3, 0);
        if (a.equal()) { a.value(0, 0); a.store<2>(23, 0x2c58, 0); }
        a.at(0xb8); a.offset(28, 29, 0x2c58); a.sign16(0, 26, true); a.load<2>(24, 29, 0x2c58, true); a.store<2>(29, 0x2c58, 25);
        if (a.equal()) {
            a.sign16(0, 25); a.multiply(0, 0, row_stride); a.add(3, 23, 0); a.load(3, 3, 0x10); a.compare(3, 0, false);
            if (!a.equal()) { a.load(12, 3, 4); a.compare(12, 0, false); if (!a.equal()) { a.call(0x10781320); a.load(2, 1, 20); } }
        }
        a.at(0xf8); a.sign16(0, 26, true);
        if (!a.equal()) {
            a.sign16(3, 25); a.sign16(0, 26); a.multiply(3, 3, row_stride); a.value(5, record_stride);
            a.multiply(0, 0, record_stride); a.add(4, 23, 3); a.add(3, 4, 0); a.call(0x10745e00);
        }
        a.at(0x120); a.move(3, 29); a.call(0x1073ee30); a.sign16(22, 25); a.multiply(27, 22, row_stride);
        a.move(4, 22); a.add(3, 23, 27); a.load(3, 3, 36); a.call(0x10718f30);
        a.offset(3, 29, 0x260000); a.load(12, 3, -0x7e4c); a.compare(12, 0, false);
        if (!a.equal()) { a.indexed_load<2>(3, 23, 27); a.call(0x10781320); a.load(2, 1, 20); }
        a.at(0x15c); a.scale(26, 22, 1); a.store<2>(28, 0, 24); a.add(22, 29, 27); a.value(21, 1);
        a.offset(24, 26, 0x2c38); a.value(28, record_stride); a.jump(0x19c);
        for (;;) {
            a.at(0x19c); a.indexed_load<2>(0, 29, 24, true); a.sign16(3, 21); a.compare_registers(3, 0); if (!a.less()) break;
            a.at(0x178); a.sign16(3, 21); a.add(4, 22, 28); a.offset(0, 3, -1); a.value(5, record_stride);
            a.multiply(0, 0, record_stride); a.add(3, 22, 0); a.call(0x10739ef0); a.offset(28, 28, record_stride); a.offset(21, 21, 1);
        }
        a.offset(0, 3, -1); a.add(28, 29, 27); a.multiply(0, 0, record_stride); a.value(4, 0); a.value(5, record_stride); a.add(3, 28, 0); a.call(0x10778300);
        a.add(8, 29, 26); a.load<2>(3, 8, 0x2c38, true); a.offset(3, 3, -1); a.sign16(0, 3, true); a.store_half_update(8, 0x2c38, 3);
        if (!a.equal()) { a.value(0, 1); a.store<1>(28, 24, 0); a.jump(0x304); }
        else {
            a.at(0x1ec); a.sign16(0, 25, true);
            if (a.equal()) {
                a.load<2>(3, 8, 0, true); a.value(7, 0); a.value(5, 1); a.offset(4, 29, 0x2c68); a.offset(0, 3, 1);
                a.move(3, 23); a.store<2>(8, 0, 0); a.value(0, UINT32_MAX); a.store<2>(28, 0, 7);
                for (auto field : {4, 8, 12, 16}) a.store(28, field, 7);
                a.store<2>(28, 20, 7); a.store<2>(28, 22, 7); a.store<1>(28, 24, 5); a.store(28, 36, 4);
                a.store<1>(28, 25, 7); a.store<2>(28, 28, 0); a.store(28, 32, 7); a.store(28, 40, 0);
                a.load(4, 23, 0x2c00); a.load(5, 23, 0x2c04); a.load<1>(6, 23, 0x2c08); a.load(7, 23, 0x2c0c); a.load(8, 23, 0x2c10);
                a.call(0x1073e4b0); a.jump(0x304);
            } else {
                a.at(0x264); a.value(0, 0); a.store(23, 0x2c30, 0); a.load(4, 23, 0x2c30); a.load(0, 23, 0x2c34);
                a.offset(3, 4, 1); a.compare_registers(3, 0, false);
                if (!a.nonpositive()) {
                    a.move(3, 31); a.offset(6, 1, 0x40); a.offset(7, 1, 0x38); a.value(4, 0); a.value(5, 0); a.call(0x10739890); a.call(0x10768810);
                    a.move(5, 3); a.move(3, 30); a.offset(4, 31, 10); a.offset(6, 1, 0x40); a.offset(7, 1, 0x38); a.value(8, 0x85); a.call(0x1077b720);
                    a.move(4, 30); a.offset(3, 31, 0x17); a.value(5, 0); a.call(0x10769b60); a.load(3, 2, -0x7bac); a.value(0, 0); a.store<1>(3, 0, 0); a.jump(0x2f0);
                } else {
                    a.at(0x2d8); a.load(3, 23, 0x2c2c); a.value(0, 4); a.indexed_store<1>(3, 4, 0);
                    a.load(3, 23, 0x2c30); a.offset(0, 3, 1); a.store(23, 0x2c30, 0);
                }
                a.at(0x2f0); a.load(3, 23, 0x2c64); a.move(6, 25); a.load(4, 23, 0x2c2c); a.load(5, 23, 0x2c30); a.call(0x1073a4c0);
            }
        }
    }
    a.at(0x304); a.pop(144); a.multiple(21, true); a.finish();
}
}
unsigned entry_index(std::uint32_t entry) {
    const auto found = std::find(entries.begin(), entries.end(), entry);
    if (found == entries.end()) throw std::runtime_error("Unknown record-group entry");
    return unsigned(found - entries.begin());
}
void invoke(State& state, unsigned index, DependencyDispatch* dependencies) {
    if (index >= entries.size()) throw std::runtime_error("Unknown record-group index");
    RecordCall call(state, entries[index], dependencies);
    switch (index) {
    case 0: current_label(call); break;
    case 1: set_current_label(call); break;
    case 2: release_current_chain(call); break;
    case 3: get_current_field(call); break;
    case 4: set_fields_in_groups(call); break;
    case 5: set_current_field(call); break;
    case 6: insert_group_record(call); break;
    case 7: remove_groups_of_type(call); break;
    case 8: swap_record_identifiers(call); break;
    case 9: insert_with_callbacks(call); break;
    case 10: remove_group_record(call); break;
    }
}
}
