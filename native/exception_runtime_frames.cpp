#include "exception_runtime_context.hpp"
#include "mac_abi.hpp"
#include <string>

namespace cm::aot::exception_runtime {
namespace {
struct FrameAbi : MacAbi {
    std::uint32_t guest_return;
    explicit FrameAbi(State& state) : MacAbi(state, state.exception_runtime_dispatch_escapes), guest_return(std::uint32_t(state.r<4>(4128))) {}
    std::int32_t signed_r(unsigned n) const { return std::bit_cast<std::int32_t>(r(n)); }
    void suffix_count(unsigned dest, unsigned source) {
        s.wr<1>(1027, r(source) <= 32);
        put(dest, 32 - r(source));
    }
    template<bool Floating> void saved_scalar_read(std::uint32_t address, std::uint32_t pc) {
        if constexpr (Floating) {
            s.pc = pc; s.wr<8>(0x4000, s.memory.read<8>(address));
        } else load(8, address, pc);
    }
    template<bool Floating> void saved_scalar_write(std::uint32_t pc) {
        if constexpr (Floating) {
            s.pc = pc; s.memory.write<8>(r(3) + r(0), s.r<8>(0x4000));
        } else store(r(3) + r(0), r(8), pc);
    }
    // The original batches pipeline destination scratch and advance both
    // cursors around the final load/store. Keep those fault-visible boundaries.
    template<bool Floating> void copy_scalar_batch() {
        constexpr unsigned width = Floating ? 8 : 4, index = Floating ? 8 : 9;
        constexpr auto base = Floating ? 0x218u : 0x318u;
        constexpr auto pc = Floating ? 0x10771830u : 0x10771908u;
        saved_scalar_read<Floating>(r(7), pc);
        put(0, r(index) + base); saved_scalar_write<Floating>(pc + 8);
        for (unsigned n = 1; n < 7; ++n) {
            put(0, r(index) + base + n * width);
            saved_scalar_read<Floating>(r(7) + n * width, pc + n * 12 + 4);
            saved_scalar_write<Floating>(pc + n * 12 + 8);
        }
        put(0, r(index) + base + 7 * width); put(index, r(index) + 8 * width);
        saved_scalar_read<Floating>(r(7) + 7 * width, pc + 0x5c);
        put(7, r(7) + 8 * width); saved_scalar_write<Floating>(pc + 0x64);
    }
    template<bool Floating> void copy_scalar_tail() {
        constexpr unsigned width = Floating ? 8 : 4, index = Floating ? 8 : 9;
        constexpr auto base = Floating ? 0x218u : 0x318u;
        constexpr auto pc = Floating ? 0x107718a8u : 0x10771980u;
        saved_scalar_read<Floating>(r(7), pc); put(0, r(index) + base);
        put(index, r(index) + width); put(7, r(7) + width);
        saved_scalar_write<Floating>(pc + 16);
    }
    void copy_vector_batch() {
        put(8, r(11) + r(9)); put(10, r(3) + r(7));
        load(0, r(8), 0x10771a08); put(9, r(9) + 16); put(7, r(7) + 16);
        store(r(10), r(0), 0x10771a14);
        load(0, r(8) + 4, 0x10771a18); store(r(10) + 4, r(0), 0x10771a1c);
        load(0, r(8) + 8, 0x10771a20); store(r(10) + 8, r(0), 0x10771a24);
        load(0, r(8) + 12, 0x10771a28);
        for (unsigned n = 1; n < 4; ++n) {
            const auto pc = 0x10771a34u + (n - 1) * 0x30;
            put(8, r(11) + r(9)); put(9, r(9) + 16); store(r(10) + 12, r(0), pc);
            put(10, r(3) + r(7)); put(7, r(7) + 16);
            load(0, r(8), pc + 12); store(r(10), r(0), pc + 16);
            load(0, r(8) + 4, pc + 20); store(r(10) + 4, r(0), pc + 24);
            load(0, r(8) + 8, pc + 28); store(r(10) + 8, r(0), pc + 32);
            load(0, r(8) + 12, pc + 36);
        }
        store(r(10) + 12, r(0), 0x10771abc);
    }
    void copy_vector_tail() {
        put(8, r(11) + r(9)); put(10, r(3) + r(7)); load(0, r(8), 0x10771ad8);
        put(7, r(7) + 16); put(9, r(9) + 16); store(r(10), r(0), 0x10771ae4);
        load(0, r(8) + 4, 0x10771ae8); store(r(10) + 4, r(0), 0x10771aec);
        load(0, r(8) + 8, 0x10771af0); store(r(10) + 8, r(0), 0x10771af4);
        load(0, r(8) + 12, 0x10771af8); store(r(10) + 12, r(0), 0x10771afc);
    }
};
}
void advance_action(State& state) {
    FrameAbi a(state);
    a.capture_link(); a.save(31, 0x10771644); a.put(31, a.r(3)); a.save(30, 0x1077164c);
    a.save_link(0x10771650); a.frame(64, 0x10771654);
    for (;;) {
        a.load(4, a.r(31) + 8, 0x10771658); a.compare(a.r(4));
        bool next_frame = !a.r(4);
        if (!next_frame) {
            a.load<1>(5, a.r(4), 0x10771664); a.put(0, a.r(5) & 0x80); a.record(a.r(0));
            next_frame = a.r(0) != 0;
        }
        if (!next_frame) {
            a.compare(a.r(5), 15);
            if (a.r(5) > 15) { a.call(0x10772eb0, 0x107717a8); break; }
            a.load(3, a.r(2) - 0x3b88, 0x107716f0); a.put(0, a.r(5) * 4);
            a.load(3, a.r(3) + a.r(0), 0x107716f8); state.wr<4>(4132, a.r(3));
            // The mutable guest table selects code destinations, not merely
            // record kinds. Relocated tables may redirect a kind to any case.
            const auto advance = [&](std::uint32_t size, std::uint32_t pc) {
                a.put(0, a.r(4) + size); a.store(a.r(31) + 8, a.r(0), pc);
            };
            switch (a.r(3)) {
                case 0x10771704: advance(8, 0x10771708); break;
                case 0x10771710: advance(10, 0x10771714); break;
                case 0x1077171c: advance(8, 0x10771720); break;
                case 0x10771728: advance(12, 0x1077172c); break;
                case 0x10771734:
                    a.load(3, a.r(31) + 8, 0x10771734); a.put(0, a.r(3) + 12);
                    a.store(a.r(31) + 8, a.r(0), 0x1077173c); break;
                case 0x10771744: advance(14, 0x10771748); break;
                case 0x10771750: advance(20, 0x10771754); break;
                case 0x1077175c: advance(8, 0x10771760); break;
                case 0x10771768: advance(10, 0x1077176c); break;
                case 0x10771774: advance(10, 0x10771778); break;
                case 0x10771780: advance(4, 0x10771784); break;
                case 0x1077178c:
                    a.load<2>(0, a.r(4) + 2, 0x1077178c); a.put(0, a.r(0) * 4);
                    a.put(3, a.r(0) + a.r(4)); a.put(0, a.r(3) + 12);
                    a.store(a.r(31) + 8, a.r(0), 0x1077179c); break;
                case 0x107717a4: a.call(0x10772eb0, 0x107717a8); break;
                default:
                    resume_exception_unknown_transfer(state, a.r(3), a.guest_return); return;
            }
            break;
        }
        a.load(3, a.r(31), 0x10771670); a.load(4, a.r(31) + 0x18, 0x10771674);
        a.load<1>(0, a.r(3), 0x10771678); a.load(30, a.r(4), 0x1077167c);
        a.put(0, (a.r(0) >> 3) & 31); a.record(a.r(0));
        if (a.r(0)) {
            a.put(3, a.r(30)); a.put(4, a.r(31)); a.call(0x107715f0, 0x10771694);
            a.store(a.r(31) + 0x20, a.r(3), 0x10771694);
        }
        a.load(3, a.r(30) + 8, 0x10771698); a.put(4, a.r(31)); a.call(0x107713e0, 0x107716a4);
        a.load(0, a.r(31), 0x107716a4); a.compare(a.r(0));
        if (!a.r(0)) a.call(0x10772eb0, 0x107716b4);
        a.store(a.r(31) + 0x18, a.r(30), 0x107716b4); a.load(3, a.r(31), 0x107716b8);
        a.load<1>(0, a.r(3) + 1, 0x107716bc); a.put(0, (a.r(0) >> 4) & 1); a.record(a.r(0));
        a.load(0, a.r(31) + (a.r(0) ? 0x20 : 0x18), a.r(0) ? 0x107716c8 : 0x107716d0);
        a.store(a.r(31) + 0x1c, a.r(0), 0x107716d4); a.load(0, a.r(31) + 8, 0x107716d8);
        a.compare(a.r(0)); if (a.r(0)) break;
        state.step(0x10771658);
    }
    a.load(4, a.r(31) + 8, 0x107717a8); a.load<1>(0, a.r(4), 0x107717ac);
    a.put(3, a.r(0) & 0x7f); a.compare(a.r(3), 1);
    if (a.r(3) == 1) {
        a.load(3, a.r(31), 0x107717bc); a.load<2>(0, a.r(4) + 2, 0x107717c0);
        a.put(0, a.r(3) + a.r(0)); a.store(a.r(31) + 8, a.r(0), 0x107717c8);
        a.load(3, a.r(31) + 8, 0x107717cc); a.load<1>(0, a.r(3), 0x107717d0); a.put(3, a.r(0) & 0x7f);
    }
    a.pop(64, 0x107717d8); a.restore_link(); a.restore(31, 0x107717e4); a.restore(30, 0x107717e8); a.finish();
}
void copy_saved_registers(State& state) {
    FrameAbi a(state);
    a.load(5, a.r(4), 0x107717f0); a.load(6, a.r(3) + 0x39c, 0x107717f4);
    a.load<2>(0, a.r(5), 0x107717f8); a.load(5, a.r(6), 0x107717fc);
    a.put(10, (a.r(0) >> 6) & 31); a.put(0, (a.r(0) >> 3) & 0xf8);
    a.suffix_count(6, 10); a.compare(a.signed_r(6), 32);
    a.put(9, a.r(5) - a.r(0)); a.put(7, a.r(9)); a.put(8, a.r(6) * 8);
    a.suffix_count(6, 6);
    if (a.r(6)) {
        a.put(0, a.r(6) >> 3); a.record(a.r(0)); state.wr<4>(4132, a.r(0));
        if (a.r(0)) {
            do { a.copy_scalar_batch<true>(); } while (a.count_down());
            a.put(6, a.r(6) & 7); a.record(a.r(6));
        }
        if (a.r(6)) {
            state.wr<4>(4132, a.r(6));
            do { a.copy_scalar_tail<true>(); } while (a.count_down());
        }
    }
    a.load(6, a.r(4), 0x107718c0); a.put(0, a.r(10) & 1); a.record(a.r(0));
    const bool floating_padding = a.r(0) != 0;
    a.put(10, a.r(9)); a.load<1>(0, a.r(6), 0x107718cc);
    a.put(0, (a.r(0) >> 3) & 31); if (floating_padding) a.put(10, a.r(9) - 8);
    a.suffix_count(6, 0); a.put(0, a.r(0) * 4); a.compare(a.signed_r(6), 32);
    a.put(10, a.r(10) - a.r(0)); a.put(9, a.r(6) * 4); a.put(7, a.r(10)); a.suffix_count(6, 6);
    if (a.r(6)) {
        a.put(0, a.r(6) >> 3); a.record(a.r(0)); state.wr<4>(4132, a.r(0));
        if (a.r(0)) {
            do { a.copy_scalar_batch<false>(); } while (a.count_down());
            a.put(6, a.r(6) & 7); a.record(a.r(6));
        }
        if (a.r(6)) {
            state.wr<4>(4132, a.r(6));
            do { a.copy_scalar_tail<false>(); } while (a.count_down());
        }
    }
    a.load(6, a.r(4), 0x10771998); a.load<1>(0, a.r(6) + 1, 0x1077199c);
    a.put(0, (a.r(0) >> 2) & 1); a.record(a.r(0));
    if (a.r(0)) {
        a.load<1>(0, a.r(6) + 2, 0x107719a8); a.put(6, 4); a.put(9, 0);
        a.put(8, (a.r(0) >> 2) & 1); a.put(11, (a.r(0) >> 3) & 31);
        a.put(7, 0 - a.r(8)); a.put(0, (a.r(0) << 1) & 0x1f0); a.put(7, a.r(7) | a.r(8));
        state.wr<1>(1027, (a.r(7) & 0x80000000) && (a.r(7) & 0x7fffffff));
        a.put(7, std::uint32_t(a.signed_r(7) >> 31)); a.put(6, a.r(6) & a.r(7));
        a.suffix_count(8, 11); a.put(6, a.r(6) * 16); a.put(6, a.r(10) - a.r(6));
        a.compare(a.signed_r(8), 32); a.put(6, a.r(6) - a.r(0));
        a.put(7, a.r(8) * 16); a.put(11, a.r(6) & ~std::uint32_t(15)); a.suffix_count(6, 8);
        if (a.r(6)) {
            a.put(0, a.r(6) >> 2); a.record(a.r(0)); state.wr<4>(4132, a.r(0));
            if (a.r(0)) {
                do { a.copy_vector_batch(); } while (a.count_down());
                a.put(6, a.r(6) & 3); a.record(a.r(6));
            }
            if (a.r(6)) {
                state.wr<4>(4132, a.r(6));
                do { a.copy_vector_tail(); } while (a.count_down());
            }
        }
    }
    a.load(4, a.r(4), 0x10771b04); a.load<1>(0, a.r(4) + 1, 0x10771b08);
    a.put(0, (a.r(0) >> 5) & 1); a.record(a.r(0));
    if (a.r(0)) { a.load(0, a.r(5) + 4, 0x10771b14); a.store(a.r(3) + 0x398, a.r(0), 0x10771b18); }
    a.store(a.r(3) + 0x39c, a.r(5), 0x10771b1c); a.load(3, a.r(5) + 8, 0x10771b20); a.finish();
}
}
