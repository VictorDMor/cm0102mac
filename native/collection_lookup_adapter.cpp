#include "collection_lookup_adapter.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
namespace {
std::uint32_t reg(const State& s, unsigned n) { return std::uint32_t(s.r<4>(n * 4)); }
void set(State& s, unsigned n, std::uint32_t value) { s.wr<4>(n * 4, value); }
std::int32_t signed_word(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }

// ABI compatibility operations retain the original access sites, register
// residues and partial effects on faults. Traversal decisions below use named
// collection operations; none execute a replaced original instruction body.
std::uint32_t read(State& s, std::uint32_t address, std::uint32_t pc) {
    s.pc = pc;
    return std::uint32_t(s.memory.read<4>(address));
}
void write(State& s, std::uint32_t address, std::uint32_t value, std::uint32_t pc) {
    s.pc = pc;
    s.memory.write<4>(address, value);
}
void compare(State& s, std::int64_t value, bool record = false) {
    s.wr<1>(2304, (record ? s.r<1>(2304) & 0xf0 : 0) |
        (value < 0 ? 8 : value > 0 ? 4 : 2) | (s.r<1>(1024) & 1));
}
void call(State& s, std::uint32_t entry, std::uint32_t link) {
    s.wr<4>(2440, reg(s, 2));
    s.wr<4>(4128, link);
    s.pc = entry;
    run_until(s, link);
}
void push_frame(State& s, unsigned bytes, std::uint32_t pc) {
    const auto previous = reg(s, 1);
    write(s, previous - bytes, previous, pc);
    set(s, 1, previous - bytes);
}
void return_to_caller(State& s) { s.pc = std::uint32_t(s.r<4>(4128)); }

void list_node_at_index(State& s) {
    compare(s, signed_word(reg(s, 4)));
    set(s, 3, read(s, reg(s, 3), 0x1074dd44));
    if (signed_word(reg(s, 4)) <= 0) { return_to_caller(s); return; }

    // The original groups eight successive links in one CTR iteration. Keeping
    // that grouping preserves both fault sites and the remainder's r4 residue.
    set(s, 0, reg(s, 4) >> 3);
    compare(s, signed_word(reg(s, 0)), true);
    s.wr<4>(4132, reg(s, 0));
    if (reg(s, 0)) {
        do {
            for (unsigned link = 0; link < 8; ++link) {
                const auto pc = 0x1074dd58 + link * 4;
                s.step(pc);
                set(s, 3, read(s, reg(s, 3) + 4, pc));
            }
            s.wr<4>(4132, std::uint32_t(s.r<4>(4132)) - 1);
        } while (s.r<4>(4132));
        set(s, 4, reg(s, 4) & 7);
        compare(s, signed_word(reg(s, 4)), true);
        if (!reg(s, 4)) { return_to_caller(s); return; }
    }
    s.wr<4>(4132, reg(s, 4));
    do {
        s.step(0x1074dd88);
        set(s, 3, read(s, reg(s, 3) + 4, 0x1074dd88));
        s.wr<4>(4132, std::uint32_t(s.r<4>(4132)) - 1);
    } while (s.r<4>(4132));
    return_to_caller(s);
}

void restore_wrapper_frame(State& s, bool descriptor) {
    set(s, 0, read(s, reg(s, 1) + 72, descriptor ? 0x1074dfa8 : 0x1074df7c));
    set(s, 1, reg(s, 1) + 64);
    s.wr<4>(4128, reg(s, 0));
    return_to_caller(s);
}
void tree_node_at_index(State& s) {
    set(s, 0, s.r<4>(4128));
    write(s, reg(s, 1) + 8, reg(s, 0), 0x1074df64);
    push_frame(s, 64, 0x1074df68);
    // This is a guest caller-area slot, retaining aliases with nodes/spills.
    write(s, reg(s, 1) + 92, reg(s, 4), 0x1074df6c);
    set(s, 5, reg(s, 1) + 92);
    set(s, 4, read(s, reg(s, 3), 0x1074df74));
    call(s, 0x1074e150, 0x1074df7c);
    restore_wrapper_frame(s, false);
}
void tree_node_matching_descriptor(State& s) {
    set(s, 0, s.r<4>(4128));
    set(s, 5, reg(s, 4));
    write(s, reg(s, 1) + 8, reg(s, 0), 0x1074df98);
    push_frame(s, 64, 0x1074df9c);
    set(s, 4, read(s, reg(s, 3), 0x1074dfa0));
    call(s, 0x1074e5b0, 0x1074dfa8);
    restore_wrapper_frame(s, true);
}

// The original preorder walk expands four levels per guest frame. This table
// describes the ABI placement of each expanded node, in binary heap order:
// root, L, R, LL, LR, RL, RR, LLL, LLR, LRL, LRR, RLL, RLR, RRL, RRR.
// Child/position pointers always come from live guest registers and memory.
// In particular, recursive spills may alias and change their caller's values.
struct PositionSites {
    unsigned node_register, position_register;
    std::uint32_t position_read, position_store, left_read, right_read;
    unsigned left_register;
    std::uint32_t left_return, right_return;
    bool prepare_first_call;
};
constexpr std::array<PositionSites, 15> position_sites{{
    {28, 4, 0x1074e170, 0x1074e188, 0x1074e18c, 0x1074e390, 30, 0, 0, false},
    {30, 4, 0x1074e198, 0x1074e1ac, 0x1074e1b0, 0x1074e29c, 27, 0, 0, false},
    {30, 3, 0x1074e39c, 0x1074e3b0, 0x1074e3b4, 0x1074e4a8, 27, 0, 0, false},
    {27, 4, 0x1074e1bc, 0x1074e1d0, 0x1074e1d4, 0x1074e234, 26, 0, 0, false},
    {30, 3, 0x1074e2a8, 0x1074e2bc, 0x1074e2c0, 0x1074e328, 26, 0, 0, false},
    {27, 3, 0x1074e3c0, 0x1074e3d4, 0x1074e3d8, 0x1074e440, 26, 0, 0, false},
    {30, 3, 0x1074e4b4, 0x1074e4c8, 0x1074e4cc, 0x1074e534, 26, 0, 0, false},
    {26, 4, 0x1074e1e0, 0x1074e1f4, 0x1074e1f8, 0x1074e210, 4, 0x1074e200, 0x1074e220, false},
    {27, 3, 0x1074e240, 0x1074e258, 0x1074e260, 0x1074e278, 4, 0x1074e268, 0x1074e288, true},
    {26, 3, 0x1074e2cc, 0x1074e2e4, 0x1074e2ec, 0x1074e304, 4, 0x1074e2f4, 0x1074e314, true},
    {30, 3, 0x1074e334, 0x1074e34c, 0x1074e354, 0x1074e36c, 4, 0x1074e35c, 0x1074e37c, true},
    {26, 3, 0x1074e3e4, 0x1074e3fc, 0x1074e404, 0x1074e41c, 4, 0x1074e40c, 0x1074e42c, true},
    {27, 3, 0x1074e44c, 0x1074e464, 0x1074e46c, 0x1074e484, 4, 0x1074e474, 0x1074e494, true},
    {26, 3, 0x1074e4d8, 0x1074e4f0, 0x1074e4f8, 0x1074e510, 4, 0x1074e500, 0x1074e520, true},
    {30, 3, 0x1074e540, 0x1074e558, 0x1074e560, 0x1074e578, 4, 0x1074e568, 0x1074e588, true}
}};

void visit_position_node(State& s, unsigned path) {
    const auto& sites = position_sites[path];
    set(s, sites.position_register, read(s, reg(s, 31), sites.position_read));
    compare(s, signed_word(reg(s, sites.position_register)));
    if (!reg(s, sites.position_register)) return; // Current preorder node wins.
    set(s, 0, reg(s, sites.position_register) - 1); // Deliberate 32-bit wrap.
    if (sites.prepare_first_call) set(s, 3, reg(s, 29));
    write(s, reg(s, 31), reg(s, 0), sites.position_store);

    if (path >= 7) {
        if (sites.prepare_first_call) set(s, 5, reg(s, 31));
        set(s, 4, read(s, reg(s, sites.node_register) + 8, sites.left_read));
        // The first expanded LLL call inherits r3/r5 unchanged. Every other
        // expanded leaf sets them before the decrement store/child load.
        call(s, 0x1074e150, sites.left_return);
        compare(s, reg(s, 3));
        if (reg(s, 3)) set(s, sites.node_register, reg(s, 3));
        else {
            set(s, 4, read(s, reg(s, sites.node_register) + 12, sites.right_read));
            set(s, 3, reg(s, 29));
            set(s, 5, reg(s, 31));
            call(s, 0x1074e150, sites.right_return);
            set(s, sites.node_register, reg(s, 3));
        }
        return;
    }

    set(s, sites.left_register, read(s, reg(s, sites.node_register) + 8, sites.left_read));
    compare(s, reg(s, sites.left_register));
    if (reg(s, sites.left_register)) visit_position_node(s, path * 2 + 1);
    compare(s, reg(s, sites.left_register));
    if (reg(s, sites.left_register)) set(s, sites.node_register, reg(s, sites.left_register));
    else {
        set(s, sites.node_register, read(s, reg(s, sites.node_register) + 12, sites.right_read));
        compare(s, reg(s, sites.node_register));
        if (reg(s, sites.node_register)) visit_position_node(s, path * 2 + 2);
    }
}

void save_position_registers(State& s) {
    s.pc = 0x1074e154;
    s.wr<4>(2432, reg(s, 1) - 24);
    for (unsigned n = 26; n < 32; ++n) {
        s.memory.write<4>(s.r<4>(2432), reg(s, n));
        s.wr<4>(2432, std::uint32_t(s.r<4>(2432)) + 4);
    }
}
void restore_position_frame(State& s) {
    set(s, 0, read(s, reg(s, 1) + 88, 0x1074e590));
    set(s, 1, reg(s, 1) + 80);
    s.wr<4>(4128, reg(s, 0));
    s.pc = 0x1074e59c;
    s.wr<4>(2432, reg(s, 1) - 24);
    for (unsigned n = 26; n < 32; ++n) {
        set(s, n, s.memory.read<4>(s.r<4>(2432)));
        s.wr<4>(2432, std::uint32_t(s.r<4>(2432)) + 4);
    }
    return_to_caller(s);
}
void tree_node_at_mutable_position(State& s) {
    set(s, 0, s.r<4>(4128));
    save_position_registers(s);
    set(s, 28, reg(s, 4)); compare(s, signed_word(reg(s, 28)), true);
    set(s, 29, reg(s, 3)); set(s, 31, reg(s, 5));
    write(s, reg(s, 1) + 8, reg(s, 0), 0x1074e164);
    push_frame(s, 80, 0x1074e168);
    if (!reg(s, 28)) set(s, 3, reg(s, 28));
    else {
        const auto& root = position_sites[0];
        set(s, 4, read(s, reg(s, 31), root.position_read));
        compare(s, signed_word(reg(s, 4)));
        if (!reg(s, 4)) set(s, 3, reg(s, 28));
        else {
            set(s, 0, reg(s, 4) - 1);
            write(s, reg(s, 31), reg(s, 0), root.position_store);
            set(s, 30, read(s, reg(s, 28) + 8, root.left_read));
            compare(s, reg(s, 30));
            if (reg(s, 30)) visit_position_node(s, 1);
            compare(s, reg(s, 30));
            if (reg(s, 30)) set(s, 3, reg(s, 30));
            else {
                set(s, 30, read(s, reg(s, 28) + 12, root.right_read));
                compare(s, reg(s, 30));
                if (reg(s, 30)) visit_position_node(s, 2);
                set(s, 3, reg(s, 30));
            }
        }
    }
    restore_position_frame(s);
}

// A descriptor lookup follows one comparator-selected path. The compatibility
// sites reflect four comparisons in one guest frame, then a normal dispatched
// recursive call. Negative means +8; positive means +12, regardless of the
// collection's comparator ordering convention.
struct SearchSites { std::uint32_t key_read, negative_child, positive_child; };
constexpr std::array<SearchSites, 15> search_sites{{
    {0x1074e5d8, 0x1074e5f4, 0x1074e804},
    {0x1074e600, 0x1074e61c, 0x1074e70c},
    {0x1074e810, 0x1074e82c, 0x1074e91c},
    {0x1074e628, 0x1074e644, 0x1074e6a4},
    {0x1074e718, 0x1074e734, 0x1074e794},
    {0x1074e838, 0x1074e854, 0x1074e8b4},
    {0x1074e928, 0x1074e944, 0x1074e9a4},
    {0x1074e650, 0x1074e66c, 0x1074e684},
    {0x1074e6b0, 0x1074e6cc, 0x1074e6e4},
    {0x1074e740, 0x1074e75c, 0x1074e774},
    {0x1074e7a0, 0x1074e7bc, 0x1074e7d4},
    {0x1074e860, 0x1074e87c, 0x1074e894},
    {0x1074e8c0, 0x1074e8dc, 0x1074e8f4},
    {0x1074e950, 0x1074e96c, 0x1074e984},
    {0x1074e9b0, 0x1074e9cc, 0x1074e9e4}
}};
void restore_search_frame(State& s) {
    set(s, 0, read(s, reg(s, 1) + 88, 0x1074ea14));
    set(s, 1, reg(s, 1) + 80);
    s.wr<4>(4128, reg(s, 0));
    set(s, 31, read(s, reg(s, 1) - 4, 0x1074ea20));
    set(s, 30, read(s, reg(s, 1) - 8, 0x1074ea24));
    set(s, 29, read(s, reg(s, 1) - 12, 0x1074ea28));
    return_to_caller(s);
}
void tree_node_from_comparator(State& s) {
    set(s, 0, s.r<4>(4128));
    write(s, reg(s, 1) - 4, reg(s, 31), 0x1074e5b4);
    set(s, 31, reg(s, 4)); compare(s, signed_word(reg(s, 31)), true);
    write(s, reg(s, 1) - 8, reg(s, 30), 0x1074e5bc);
    set(s, 30, reg(s, 5));
    write(s, reg(s, 1) - 12, reg(s, 29), 0x1074e5c4);
    set(s, 29, reg(s, 3));
    write(s, reg(s, 1) + 8, reg(s, 0), 0x1074e5cc);
    push_frame(s, 80, 0x1074e5d0);
    if (!reg(s, 31)) set(s, 3, reg(s, 31));
    else {
        unsigned path = 0;
        for (;;) {
            const auto& sites = search_sites[path];
            set(s, 3, read(s, reg(s, 31), sites.key_read));
            set(s, 4, reg(s, 30));
            // Reload the mutable collection vector for every comparison. The
            // existing transition stub saves caller TOC at guest SP+20, calls
            // its current target with its own TOC and retains all clobbers.
            set(s, 12, read(s, reg(s, 29) + 4, sites.key_read + 8));
            call(s, 0x10781320, sites.key_read + 16);
            set(s, 2, read(s, reg(s, 1) + 20, sites.key_read + 16));
            compare(s, signed_word(reg(s, 3)));
            const auto comparison = signed_word(reg(s, 3));
            if (!comparison) { set(s, 3, reg(s, 31)); break; }
            const bool negative = comparison < 0;
            const auto child_site = negative ? sites.negative_child : sites.positive_child;
            const auto child_offset = negative ? 8u : 12u;
            if (path >= 7) {
                set(s, 4, read(s, reg(s, 31) + child_offset, child_site));
                set(s, 3, reg(s, 29)); set(s, 5, reg(s, 30));
                call(s, 0x1074e5b0, child_site + 16);
                break;
            }
            set(s, 31, read(s, reg(s, 31) + child_offset, child_site));
            compare(s, reg(s, 31));
            if (!reg(s, 31)) { set(s, 3, reg(s, 31)); break; }
            path = path * 2 + (negative ? 1 : 2);
        }
    }
    restore_search_frame(s);
}
}

bool try_modern_collection_lookup(State& s, std::uint32_t entry) {
    s.pc = entry;
    if (s.collection_original_resume == entry) { s.collection_original_resume = 0; return false; }
    const auto found = std::find(collection_lookup_entries.begin(), collection_lookup_entries.end(), entry);
    if (found == collection_lookup_entries.end()) throw std::runtime_error("Unknown collection lookup entry");
    const auto index = std::size_t(found - collection_lookup_entries.begin());
    const bool original = s.collection_original || s.collection_entry_original[index];
    ++(original ? s.collection_original_calls[index] : s.collection_calls[index]);
    if (original && !s.collection_observer) return false;
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (s.collection_observer) s.collection_observer->enter(s, entry, original);
    if (original) {
        s.collection_original_resume = entry;
        try { run_until(s, caller); }
        catch (...) { s.collection_original_resume = 0; throw; }
    } else {
        s.step(entry);
        switch (index) {
        case 0: list_node_at_index(s); break;
        case 1: tree_node_at_index(s); break;
        case 2: tree_node_matching_descriptor(s); break;
        case 3: tree_node_at_mutable_position(s); break;
        case 4: tree_node_from_comparator(s); break;
        }
    }
    if (s.collection_observer) s.collection_observer->leave(s, entry, original);
    return true;
}
void reset_collection_lookup_diagnostics(State& s) {
    s.collection_original = false;
    s.collection_entry_original.fill(false);
    s.collection_calls.fill(0); s.collection_original_calls.fill(0);
    s.collection_original_resume = 0;
    s.collection_observer = nullptr;
}
std::string collection_lookup_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < collection_lookup_entries.size(); ++i)
        out << "collection.entry=" << hex_address(collection_lookup_entries[i])
            << " mode=" << (s.collection_original || s.collection_entry_original[i] ? "original" : "modern")
            << " modern=" << s.collection_calls[i] << " original=" << s.collection_original_calls[i] << '\n';
    return out.str();
}
}
