#include "register_context.hpp"
#include "mac_abi.hpp"

namespace cm::aot::register_context {
namespace {
struct RegisterAbi : MacAbi {
    std::uint64_t escapes = 0;
    explicit RegisterAbi(State& state) : MacAbi(state, escapes) {}
    std::uint64_t floating(unsigned n) const { return s.r<8>(0x4000 + n * 16); }
    void floating_store(unsigned n, std::uint32_t address, std::uint32_t pc) {
        s.pc = pc; s.memory.write<8>(address, floating(n));
    }
    void floating_load(unsigned n, std::uint32_t address, std::uint32_t pc) {
        s.pc = pc; s.wr<8>(0x4000 + n * 16, s.memory.read<8>(address));
    }
    std::uint32_t condition_word() const {
        std::uint32_t word = 0;
        for (unsigned n = 0; n < 8; ++n) word |= std::uint32_t(s.r<1>(2304 + n) & 15) << (28 - n * 4);
        return word;
    }
    void restore_condition(std::uint32_t word) {
        for (unsigned n = 0; n < 8; ++n) s.wr<1>(2304 + n, (word >> (28 - n * 4)) & 15);
    }
    std::uint32_t floating_status() const {
        std::uint32_t word = 0;
        for (unsigned n = 0; n < 32; ++n) word |= std::uint32_t(s.r<1>(1280 + n) & 1) << (31 - n);
        return word;
    }
    void restore_floating_status(std::uint32_t word) {
        for (unsigned n = 0; n < 32; ++n) s.wr<1>(1280 + n, (word >> (31 - n)) & 1);
    }
    void general_suffix(std::uint32_t pc, bool restoring) {
        s.wr<4>(2432, r(3) + 20);
        for (unsigned n = 13; n < 32; ++n) {
            const auto address = std::uint32_t(s.r<4>(2432));
            if (restoring) load(n, address, pc); else store(address, r(n), pc);
            s.wr<4>(2432, std::uint32_t(s.r<4>(2432)) + 4);
        }
    }
};
void floating_suffix(State& state, unsigned first, bool restoring) {
    if (first < 18 || first >= 32) throw std::runtime_error("Unknown Mac floating-register suffix");
    RegisterAbi a(state);
    const auto root = restoring ? 0x10773830u : 0x107737e0u;
    for (unsigned n = first; n < 32; ++n) {
        const auto address = a.r(1) - (32 - n) * 8, pc = root + (n - 18) * 4;
        if (restoring) a.floating_load(n, address, pc); else a.floating_store(n, address, pc);
    }
    a.finish();
}
}
void save_floating_suffix(State& state, unsigned first) { floating_suffix(state, first, false); }
void restore_floating_suffix(State& state, unsigned first) { floating_suffix(state, first, true); }
void capture_nonlocal_context(State& state) {
    RegisterAbi a(state);
    a.put(5, std::uint32_t(state.r<4>(4128))); a.put(6, a.condition_word());
    scalar_context_capture_link(state, a.r(5));
    a.store(a.r(3), a.r(5), 0x10773eb8); a.store(a.r(3) + 4, a.r(6), 0x10773ebc);
    scalar_context_capture_stack(state, a.r(1));
    a.store(a.r(3) + 8, a.r(1), 0x10773ec0); a.store(a.r(3) + 12, a.r(2), 0x10773ec4);
    a.general_suffix(0x10773ec8, false);
    state.wr<8>(0x4000, a.floating_status());
    for (unsigned n = 14; n < 32; ++n)
        a.floating_store(n, a.r(3) + 96 + (n - 14) * 8, 0x10773ed0 + (n - 14) * 4);
    a.floating_store(0, a.r(3) + 240, 0x10773f18);
    a.put(3, 0); a.finish();
}
void restore_nonlocal_context(State& state) {
    RegisterAbi a(state);
    a.load(5, a.r(3), 0x10773f30); a.load(6, a.r(3) + 4, 0x10773f34);
    state.wr<4>(4128, a.r(5)); a.restore_condition(a.r(6));
    a.load(1, a.r(3) + 8, 0x10773f40); a.load(2, a.r(3) + 12, 0x10773f44);
    a.general_suffix(0x10773f48, true);
    for (unsigned n = 14; n < 31; ++n)
        a.floating_load(n, a.r(3) + 96 + (n - 14) * 8, 0x10773f4c + (n - 14) * 4);
    // The status scratch is loaded before f31, and published only afterwards.
    // Keep those live accesses separate, including their fault prefixes.
    a.floating_load(0, a.r(3) + 240, 0x10773f90);
    a.floating_load(31, a.r(3) + 232, 0x10773f94);
    a.compare(std::bit_cast<std::int32_t>(a.r(4))); a.put(3, a.r(4));
    a.restore_floating_status(std::uint32_t(a.floating(0)));
    if (state.r<1>(2304) & 2) a.put(3, 1);
    a.finish();
}
}
