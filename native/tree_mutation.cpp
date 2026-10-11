#include "tree_mutation.hpp"
#include "guest_context_transfer.hpp"
#include <algorithm>

namespace cm::aot::tree_mutation {
namespace {
// Compatibility checkpoints retain fault prefixes, finite budgets and the
// live guest ABI around the named collection operations below.
class TreeCall {
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
    TreeCall(State& s, std::uint32_t root, DependencyDispatch* d)
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
    void multiple(unsigned first, bool restore = false) {
        step(); state.wr<4>(2432, r(1) - (32 - first) * 4);
        for (unsigned n = first; n < 32; ++n) {
            if (restore) state.wr<4>(n * 4, state.memory.read<4>(state.r<4>(2432)));
            else state.memory.write<4>(state.r<4>(2432), r(n));
            state.wr<4>(2432, state.r<4>(2432) + 4);
        }
    }
    void signed_zero(unsigned source) { step(); condition(std::bit_cast<std::int32_t>(r(source)), 0); }
    bool nonnegative() { step(); return (state.r<1>(2304) & 8) == 0; }
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
void insert_node(TreeCall& a) {
    a.link(); a.multiple(27); a.move(28, 3); a.move(29, 4); a.move(30, 5); a.move(31, 6);
    a.save_link(); a.frame(80); a.load(3, 4, 0); a.compare_zero(3);
    if (a.equal()) {
        a.value(3, 16); a.call(0x10773520); a.compare_zero(3);
        if (!a.equal()) {
            a.store(3, 0, 30); a.value(0, 0); a.store(3, 4, 31); a.store(3, 8, 0); a.store(3, 12, 0);
        }
        // The original increments count even when allocation returns null.
        a.at(0x50); a.store(29, 0, 3); a.load(3, 28, 12); a.offset(0, 3, 1); a.store(28, 12, 0); a.jump(0x100);
        a.load(3, 29, 0);
    } else {
        a.at(0x64); a.load(3, 3, 0); a.move(4, 30); a.load(12, 28, 4); a.call(0x10781320);
        a.load(2, 1, 20); a.signed_zero(3);
        if (!a.nonnegative()) {
            a.load(4, 29, 0); a.move(3, 28); a.move(5, 30); a.move(6, 31); a.offset(4, 4, 8);
            a.call(0x1074e030); a.jump(0x104);
        } else {
            a.at(0x9c);
            if (a.positive()) {
                a.load(4, 29, 0); a.move(3, 28); a.move(5, 30); a.move(6, 31); a.offset(4, 4, 12);
                a.call(0x1074e030); a.jump(0x104);
            } else {
                // Equal keys replace the node but preserve its live children.
                // A failed replacement still disposes the old node and stores null.
                a.at(0xbc); a.value(3, 16); a.call(0x10773520); a.move(27, 3, true);
                if (!a.equal()) {
                    a.load(4, 29, 0); a.load(3, 4, 12); a.load(0, 4, 8);
                    a.store(27, 0, 30); a.store(27, 4, 31); a.store(27, 8, 0); a.store(27, 12, 3);
                }
                a.at(0xe8); a.load(3, 29, 0); a.value(4, 0); a.load(12, 28, 8); a.call(0x10781320);
                a.load(2, 1, 20); a.store(29, 0, 27); a.load(3, 29, 0);
            }
        }
    }
    a.at(0x104); a.pop(80); a.multiple(27, true); a.finish();
}
void insert_root(TreeCall& a) {
    // This facade deliberately consumes the caller's saved-link slot. It has
    // no link capture/save of its own in the original Mac body.
    a.move(4, 3); a.frame(64); a.call(0x1074e030); a.pop(64); a.finish();
}
struct CleanupSites {
    unsigned node, child;
    unsigned left, right, dispose;
};
// Four expanded levels share one guest frame; deeper children use the actual
// recursive dispatcher. Heap-order sites describe ABI placement, not opcodes.
constexpr std::array<CleanupSites, 15> cleanup_sites{{
    {26,31,0x24,0x198,0x318}, {31,30,0x30,0xd4,0x184}, {31,30,0x1a4,0x254,0x304},
    {30,29,0x3c,0x78,0xc0}, {30,29,0xe0,0x128,0x170},
    {30,29,0x1b0,0x1f8,0x240}, {30,29,0x260,0x2a8,0x2f0},
    {29,4,0x48,0x50,0x64}, {29,4,0x84,0x98,0xac},
    {29,4,0xec,0x100,0x114}, {29,4,0x134,0x148,0x15c},
    {29,4,0x1bc,0x1d0,0x1e4}, {29,4,0x204,0x218,0x22c},
    {29,4,0x26c,0x280,0x294}, {29,4,0x2b4,0x2c8,0x2dc}
}};
void recursive_child(TreeCall& a, const CleanupSites& sites, bool right, bool inherit) {
    a.at(right ? sites.right : sites.left); a.load(4, sites.node, right ? 12 : 8);
    if (!inherit) { a.move(3, 25); a.move(5, 27); a.move(6, 28); }
    a.call(0x1074ea30);
}
void clear_subtree(TreeCall& a, unsigned path) {
    const auto& sites = cleanup_sites[path];
    if (path >= 7) {
        recursive_child(a, sites, false, path == 7);
        recursive_child(a, sites, true, false);
    } else {
        for (bool right : {false, true}) {
            // Load the second child after all first-child callbacks. Capturing
            // both links early would change original callback/alias effects.
            a.at(right ? sites.right : sites.left);
            a.load(sites.child, sites.node, right ? 12 : 8); a.compare_zero(sites.child);
            if (!a.equal()) clear_subtree(a, path * 2 + (right ? 2 : 1));
        }
    }
    a.at(sites.dispose); a.move(3, sites.node); a.move(4, 28); a.move(12, 27);
    a.call(0x10781320); a.load(2, 1, 20);
}
void clear_tree(TreeCall& a) {
    a.link(); a.multiple(25); a.move(26, 4, true); a.move(25, 3); a.move(27, 5); a.move(28, 6);
    a.save_link(); a.frame(96);
    if (!a.equal()) clear_subtree(a, 0);
    a.at(0x32c); a.pop(96); a.multiple(25, true); a.finish();
}
}
unsigned entry_index(std::uint32_t entry) {
    const auto found = std::find(entries.begin(), entries.end(), entry);
    if (found == entries.end()) throw std::runtime_error("Unknown tree-mutation entry");
    return unsigned(found - entries.begin());
}
void invoke(State& state, unsigned index, DependencyDispatch* dependencies) {
    if (index >= entries.size()) throw std::runtime_error("Unknown tree-mutation index");
    TreeCall call(state, entries[index], dependencies);
    switch (index) {
    case 0: insert_root(call); break;
    case 1: insert_node(call); break;
    case 2: clear_tree(call); break;
    }
}
}
