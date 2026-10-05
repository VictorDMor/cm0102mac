#include "background_selection_adapter.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
namespace {
std::uint32_t reg(const State& s, unsigned n) { return std::uint32_t(s.r<4>(n * 4)); }
void set(State& s, unsigned n, std::uint32_t value) { s.wr<4>(n * 4, value); }
std::int32_t signed_word(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
std::int32_t effective_count(std::uint32_t value) {
    return std::bit_cast<std::int16_t>(std::uint16_t(value));
}
std::int32_t effective_selector(std::uint32_t value) {
    return std::bit_cast<std::int8_t>(std::uint8_t(value));
}
void compare(State& s, std::int64_t first, std::int64_t second, bool record = false) {
    s.wr<1>(2304, (record ? s.r<1>(2304) & 0xf0 : 0) |
        (first < second ? 8 : first > second ? 4 : 2) | (s.r<1>(1024) & 1));
}
void record(State& s, unsigned n) { compare(s, signed_word(reg(s, n)), 0, true); }

// Access sites keep faults and partial ABI effects at the original address.
// The named operations below retain access ordering without executing the
// replaced instruction bodies or introducing a second random state.
template <unsigned N> std::uint32_t read(State& s, std::uint32_t address, std::uint32_t pc) {
    s.pc = pc;
    return std::uint32_t(s.memory.read<N>(address));
}
template <unsigned N> void write(State& s, std::uint32_t address, std::uint32_t value, std::uint32_t pc) {
    s.pc = pc;
    s.memory.write<N>(address, value);
}
void call(State& s, std::uint32_t entry, std::uint32_t link) {
    s.wr<4>(2440, reg(s, 2));
    s.wr<4>(4128, link);
    s.pc = entry;
    run_until(s, link);
}
void push_frame(State& s, unsigned size, std::uint32_t pc) {
    const auto previous = reg(s, 1);
    write<4>(s, previous - size, previous, pc);
    set(s, 1, previous - size);
}
void save_catalog_registers(State& s) {
    s.pc = 0x107122e4;
    s.wr<4>(2432, reg(s, 1) - 20);
    for (unsigned n = 27; n < 32; ++n) {
        s.memory.write<4>(s.r<4>(2432), reg(s, n));
        s.wr<4>(2432, s.r<4>(2432) + 4);
    }
}
void restore_catalog_frame(State& s) {
    set(s, 0, read<4>(s, reg(s, 1) + 120, 0x107124cc));
    set(s, 1, reg(s, 1) + 112);
    s.wr<4>(4128, reg(s, 0));
    s.pc = 0x107124d8;
    s.wr<4>(2432, reg(s, 1) - 20);
    for (unsigned n = 27; n < 32; ++n) {
        set(s, n, s.memory.read<4>(s.r<4>(2432)));
        s.wr<4>(2432, s.r<4>(2432) + 4);
    }
    s.pc = std::uint32_t(s.r<4>(4128));
}
void restore_list_frame(State& s) {
    set(s, 0, read<4>(s, reg(s, 1) + 104, 0x10712c2c));
    set(s, 1, reg(s, 1) + 96);
    s.wr<4>(4128, reg(s, 0));
    for (unsigned n = 31; n >= 28; --n)
        set(s, n, read<4>(s, reg(s, 1) - (32 - n) * 4, 0x10712c38 + (31 - n) * 4));
    s.pc = std::uint32_t(s.r<4>(4128));
}

// Both consumers use this same pool protocol. The second body's access sites
// are exactly 0x788 bytes after the first body's corresponding sites.
void consume_pool_index(State& s, bool list) {
    const auto site = [list](std::uint32_t address) { return address + (list ? 0x788u : 0u); };
    set(s, 3, read<4>(s, reg(s, 2) - 0x7b88, site(0x10712330)));
    set(s, 4, read<4>(s, reg(s, 31), site(0x10712334)));
    set(s, 0, read<4>(s, reg(s, 3), site(0x10712338)));
    set(s, 3, reg(s, 4) + 2);
    compare(s, reg(s, 3), reg(s, 0));
    const bool wrapped = reg(s, 3) > reg(s, 0);
    write<4>(s, reg(s, 31), reg(s, 3), site(0x10712344));
    if (wrapped) {
        set(s, 0, read<4>(s, reg(s, 2) - 0x7b8c, site(0x1071234c)));
        write<4>(s, reg(s, 31), reg(s, 0), site(0x10712350));
        call(s, 0x1077b770, site(0x10712358));
        set(s, 0, reg(s, 3) & 0xffff);
        write<4>(s, reg(s, 30), reg(s, 0), site(0x1071235c));
    }

    // The effective count is signed16. Consequently the legacy floating path
    // for counts outside [-65535, 65535] is unreachable at both entries. Keep
    // the integer path's ABI residues while expressing selection as a signed
    // remainder. The uint32 addition must precede signed interpretation.
    set(s, 3, 0xffff0000);
    set(s, 0, 0xffff0001);
    compare(s, signed_word(reg(s, 29)), -65535);
    set(s, 3, read<4>(s, reg(s, 31), site(0x10712458)));
    compare(s, signed_word(reg(s, 29)), 0);
    set(s, 0, read<4>(s, reg(s, 30), site(0x10712460)));
    set(s, 3, read<2>(s, reg(s, 3), site(0x10712464)));
    set(s, 3, reg(s, 3) + reg(s, 0));
    const auto count = signed_word(reg(s, 29));
    const auto sum = signed_word(reg(s, 3));
    set(s, 0, std::uint32_t(std::int64_t(sum) % count));
    set(s, 30, count > 0 ? reg(s, 0) : 0u - reg(s, 0));
}

void select_catalog(State& s) {
    set(s, 0, s.r<4>(4128));
    save_catalog_registers(s);
    set(s, 27, reg(s, 3)); record(s, 27);
    set(s, 30, read<4>(s, reg(s, 2) - 0x7b90, 0x107122ec));
    set(s, 31, read<4>(s, reg(s, 2) - 0x7b84, 0x107122f0));
    set(s, 28, reg(s, 4));
    write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x107122f8);
    push_frame(s, 112, 0x107122fc);
    bool usable = reg(s, 27) != 0;
    if (usable) {
        set(s, 0, read<4>(s, reg(s, 27) + 12, 0x1071230c));
        set(s, 0, std::uint32_t(effective_count(reg(s, 0)))); record(s, 0);
        usable = reg(s, 0) != 0;
    }
    if (!usable) set(s, 3, 0);
    else {
        set(s, 29, std::uint32_t(effective_count(reg(s, 0)))); record(s, 29);
        consume_pool_index(s, false);
        set(s, 3, reg(s, 27)); set(s, 4, reg(s, 30));
        call(s, 0x1074df60, 0x10712490);
        compare(s, reg(s, 3), 0);
        if (reg(s, 3)) {
            set(s, 3, read<4>(s, reg(s, 3) + 4, 0x107124a0));
            compare(s, reg(s, 3), 0);
            if (reg(s, 3)) {
                set(s, 0, std::uint32_t(effective_selector(reg(s, 28)))); record(s, 0);
                const bool alternate = reg(s, 0) != 0;
                if (alternate) set(s, 3, reg(s, 3) + 12);
                call(s, 0x10712a60, alternate ? 0x107124c4 : 0x107124cc);
            }
        }
    }
    restore_catalog_frame(s);
}

void select_list(State& s) {
    set(s, 0, s.r<4>(4128));
    write<4>(s, reg(s, 1) - 4, reg(s, 31), 0x10712a64);
    set(s, 31, read<4>(s, reg(s, 2) - 0x7b84, 0x10712a68));
    write<4>(s, reg(s, 1) - 8, reg(s, 30), 0x10712a6c);
    set(s, 30, read<4>(s, reg(s, 2) - 0x7b90, 0x10712a70));
    write<4>(s, reg(s, 1) - 12, reg(s, 29), 0x10712a74);
    write<4>(s, reg(s, 1) - 16, reg(s, 28), 0x10712a78);
    set(s, 28, reg(s, 3)); record(s, 28);
    write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x10712a80);
    push_frame(s, 96, 0x10712a84);
    bool usable = reg(s, 28) != 0;
    if (usable) {
        set(s, 0, read<4>(s, reg(s, 28) + 8, 0x10712a94));
        set(s, 0, std::uint32_t(effective_count(reg(s, 0)))); record(s, 0);
        usable = reg(s, 0) != 0;
    }
    if (!usable) set(s, 3, 0);
    else {
        set(s, 29, std::uint32_t(effective_count(reg(s, 0)))); record(s, 29);
        consume_pool_index(s, true);
        set(s, 3, reg(s, 28)); set(s, 4, reg(s, 30));
        call(s, 0x1074dd40, 0x10712c18);
        compare(s, reg(s, 3), 0);
        if (reg(s, 3)) set(s, 3, read<4>(s, reg(s, 3), 0x10712c20));
    }
    restore_list_frame(s);
}

bool backgrounds_enabled(State& s, std::uint32_t site) {
    set(s, 0, read<1>(s, reg(s, 2) - 0x35d4, site));
    set(s, 0, std::uint32_t(effective_selector(reg(s, 0)))); record(s, 0);
    return reg(s, 0) != 0;
}
void restore_category_frame(State& s, bool object) {
    set(s, 0, read<4>(s, reg(s, 1) + 72, object ? 0x10713a60 : 0x107139a4));
    set(s, 1, reg(s, 1) + 64);
    s.wr<4>(4128, reg(s, 0));
    if (object) set(s, 31, read<4>(s, reg(s, 1) - 4, 0x10713a6c));
    s.pc = std::uint32_t(s.r<4>(4128));
}
void select_default_category(State& s) {
    set(s, 0, s.r<4>(4128));
    write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x10713964);
    push_frame(s, 64, 0x10713968);
    if (!backgrounds_enabled(s, 0x1071396c)) set(s, 3, 0);
    else {
        set(s, 4, read<4>(s, reg(s, 2) - 0x5c3c, 0x10713980));
        set(s, 0, (reg(s, 3) << 2) & 0x3fc);
        set(s, 3, read<4>(s, reg(s, 4) + reg(s, 0), 0x10713988));
        compare(s, reg(s, 3), 0);
        if (reg(s, 3)) {
            set(s, 4, 0);
            call(s, 0x107122e0, 0x1071399c);
        }
    }
    restore_category_frame(s, false);
}
void select_object_category(State& s) {
    set(s, 0, s.r<4>(4128));
    write<4>(s, reg(s, 1) - 4, reg(s, 31), 0x107139c4);
    set(s, 31, reg(s, 6));
    write<4>(s, reg(s, 1) + 8, reg(s, 0), 0x107139cc);
    push_frame(s, 64, 0x107139d0);
    if (!backgrounds_enabled(s, 0x107139d4)) set(s, 3, 0);
    else {
        set(s, 0, std::uint32_t(effective_count(reg(s, 3))));
        set(s, 3, read<4>(s, reg(s, 2) - 0x5c3c, 0x107139ec));
        // This entry sign-extends the category, unlike the default mask.
        set(s, 0, reg(s, 0) << 2);
        set(s, 3, read<4>(s, reg(s, 3) + reg(s, 0), 0x107139f4));
        compare(s, reg(s, 3), 0);
        if (reg(s, 3)) {
            compare(s, reg(s, 4), 0);
            if (!reg(s, 4)) {
                set(s, 4, reg(s, 31));
                call(s, 0x107122e0, 0x10713a18);
            } else {
                set(s, 4, reg(s, 5));
                call(s, 0x1074df90, 0x10713a24);
                compare(s, reg(s, 3), 0);
                if (reg(s, 3)) {
                    set(s, 3, read<4>(s, reg(s, 3) + 4, 0x10713a34));
                    compare(s, reg(s, 3), 0);
                    if (reg(s, 3)) {
                        set(s, 0, std::uint32_t(effective_selector(reg(s, 31)))); record(s, 0);
                        const bool alternate = reg(s, 0) != 0;
                        if (alternate) set(s, 3, reg(s, 3) + 12);
                        call(s, 0x10712a60, alternate ? 0x10713a58 : 0x10713a60);
                    }
                }
            }
        }
    }
    restore_category_frame(s, true);
}
}

bool try_modern_background_selection(State& s, std::uint32_t entry) {
    s.pc = entry;
    if (s.background_original_resume == entry) { s.background_original_resume = 0; return false; }
    const auto found = std::find(background_selection_entries.begin(), background_selection_entries.end(), entry);
    if (found == background_selection_entries.end()) throw std::runtime_error("Unknown background selection entry");
    const auto index = std::size_t(found - background_selection_entries.begin());
    const bool original = s.background_original || s.background_entry_original[index];
    ++(original ? s.background_original_calls[index] : s.background_calls[index]);
    if (original && !s.background_observer) return false;
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (s.background_observer) s.background_observer->enter(s, entry, original);
    if (original) {
        s.background_original_resume = entry;
        try { run_until(s, caller); }
        catch (...) { s.background_original_resume = 0; throw; }
    } else {
        s.step(entry);
        switch (index) {
        case 0: select_catalog(s); break;
        case 1: select_list(s); break;
        case 2: select_default_category(s); break;
        case 3: select_object_category(s); break;
        }
    }
    if (s.background_observer) s.background_observer->leave(s, entry, original);
    return true;
}
void reset_background_diagnostics(State& s) {
    s.background_original = false;
    s.background_entry_original.fill(false);
    s.background_calls.fill(0); s.background_original_calls.fill(0);
    s.background_original_resume = 0;
    s.background_observer = nullptr;
}
std::string background_selection_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < background_selection_entries.size(); ++i)
        out << "background.entry=" << hex_address(background_selection_entries[i])
            << " mode=" << (s.background_original || s.background_entry_original[i] ? "original" : "modern")
            << " modern=" << s.background_calls[i] << " original=" << s.background_original_calls[i] << '\n';
    return out.str();
}
}
