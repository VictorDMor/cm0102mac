#include "string_core_adapter.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
namespace {
std::uint32_t reg(const State& s, unsigned n) { return std::uint32_t(s.r<4>(n * 4)); }
void set(State& s, unsigned n, std::uint32_t v) { s.wr<4>(n * 4, v); }
std::int32_t word(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
std::uint32_t byte(std::uint32_t v) { return std::uint32_t(signed_value<1>(v & 255)); }
std::uint32_t short_word(std::uint32_t v) { return std::uint32_t(signed_value<2>(v & 65535)); }
void compare(State& s, std::int64_t a, std::int64_t b = 0, bool record = false) {
    s.wr<1>(2304, (record ? s.r<1>(2304) & 0xf0 : 0) |
        (a < b ? 8 : a > b ? 4 : 2) | (s.r<1>(1024) & 1));
}
template<unsigned N> std::uint32_t read(State& s, std::uint32_t address, std::uint32_t pc) {
    s.pc = pc;
    return std::uint32_t(s.memory.read<N>(address));
}
template<unsigned N> void write(State& s, std::uint32_t address, std::uint32_t value, std::uint32_t pc) {
    s.pc = pc;
    s.memory.write<N>(address, value);
}
// Updating loads/stores commit their base only after a successful access. The
// load result and comparison precede the store, even for overlapping strings.
void next_byte(State& s, unsigned result, unsigned cursor, std::uint32_t pc) {
    const auto address = reg(s, cursor) + 1;
    set(s, result, read<1>(s, address, pc));
    set(s, cursor, address);
}
void store_next(State& s, unsigned cursor, std::uint32_t pc) {
    const auto address = reg(s, cursor) + 1;
    write<1>(s, address, reg(s, 0), pc);
    set(s, cursor, address);
}
bool decrement(State& s, unsigned counter) {
    const auto before = reg(s, counter);
    s.wr<1>(1027, before != 0);
    set(s, counter, before - 1);
    compare(s, word(reg(s, counter)), 0, true);
    return reg(s, counter) != 0;
}
void finish(State& s) { s.pc = std::uint32_t(s.r<4>(4128)); }
void call(State& s, std::uint32_t entry, std::uint32_t link) {
    s.wr<4>(2440, reg(s, 2)); s.wr<4>(4128, link);
    s.pc = entry; run_until(s, link);
}
void frame(State& s, std::uint32_t pc) {
    const auto previous = reg(s, 1);
    write<4>(s, previous - 64, previous, pc); set(s, 1, previous - 64);
}
void restore_link(State& s, std::uint32_t pc) {
    set(s, 0, read<4>(s, reg(s, 1) + 72, pc));
    set(s, 1, reg(s, 1) + 64); s.wr<4>(4128, reg(s, 0));
}

// The two Mac comparisons share byte semantics, but have distinct ABI
// placements and access sites. Raw NUL terminates; mapped zero does not.
struct MacComparison {
    unsigned table, cursor, index, left, right, scratch;
    std::uint32_t slot, left_read, right_read, left_map, right_map, final_left, final_right;
    bool limited;
};
constexpr MacComparison complete{6,5,9,7,8,0,0x107393f4,0x10739484,0x10739490,
    0x10739430,0x10739450,0x107394a4,0x107394c0,false};
constexpr MacComparison bounded{8,6,11,9,10,7,0x107394e4,0x1073957c,0x10739588,
    0x10739528,0x10739548,0x107395b8,0x107395d4,true};
void signed_byte(State& s, unsigned dest, unsigned source, bool record = false) {
    set(s, dest, byte(reg(s, source)));
    if (record) compare(s, word(reg(s, dest)), 0, true);
}
void map_byte(State& s, unsigned scratch, unsigned source, unsigned table, std::uint32_t pc) {
    signed_byte(s, scratch, source);
    compare(s, word(reg(s, scratch)), -1);
    if (reg(s, scratch) == UINT32_MAX) set(s, scratch, UINT32_MAX);
    else {
        set(s, scratch, reg(s, scratch) & 255);
        set(s, scratch, read<1>(s, reg(s, table) + reg(s, scratch), pc));
    }
}
void mac_compare(State& s, const MacComparison& c) {
    compare(s, reg(s, 3));
    set(s, c.table, read<4>(s, reg(s, 2) - 0x62bc, c.slot));
    if (reg(s, 3)) compare(s, reg(s, 4));
    if (!reg(s, 3) || !reg(s, 4)) { set(s, 3, 0); finish(s); return; }
    set(s, c.cursor, reg(s, 4));
    if (c.limited) set(s, 0, short_word(reg(s, 5)));
    set(s, c.index, 0);
    for (;;) {
        s.step(c.left_read);
        set(s, c.left, read<1>(s, reg(s, 3), c.left_read));
        signed_byte(s, c.scratch, c.left, true);
        bool present = reg(s, c.scratch) != 0;
        if (present) {
            set(s, c.right, read<1>(s, reg(s, c.cursor), c.right_read));
            signed_byte(s, c.scratch, c.right, true);
            present = reg(s, c.scratch) != 0;
        }
        if (present && c.limited) {
            compare(s, word(reg(s, c.index)), word(reg(s, 0)));
            present = word(reg(s, c.index)) < word(reg(s, 0));
        }
        if (!present) break;
        map_byte(s, c.scratch, c.left, c.table, c.left_map);
        signed_byte(s, c.left, c.right);
        signed_byte(s, c.right, c.scratch);
        compare(s, word(reg(s, c.left)), -1);
        if (reg(s, c.left) == UINT32_MAX) set(s, c.scratch, UINT32_MAX);
        else {
            set(s, c.scratch, reg(s, c.left) & 255);
            set(s, c.scratch, read<1>(s, reg(s, c.table) + reg(s, c.scratch), c.right_map));
        }
        signed_byte(s, c.scratch, c.scratch);
        compare(s, word(reg(s, c.right)), word(reg(s, c.scratch)));
        if (word(reg(s, c.right)) != word(reg(s, c.scratch))) {
            set(s, 3, word(reg(s, c.right)) < word(reg(s, c.scratch)) ? UINT32_MAX : 1);
            finish(s); return;
        }
        set(s, c.index, reg(s, c.index) + 1);
        set(s, c.cursor, reg(s, c.cursor) + 1); set(s, 3, reg(s, 3) + 1);
    }
    if (c.limited) {
        set(s, 0, short_word(reg(s, 5)));
        compare(s, word(reg(s, c.index)), word(reg(s, 0)));
        if (word(reg(s, c.index)) >= word(reg(s, 0))) { set(s, 3, 0); finish(s); return; }
    }
    signed_byte(s, 0, c.left, true);
    if (!reg(s, 0)) {
        set(s, 0, read<1>(s, reg(s, 4) + reg(s, c.index), c.final_left));
        signed_byte(s, 0, 0, true);
        if (reg(s, 0)) { set(s, 3, UINT32_MAX); finish(s); return; }
    }
    signed_byte(s, 0, c.left, true);
    if (reg(s, 0)) {
        set(s, 0, read<1>(s, reg(s, 4) + reg(s, c.index), c.final_right));
        signed_byte(s, 0, 0, true);
        if (!reg(s, 0)) { set(s, 3, 1); finish(s); return; }
    }
    set(s, 3, 0); finish(s);
}
void descriptor_compare(State& s) {
    set(s, 0, s.r<4>(4128)); write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x1074de14);
    frame(s, 0x1074de18); call(s, 0x107393f0, 0x1074de20);
    restore_link(s, 0x1074de20); finish(s);
}
void length(State& s) {
    set(s, 4, reg(s, 3) - 1); set(s, 3, UINT32_MAX);
    do {
        s.step(0x1077dbd8); next_byte(s, 0, 4, 0x1077dbd8);
        set(s, 3, reg(s, 3) + 1); compare(s, reg(s, 0));
    } while (reg(s, 0));
    finish(s);
}
void copy_bytes(State& s, bool concatenate) {
    set(s, 4, reg(s, 4) - 1); set(s, 5, reg(s, 3) - 1);
    if (concatenate) {
        do {
            s.step(0x1077dc68); next_byte(s, 0, 5, 0x1077dc68); compare(s, reg(s, 0));
        } while (reg(s, 0));
        set(s, 5, reg(s, 5) - 1);
    }
    const auto load = concatenate ? 0x1077dc78u : 0x1077dbf8u;
    const auto store = concatenate ? 0x1077dc80u : 0x1077dc00u;
    do {
        s.step(load); next_byte(s, 0, 4, load); compare(s, reg(s, 0)); store_next(s, 5, store);
    } while (reg(s, 0));
    finish(s);
}
void copy_bounded(State& s) {
    set(s, 4, reg(s, 4) - 1); set(s, 6, reg(s, 3) - 1); set(s, 5, reg(s, 5) + 1);
    while (decrement(s, 5)) {
        s.step(0x1077dc20); next_byte(s, 0, 4, 0x1077dc20);
        compare(s, reg(s, 0)); store_next(s, 6, 0x1077dc28);
        if (!reg(s, 0)) {
            set(s, 0, 0);
            while (decrement(s, 5)) { s.step(0x1077dc38); store_next(s, 6, 0x1077dc38); }
            break;
        }
    }
    finish(s);
}
void bytes_compare(State& s, bool limited) {
    set(s, 3, reg(s, 3) - 1); set(s, 4, reg(s, 4) - 1);
    if (limited) set(s, 6, reg(s, 5) + 1);
    const auto load = limited ? 0x1077dce0u : 0x1077dcb0u;
    while (!limited || decrement(s, 6)) {
        s.step(load); next_byte(s, 0, 3, load); next_byte(s, 5, 4, load + 4);
        compare(s, reg(s, 0), reg(s, 5));
        if (reg(s, 0) != reg(s, 5)) { set(s, 3, reg(s, 0) - reg(s, 5)); finish(s); return; }
        compare(s, reg(s, 0));
        if (!reg(s, 0)) break;
    }
    set(s, 3, 0); finish(s);
}
void find_byte(State& s, bool last) {
    const unsigned cursor = last ? 5 : 3;
    set(s, cursor, reg(s, 3) - 1); set(s, 0, reg(s, 4) & 255);
    if (last) set(s, 3, 0);
    const auto load = last ? 0x1077dd6cu : 0x1077dd28u;
    for (;;) {
        s.step(load); next_byte(s, 4, cursor, load); compare(s, reg(s, 4));
        if (!reg(s, 4)) break;
        compare(s, reg(s, 4), reg(s, 0));
        if (reg(s, 4) == reg(s, 0)) {
            if (!last) { finish(s); return; }
            set(s, 3, reg(s, 5));
        }
    }
    if (last) {
        compare(s, reg(s, 3));
        if (reg(s, 3)) { finish(s); return; }
    }
    compare(s, reg(s, 0));
    set(s, 3, reg(s, 0) ? 0 : reg(s, cursor)); finish(s);
}
void find_substring(State& s) {
    compare(s, reg(s, 4)); set(s, 5, reg(s, 3) - 1);
    if (!reg(s, 4)) { finish(s); return; }
    set(s, 6, read<1>(s, reg(s, 4), 0x1077ddac)); compare(s, reg(s, 6));
    if (!reg(s, 6)) { finish(s); return; }
    for (;;) {
        s.step(0x1077ddf8); next_byte(s, 0, 5, 0x1077ddf8); compare(s, reg(s, 0));
        if (!reg(s, 0)) { set(s, 3, 0); finish(s); return; }
        compare(s, reg(s, 0), reg(s, 6));
        if (reg(s, 0) != reg(s, 6)) continue;
        set(s, 7, reg(s, 5) - 1); set(s, 8, reg(s, 4) - 1);
        for (;;) {
            s.step(0x1077ddd0); next_byte(s, 0, 7, 0x1077ddd0); next_byte(s, 3, 8, 0x1077ddd4);
            compare(s, reg(s, 0), reg(s, 3));
            if (reg(s, 0) != reg(s, 3)) break;
            compare(s, reg(s, 0)); if (!reg(s, 0)) break;
        }
        compare(s, reg(s, 3));
        if (!reg(s, 3)) { set(s, 3, reg(s, 5)); finish(s); return; }
    }
}
void duplicate(State& s) {
    set(s, 0, s.r<4>(4128));
    write<4>(s, reg(s, 1) - 4, reg(s, 31), 0x107395f4);
    write<4>(s, reg(s, 1) - 8, reg(s, 30), 0x107395f8);
    set(s, 30, reg(s, 3)); write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x10739600);
    frame(s, 0x10739604); call(s, 0x1077dbd0, 0x1073960c);
    set(s, 3, reg(s, 3) + 1); call(s, 0x10775130, 0x10739614);
    set(s, 31, reg(s, 3)); compare(s, word(reg(s, 31)), 0, true);
    if (reg(s, 31)) { set(s, 4, reg(s, 30)); call(s, 0x1077dbf0, 0x10739624); }
    set(s, 3, reg(s, 31)); restore_link(s, 0x10739628);
    set(s, 31, read<4>(s, reg(s, 1) - 4, 0x10739634));
    set(s, 30, read<4>(s, reg(s, 1) - 8, 0x10739638)); finish(s);
}
void transform(State& s) {
    set(s, 4, read<4>(s, reg(s, 2) - 0x618c, 0x10739640));
    for (;;) {
        s.step(0x1073966c); set(s, 5, read<1>(s, reg(s, 3), 0x1073966c));
        signed_byte(s, 0, 5, true);
        if (!reg(s, 0)) { finish(s); return; }
        map_byte(s, 0, 5, 4, 0x10739660);
        write<1>(s, reg(s, 3), reg(s, 0), 0x10739664); set(s, 3, reg(s, 3) + 1);
    }
}
}
bool try_modern_string_core(State& s, std::uint32_t entry) {
    s.pc = entry;
    if (s.string_original_resume == entry) { s.string_original_resume = 0; return false; }
    const auto found = std::find(string_core_entries.begin(), string_core_entries.end(), entry);
    if (found == string_core_entries.end()) throw std::runtime_error("Unknown string core entry");
    const auto index = std::size_t(found - string_core_entries.begin());
    const bool original = s.string_original || s.string_entry_original[index];
    ++(original ? s.string_original_calls[index] : s.string_calls[index]);
    if (original && !s.string_observer) return false;
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (s.string_observer) s.string_observer->enter(s, entry, original);
    if (original) {
        s.string_original_resume = entry;
        try { run_until(s, caller); } catch (...) { s.string_original_resume = 0; throw; }
    } else {
        s.step(entry);
        switch (index) {
        case 0: mac_compare(s, complete); break;
        case 1: mac_compare(s, bounded); break;
        case 2: descriptor_compare(s); break;
        case 3: duplicate(s); break;
        case 4: transform(s); break;
        case 5: length(s); break;
        case 6: copy_bytes(s, false); break;
        case 7: copy_bounded(s); break;
        case 8: copy_bytes(s, true); break;
        case 9: bytes_compare(s, false); break;
        case 10: bytes_compare(s, true); break;
        case 11: find_byte(s, false); break;
        case 12: find_byte(s, true); break;
        case 13: find_substring(s); break;
        }
    }
    if (s.string_observer) s.string_observer->leave(s, entry, original);
    return true;
}
void reset_string_core_diagnostics(State& s) {
    s.string_original = false; s.string_entry_original.fill(false);
    s.string_calls.fill(0); s.string_original_calls.fill(0);
    s.string_original_resume = 0; s.string_observer = nullptr;
}
std::string string_core_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < string_core_entries.size(); ++i)
        out << "strings.entry=" << hex_address(string_core_entries[i]) << " mode="
            << (s.string_original || s.string_entry_original[i] ? "original" : "modern")
            << " modern=" << s.string_calls[i] << " original=" << s.string_original_calls[i] << '\n';
    return out.str();
}
}
