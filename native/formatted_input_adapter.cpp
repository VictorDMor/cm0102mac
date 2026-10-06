#include "formatted_input_adapter.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
namespace {
std::uint32_t reg(const State& s, unsigned n) { return std::uint32_t(s.r<4>(n * 4)); }
void set(State& s, unsigned n, std::uint32_t v) { s.wr<4>(n * 4, v); }
std::int32_t word(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
std::uint32_t byte(std::uint32_t v) { return std::uint32_t(signed_value<1>(v)); }
void compare(State& s, std::int64_t a, std::int64_t b = 0, bool record = false) {
    s.wr<1>(2304, (record ? s.r<1>(2304) & 0xf0 : 0) |
        (a < b ? 8 : a > b ? 4 : 2) | (s.r<1>(1024) & 1));
}
template<unsigned N> std::uint64_t read(State& s, std::uint32_t address, std::uint32_t pc) {
    s.pc = pc; return s.memory.read<N>(address);
}
template<unsigned N> void write(State& s, std::uint32_t address, std::uint64_t value, std::uint32_t pc) {
    s.pc = pc; s.memory.write<N>(address, value);
}
void call(State& s, std::uint32_t entry, std::uint32_t link) {
    s.wr<4>(2440, reg(s, 2)); s.wr<4>(4128, link); s.pc = entry; run_until(s, link);
}
void frame(State& s, unsigned size, std::uint32_t pc) {
    const auto previous = reg(s, 1); write<4>(s, previous - size, previous, pc); set(s, 1, previous - size);
}
void finish(State& s) { s.pc = std::uint32_t(s.r<4>(4128)); }
void nonvolatile_words(State& s, unsigned first, std::uint32_t pc, bool restore) {
    s.wr<4>(2432, reg(s, 1) - (32 - first) * 4);
    for (unsigned n = first; n < 32; ++n) {
        const auto address = std::uint32_t(s.r<4>(2432));
        if (restore) set(s, n, read<4>(s, address, pc));
        else write<4>(s, address, reg(s, n), pc);
        s.wr<4>(2432, address + 4);
    }
}
void restore_link(State& s, std::uint32_t pc) {
    set(s, 0, read<4>(s, reg(s, 1) + 72, pc));
    set(s, 1, reg(s, 1) + 64); s.wr<4>(4128, reg(s, 0));
}

void string_callback(State& s) {
    compare(s, word(reg(s, 5)), 1);
    if (reg(s, 5) == 1) {
        set(s, 0, read<4>(s, reg(s, 3) + 4, 0x1077d9b4)); compare(s, word(reg(s, 0)));
        if (!reg(s, 0)) {
            set(s, 5, read<4>(s, reg(s, 3), 0x1077d9c0)); set(s, 0, reg(s, 5) - 1);
            write<4>(s, reg(s, 3), reg(s, 0), 0x1077d9c8);
        } else { set(s, 0, 0); write<4>(s, reg(s, 3) + 4, 0, 0x1077d9d4); }
        set(s, 3, reg(s, 4));
    } else if (word(reg(s, 5)) < 1) {
        compare(s, word(reg(s, 5)));
        if (word(reg(s, 5)) >= 0) {
            set(s, 4, read<4>(s, reg(s, 3), 0x1077d984));
            set(s, 5, read<1>(s, reg(s, 4), 0x1077d988)); set(s, 0, byte(reg(s, 5)));
            compare(s, word(reg(s, 0)), 0, true);
            if (!reg(s, 0)) {
                set(s, 0, 1); write<4>(s, reg(s, 3) + 4, 1, 0x1077d998); set(s, 3, UINT32_MAX);
            } else {
                set(s, 0, reg(s, 4) + 1); write<4>(s, reg(s, 3), reg(s, 0), 0x1077d9a8);
                set(s, 3, reg(s, 5));
            }
        } else set(s, 3, 0);
    } else {
        compare(s, word(reg(s, 5)), 3);
        set(s, 3, word(reg(s, 5)) < 3 ? read<4>(s, reg(s, 3) + 4, 0x1077d9e0) : 0);
    }
    finish(s);
}
void stream_callback(State& s) {
    set(s, 0, s.r<4>(4128)); compare(s, word(reg(s, 5)), 1);
    write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x1077d8d8); set(s, 6, reg(s, 3)); frame(s, 64, 0x1077d8e0);
    if (reg(s, 5) == 1) {
        set(s, 3, reg(s, 4)); set(s, 4, reg(s, 6)); call(s, 0x10775d80, 0x1077d918);
    } else if (word(reg(s, 5)) < 1) {
        compare(s, word(reg(s, 5)));
        if (word(reg(s, 5)) >= 0) call(s, 0x10775c10, 0x1077d908);
        else set(s, 3, 0);
    } else {
        compare(s, word(reg(s, 5)), 3);
        if (word(reg(s, 5)) >= 3) set(s, 3, 0);
        else {
            set(s, 0, read<1>(s, reg(s, 6) + 13, 0x1077d91c)); set(s, 3, 0); compare(s, reg(s, 0));
            if (!reg(s, 0)) { set(s, 0, read<1>(s, reg(s, 6) + 12, 0x1077d92c)); compare(s, reg(s, 0)); }
            if (reg(s, 0)) set(s, 3, 1);
        }
    }
    restore_link(s, 0x1077d944); finish(s);
}
void string_vector_facade(State& s) {
    set(s, 0, s.r<4>(4128)); compare(s, reg(s, 3));
    write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x1077da78); set(s, 6, reg(s, 5)); frame(s, 64, 0x1077da80);
    write<4>(s, reg(s, 1) + 56, reg(s, 3), 0x1077da84);
    bool empty = !reg(s, 3);
    if (!empty) { set(s, 0, read<1>(s, reg(s, 3), 0x1077da8c)); set(s, 0, byte(reg(s, 0))); compare(s, word(reg(s, 0)), 0, true); empty = !reg(s, 0); }
    if (empty) set(s, 3, UINT32_MAX);
    else {
        set(s, 0, 0); set(s, 3, read<4>(s, reg(s, 2) - 0x5a44, 0x1077daa4));
        write<4>(s, reg(s, 1) + 60, reg(s, 0), 0x1077daa8);
        set(s, 5, reg(s, 4)); set(s, 4, reg(s, 1) + 56); call(s, 0x1077bf20, 0x1077dab8);
    }
    restore_link(s, 0x1077dab8); finish(s);
}
void string_variadic_facade(State& s) {
    set(s, 0, s.r<4>(4128)); write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x1077dad4); frame(s, 64, 0x1077dad8);
    write<4>(s, reg(s, 1) + 92, reg(s, 4), 0x1077dadc);
    write<4>(s, reg(s, 1) + 96, reg(s, 5), 0x1077dae0); set(s, 5, reg(s, 1) + 96);
    for (unsigned n = 6; n <= 10; ++n) write<4>(s, reg(s, 1) + 76 + n * 4, reg(s, n), 0x1077dae8 + (n - 6) * 4);
    set(s, 4, read<4>(s, reg(s, 1) + 92, 0x1077dafc)); call(s, 0x1077da70, 0x1077db04);
    restore_link(s, 0x1077db04); finish(s);
}
void stream_variadic_facade(State& s) {
    write<4>(s, reg(s, 1) - 4, reg(s, 31), 0x1077d9f0); set(s, 0, s.r<4>(4128)); set(s, 31, reg(s, 3));
    compare(s, word(reg(s, 31)), 0, true); write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x1077d9fc); frame(s, 64, 0x1077da00);
    for (unsigned n = 4; n <= 10; ++n) write<4>(s, reg(s, 1) + 76 + n * 4, reg(s, n), 0x1077da04 + (n - 4) * 4);
    if (!reg(s, 31)) set(s, 3, UINT32_MAX);
    else {
        set(s, 4, UINT32_MAX); call(s, 0x1077fa50, 0x1077da34); compare(s, word(reg(s, 3)));
        if (word(reg(s, 3)) >= 0) set(s, 3, UINT32_MAX);
        else {
            set(s, 3, read<4>(s, reg(s, 2) - 0x5a40, 0x1077da44)); set(s, 4, reg(s, 31));
            set(s, 5, read<4>(s, reg(s, 1) + 92, 0x1077da4c)); set(s, 6, reg(s, 1) + 96); call(s, 0x1077bf20, 0x1077da58);
        }
    }
    set(s, 0, read<4>(s, reg(s, 1) + 72, 0x1077da58)); set(s, 1, reg(s, 1) + 64);
    set(s, 31, read<4>(s, reg(s, 1) - 4, 0x1077da60)); s.wr<4>(4128, reg(s, 0)); finish(s);
}

// A format record lives in guest memory throughout parsing. Template fields,
// output, argument cursor and frame may alias; never snapshot the record.
class FormatParser {
    // Live ABI: r27 argument-cursor pointer, r28 output record, r30 format
    // cursor, r31 ctype table. The local record starts at frame offset 56.
    State& s;
    std::uint32_t get(unsigned n) const { return reg(s, n); }
    void put(unsigned n, std::uint32_t v) { set(s, n, v); }
    std::uint32_t local(unsigned off) const { return get(1) + off; }
    template<unsigned N> void load(unsigned n, unsigned off, std::uint32_t pc) { put(n, read<N>(s, local(off), pc)); }
    template<unsigned N> void store(unsigned off, unsigned n, std::uint32_t pc) { write<N>(s, local(off), get(n), pc); }
    void next(unsigned n, unsigned increment, std::uint32_t pc) {
        const auto address = get(30) + increment; put(n, read<1>(s, address, pc)); put(30, address); put(n, byte(get(n)));
    }
    void invalid(std::uint32_t pc) { put(0, 255); store<1>(59, 0, pc); }
    void initialize() {
        nonvolatile_words(s, 21, 0x1077b7a0, false); put(0, s.r<4>(4128));
        put(31, read<4>(s, get(2) - 0x3b40, 0x1077b7a8)); put(27, get(4)); put(28, get(5)); put(30, get(3) + 1); put(29, 1);
        write<4>(s, get(1) + 8, get(0), 0x1077b7bc); frame(s, 176, 0x1077b7c0);
        put(4, read<1>(s, get(3) + 1, 0x1077b7c4)); put(5, read<4>(s, get(31), 0x1077b7c8)); put(4, byte(get(4)));
        put(21, read<4>(s, get(31) + 4, 0x1077b7d0)); put(22, read<4>(s, get(31) + 8, 0x1077b7d4)); compare(s, word(get(4)), '%');
        constexpr std::array<unsigned, 11> slots{23,24,25,26,12,11,10,9,8,7,6};
        for (unsigned i = 0; i < slots.size(); ++i) put(slots[i], read<4>(s, get(31) + 12 + i * 4, 0x1077b7dc + i * 4));
        put(3, read<2>(s, get(31) + 56, 0x1077b808)); put(0, read<2>(s, get(31) + 58, 0x1077b80c));
        store<4>(56, 5, 0x1077b810); put(31, read<4>(s, get(2) - 0x5a48, 0x1077b814));
        constexpr std::array<unsigned, 13> saved{21,22,23,24,25,26,12,11,10,9,8,7,6};
        for (unsigned i = 0; i < saved.size(); ++i) store<4>(60 + i * 4, saved[i], 0x1077b818 + i * 4);
        store<2>(112, 3, 0x1077b84c); store<2>(114, 0, 0x1077b850);
    }
    bool field_width() {
        put(0, get(4) & 255); put(0, read<1>(s, get(31) + get(0), 0x1077b8cc)); put(0, get(0) & 16); compare(s, word(get(0)), 0, true);
        if (!get(0)) return true;
        put(0, 0); store<4>(60, 0, 0x1077b8dc);
        do {
            s.step(0x1077b8e0); load<4>(0, 60, 0x1077b8e0); put(0, get(0) * 10); put(3, get(4) + get(0));
            next(4, 1, 0x1077b8ec); put(7, get(3) - '0'); store<4>(60, 7, 0x1077b8f8);
            put(0, get(4) & 255); put(0, read<1>(s, get(31) + get(0), 0x1077b900)); put(0, get(0) & 16); compare(s, word(get(0)), 0, true);
        } while (get(0));
        compare(s, word(get(7)));
        if (!get(7)) return false;
        put(0, 1); store<1>(57, 0, 0x1077b9a4); return true;
    }
    void modifiers() {
        compare(s, word(get(4)), 'v');
        if (get(4) == 'v') { next(4, 1, 0x1077b9b0); put(0, 1); store<4>(64, 0, 0x1077b9b8); }
        compare(s, word(get(4)), 'h'); put(5, 1);
        if (get(4) == 'h') {
            put(0, read<1>(s, get(30) + 1, 0x1077b9e8)); put(3, 2); store<1>(58, 3, 0x1077b9f0); put(3, byte(get(0))); compare(s, word(get(3)), 'h');
            if (get(3) == 'h') { put(0, 1); store<1>(58, 0, 0x1077ba04); put(4, get(3)); put(30, get(30) + 1); }
            else { put(0, 8); store<4>(68, 0, 0x1077ba18); }
        } else {
            if (word(get(4)) >= 'h') compare(s, word(get(4)), 'l'); else compare(s, word(get(4)), 'L');
            if (get(4) == 'l') {
                put(0, read<1>(s, get(30) + 1, 0x1077ba20)); put(3, 3); store<1>(58, 3, 0x1077ba28); put(3, byte(get(0))); compare(s, word(get(3)), 'l');
                if (get(3) == 'l') { put(0, 4); store<1>(58, 0, 0x1077ba3c); put(4, get(3)); put(30, get(30) + 1); }
                else { put(0, 4); store<4>(68, 0, 0x1077ba50); }
            } else if (get(4) == 'L') { put(0, 6); store<1>(58, 0, 0x1077ba5c); }
            else { put(0, 16); store<4>(68, 0, 0x1077ba68); put(5, 0); }
        }
        compare(s, word(get(5))); if (get(5)) next(4, 1, 0x1077ba78);
        load<4>(0, 64, 0x1077ba80); compare(s, word(get(0)));
        if (!get(0)) {
            compare(s, word(get(4)), 'v');
            if (get(4) == 'v') {
                load<4>(0, 68, 0x1077ba94); compare(s, word(get(0)));
                if (get(0)) { next(4, 1, 0x1077baa0); put(0, 1); store<4>(64, 0, 0x1077baa8); }
            }
        }
    }
    void bitmap(bool invert) {
        put(0, 4); s.wr<4>(4132, get(0)); put(3, local(84));
        do {
            s.step(invert ? 0x1077bd1c : 0x1077bbb8);
            for (unsigned i = 0; i < 8; ++i) {
                if (invert) { put(0, read<1>(s, get(3) + i, 0x1077bd1c + i * 12)); put(0, ~get(0)); }
                else put(0, 255);
                write<1>(s, get(3) + i, get(0), invert ? 0x1077bd24 + i * 12 : 0x1077bbb8 + i * 4);
            }
            put(3, get(3) + 8); s.wr<4>(4132, s.r<4>(4132) - 1);
        } while (s.r<4>(4132));
    }
    void scanset() {
        load<1>(0, 58, 0x1077bbf4); compare(s, get(0), 3);
        if (get(0) == 3) { put(0, 7); store<1>(58, 0, 0x1077bc04); }
        else { compare(s, get(0)); if (get(0)) invalid(0x1077bc18); }
        next(8, 1, 0x1077bc1c); put(9, 0); compare(s, word(get(8)), '^');
        if (get(8) == '^') { next(8, 1, 0x1077bc30); put(9, 1); }
        compare(s, word(get(8)), ']');
        if (get(8) == ']') { load<1>(0, 95, 0x1077bc44); next(8, 1, 0x1077bc48); put(0, get(0) | 32); store<1>(95, 0, 0x1077bc50); }
        put(7, local(56));
        for (;;) {
            compare(s, word(get(8))); if (!get(8)) break;
            compare(s, word(get(8)), ']'); if (get(8) == ']') break;
            s.step(0x1077bc60);
            put(3, (get(8) & 255) >> 3); put(0, read<1>(s, get(30) + 1, 0x1077bc64));
            put(5, get(3) + 28); put(3, get(8) & 7); put(4, read<1>(s, get(7) + get(5), 0x1077bc70)); put(6, 1);
            put(3, shift_left<4>(get(6), get(3))); put(3, get(4) | get(3)); compare(s, word(get(0)), '-'); write<1>(s, get(7) + get(5), get(3), 0x1077bc84);
            bool range = get(0) == '-';
            if (range) { put(10, read<1>(s, get(30) + 2, 0x1077bc8c)); put(10, byte(get(10))); compare(s, word(get(10)), 0, true); range = get(10) != 0;
                if (range) { compare(s, word(get(10)), ']'); range = get(10) != ']'; } }
            if (range) {
                put(5, local(56));
                for (;;) {
                    put(8, get(8) + 1); compare(s, word(get(8)), word(get(10))); if (word(get(8)) > word(get(10))) break;
                    s.step(0x1077bca8); put(3, (get(8) & 255) >> 3); put(4, get(3) + 28); put(0, get(8) & 7);
                    put(3, read<1>(s, get(5) + get(4), 0x1077bcb4)); put(0, shift_left<4>(get(6), get(0))); put(0, get(3) | get(0)); write<1>(s, get(5) + get(4), get(0), 0x1077bcc0);
                }
                next(8, 3, 0x1077bcd0);
            } else next(8, 1, 0x1077bcdc);
        }
        compare(s, word(get(8))); if (!get(8)) { invalid(0x1077bd00); return; }
        compare(s, word(get(9))); if (get(9)) bitmap(true);
    }
    // @ consumes a delimiter string; any other unrecognized prefix is a
    // single-byte delimiter. Both restart width/modifier parsing once.
    bool separator_prefix() {
        load<4>(0, 64, 0x1077bd88); compare(s, word(get(0)));
        if (get(0)) { invalid(0x1077be84); return false; }
        compare(s, word(get(29))); if (!get(29)) { invalid(0x1077be84); return false; }
        compare(s, word(get(4)), '@'); put(29, 0);
        if (get(4) == '@') {
            put(3, read<4>(s, get(27), 0x1077bda8)); put(3, get(3) + 4); write<4>(s, get(27), get(3), 0x1077bdb0);
            put(3, read<4>(s, get(3) - 4, 0x1077bdb4)); store<4>(72, 3, 0x1077bdb8); call(s, 0x1077dbd0, 0x1077bdc0); store<4>(80, 3, 0x1077bdc0);
            for (;;) {
                load<4>(0, 80, 0x1077bde4); compare(s, word(get(0))); if (word(get(0)) <= 0) break;
                load<4>(3, 72, 0x1077bdf0); put(0, read<1>(s, get(3), 0x1077bdf4)); compare(s, word(get(0)), ' '); if (get(0) != ' ') break;
                s.step(0x1077bdcc); load<4>(4, 72, 0x1077bdcc); load<4>(3, 80, 0x1077bdd0); put(4, get(4) + 1); put(0, get(3) - 1); store<4>(72, 4, 0x1077bddc); store<4>(80, 0, 0x1077bde0);
            }
            load<4>(3, 72, 0x1077be00);
            for (;;) {
                load<4>(4, 80, 0x1077be18); compare(s, word(get(4))); if (word(get(4)) <= 0) break;
                put(0, get(4) - 1); put(0, read<1>(s, get(3) + get(0), 0x1077be28)); put(0, byte(get(0))); compare(s, word(get(0)), ' '); if (get(0) != ' ') break;
                s.step(0x1077be0c); load<4>(4, 80, 0x1077be0c); put(0, get(4) - 1); store<4>(80, 0, 0x1077be14);
            }
        } else { put(0, 1); store<4>(72, 30, 0x1077be40); store<4>(80, 0, 0x1077be44); }
        load<1>(0, 56, 0x1077be48); load<4>(3, 72, 0x1077be4c); next(4, 1, 0x1077be50); compare(s, get(0)); store<4>(76, 3, 0x1077be58);
        if (!get(0)) { compare(s, word(get(4)), '*'); if (get(4) == '*') { next(4, 1, 0x1077be6c); put(0, 1); store<1>(56, 0, 0x1077be74); } }
        return true;
    }
    void output_record() {
        load<4>(4, 56, 0x1077be88); put(3, get(30) + 1); load<4>(0, 60, 0x1077be90);
        write<4>(s, get(28), get(4), 0x1077be94);
        for (unsigned i = 0; i < 12; ++i) {
            const unsigned current = i % 2 ? 4 : 0, next_reg = i % 2 ? 0 : 4;
            load<4>(next_reg, 64 + i * 4, 0x1077be98 + i * 8);
            write<4>(s, get(28) + 4 + i * 4, get(current), 0x1077be9c + i * 8);
        }
        load<2>(4, 112, 0x1077bef8); write<4>(s, get(28) + 52, get(0), 0x1077befc);
        load<2>(0, 114, 0x1077bf00); write<2>(s, get(28) + 56, get(4), 0x1077bf04); write<2>(s, get(28) + 58, get(0), 0x1077bf08);
    }
public:
    explicit FormatParser(State& state) : s(state) {}
    void parse() {
        const auto caller = std::uint32_t(s.r<4>(4128));
        initialize();
        if (get(4) == '%') {
            store<1>(59, 4, 0x1077b858); put(3, get(30) + 1); load<2>(4, 112, 0x1077b860); load<4>(5, 56, 0x1077b864); load<2>(0, 114, 0x1077b868);
            write<4>(s, get(28), get(5), 0x1077b86c);
            constexpr std::array<unsigned, 13> slots{21,22,23,24,25,26,12,11,10,9,8,7,6};
            for (unsigned i = 0; i < slots.size(); ++i) write<4>(s, get(28) + 4 + i * 4, get(slots[i]), 0x1077b870 + i * 4);
            write<2>(s, get(28) + 56, get(4), 0x1077b8a4); write<2>(s, get(28) + 58, get(0), 0x1077b8a8);
        } else {
            compare(s, word(get(4)), '*'); if (get(4) == '*') { next(4, 1, 0x1077b8b8); put(0, 1); store<1>(56, 0, 0x1077b8c0); }
            for (;;) {
                if (!field_width()) { zero_width_record(); break; }
                modifiers(); put(0, get(4) - 'E'); store<1>(59, 4, 0x1077bab4); compare(s, get(0), 51);
                std::uint32_t destination = 0x1077bd88;
                if (get(0) <= 51) {
                    put(3, read<4>(s, get(2) - 0x3b44, 0x1077bac0)); put(0, get(0) * 4);
                    put(3, read<4>(s, get(3) + get(0), 0x1077bac8)); s.wr<4>(4132, get(3)); destination = get(3);
                }
                bool restart = false;
                switch (destination) {
                case 0x1077bad4: load<1>(0, 58, 0x1077bad4); compare(s, get(0), 6); if (get(0) == 6) invalid(0x1077bae4); break;
                case 0x1077baec:
                    load<1>(0, 58, 0x1077baec); compare(s, get(0), 1);
                    if (get(0) != 1) { compare(s, get(0), 2); if (get(0) != 2) compare(s, get(0), 4); }
                    if (get(0) == 1 || get(0) == 2 || get(0) == 4) invalid(0x1077bb0c);
                    else { compare(s, get(0), 3); if (get(0) == 3) { put(0, 5); store<1>(58, 0, 0x1077bb20); } } break;
                case 0x1077bb28:
                    load<4>(0, 64, 0x1077bb28); compare(s, word(get(0))); if (!get(0)) { put(0, 3); store<1>(58, 0, 0x1077bb38); }
                    put(0, 'x'); store<1>(59, 0, 0x1077bb40); break;
                case 0x1077bb48:
                    load<4>(0, 64, 0x1077bb48); compare(s, word(get(0))); if (!get(0)) {
                        load<1>(0, 58, 0x1077bb54); compare(s, get(0), 3); if (get(0) == 3) { put(0, 7); store<1>(58, 0, 0x1077bb64); }
                        else { compare(s, get(0)); if (get(0)) invalid(0x1077bb78); }
                    } break;
                case 0x1077bb80:
                    load<1>(0, 58, 0x1077bb80); compare(s, get(0), 3); if (get(0) == 3) { put(0, 7); store<1>(58, 0, 0x1077bb90); }
                    else { compare(s, get(0)); if (get(0)) invalid(0x1077bba4); }
                    bitmap(false); put(3, 193); put(0, 254); store<1>(85, 3, 0x1077bbe8); store<1>(88, 0, 0x1077bbec); break;
                case 0x1077bbf4: scanset(); break;
                case 0x1077bd88: restart = separator_prefix(); break;
                case 0x1077be88: break;
                default:
                    ++s.formatted_input_dispatch_escapes; s.pc = destination; run_until(s, caller); return;
                }
                if (restart) continue;
                output_record(); break;
            }
        }
        put(0, read<4>(s, get(1) + 184, 0x1077bf0c)); put(1, get(1) + 176); nonvolatile_words(s, 21, 0x1077bf14, true); s.wr<4>(4128, get(0)); finish(s);
    }
private:
    void zero_width_record() {
        put(0, 255); load<4>(6, 64, 0x1077b918); store<1>(59, 0, 0x1077b91c); put(3, get(30) + 1);
        load<4>(5, 68, 0x1077b924); load<4>(0, 56, 0x1077b928); load<4>(4, 72, 0x1077b92c);
        write<4>(s, get(28), get(0), 0x1077b930); load<4>(0, 76, 0x1077b934); write<4>(s, get(28) + 4, get(7), 0x1077b938);
        constexpr std::array<unsigned, 11> loads{7,6,5,4,0,7,6,5,4,0,7};
        constexpr std::array<unsigned, 11> stores{6,5,4,0,7,6,5,4,0,7,6};
        for (unsigned i = 0; i < 8; ++i) { load<4>(loads[i], 80 + i * 4, 0x1077b93c + i * 8); write<4>(s, get(28) + 8 + i * 4, get(stores[i]), 0x1077b940 + i * 8); }
        load<2>(4, 112, 0x1077b97c); write<4>(s, get(28) + 40, get(0), 0x1077b980); load<2>(0, 114, 0x1077b984);
        write<4>(s, get(28) + 44, get(7), 0x1077b988); write<4>(s, get(28) + 48, get(6), 0x1077b98c); write<4>(s, get(28) + 52, get(5), 0x1077b990);
        write<2>(s, get(28) + 56, get(4), 0x1077b994); write<2>(s, get(28) + 58, get(0), 0x1077b998);
    }
};

enum class Conversion { stop, signed_integer, unsigned_integer, floating, characters, percent, text, count };
class Scanner {
    // Live ABI: r19 ctype, r22 descriptor, r23 context, r31 format cursor;
    // r17 consumed bytes, r16 assignments, r25 packed lane, r26 destination.
    // Frame offset 156 counts completed conversions separately from r16.
    State& s;
    std::uint32_t get(unsigned n) const { return reg(s, n); }
    void put(unsigned n, std::uint32_t v) { set(s, n, v); }
    std::uint32_t local(unsigned off) const { return get(1) + off; }
    template<unsigned N> void load(unsigned n, unsigned off, std::uint32_t pc) { put(n, read<N>(s, local(off), pc)); }
    template<unsigned N> void store(unsigned off, unsigned n, std::uint32_t pc) { write<N>(s, local(off), get(n), pc); }
    void callback(unsigned mode, std::uint32_t link, bool clear_character = true) {
        put(3, get(23)); put(12, get(22)); if (clear_character) put(4, 0); put(5, mode);
        call(s, 0x10781320, link); put(2, read<4>(s, local(20), link));
    }
    void unread(std::uint32_t load_pc, std::uint32_t link) {
        load<1>(4, 56, load_pc); put(3, get(23)); put(12, get(22)); put(4, byte(get(4))); put(5, 1);
        call(s, 0x10781320, link); put(2, read<4>(s, local(20), link));
    }
    void classify(unsigned source, std::uint32_t pc) {
        put(0, get(source) & 255); put(0, read<1>(s, get(19) + get(0), pc)); put(0, get(0) & 6); compare(s, word(get(0)), 0, true);
    }
    bool format_whitespace() {
        classify(14, 0x1077bf5c); if (!get(0)) return false;
        do {
            s.step(0x1077bf68); const auto address = get(31) + 1; put(0, read<1>(s, address, 0x1077bf68)); put(31, address);
            put(0, read<1>(s, get(19) + get(0), 0x1077bf6c)); put(0, get(0) & 6); compare(s, word(get(0)), 0, true);
        } while (get(0));
        for (;;) {
            callback(0, 0x1077bf98); put(0, get(3) & 255); store<1>(56, 3, 0x1077bfa0);
            put(0, read<1>(s, get(19) + get(0), 0x1077bfa4)); put(0, get(0) & 6); compare(s, word(get(0)), 0, true);
            if (!get(0)) break;
            put(17, get(17) + 1); s.step(0x1077bf80);
        }
        unread(0x1077bfb0, 0x1077bfc8); return true;
    }
    bool literal() {
        callback(0, 0x1077bfec); put(4, byte(get(3))); store<1>(56, 3, 0x1077bff4); put(0, get(14) & 255); compare(s, word(get(0)), word(get(4)));
        if (get(0) != get(4)) { unread(0x1077c004, 0x1077c01c); return false; }
        put(17, get(17) + 1); put(31, get(31) + 1); return true;
    }
    bool parse_conversion() {
        put(3, get(31)); put(4, local(276)); put(5, local(88)); call(s, 0x1077b7a0, 0x1077c040);
        load<4>(0, 96, 0x1077c040); put(31, get(3)); compare(s, word(get(0)));
        if (get(0)) {
            put(0, 0); store<4>(72, 0, 0x1077c054); put(25, 0);
            store<4>(76, 0, 0x1077c05c); store<4>(80, 0, 0x1077c060); store<4>(84, 0, 0x1077c064);
            for (;;) {
                callback(0, 0x1077c088); put(0, get(3) & 255); store<1>(56, 3, 0x1077c090);
                put(0, read<1>(s, get(19) + get(0), 0x1077c094)); put(0, get(0) & 6); compare(s, word(get(0)), 0, true);
                if (!get(0)) break;
                put(17, get(17) + 1); s.step(0x1077c070);
            }
            unread(0x1077c0a0, 0x1077c0b8);
        } else {
            load<1>(0, 88, 0x1077c0c0); compare(s, get(0));
            bool argument = !get(0);
            if (argument) { load<1>(0, 91, 0x1077c0cc); compare(s, get(0), '%'); argument = get(0) != '%'; }
            if (argument) {
                load<4>(3, 276, 0x1077c0d8); put(3, get(3) + 4); store<4>(276, 3, 0x1077c0e0); put(26, read<4>(s, get(3) - 4, 0x1077c0e4));
            } else put(26, 0);
        }
        load<1>(0, 91, 0x1077c0f0); compare(s, get(0), 'n');
        if (get(0) != 'n') { callback(2, 0x1077c110); compare(s, word(get(3))); if (get(3)) return false; }
        return true;
    }
    Conversion conversion() {
        load<1>(0, 91, 0x1077c11c); compare(s, word(get(0)), 'd'); if (get(0) == 'd') { put(18, 10); return Conversion::signed_integer; }
        if (word(get(0)) < 'd') {
            compare(s, word(get(0)), 'X'); if (get(0) == 'X') { put(18, 16); return Conversion::unsigned_integer; }
            if (word(get(0)) < 'X') {
                compare(s, word(get(0)), 'E'); if (get(0) == 'E') return Conversion::floating;
                if (word(get(0)) < 'E') { compare(s, word(get(0)), '%'); return get(0) == '%' ? Conversion::percent : Conversion::stop; }
                compare(s, word(get(0)), 'G'); return get(0) == 'G' ? Conversion::floating : Conversion::stop;
            }
            compare(s, word(get(0)), 'a'); if (get(0) == 'a') return Conversion::floating;
            if (word(get(0)) < 'a') { compare(s, word(get(0)), '['); return get(0) == '[' ? Conversion::text : Conversion::stop; }
            compare(s, word(get(0)), 'c'); return word(get(0)) >= 'c' ? Conversion::characters : Conversion::stop;
        }
        compare(s, word(get(0)), 's'); if (get(0) == 's') return Conversion::text;
        if (word(get(0)) < 's') {
            compare(s, word(get(0)), 'n'); if (get(0) == 'n') return Conversion::count;
            if (word(get(0)) < 'n') {
                compare(s, word(get(0)), 'i'); if (get(0) == 'i') { put(18, 0); return Conversion::signed_integer; }
                if (word(get(0)) >= 'i') return Conversion::stop;
                compare(s, word(get(0)), 'h'); return word(get(0)) >= 'h' ? Conversion::stop : Conversion::floating;
            }
            compare(s, word(get(0)), 'p'); if (word(get(0)) >= 'p') return Conversion::stop;
            put(18, 8); return Conversion::unsigned_integer;
        }
        compare(s, word(get(0)), 'x'); if (get(0) == 'x') { put(18, 16); return Conversion::unsigned_integer; }
        if (word(get(0)) < 'x') { compare(s, word(get(0)), 'u'); if (get(0) == 'u') { put(18, 10); return Conversion::unsigned_integer; } }
        else compare(s, word(get(0)), 255);
        return Conversion::stop;
    }
    void progress(std::uint32_t pc) { load<4>(3, 156, pc); put(3, get(3) + 1); store<4>(156, 3, pc + 8); }
    // Repeated values are packed in the 16-byte guest accumulator. Every
    // delimiter/lookahead access remains live, including the Mac mode-1 loop.
    bool repeat_separator(std::uint32_t start, unsigned equal_reg, unsigned cursor_reg, bool gap = false) {
        load<4>(0, 112, start); put(0, std::countl_zero(get(0))); put(equal_reg, get(0) >> 5); compare(s, word(get(equal_reg)), 0, true);
        const auto skip = start + 24 + (gap ? 4 : 0);
        if (!get(equal_reg)) {
            put(cursor_reg, 0);
            for (;;) {
                load<1>(0, 56, skip + 32); put(0, read<1>(s, get(19) + get(0), skip + 36)); put(0, get(0) & 6); compare(s, word(get(0)), 0, true);
                if (!get(0)) break;
                s.step(skip); callback(1, skip + 20); store<1>(56, 3, skip + 24); put(17, get(17) + 1);
            }
            for (;;) {
                load<1>(4, 56, skip + 132); put(0, byte(get(4))); compare(s, word(get(0)), -1); if (get(0) == UINT32_MAX) break;
                s.step(skip + 52); load<4>(3, 104, skip + 52); put(4, byte(get(4)));
                put(0, read<1>(s, get(3) + get(cursor_reg), skip + 60)); put(0, byte(get(0))); put(0, get(0) - get(4));
                put(0, std::countl_zero(get(0))); put(equal_reg, get(0) >> 5); compare(s, word(get(equal_reg)), 0, true); if (!get(equal_reg)) break;
                callback(0, skip + 104); load<4>(0, 112, skip + 108); put(cursor_reg, get(cursor_reg) + 1); store<1>(56, 3, skip + 116); put(17, get(17) + 1);
                compare(s, word(get(cursor_reg)), word(get(0))); if (word(get(cursor_reg)) >= word(get(0))) break;
            }
        }
        compare(s, word(get(equal_reg))); if (!get(equal_reg)) return false;
        load<1>(0, 56, skip + 156); put(0, byte(get(0))); compare(s, word(get(0)), -1); if (get(0) == UINT32_MAX) return false;
        for (;;) {
            load<1>(4, 56, skip + 208); put(4, byte(get(4))); classify(4, skip + 220); if (!get(0)) break;
            s.step(skip + 176); callback(0, skip + 196); store<1>(56, 3, skip + 200); put(17, get(17) + 1);
        }
        callback(1, skip + 248, false); return true;
    }
    void packed_assignment(std::uint32_t pc) {
        load<4>(3, 276, pc); put(16, get(16) + 1); load<4>(5, 72, pc + 8); put(6, get(3) + 4);
        load<4>(4, 76, pc + 16); store<4>(276, 6, pc + 20); load<4>(3, 80, pc + 24);
        put(6, read<4>(s, get(6) - 4, pc + 28)); load<4>(0, 84, pc + 32);
        write<4>(s, get(6), get(5), pc + 36); write<4>(s, get(6) + 4, get(4), pc + 40);
        write<4>(s, get(6) + 8, get(3), pc + 44); write<4>(s, get(6) + 12, get(0), pc + 48);
    }
    bool integer(bool signed_conversion) {
        const auto phase = signed_conversion ? 0x1077c1f4u : 0x1077c6d0u;
        const unsigned half_index = signed_conversion ? 21 : 20, word_index = signed_conversion ? 20 : 21;
        put(half_index, get(25) * 2); put(word_index, get(25) * 4);
        for (;;) {
            s.step(phase); load<1>(0, 90, phase); s.wr<1>(1027, get(0) <= 4); put(0, 4 - get(0)); put(0, std::countl_zero(get(0))); put(14, get(0) >> 5); compare(s, word(get(14)), 0, true);
            if (get(14)) {
                load<4>(4, 92, phase + 20); put(3, get(18)); put(5, get(22)); put(6, get(23)); put(7, local(68)); put(8, local(64)); put(9, local(60));
                call(s, 0x1077f180, phase + 52); put(27, get(4)); put(28, get(3));
            }
            compare(s, word(get(14)));
            if (!get(14)) {
                load<4>(4, 92, phase + 68); put(3, get(18)); put(5, get(22)); put(6, get(23)); put(7, local(68)); put(8, local(64)); put(9, local(60));
                call(s, 0x1077ee20, phase + 100); put(29, get(3));
            }
            const auto converted = signed_conversion ? 0x1077c25cu : 0x1077c738u;
            load<4>(signed_conversion ? 0 : 3, 68, converted); compare(s, word(get(signed_conversion ? 0 : 3))); if (!get(signed_conversion ? 0 : 3)) return false;
            if (signed_conversion) {
                load<1>(4, 90, 0x1077c268); put(17, get(17) + get(0)); compare(s, get(4), 4);
                if (get(4) == 4) {
                    load<4>(0, 64, 0x1077c278); compare(s, word(get(0)));
                    if (get(0)) { s.wr<1>(1027, get(27) == 0); put(13, 0 - get(27)); const auto carry_in = s.r<1>(1027); s.wr<1>(1027, get(28) == 0 && carry_in); put(0, ~get(28) + carry_in); }
                    else { put(13, get(27)); put(0, get(28)); }
                    store<4>(148, 0, 0x1077c298);
                } else { load<4>(0, 64, 0x1077c2a0); put(3, get(29)); compare(s, word(get(0))); if (get(0)) put(3, 0 - get(29)); put(30, get(3)); }
            } else {
                load<4>(0, 64, 0x1077c744); put(17, get(17) + get(3)); compare(s, word(get(0)));
                if (get(0)) { load<1>(0, 90, 0x1077c754); compare(s, get(0), 4);
                    if (get(0) == 4) { s.wr<1>(1027, get(27) == 0); put(27, 0 - get(27)); const auto carry_in = s.r<1>(1027); s.wr<1>(1027, get(28) == 0 && carry_in); put(28, ~get(28) + carry_in); }
                    else put(29, 0 - get(29)); }
            }
            const auto repeat = signed_conversion ? 0x1077c2b8u : 0x1077c770u;
            load<4>(0, 96, repeat); compare(s, word(get(0)));
            if (!get(0)) { integer_assignment(signed_conversion); break; }
            load<1>(0, 88, repeat + 12); compare(s, get(0)); const bool suppressed = get(0) != 0;
            if (!suppressed) {
                const auto pack = signed_conversion ? 0x1077c2d0u : 0x1077c788u;
                if (!signed_conversion) load<1>(0, 90, pack);
                const unsigned length = signed_conversion ? 4 : 0;
                compare(s, word(get(length)), 2);
                if (get(length) == 2) { put(3, local(72)); write<2>(s, get(3) + get(half_index), get(signed_conversion ? 30 : 29), signed_conversion ? 0x1077c304 : 0x1077c7c0); }
                else {
                    if (word(get(length)) < 2) compare(s, word(get(length))); else compare(s, word(get(length)), 4);
                    if (!get(length)) { put(3, local(72)); write<1>(s, get(3) + get(25), get(signed_conversion ? 30 : 29), signed_conversion ? 0x1077c2f8 : 0x1077c7b4); }
                    else if (get(length) == 3) { put(3, local(72)); write<4>(s, get(3) + get(word_index), get(signed_conversion ? 30 : 29), signed_conversion ? 0x1077c310 : 0x1077c7cc); }
                }
            }
            const auto look = signed_conversion ? (suppressed ? 0x1077c4d8u : 0x1077c328u) : (suppressed ? 0x1077c990u : 0x1077c7e4u);
            callback(0, look); put(0, byte(get(3))); store<1>(56, 3, look + 8); compare(s, word(get(0)), -1);
            bool at_end = get(0) == UINT32_MAX;
            if (!at_end) {
                load<4>(0, 100, look + 20); put(25, get(25) + 1); put(half_index, get(half_index) + 2); compare(s, word(get(25)), word(get(0))); put(word_index, get(word_index) + 4); at_end = word(get(25)) >= word(get(0));
            }
            if (at_end) unread(look + (signed_conversion || suppressed ? 328 : 324), look + (signed_conversion || suppressed ? 352 : 348));
            else {
                const unsigned equal = signed_conversion ? (suppressed ? 14 : 15) : 14;
                const unsigned cursor = signed_conversion ? (suppressed ? 15 : 14) : 15;
                if (repeat_separator(look + 44, equal, cursor, signed_conversion || suppressed)) continue;
            }
            if (!suppressed) packed_assignment(signed_conversion ? 0x1077c48c : 0x1077c944);
            break;
        }
        progress(signed_conversion ? 0x1077c6a4 : 0x1077cb5c); return true;
    }
    void integer_assignment(bool signed_conversion) {
        compare(s, get(26)); if (!get(26)) return;
        if (!signed_conversion) load<1>(0, 90, 0x1077cb00);
        const unsigned length = signed_conversion ? 4 : 0;
        compare(s, word(get(length)), 2);
        if (get(length) != 2) {
            if (word(get(length)) < 2) compare(s, word(get(length))); else compare(s, word(get(length)), 4);
        }
        if (get(length) == 0 || get(length) == 3) write<4>(s, get(26), get(signed_conversion ? 30 : 29), signed_conversion ? (get(length) ? 0x1077c68c : 0x1077c674) : (get(length) ? 0x1077cb48 : 0x1077cb30));
        else if (get(length) == 1) write<1>(s, get(26), get(signed_conversion ? 30 : 29), signed_conversion ? 0x1077c67c : 0x1077cb38);
        else if (get(length) == 2) write<2>(s, get(26), get(signed_conversion ? 30 : 29), signed_conversion ? 0x1077c684 : 0x1077cb40);
        else if (get(length) == 4) {
            write<4>(s, get(26) + 4, get(signed_conversion ? 13 : 27), signed_conversion ? 0x1077c694 : 0x1077cb50);
            if (signed_conversion) load<4>(0, 148, 0x1077c698);
            write<4>(s, get(26), get(signed_conversion ? 0 : 28), signed_conversion ? 0x1077c69c : 0x1077cb54);
        }
        put(16, get(16) + 1);
    }
    bool floating() {
        put(14, get(25) * 4);
        for (;;) {
            s.step(0x1077cb70); load<4>(3, 92, 0x1077cb70); put(4, get(22)); put(5, get(23)); put(6, local(68)); put(7, local(60)); call(s, 0x1077de10, 0x1077cb88);
            load<4>(3, 68, 0x1077cb88); compare(s, word(get(3))); if (!get(3)) return false;
            load<4>(0, 96, 0x1077cb94); put(17, get(17) + get(3)); compare(s, word(get(0)));
            if (!get(0)) {
                compare(s, get(26)); if (get(26)) {
                    load<1>(0, 90, 0x1077cef4); compare(s, word(get(0)), 5);
                    if (get(0) == 5) write<8>(s, get(26), s.r<8>(16400), 0x1077cf28);
                    else {
                        if (word(get(0)) < 5) compare(s, word(get(0))); else compare(s, word(get(0)), 7);
                        if (!get(0)) { round_float(); write<4>(s, get(26), float_bits<4>(float_value<8>(s.r<8>(16384))), 0x1077cf20); }
                        else if (get(0) == 6) write<8>(s, get(26), s.r<8>(16400), 0x1077cf30);
                    }
                    put(16, get(16) + 1);
                }
                break;
            }
            load<1>(0, 88, 0x1077cba4); compare(s, get(0)); const bool suppressed = get(0) != 0;
            if (!suppressed) {
                load<1>(0, 90, 0x1077cbb0); round_float(); put(3, local(72)); compare(s, get(0));
                write<4>(s, get(3) + get(14), float_bits<4>(float_value<8>(s.r<8>(16384))), 0x1077cbc0);
                if (!get(0)) { put(0, 4); store<4>(100, 0, 0x1077cbcc); }
            }
            const auto look = suppressed ? 0x1077cd8cu : 0x1077cbe4u;
            callback(0, look); put(0, byte(get(3))); store<1>(56, 3, look + 8); compare(s, word(get(0)), -1); bool end = get(0) == UINT32_MAX;
            if (!end) { load<4>(0, 100, look + 20); put(25, get(25) + 1); put(14, get(14) + 4); compare(s, word(get(25)), word(get(0))); end = word(get(25)) >= word(get(0)); }
            if (end) unread(look + 320, look + 344);
            else if (repeat_separator(look + 40, 15, 18)) continue;
            if (!suppressed) packed_assignment(0x1077cd40);
            break;
        }
        progress(0x1077cf38); return true;
    }
    void round_float() {
        const auto rounded = double(float(float_value<8>(s.r<8>(16400))));
        s.wr<8>(16384, float_bits<8>(rounded));
        s.wr<1>(1296, rounded < 0); s.wr<1>(1297, rounded > 0);
        s.wr<1>(1298, rounded == 0); s.wr<1>(1299, std::isnan(rounded));
    }
    bool percent() {
        load<4>(0, 96, 0x1077d524); compare(s, word(get(0))); if (get(0)) return false;
        for (;;) {
            callback(0, 0x1077d550); put(0, get(3) & 255); store<1>(56, 3, 0x1077d558);
            put(0, read<1>(s, get(19) + get(0), 0x1077d55c)); put(0, get(0) & 6); compare(s, word(get(0)), 0, true); if (!get(0)) break;
            s.step(0x1077d538); put(17, get(17) + 1);
        }
        load<1>(0, 56, 0x1077d568); put(4, byte(get(0))); compare(s, word(get(4)), '%');
        if (get(4) != '%') { callback(1, 0x1077d588, false); return false; }
        put(17, get(17) + 1); return true;
    }
    bool text() {
        // s additionally skips leading guest whitespace; both forms then use
        // the live bitmap in the record, never host locale classification.
        if (get(0) == 's') {
            load<4>(0, 96, 0x1077d598); compare(s, word(get(0))); if (get(0)) return false;
            callback(0, 0x1077d5b8); store<1>(56, 3, 0x1077d5bc);
            for (;;) {
                load<1>(4, 56, 0x1077d5e8); put(4, byte(get(4))); classify(4, 0x1077d5f4); if (!get(0)) break;
                s.step(0x1077d5c8); put(3, get(23)); put(12, get(22)); put(4, 0); put(5, 0); put(17, get(17) + 1);
                call(s, 0x10781320, 0x1077d5e0); put(2, read<4>(s, local(20), 0x1077d5e0)); store<1>(56, 3, 0x1077d5e4);
            }
            callback(1, 0x1077d610, false);
        }
        load<4>(0, 96, 0x1077d614); compare(s, word(get(0))); if (get(0)) return false;
        compare(s, get(26)); const bool assigned = get(26) != 0;
        put(0, 0); store<4>(68, 0, assigned ? 0x1077d62c : 0x1077d72c); put(14, local(88));
        for (;;) {
            const auto width = assigned ? 0x1077d670u : 0x1077d748u;
            s.step(width); load<4>(3, 92, width); put(0, get(3) - 1); compare(s, word(get(3))); store<4>(92, 0, width + 12); if (!get(3)) break;
            const auto look = assigned ? 0x1077d698u : 0x1077d770u;
            callback(0, look); put(0, byte(get(3))); store<1>(56, 3, look + 8); compare(s, word(get(0)), -1); if (get(0) == UINT32_MAX) break;
            const auto bitmap = assigned ? 0x1077d6acu : 0x1077d784u;
            load<1>(assigned ? 5 : 0, 56, bitmap); put(3, 1);
            const auto character = get(assigned ? 5 : 0); put(4, (character & 255) >> 3); put(4, get(4) + 28); put(0, character & 7);
            put(4, read<1>(s, get(14) + get(4), bitmap + 20)); put(0, shift_left<4>(get(3), get(0))); put(0, get(4) & get(0)); compare(s, word(get(0)), 0, true); if (!get(0)) break;
            if (assigned) {
                load<1>(0, 90, 0x1077d638); compare(s, get(0), 7);
                if (get(0) == 7) { put(3, get(26)); put(4, local(56)); put(5, 1); call(s, 0x10778170, 0x1077d654); put(26, get(26) + 2); }
                else { write<1>(s, get(26), get(5), 0x1077d65c); put(26, get(26) + 1); }
            }
            load<4>(3, 68, assigned ? 0x1077d664 : 0x1077d73c); put(0, get(3) + 1); store<4>(68, 0, assigned ? 0x1077d66c : 0x1077d744);
        }
        load<4>(assigned ? 3 : 0, 68, assigned ? 0x1077d6d0 : 0x1077d7a8); compare(s, word(get(assigned ? 3 : 0)));
        if (!get(assigned ? 3 : 0)) { unread(assigned ? 0x1077d6dc : 0x1077d7b4, assigned ? 0x1077d6f4 : 0x1077d7cc); return false; }
        if (assigned) {
            load<1>(0, 90, 0x1077d6fc); put(17, get(17) + get(3)); compare(s, get(0), 7); put(0, 0);
            if (s.r<1>(2304) & 2) write<2>(s, get(26), get(0), 0x1077d710); else write<1>(s, get(26), get(0), 0x1077d71c);
            put(16, get(16) + 1);
        } else put(17, get(17) + get(0));
        load<4>(0, 92, 0x1077d7d8); compare(s, word(get(0))); if (word(get(0)) >= 0) unread(0x1077d7e4, 0x1077d7fc);
        progress(0x1077d800); return true;
    }
    void count() {
        compare(s, get(26)); if (!get(26)) return;
        load<1>(0, 90, 0x1077d818); compare(s, word(get(0)), 2);
        if (get(0) != 2) { if (word(get(0)) < 2) compare(s, word(get(0))); else compare(s, word(get(0)), 4); }
        switch (get(0)) {
        case 0: write<4>(s, get(26), get(17), 0x1077d848); break;
        case 1: write<1>(s, get(26), get(17), 0x1077d860); break;
        case 2: write<2>(s, get(26), get(17), 0x1077d850); break;
        case 3: write<4>(s, get(26), get(17), 0x1077d858); break;
        case 4:
            write<4>(s, get(26) + 4, get(17), 0x1077d868); s.wr<1>(1027, bool(get(17) & 0x80000000u) && bool(get(17) & 0x7fffffffu)); put(0, word(get(17)) < 0 ? UINT32_MAX : 0);
            write<4>(s, get(26), get(0), 0x1077d870); break;
        }
    }
    bool characters();
public:
    explicit Scanner(State& state) : s(state) {}
    void scan() {
        nonvolatile_words(s, 13, 0x1077bf20, false); put(0, s.r<4>(4128)); put(19, read<4>(s, get(2) - 0x5a48, 0x1077bf28));
        put(22, get(3)); put(23, get(4)); put(31, get(5)); put(17, 0); put(16, 0);
        write<4>(s, get(1) + 8, get(0), 0x1077bf40); put(0, 0); frame(s, 240, 0x1077bf48); store<4>(276, 6, 0x1077bf4c); store<4>(156, 0, 0x1077bf50);
        for (;;) {
            s.step(0x1077d874); put(0, read<1>(s, get(31), 0x1077d874)); put(14, byte(get(0))); compare(s, word(get(14)), 0, true); if (!get(14)) break;
            if (format_whitespace()) continue;
            compare(s, word(get(14)), '%');
            if (get(14) != '%') { if (!literal()) break; continue; }
            if (!parse_conversion()) break;
            bool success = true;
            switch (conversion()) {
            case Conversion::signed_integer: success = integer(true); break;
            case Conversion::unsigned_integer: success = integer(false); break;
            case Conversion::floating: success = floating(); break;
            case Conversion::characters: success = characters(); break;
            case Conversion::percent: success = percent(); break;
            case Conversion::text: success = text(); break;
            case Conversion::count: count(); break;
            case Conversion::stop: success = false; break;
            }
            if (!success) break;
        }
        callback(2, 0x1077d894); compare(s, word(get(3)));
        bool eof = get(3) != 0;
        if (eof) { load<4>(0, 156, 0x1077d8a0); compare(s, word(get(0))); eof = !get(0); }
        put(3, eof ? UINT32_MAX : get(16)); put(0, read<4>(s, local(248), 0x1077d8b8)); put(1, get(1) + 240);
        nonvolatile_words(s, 13, 0x1077d8c0, true); s.wr<4>(4128, get(0)); finish(s);
    }
};
bool Scanner::characters() {
    load<1>(0, 89, 0x1077cf48); compare(s, get(0)); if (!get(0)) { put(0, 1); store<4>(92, 0, 0x1077cf58); }
    load<4>(0, 96, 0x1077cf5c); compare(s, word(get(0)));
    if (get(0)) {
        load<1>(0, 88, 0x1077cf68); compare(s, get(0)); const bool suppressed = get(0) != 0;
        if (!suppressed) {
            compare(s, word(get(25)));
            if (!get(25)) {
                load<4>(3, 108, 0x1077cf7c); store<4>(104, 3, 0x1077cf80); put(0, read<1>(s, get(3), 0x1077cf84)); put(3, byte(get(0)));
                put(0, 0 - get(3)); put(0, get(0) | get(3)); put(0, get(0) >> 31); store<4>(112, 0, 0x1077cf98);
            }
            put(14, get(25) * 4); put(15, get(25) * 2);
        }
        for (;;) {
            if (!suppressed) {
                s.step(0x1077cfa4); load<1>(0, 90, 0x1077cfa4); compare(s, word(get(0)), 2);
                if (get(0) == 2) {
                    load<4>(3, 92, 0x1077cfe8); compare(s, word(get(3)), 2); if (word(get(3)) > 2) return false;
                    put(0, get(15) + 2); put(0, get(0) - get(3)); put(24, local(72)); put(24, get(24) + get(0));
                } else {
                    if (word(get(0)) < 2) compare(s, word(get(0))); else compare(s, word(get(0)), 4);
                    if (!get(0)) {
                        load<4>(0, 92, 0x1077d008); compare(s, word(get(0)), 1); if (word(get(0)) > 1) return false;
                        put(24, local(72)); put(24, get(24) + get(25));
                    } else if (get(0) == 3) {
                        load<4>(3, 92, 0x1077cfc8); compare(s, word(get(3)), 4); if (word(get(3)) > 4) return false;
                        put(0, get(14) + 4); put(0, get(0) - get(3)); put(24, local(72)); put(24, get(24) + get(0));
                    }
                }
                put(0, 0); load<4>(18, 92, 0x1077d020); store<4>(68, 0, 0x1077d024);
                for (;;) {
                    compare(s, word(get(18))); put(18, get(18) - 1); if (s.r<1>(2304) & 2) break;
                    callback(0, 0x1077d064); put(0, byte(get(3))); store<1>(56, 3, 0x1077d06c); compare(s, word(get(0)), -1); if (get(0) == UINT32_MAX) break;
                    load<4>(3, 68, 0x1077d02c); load<1>(4, 56, 0x1077d030); put(0, get(3) + 1); write<1>(s, get(24), get(4), 0x1077d038); put(24, get(24) + 1); store<4>(68, 0, 0x1077d040);
                }
                load<4>(0, 68, 0x1077d078); compare(s, word(get(0))); if (!get(0)) return false;
                put(3, get(23)); put(12, get(22)); put(17, get(17) + get(0)); put(4, 0); put(5, 0);
                call(s, 0x10781320, 0x1077d09c); put(2, read<4>(s, local(20), 0x1077d09c));
            } else {
                s.step(0x1077d238); put(0, 0); store<4>(68, 0, 0x1077d23c);
                for (;;) {
                    load<4>(3, 92, 0x1077d250); put(0, get(3) - 1); compare(s, word(get(3))); store<4>(92, 0, 0x1077d25c); if (!get(3)) break;
                    callback(0, 0x1077d278); put(0, byte(get(3))); store<1>(56, 3, 0x1077d280); compare(s, word(get(0)), -1); if (get(0) == UINT32_MAX) break;
                    load<4>(3, 68, 0x1077d244); put(0, get(3) + 1); store<4>(68, 0, 0x1077d24c);
                }
                load<4>(0, 68, 0x1077d28c); compare(s, word(get(0))); if (!get(0)) return false;
                callback(0, 0x1077d2ac);
            }
            const auto look = suppressed ? 0x1077d2acu : 0x1077d09cu;
            put(0, byte(get(3))); store<1>(56, 3, look + 8); compare(s, word(get(0)), -1); bool end = get(0) == UINT32_MAX;
            if (!end) {
                load<4>(0, 100, look + 20); put(25, get(25) + 1);
                if (!suppressed) put(15, get(15) + 2);
                compare(s, word(get(25)), word(get(0))); if (!suppressed) put(14, get(14) + 4); end = word(get(25)) >= word(get(0));
            }
            if (end) unread(suppressed ? 0x1077d3ec : 0x1077d1e4, suppressed ? 0x1077d404 : 0x1077d1fc);
            else if (repeat_separator(suppressed ? 0x1077d2d0 : 0x1077d0c8, suppressed ? 14 : 18, suppressed ? 15 : 20, true)) continue;
            if (!suppressed) packed_assignment(0x1077d200);
            break;
        }
    } else {
        compare(s, get(26)); const bool assigned = get(26) != 0;
        put(0, 0); store<4>(68, 0, assigned ? 0x1077d418 : 0x1077d4b4);
        for (;;) {
            const auto width = assigned ? 0x1077d464u : 0x1077d4ccu;
            s.step(width); load<4>(3, 92, width); put(0, get(3) - 1); compare(s, word(get(3))); store<4>(92, 0, width + 12); if (!get(3)) break;
            const auto look = assigned ? 0x1077d48cu : 0x1077d4f4u;
            callback(0, look);
            if (assigned) {
                compare(s, word(get(3)), -1); if (get(3) == UINT32_MAX) break;
                load<1>(0, 90, 0x1077d424); store<1>(56, 3, 0x1077d428); compare(s, get(0), 7);
                if (get(0) == 7) { put(3, get(26)); put(4, local(56)); put(5, 1); call(s, 0x10778170, 0x1077d444); put(26, get(26) + 1); }
                else { load<1>(0, 56, 0x1077d44c); write<1>(s, get(26), get(0), 0x1077d450); put(26, get(26) + 1); }
            } else { put(0, byte(get(3))); store<1>(56, 3, 0x1077d4fc); compare(s, word(get(0)), -1); if (get(0) == UINT32_MAX) break; }
            load<4>(3, 68, assigned ? 0x1077d458 : 0x1077d4c0); put(0, get(3) + 1); store<4>(68, 0, assigned ? 0x1077d460 : 0x1077d4c8);
        }
        load<4>(0, 68, assigned ? 0x1077d498 : 0x1077d508); compare(s, word(get(0))); if (!get(0)) return false;
        if (assigned) { put(17, get(17) + get(0)); put(16, get(16) + 1); }
    }
    progress(0x1077d514); return true;
}
}
bool try_modern_formatted_input(State& s, std::uint32_t entry) {
    if (s.formatted_input_original_resume == entry) { s.formatted_input_original_resume = 0; return false; }
    const auto found = std::find(formatted_input_entries.begin(), formatted_input_entries.end(), entry);
    if (found == formatted_input_entries.end()) throw std::runtime_error("Unknown formatted input entry");
    const auto index = std::size_t(found - formatted_input_entries.begin());
    const bool original = s.formatted_input_original || s.formatted_input_entry_original[index];
    ++(original ? s.formatted_input_original_calls[index] : s.formatted_input_calls[index]);
    if (original && !s.formatted_input_observer) return false;
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (s.formatted_input_observer) s.formatted_input_observer->enter(s, entry, original);
    if (original) {
        s.formatted_input_original_resume = entry;
        try { run_until(s, caller); } catch (...) { s.formatted_input_original_resume = 0; throw; }
    } else {
        s.step(entry);
        switch (index) {
        case 0: FormatParser(s).parse(); break;
        case 1: Scanner(s).scan(); break;
        case 2: string_callback(s); break;
        case 3: string_vector_facade(s); break;
        case 4: string_variadic_facade(s); break;
        case 5: stream_callback(s); break;
        case 6: stream_variadic_facade(s); break;
        }
    }
    if (s.formatted_input_observer) s.formatted_input_observer->leave(s, entry, original);
    return true;
}
void reset_formatted_input_diagnostics(State& s) {
    s.formatted_input_original = false; s.formatted_input_entry_original.fill(false);
    s.formatted_input_calls.fill(0); s.formatted_input_original_calls.fill(0);
    s.formatted_input_dispatch_escapes = 0; s.formatted_input_original_resume = 0; s.formatted_input_observer = nullptr;
}
std::string formatted_input_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < formatted_input_entries.size(); ++i)
        out << "formatted.entry=" << hex_address(formatted_input_entries[i]) << " mode="
            << (s.formatted_input_original || s.formatted_input_entry_original[i] ? "original" : "modern")
            << " modern=" << s.formatted_input_calls[i] << " original=" << s.formatted_input_original_calls[i] << '\n';
    out << "formatted.dispatch_escapes=" << s.formatted_input_dispatch_escapes << '\n'; return out.str();
}
}
