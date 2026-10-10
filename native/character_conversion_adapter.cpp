#include "character_conversion_adapter.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
namespace {
std::uint32_t reg(const State& s, unsigned n) { return std::uint32_t(s.r<4>(n * 4)); }
void set(State& s, unsigned n, std::uint32_t value) { s.wr<4>(n * 4, value); }
std::int32_t word(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
void compare(State& s, std::int64_t a, std::int64_t b = 0, bool record = false) {
    s.wr<1>(2304, (record ? s.r<1>(2304) & 0xf0 : 0) |
        (a < b ? 8 : a > b ? 4 : 2) | (s.r<1>(1024) & 1));
}
template<unsigned N> std::uint32_t read(State& s, std::uint32_t address, std::uint32_t pc) {
    s.pc = pc; return std::uint32_t(s.memory.read<N>(address));
}
template<unsigned N> void write(State& s, std::uint32_t address, std::uint32_t value, std::uint32_t pc) {
    s.pc = pc; s.memory.write<N>(address, value);
}
void frame(State& s, unsigned size, std::uint32_t pc) {
    const auto previous = reg(s, 1); write<4>(s, previous - size, previous, pc); set(s, 1, previous - size);
}
void call(State& s, std::uint32_t entry, std::uint32_t link) {
    s.wr<4>(2440, reg(s, 2)); s.wr<4>(4128, link); s.pc = entry; run_until(s, link);
}
void finish(State& s) { s.pc = std::uint32_t(s.r<4>(4128)); }
void words(State& s, unsigned first, std::uint32_t pc, bool restore) {
    s.wr<4>(2432, reg(s, 1) - (32 - first) * 4);
    for (unsigned n = first; n < 32; ++n) {
        const auto address = std::uint32_t(s.r<4>(2432));
        if (restore) set(s, n, read<4>(s, address, pc));
        else write<4>(s, address, reg(s, n), pc);
        s.wr<4>(2432, address + 4);
    }
}

// The classifier checks only bit 7 of continuation bytes. The decoder below
// separately rejects overlong encodings by comparing decoded value and length.
void sequence_length(State& s) {
    compare(s, reg(s, 4));
    if (!reg(s, 4)) { set(s, 3, UINT32_MAX); finish(s); return; }
    set(s, 5, read<1>(s, reg(s, 3), 0x10777f70));
    set(s, 0, std::uint32_t(signed_value<1>(reg(s, 5)))); compare(s, word(reg(s, 0)), 0, true);
    if (!reg(s, 0)) { set(s, 3, 0); finish(s); return; }
    set(s, 5, std::uint32_t(signed_value<1>(reg(s, 5)))); set(s, 0, reg(s, 5) & 0x80);
    compare(s, word(reg(s, 0)), 0, true);
    if (!reg(s, 0)) { set(s, 3, 1); finish(s); return; }
    set(s, 0, reg(s, 5) & 0xe0); compare(s, word(reg(s, 0)), 0xc0);
    if (reg(s, 0) == 0xc0) {
        compare(s, reg(s, 4), 2);
        if (reg(s, 4) < 2) set(s, 3, UINT32_MAX - 1);
        else {
            set(s, 0, read<1>(s, reg(s, 3) + 1, 0x10777fac)); set(s, 3, UINT32_MAX);
            set(s, 0, reg(s, 0) & 0x80); compare(s, word(reg(s, 0)), 0x80);
            if (reg(s, 0) == 0x80) set(s, 3, 2);
        }
    } else {
        set(s, 0, reg(s, 5) & 0xf0); compare(s, word(reg(s, 0)), 0xe0);
        if (reg(s, 0) != 0xe0) set(s, 3, UINT32_MAX);
        else {
            compare(s, reg(s, 4), 3);
            if (reg(s, 4) >= 3) {
                set(s, 0, read<1>(s, reg(s, 3) + 1, 0x10777fe4)); set(s, 0, reg(s, 0) & 0x80);
                compare(s, word(reg(s, 0)), 0x80);
                if (reg(s, 0) == 0x80) {
                    set(s, 0, read<1>(s, reg(s, 3) + 2, 0x10777ff4)); set(s, 0, reg(s, 0) & 0x80);
                    compare(s, word(reg(s, 0)), 0x80);
                }
                set(s, 3, reg(s, 0) == 0x80 ? 3 : UINT32_MAX);
            } else {
                compare(s, reg(s, 4), 2);
                bool incomplete = false;
                if (reg(s, 4) == 2) {
                    set(s, 0, read<1>(s, reg(s, 3) + 1, 0x1077801c)); set(s, 0, reg(s, 0) & 0x80);
                    compare(s, word(reg(s, 0)), 0x80); incomplete = reg(s, 0) == 0x80;
                }
                if (!incomplete) { compare(s, reg(s, 4), 1); incomplete = reg(s, 4) == 1; }
                set(s, 3, incomplete ? UINT32_MAX - 1 : UINT32_MAX);
            }
        }
    }
    finish(s);
}
void decode_character(State& s) {
    write<4>(s, reg(s, 1) - 4, reg(s, 31), 0x10778050); set(s, 0, s.r<4>(4128)); set(s, 31, reg(s, 3));
    write<4>(s, reg(s, 1) - 8, reg(s, 30), 0x1077805c); set(s, 30, 0);
    write<4>(s, reg(s, 1) - 12, reg(s, 29), 0x10778064); set(s, 29, reg(s, 4)); compare(s, word(reg(s, 29)), 0, true);
    write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x1077806c); frame(s, 80, 0x10778070);
    if (!reg(s, 29)) set(s, 3, 0);
    else {
        compare(s, reg(s, 5));
        if (!reg(s, 5)) set(s, 3, UINT32_MAX);
        else {
            set(s, 3, reg(s, 29)); set(s, 4, reg(s, 5)); call(s, 0x10777f60, 0x1077809c); compare(s, word(reg(s, 3)));
            if (word(reg(s, 3)) < 0) set(s, 3, UINT32_MAX);
            else {
                compare(s, word(reg(s, 3)), 2); set(s, 4, reg(s, 29));
                if (reg(s, 3) < 2) compare(s, word(reg(s, 3)), 1);
                else if (reg(s, 3) > 2) {
                    compare(s, word(reg(s, 3)), 4);
                    set(s, 0, read<1>(s, reg(s, 29), 0x107780d0)); set(s, 4, reg(s, 29) + 1);
                    set(s, 0, (reg(s, 0) << 6) & 0x3c0); set(s, 30, reg(s, 0));
                }
                if (reg(s, 3) >= 2) {
                    set(s, 0, read<1>(s, reg(s, 4), 0x107780e0)); set(s, 4, reg(s, 4) + 1);
                    set(s, 0, (reg(s, 0) & 0x3f) | reg(s, 30)); set(s, 30, (reg(s, 0) << 6) & 0xffc0);
                }
                if (reg(s, 3)) {
                    set(s, 0, read<1>(s, reg(s, 4), 0x107780f4)); set(s, 0, (reg(s, 0) & 0x7f) | reg(s, 30)); set(s, 30, reg(s, 0) & 0xffff);
                }
                set(s, 0, reg(s, 30) & 0xffff); compare(s, word(reg(s, 0)), 0, true);
                if (reg(s, 0)) {
                    compare(s, reg(s, 0), 0x80);
                    if (reg(s, 0) < 0x80) set(s, 0, 1);
                    else { compare(s, reg(s, 0), 0x800); set(s, 0, reg(s, 0) < 0x800 ? 2 : 3); }
                }
                compare(s, word(reg(s, 0)), word(reg(s, 3)));
                if (reg(s, 0) != reg(s, 3)) set(s, 3, UINT32_MAX);
                else { compare(s, reg(s, 31)); if (reg(s, 31)) write<2>(s, reg(s, 31), reg(s, 30), 0x10778150); }
            }
        }
    }
    set(s, 0, read<4>(s, reg(s, 1) + 88, 0x10778154)); set(s, 1, reg(s, 1) + 80);
    set(s, 31, read<4>(s, reg(s, 1) - 4, 0x1077815c)); set(s, 30, read<4>(s, reg(s, 1) - 8, 0x10778160));
    s.wr<4>(4128, reg(s, 0)); set(s, 29, read<4>(s, reg(s, 1) - 12, 0x10778168)); finish(s);
}
void encode_character(State& s) {
    set(s, 5, read<2>(s, reg(s, 2) - 0x34f8, 0x10778190)); compare(s, reg(s, 3));
    set(s, 0, read<2>(s, reg(s, 2) - 0x34f6, 0x10778198));
    write<2>(s, reg(s, 1) - 8, reg(s, 5), 0x1077819c); write<2>(s, reg(s, 1) - 6, reg(s, 0), 0x107781a0);
    if (!reg(s, 3)) { set(s, 3, 0); finish(s); return; }
    set(s, 0, reg(s, 4) & 0xffff); compare(s, reg(s, 0), 0x80);
    if (reg(s, 0) < 0x80) set(s, 5, 1);
    else { compare(s, reg(s, 0), 0x800); set(s, 5, reg(s, 0) < 0x800 ? 2 : 3); }
    compare(s, word(reg(s, 5)), 2); set(s, 6, reg(s, 3) + reg(s, 5));
    if (reg(s, 5) == 1) compare(s, word(reg(s, 5)), 1);
    if (reg(s, 5) == 3) {
        compare(s, word(reg(s, 5)), 4); set(s, 0, (reg(s, 4) & 0x3f) | 0x80);
        write<1>(s, reg(s, 6) - 1, reg(s, 0), 0x10778204); set(s, 6, reg(s, 6) - 1); set(s, 4, (reg(s, 4) >> 6) & 0x3ff);
    }
    if (reg(s, 5) >= 2) {
        set(s, 0, (reg(s, 4) & 0x3f) | 0x80);
        write<1>(s, reg(s, 6) - 1, reg(s, 0), 0x10778214); set(s, 6, reg(s, 6) - 1); set(s, 4, (reg(s, 4) >> 6) & 0x3ff);
    }
    set(s, 3, reg(s, 1) - 8); set(s, 0, read<1>(s, reg(s, 3) + reg(s, 5), 0x10778220));
    set(s, 0, reg(s, 4) | reg(s, 0)); write<1>(s, reg(s, 6) - 1, reg(s, 0), 0x10778228); set(s, 3, reg(s, 5)); finish(s);
}
void character_wrapper(State& s, bool encode) {
    const auto entry = encode ? 0x10778240u : 0x10778170u;
    set(s, 0, s.r<4>(4128)); write<4>(s, reg(s, 1) + 8, reg(s, 0), entry + 4); frame(s, 64, entry + 8);
    call(s, encode ? 0x10778190 : 0x10778050, entry + 16);
    set(s, 0, read<4>(s, reg(s, 1) + 72, entry + 16)); set(s, 1, reg(s, 1) + 64); s.wr<4>(4128, reg(s, 0)); finish(s);
}
void encode_string(State& s) {
    words(s, 27, 0x10778260, false); set(s, 0, s.r<4>(4128)); set(s, 27, reg(s, 3)); compare(s, word(reg(s, 27)), 0, true);
    set(s, 28, reg(s, 5)); set(s, 31, 0); write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x10778274); frame(s, 96, 0x10778278);
    bool valid = reg(s, 27) != 0;
    if (valid) { compare(s, reg(s, 4)); valid = reg(s, 4) != 0; }
    if (!valid) set(s, 3, 0);
    else {
        set(s, 29, reg(s, 4));
        for (;;) {
            s.step(0x107782e0); compare(s, reg(s, 31), reg(s, 28)); if (reg(s, 31) > reg(s, 28)) break;
            set(s, 4, read<2>(s, reg(s, 29), 0x10778298)); compare(s, reg(s, 4));
            if (!reg(s, 4)) { set(s, 0, 0); write<1>(s, reg(s, 27) + reg(s, 31), 0, 0x107782a8); break; }
            set(s, 3, reg(s, 1) + 56); set(s, 29, reg(s, 29) + 2); call(s, 0x10778240, 0x107782bc);
            set(s, 30, reg(s, 3)); set(s, 0, reg(s, 31) + reg(s, 30)); compare(s, reg(s, 0), reg(s, 28)); if (reg(s, 0) > reg(s, 28)) break;
            set(s, 5, reg(s, 30)); set(s, 3, reg(s, 27) + reg(s, 31)); set(s, 4, reg(s, 1) + 56); call(s, 0x1077dc10, 0x107782dc);
            set(s, 31, reg(s, 31) + reg(s, 30));
        }
        set(s, 3, reg(s, 31));
    }
    set(s, 0, read<4>(s, reg(s, 1) + 104, 0x107782ec)); set(s, 1, reg(s, 1) + 96);
    words(s, 27, 0x107782f4, true); s.wr<4>(4128, reg(s, 0)); finish(s);
}
}
bool try_modern_character_conversion(State& s, std::uint32_t entry) {
    if (s.character_conversion_original_resume == entry) { s.character_conversion_original_resume = 0; return false; }
    const auto found = std::find(character_conversion_entries.begin(), character_conversion_entries.end(), entry);
    if (found == character_conversion_entries.end()) throw std::runtime_error("Unknown character conversion entry");
    const auto index = std::size_t(found - character_conversion_entries.begin());
    const bool original = s.character_conversion_original || s.character_conversion_entry_original[index];
    ++(original ? s.character_conversion_original_calls[index] : s.character_conversion_calls[index]);
    if (original && !s.character_conversion_observer) return false;
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (s.character_conversion_observer) s.character_conversion_observer->enter(s, entry, original);
    if (original) {
        s.character_conversion_original_resume = entry;
        try { run_until(s, caller); } catch (...) { s.character_conversion_original_resume = 0; throw; }
    } else {
        s.step(entry);
        switch (index) {
        case 0: sequence_length(s); break;
        case 1: decode_character(s); break;
        case 2: character_wrapper(s, false); break;
        case 3: encode_character(s); break;
        case 4: character_wrapper(s, true); break;
        case 5: encode_string(s); break;
        }
    }
    if (s.character_conversion_observer) s.character_conversion_observer->leave(s, entry, original);
    return true;
}
void reset_character_conversion_diagnostics(State& s) {
    s.character_conversion_original = false; s.character_conversion_entry_original.fill(false);
    s.character_conversion_calls.fill(0); s.character_conversion_original_calls.fill(0);
    s.character_conversion_original_resume = 0; s.character_conversion_observer = nullptr;
}
std::string character_conversion_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < character_conversion_entries.size(); ++i)
        out << "character.entry=" << hex_address(character_conversion_entries[i]) << " mode="
            << (s.character_conversion_original || s.character_conversion_entry_original[i] ? "original" : "modern")
            << " modern=" << s.character_conversion_calls[i] << " original=" << s.character_conversion_original_calls[i] << '\n';
    return out.str();
}
}
