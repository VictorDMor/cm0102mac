#include "memory_primitives_adapter.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
namespace {
std::uint32_t reg(const State& s, unsigned n) { return std::uint32_t(s.r<4>(n * 4)); }
void set(State& s, unsigned n, std::uint32_t value) { s.wr<4>(n * 4, value); }
void compare(State& s, std::int64_t a, std::int64_t b = 0, bool record = false) {
    s.wr<1>(2304, (record ? s.r<1>(2304) & 0xf0 : 0) |
        (a < b ? 8 : a > b ? 4 : 2) | (s.r<1>(1024) & 1));
}
void record(State& s, std::uint32_t value) {
    compare(s, std::bit_cast<std::int32_t>(value), 0, true);
}
template<unsigned N> std::uint32_t read(State& s, std::uint32_t address, std::uint32_t pc) {
    s.pc = pc;
    return std::uint32_t(s.memory.read<N>(address));
}
template<unsigned N> void write(State& s, std::uint32_t address, std::uint32_t value, std::uint32_t pc) {
    s.pc = pc;
    s.memory.write<N>(address, value);
}
void load_byte(State& s, unsigned result, unsigned cursor, std::uint32_t delta, std::uint32_t pc) {
    const auto address = reg(s, cursor) + delta;
    set(s, result, read<1>(s, address, pc));
    set(s, cursor, address);
}
template<unsigned N> void store_next(State& s, unsigned cursor, std::uint32_t delta, std::uint32_t pc) {
    const auto address = reg(s, cursor) + delta;
    write<N>(s, address, reg(s, 7), pc);
    set(s, cursor, address);
}
bool decrement(State& s, unsigned counter) {
    const auto before = reg(s, counter);
    s.wr<1>(1027, before != 0);
    set(s, counter, before - 1);
    record(s, reg(s, counter));
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
void restore(State& s, std::uint32_t link_pc, std::uint32_t spill_pc) {
    set(s, 0, read<4>(s, reg(s, 1) + 72, link_pc));
    set(s, 1, reg(s, 1) + 64); s.wr<4>(4128, reg(s, 0));
    set(s, 31, read<4>(s, reg(s, 1) - 4, spill_pc)); finish(s);
}

void copy_block(State& s) {
    set(s, 0, s.r<4>(4128));
    write<4>(s, reg(s, 1) - 4, reg(s, 31), 0x10739ef4);
    set(s, 31, reg(s, 3)); set(s, 3, reg(s, 4));
    write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x10739f00);
    set(s, 4, reg(s, 31)); frame(s, 0x10739f08);
    // The generated mutable vector stub and existing Carbon bridge retain
    // their bulk source snapshot, overlap behavior and zero-count import.
    call(s, 0x1077fc28, 0x10739f10);
    set(s, 2, read<4>(s, reg(s, 1) + 20, 0x10739f10));
    set(s, 3, reg(s, 31)); restore(s, 0x10739f18, 0x10739f24);
}
void fill_wrapper(State& s) {
    write<4>(s, reg(s, 1) - 4, reg(s, 31), 0x10778300);
    set(s, 0, s.r<4>(4128)); set(s, 31, reg(s, 3));
    write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x1077830c);
    frame(s, 0x10778310); call(s, 0x107783e0, 0x10778318);
    set(s, 3, reg(s, 31)); restore(s, 0x1077831c, 0x10778328);
}
void find_byte(State& s, bool reverse) {
    set(s, 4, reg(s, 4) & 255);
    set(s, 3, reverse ? reg(s, 3) + reg(s, 5) : reg(s, 3) - 1);
    set(s, 5, reg(s, 5) + 1);
    const auto pc = reverse ? 0x10778370u : 0x10778340u;
    while (decrement(s, 5)) {
        s.step(pc); load_byte(s, 0, 3, reverse ? UINT32_MAX : 1, pc);
        compare(s, reg(s, 0), reg(s, 4));
        if (reg(s, 0) == reg(s, 4)) { finish(s); return; }
    }
    set(s, 3, 0); finish(s);
}
void compare_blocks(State& s) {
    set(s, 6, reg(s, 3) - 1); set(s, 7, reg(s, 4) - 1);
    set(s, 4, reg(s, 5) + 1);
    while (decrement(s, 4)) {
        s.step(0x107783a0); load_byte(s, 3, 6, 1, 0x107783a0);
        load_byte(s, 0, 7, 1, 0x107783a4);
        compare(s, reg(s, 3), reg(s, 0));
        if (reg(s, 3) != reg(s, 0)) {
            // The original decides from fresh bytes, even if these now compare
            // equal. Such equality still returns +1 on this divergence path.
            set(s, 4, read<1>(s, reg(s, 6), 0x107783b0)); set(s, 3, 1);
            set(s, 0, read<1>(s, reg(s, 7), 0x107783b8));
            compare(s, reg(s, 4), reg(s, 0));
            if (reg(s, 4) < reg(s, 0)) set(s, 3, UINT32_MAX);
            finish(s); return;
        }
    }
    set(s, 3, 0); finish(s);
}
void fill_block(State& s) {
    const bool large = reg(s, 5) >= 32;
    compare(s, reg(s, 5), 32);
    set(s, 4, reg(s, 4) & 255); set(s, 7, reg(s, 4)); set(s, 6, reg(s, 3) - 1);
    if (large) {
        // Align with individual bytes before the word groups. The leaf uses
        // r3 for counts and does not share the wrapper's destination return.
        set(s, 0, ~reg(s, 6)); set(s, 3, reg(s, 0) & 3); record(s, reg(s, 3));
        if (reg(s, 3)) {
            set(s, 5, reg(s, 5) - reg(s, 3));
            do {
                s.step(0x10778408); decrement(s, 3); store_next<1>(s, 6, 1, 0x1077840c);
            } while (reg(s, 3));
        }
        compare(s, reg(s, 7));
        if (reg(s, 7)) {
            set(s, 3, reg(s, 7) << 24); set(s, 0, reg(s, 7) << 16); set(s, 4, reg(s, 7) << 8);
            set(s, 0, reg(s, 3) | reg(s, 0)); set(s, 0, reg(s, 4) | reg(s, 0));
            set(s, 7, reg(s, 7) | reg(s, 0));
        }
        set(s, 3, reg(s, 5) >> 5); record(s, reg(s, 3)); set(s, 4, reg(s, 6) - 3);
        while (reg(s, 3)) {
            s.step(0x10778440);
            write<4>(s, reg(s, 4) + 4, reg(s, 7), 0x10778440);
            decrement(s, 3);
            for (unsigned offset = 8; offset <= 28; offset += 4)
                write<4>(s, reg(s, 4) + offset, reg(s, 7), 0x10778448 + offset - 8);
            store_next<4>(s, 4, 32, 0x10778460);
        }
        set(s, 3, (reg(s, 5) >> 2) & 7); record(s, reg(s, 3));
        while (reg(s, 3)) {
            s.step(0x10778470); decrement(s, 3); store_next<4>(s, 4, 4, 0x10778474);
        }
        set(s, 0, 3); set(s, 6, reg(s, 4) + 3); set(s, 5, reg(s, 5) & reg(s, 0));
    }
    compare(s, reg(s, 5));
    while (reg(s, 5)) {
        s.step(0x10778490); decrement(s, 5); store_next<1>(s, 6, 1, 0x10778494);
    }
    finish(s);
}
}
bool try_modern_memory_primitives(State& s, std::uint32_t entry) {
    s.pc = entry;
    if (s.memory_primitives_original_resume == entry) { s.memory_primitives_original_resume = 0; return false; }
    const auto found = std::find(memory_primitives_entries.begin(), memory_primitives_entries.end(), entry);
    if (found == memory_primitives_entries.end()) throw std::runtime_error("Unknown memory primitive entry");
    const auto index = std::size_t(found - memory_primitives_entries.begin());
    const bool original = s.memory_primitives_original || s.memory_primitives_entry_original[index];
    ++(original ? s.memory_primitives_original_calls[index] : s.memory_primitives_calls[index]);
    if (original && !s.memory_primitives_observer) return false;
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (s.memory_primitives_observer) s.memory_primitives_observer->enter(s, entry, original);
    if (original) {
        s.memory_primitives_original_resume = entry;
        try { run_until(s, caller); } catch (...) { s.memory_primitives_original_resume = 0; throw; }
    } else {
        s.step(entry);
        switch (index) {
        case 0: copy_block(s); break;
        case 1: fill_wrapper(s); break;
        case 2: find_byte(s, false); break;
        case 3: find_byte(s, true); break;
        case 4: compare_blocks(s); break;
        case 5: fill_block(s); break;
        }
    }
    if (s.memory_primitives_observer) s.memory_primitives_observer->leave(s, entry, original);
    return true;
}
void reset_memory_primitives_diagnostics(State& s) {
    s.memory_primitives_original = false; s.memory_primitives_entry_original.fill(false);
    s.memory_primitives_calls.fill(0); s.memory_primitives_original_calls.fill(0);
    s.memory_primitives_original_resume = 0; s.memory_primitives_observer = nullptr;
}
std::string memory_primitives_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < memory_primitives_entries.size(); ++i)
        out << "memory.entry=" << hex_address(memory_primitives_entries[i]) << " mode="
            << (s.memory_primitives_original || s.memory_primitives_entry_original[i] ? "original" : "modern")
            << " modern=" << s.memory_primitives_calls[i] << " original=" << s.memory_primitives_original_calls[i] << '\n';
    return out.str();
}
}
