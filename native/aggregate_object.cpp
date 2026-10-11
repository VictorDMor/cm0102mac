#include "aggregate_object.hpp"
#include "guest_context_transfer.hpp"
#include <algorithm>

namespace cm::aot::aggregate_object {
namespace {
// Checkpoints retain the guest ABI and ordered fault/budget prefixes around
// the named field initialization, record iteration and lifetime operations.
class AggregateCall {
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
    AggregateCall(State& s, std::uint32_t root, DependencyDispatch* d)
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
    void compare(unsigned source, std::int32_t value, bool sign = true) {
        step(); condition(sign ? std::bit_cast<std::int32_t>(r(source)) : std::int64_t(r(source)), value);
    }
    void compare_registers(unsigned first, unsigned second) {
        step(); condition(std::bit_cast<std::int32_t>(r(first)), std::bit_cast<std::int32_t>(r(second)));
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
void construct_table(AggregateCall& a) {
    a.offset(6, 3, 0x130000); a.value(7, 0); a.store(6, -0x56d8, 7);
    a.value(5, 1); a.value(0, UINT32_MAX); a.store(6, -0x56dc, 7);
    for (auto field : {-0x56f8, -0x56bc, -0x56c2, -0x56c8, -0x56c0}) a.store<1>(6, field, 7);
    a.store<1>(6, -0x56c1, 4); a.store<1>(3, 0, 7); a.store<1>(6, -0x56ba, 5); a.store(6, -0x56d4, 7);
    for (auto field : {-0x625c, -0x625a, -0x56c6, -0x56c4}) a.store<2>(6, field, 7);
    a.store<2>(6, -0x56fe, 0); a.store<1>(6, -0x56ff, 7);
    a.store<2>(6, -0x56fc, 0); a.store<2>(6, -0x56fa, 0); a.finish();
}
void destroy_table(AggregateCall& a) {
    a.link(); a.multiple(25); a.move(28, 3, true); a.load(30, 2, -0x5bf0); a.load(31, 2, -0x5bec);
    a.move(29, 4); a.save_link(); a.frame(96);
    // The two TOC accesses above occur even for a null table.
    if (!a.equal()) {
        a.offset(27, 28, 0x130000); a.load(0, 27, -0x56d4); a.compare(0, 0, false);
        if (!a.equal()) {
            a.value(25, 0); a.value(26, 0); a.jump(0x54);
            for (;;) {
                a.at(0x54); a.load(4, 27, -0x56d4); a.sign16(3, 25); a.load(0, 4, 4); a.compare_registers(3, 0);
                if (!a.less()) break;
                a.at(0x40); a.load(3, 4, 0); a.indexed_load(3, 3, 26); a.call(0x10775170);
                a.offset(26, 26, 4); a.offset(25, 25, 1);
            }
            a.load(3, 4, 0); a.call(0x10775170); a.offset(3, 28, 0x130000); a.value(0, 0); a.store(3, -0x56d4, 0);
        }
        a.at(0x7c); a.move(3, 28); a.call(0x10718ef0); a.value(25, 0); a.offset(27, 28, 0x130000); a.move(26, 25); a.jump(0xa8);
        for (;;) {
            a.at(0xa8); a.load<2>(0, 27, -0x625c, true); a.sign16(3, 25); a.compare_registers(3, 0);
            if (!a.less()) break;
            a.at(0x94); a.offset(3, 26, 4); a.add(3, 28, 3); a.call(0x10714440);
            a.offset(26, 26, 0xbf0); a.offset(25, 25, 1);
        }
        a.value(25, 0); a.offset(27, 28, 0x130000); a.move(26, 25); a.jump(0xe0);
        for (;;) {
            a.at(0xe0); a.load<2>(0, 27, -0x625a, true); a.sign16(3, 25); a.compare_registers(3, 0);
            if (!a.less()) break;
            a.at(0xc8); a.offset(3, 26, 0xc0000); a.offset(3, 3, -0x579c); a.add(3, 28, 3); a.call(0x10726a50);
            a.offset(26, 26, 0x17c); a.offset(25, 25, 1);
        }
        a.value(0, 0); a.store<2>(27, -0x625c, 0); a.store<2>(27, -0x625a, 0);
        a.load(3, 27, -0x56dc); a.compare(3, 0, false);
        if (!a.equal()) { a.call(0x1076b7e0); a.offset(3, 28, 0x130000); a.value(0, 0); a.store(3, -0x56dc, 0); }
        // The original checks this same field again after its clear. Keep the
        // live reread: a memory callback can republish an owned pointer there.
        a.at(0x118); a.offset(3, 28, 0x130000); a.load(3, 3, -0x56dc); a.compare(3, 0, false);
        if (!a.equal()) { a.call(0x1076b7e0); a.offset(3, 28, 0x130000); a.value(0, 0); a.store(3, -0x56dc, 0); }
        a.at(0x138); a.offset(3, 28, 0x130000); a.load(3, 3, -0x56d8); a.compare(3, 0, false);
        if (!a.equal()) { a.call(0x1076b7e0); a.offset(3, 28, 0x130000); a.value(0, 0); a.store(3, -0x56d8, 0); }
        a.at(0x158); a.load(3, 31, 0); a.compare(3, 0, false);
        if (!a.equal()) {
            a.load<2>(0, 30, 0, true); a.compare(0, 0);
            if (!a.positive()) { a.call(0x1076b7e0); a.value(0, 0); a.store(31, 0, 0); a.store<2>(30, 0, 0); }
        }
        a.at(0x180); a.offset(3, 28, 0xc0000); a.load(4, 2, -0x5bf4); a.value(5, 0x17c); a.value(6, 1200);
        a.offset(3, 3, -0x579c); a.call(0x10773420);
        a.load(4, 2, -0x5bf8); a.offset(3, 28, 4); a.value(5, 0xbf0); a.value(6, 250); a.call(0x10773420);
        a.sign16(0, 29, true);
        if (!a.nonpositive()) { a.move(3, 28); a.call(0x107735a0); }
    }
    a.at(0x1bc); a.move(3, 28); a.pop(96); a.multiple(25, true); a.finish();
}
struct Field { int high, displacement; unsigned width, source; };
void initialize_aggregate_fields(AggregateCall& a) {
    constexpr std::array<Field, 16> fields{{
        {0,0x2c61,1,3}, {0x260000,-0x7e50,4,3}, {0x260000,-0x7e4c,4,3}, {0,0x2c5e,1,3},
        {0x260000,-0x7f08,4,3}, {0x260000,-0x7e64,1,3}, {0,0x2c28,4,3}, {0x260000,-0x7e58,4,3},
        {0x260000,-0x7e54,2,3}, {0x260000,-0x7e52,2,3}, {0,0x2c58,2,3}, {0,0x2c5a,2,0},
        {0,0x2c5f,1,3}, {0x260000,-0x7e60,4,3}, {0x260000,-0x7e5c,4,3}, {0x260000,-0x7e62,2,3}}};
    for (const auto& field : fields) {
        a.load(6, 1, 88); if (field.high) a.offset(6, 6, field.high);
        if (field.width == 1) a.store<1>(6, field.displacement, field.source);
        else if (field.width == 2) a.store<2>(6, field.displacement, field.source);
        else a.store(6, field.displacement, field.source);
    }
}
void initialize_record_rows(AggregateCall& a) {
    a.jump(0x1cc);
    for (;;) {
        a.at(0x1cc); a.sign16(0, 5); a.compare(0, 16);
        if (!a.less()) break;
        a.at(0x10c); a.load(7, 1, 88); a.offset(0, 3, 0x2c38); a.value(8, 0); a.value(6, 0);
        a.indexed_store<2>(7, 0, 8); a.jump(0x1b4);
        for (;;) {
            a.at(0x1b4); a.sign16(0, 6); a.compare(0, 10);
            if (!a.less()) break;
            a.at(0x124); a.load(7, 1, 88); a.sign16(0, 6); a.offset(9, 6, 1); a.offset(8, 6, 2);
            a.add(10, 7, 0); a.offset(7, 6, 3); a.offset(10, 10, 0x260000); a.offset(0, 6, 4); a.value(11, 0);
            a.sign16(9, 9); a.offset(10, 10, -0x7f04); a.sign16(8, 8); a.indexed_store<1>(4, 10, 11);
            a.sign16(7, 7); a.sign16(0, 0); a.offset(6, 6, 5);
            a.load(10, 1, 88); a.add(9, 10, 9); a.offset(9, 9, 0x260000); a.offset(9, 9, -0x7f04); a.indexed_store<1>(4, 9, 11);
            a.load(9, 1, 88); a.add(8, 9, 8); a.offset(8, 8, 0x260000); a.offset(8, 8, -0x7f04); a.indexed_store<1>(4, 8, 11);
            a.load(8, 1, 88); a.add(7, 8, 7); a.offset(7, 7, 0x260000); a.offset(7, 7, -0x7f04); a.indexed_store<1>(4, 7, 11);
            a.load(7, 1, 88); a.add(7, 7, 0); a.offset(7, 7, 0x260000); a.offset(7, 7, -0x7f04); a.indexed_store<1>(4, 7, 11);
        }
        a.offset(3, 3, 2); a.offset(4, 4, 10); a.offset(5, 5, 1);
    }
}
void construct_aggregate(AggregateCall& a) {
    a.link(); a.save_link(); a.frame(64); a.store(1, 88, 3); a.value(3, 0x10000); a.offset(4, 3, -0x3cb0);
    a.load(3, 1, 88); a.offset(3, 3, 0x2c2c); a.call(0x1073ae10);
    a.load(3, 1, 88); a.value(4, 0); a.offset(3, 3, 0x2c68); a.call(0x10718b40);
    a.load(3, 1, 88); a.value(4, 1); a.offset(3, 3, 0x130000); a.offset(3, 3, -0x2950); a.call(0x10718b40);
    a.load(4, 1, 88); a.value(3, 0); a.value(0, UINT32_MAX); a.value(5, 0); a.store(4, 0x2c64, 3); a.move(4, 3);
    initialize_aggregate_fields(a); initialize_record_rows(a);
    a.load(3, 1, 88); a.value(5, 1); a.value(4, 0); a.value(0, UINT32_MAX); a.store<2>(3, 0x2c38, 5);
    for (const auto& field : std::array<Field, 8>{{{0,0,2,4},{0,4,4,4},{0,8,4,4},{0,12,4,4},{0,16,4,4},{0,20,2,4},{0,22,2,4},{0,24,1,5}}}) {
        a.load(3, 1, 88);
        if (field.width == 1) a.store<1>(3, field.displacement, field.source);
        else if (field.width == 2) a.store<2>(3, field.displacement, field.source);
        else a.store(3, field.displacement, field.source);
    }
    a.load(5, 1, 88); a.offset(3, 5, 0x2c68); a.store(5, 36, 3);
    for (const auto& field : std::array<Field, 12>{{{0,28,2,0},{0,32,4,4},{0,0x2c1c,4,4},{0,0x2c20,4,4},{0,0x2c24,4,4},{0,0x2c00,4,4},
        {0,0x2c04,4,4},{0,0x2c08,1,4},{0,0x2c0c,4,4},{0,0x2c10,4,4},{0,0x2c14,4,4},{0,0x2c18,2,4}}}) {
        a.load(3, 1, 88);
        if (field.width == 1) a.store<1>(3, field.displacement, field.source);
        else if (field.width == 2) a.store<2>(3, field.displacement, field.source);
        else a.store(3, field.displacement, field.source);
    }
    a.load(3, 1, 88); a.pop(64); a.finish();
}
void clear_record_groups(AggregateCall& a) {
    a.link(); a.multiple(24); a.value(28, 0); a.move(24, 3); a.move(27, 28); a.value(26, 0); a.save_link(); a.frame(96); a.jump(0xa0);
    for (;;) {
        a.at(0xa0); a.sign16(0, 26); a.compare(0, 16);
        if (!a.less()) break;
        a.at(0x24); a.offset(30, 28, 16); a.offset(31, 27, 0x2c38); a.value(25, 0); a.value(29, 0); a.jump(0x84);
        for (;;) {
            a.at(0x84); a.indexed_load<2>(0, 24, 31, true); a.sign16(3, 25); a.compare_registers(3, 0);
            if (!a.less()) break;
            a.at(0x38); a.sign16(0, 25, true); a.store<2>(24, 0x2c58, 26);
            if (!a.equal()) { a.add(4, 24, 28); a.value(5, 44); a.add(3, 4, 29); a.call(0x10745e00); }
            a.at(0x54); a.indexed_load(3, 24, 30); a.compare(3, 0, false);
            if (!a.equal()) {
                a.load(12, 3, 4); a.compare(12, 0, false);
                if (!a.equal()) { a.call(0x10781320); a.load(2, 1, 20); }
            }
            a.at(0x74); a.move(3, 24); a.call(0x1073ee30); a.offset(29, 29, 44); a.offset(25, 25, 1);
        }
        a.offset(28, 28, 0x2c0); a.offset(27, 27, 2); a.offset(26, 26, 1);
    }
    a.pop(96); a.multiple(24, true); a.finish();
}
void detach_stream(AggregateCall& a) {
    a.link(); a.save(31); a.move(31, 3); a.save_link(); a.frame(64); a.load(0, 3, 0x2c64); a.compare(0, 0, false);
    if (!a.equal()) {
        a.call(0x1073c5a0); a.load(3, 31, 0x2c64); a.value(4, 1); a.call(0x1073a120); a.value(0, 0); a.store(31, 0x2c64, 0);
    }
    a.at(0x38); a.pop(64); a.restore(31); a.finish();
}
void destroy_aggregate(AggregateCall& a) {
    a.link(); a.save(31); a.move(31, 4); a.save(30); a.move(30, 3, true); a.save_link(); a.frame(64);
    if (!a.equal()) {
        a.call(0x1073c4e0); a.move(3, 30); a.call(0x1073f4e0); a.offset(3, 30, 0x130000); a.value(4, UINT32_MAX);
        a.offset(3, 3, -0x2950); a.call(0x10718ba0); a.offset(3, 30, 0x2c68); a.value(4, UINT32_MAX); a.call(0x10718ba0);
        a.offset(3, 30, 0x2c2c); a.value(4, UINT32_MAX); a.call(0x1073aee0); a.sign16(0, 31, true);
        if (!a.nonpositive()) { a.move(3, 30); a.call(0x107735a0); }
    }
    a.at(0x64); a.move(3, 30); a.pop(64); a.restore(31); a.restore(30); a.finish();
}
}
unsigned entry_index(std::uint32_t entry) {
    const auto found = std::find(entries.begin(), entries.end(), entry);
    if (found == entries.end()) throw std::runtime_error("Unknown aggregate-object entry");
    return unsigned(found - entries.begin());
}
void invoke(State& state, unsigned index, DependencyDispatch* dependencies) {
    if (index >= entries.size()) throw std::runtime_error("Unknown aggregate-object index");
    AggregateCall call(state, entries[index], dependencies);
    switch (index) {
    case 0: construct_table(call); break;
    case 1: destroy_table(call); break;
    case 2: construct_aggregate(call); break;
    case 3: destroy_aggregate(call); break;
    case 4: clear_record_groups(call); break;
    case 5: detach_stream(call); break;
    }
}
}
