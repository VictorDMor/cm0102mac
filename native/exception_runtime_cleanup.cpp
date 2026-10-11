#include "exception_runtime_context.hpp"
#include "mac_abi.hpp"

namespace cm::aot::exception_runtime {
namespace {
struct CleanupAbi : MacAbi {
    std::uint32_t guest_return;
    explicit CleanupAbi(State& state) : MacAbi(state, state.exception_runtime_dispatch_escapes), guest_return(std::uint32_t(state.r<4>(4128))) {}
    std::int32_t signed_r(unsigned n) const { return std::bit_cast<std::int32_t>(r(n)); }
    void signed_half(unsigned n, std::uint32_t value) { put(n, std::uint32_t(std::int32_t(std::int16_t(value)))); }
    void signed_byte(unsigned n, std::uint32_t value) { put(n, std::uint32_t(std::int32_t(std::int8_t(value)))); }
    void offset(unsigned n, unsigned field, bool wide, std::uint32_t pc) {
        if (wide) load(n, r(31) + field, pc);
        else { load<2>(n, r(31) + field, pc); signed_half(n, r(n)); }
    }
    void callback(std::uint32_t link) { call(0x10781320, link); load(2, r(1) + 20, link); }
    void advance(unsigned size, std::uint32_t pc) {
        load(3, r(28) + 8, pc); put(0, r(3) + size); store(r(28) + 8, r(0), pc + 8);
    }
    void saved_frame() {
        put(3, r(27)); put(4, r(28));
        s.wr<4>(2440, r(2)); s.wr<4>(4128, 0x10771b64); s.pc = 0x107717f0;
        invoke_exception_runtime(s, 0x107717f0);
        put(4, r(28)); call(0x107713e0, 0x10771b6c);
        load(0, r(28), 0x10771b6c); compare(r(0)); if (!r(0)) call(0x10772eb0, 0x10771b7c);
        load(3, r(28), 0x10771b7c); load<1>(0, r(3) + 1, 0x10771b80);
        put(0, (r(0) >> 4) & 1); record(r(0));
        load(0, r(27) + (r(0) ? 0x394 : 0x39c), r(0) ? 0x10771b8c : 0x10771b94);
        store(r(27) + 0x3a0, r(0), 0x10771b98);
    }
    // A location is either a live frame offset or an index into the saved GPR
    // context. The flag, index/offset, and selected value are all guest reads.
    void pointer(std::uint32_t pc, bool wide, unsigned field, unsigned dest = 3, unsigned flag = 7) {
        load<1>(0, r(31) + 1, pc); put(0, (r(0) >> flag) & 1); record(r(0));
        if (r(0)) {
            offset(0, field, wide, pc + 12); put(3, r(0) * 4); put(0, r(3) + 0x318);
            load(dest, r(27) + r(0), pc + 24);
        } else {
            load(3, r(27) + 0x3a0, pc + 32); offset(0, field, wide, pc + 36);
            load(dest, r(3) + r(0), pc + 40);
        }
    }
    void condition(std::uint32_t pc, bool wide, bool half = false, unsigned base = 3) {
        load<1>(0, r(31) + 1, pc); put(0, (r(0) >> 7) & 1); record(r(0));
        if (r(0)) {
            offset(0, 2, wide, pc + 12); put(base, r(0) * 4); put(0, r(base) + 0x318);
            load(0, r(27) + r(0), pc + 24);
            if (half) signed_half(0, r(0)); else signed_byte(0, r(0));
        } else {
            load(base, r(27) + 0x3a0, pc + 36); offset(0, 2, wide, pc + 40);
            if (half) { load<2>(0, r(base) + r(0), pc + 44); signed_half(0, r(0)); }
            else load<1>(0, r(base) + r(0), pc + 44);
        }
        if (half) signed_half(0, r(0)); else signed_byte(0, r(0));
        record(r(0));
    }
    void destroy_inline(bool wide) {
        const auto pc = wide ? 0x10771c04u : 0x10771bd8u;
        load(3, r(27) + 0x3a0, pc); put(4, 0xffffffff); offset(0, 2, wide, pc + 8);
        load(12, r(31) + (wide ? 6 : 4), pc + 12); put(3, r(3) + r(0)); callback(pc + 24);
        advance(wide ? 10 : 8, pc + 28);
    }
    void destroy_conditional_inline(bool wide) {
        const auto pc = wide ? 0x10771c94u : 0x10771c30u;
        condition(pc, wide);
        if (r(0)) {
            load(3, r(27) + 0x3a0, pc + 56); put(4, 0xffffffff);
            offset(0, wide ? 6 : 4, wide, pc + 64); load(12, r(31) + (wide ? 10 : 6), pc + 68);
            put(3, r(3) + r(0)); callback(pc + 80);
        }
        advance(wide ? 14 : 10, pc + 84);
    }
    void destroy_pointer(bool wide) {
        const auto pc = wide ? 0x10771d44u : 0x10771cf8u;
        pointer(pc, wide, 2); load(12, r(31) + (wide ? 6 : 4), pc + 44);
        put(4, 0xffffffff); callback(pc + 56); advance(wide ? 10 : 8, pc + 60);
    }
    void reverse_inline_array(bool wide) {
        const auto pc = wide ? 0x10771de8u : 0x10771d90u;
        const auto cursor = wide ? 26u : 24u, stride = wide ? 24u : 26u;
        if (wide) { load(25, r(31) + 6, pc); load(stride, r(31) + 10, pc + 4); }
        else { load<2>(25, r(31) + 4, pc); load<2>(stride, r(31) + 6, pc + 4); }
        load(4, r(27) + 0x3a0, pc + 8); put(0, r(stride) * r(25)); offset(3, 2, wide, pc + 16);
        put(cursor, r(4) + r(3)); put(cursor, r(cursor) + r(0));
        for (;;) {
            compare(signed_r(25)); if (signed_r(25) <= 0) break;
            put(cursor, r(cursor) - r(stride)); load(12, r(31) + (wide ? 14 : 8), pc + 40);
            put(3, r(cursor)); put(4, 0xffffffff); callback(pc + 56); put(25, r(25) - 1); s.step(pc + 64);
        }
        advance(wide ? 18 : 12, pc + 72);
    }
    void destroy_adjusted_pointer(bool wide, bool complete) {
        const auto pc = wide ? (complete ? 0x10771f3cu : 0x10771e94u) : (complete ? 0x10771ee8u : 0x10771e40u);
        pointer(pc, wide, 2); load(0, r(31) + (wide ? 6 : 4), pc + 44); put(4, complete ? 0xffffffff : 0);
        load(12, r(31) + (wide ? 10 : 8), pc + 52); put(3, r(3) + r(0)); callback(pc + 64);
        advance(wide ? 14 : 12, pc + 68);
    }
    void destroy_conditional_adjusted(bool wide) {
        const auto pc = wide ? 0x1077201cu : 0x10771f90u;
        pointer(pc, wide, wide ? 6 : 4, 5, 6); condition(pc + 44, wide, true);
        if (r(0)) {
            load(0, r(31) + (wide ? 10 : 6), pc + 100); put(4, 0);
            load(12, r(31) + (wide ? 14 : 10), pc + 108); put(3, r(5) + r(0)); callback(pc + 120);
        }
        advance(wide ? 18 : 14, pc + 124);
    }
    void reverse_adjusted_array(bool wide) {
        const auto pc = wide ? 0x10772128u : 0x107720a8u;
        pointer(pc, wide, 2, 26); load(25, r(31) + (wide ? 10 : 8), pc + 44);
        load(24, r(31) + (wide ? 14 : 12), pc + 48); load(3, r(31) + (wide ? 6 : 4), pc + 52);
        put(0, r(24) * r(25)); put(26, r(26) + r(3)); put(26, r(26) + r(0));
        for (;;) {
            compare(signed_r(25)); if (signed_r(25) <= 0) break;
            put(26, r(26) - r(24)); load(12, r(31) + (wide ? 18 : 16), pc + 80);
            put(3, r(26)); put(4, 0xffffffff); callback(pc + 96); put(25, r(25) - 1); s.step(pc + 104);
        }
        advance(wide ? 22 : 20, pc + 112);
    }
    void call_pointer(bool wide) {
        const auto pc = wide ? 0x107721f0u : 0x107721a8u;
        pointer(pc, wide, 2); load(12, r(31) + (wide ? 6 : 4), pc + 44); callback(pc + 52);
        advance(wide ? 10 : 8, pc + 56);
    }
    void call_conditional_pointer(bool wide) {
        const auto pc = wide ? 0x107722b8u : 0x10772238u;
        pointer(pc, wide, wide ? 6 : 4, 3, 6); condition(pc + 44, wide, false, 4);
        if (r(0)) { load(12, r(31) + (wide ? 10 : 6), pc + 100); callback(pc + 108); }
        advance(wide ? 14 : 10, pc + 112);
    }
    void release_exception_object(bool wide) {
        const auto pc = wide ? 0x10772398u : 0x1077234cu;
        load(3, r(27) + 0x3a0, pc); offset(0, 2, wide, pc + 4); put(3, r(3) + r(0));
        load(12, r(3) + 8, pc + 12); compare(r(12));
        if (r(12)) {
            load(0, r(27) + 0x3b0, pc + 24); load(3, r(3), pc + 28); compare(r(0), r(3));
            if (r(0) == r(3)) store(r(27) + 0x3b4, r(12), pc + 40);
            else { put(4, 0xffffffff); callback(pc + 56); }
        }
        advance(wide ? 6 : 4, pc + 60);
    }
};
}
void cleanup_actions(State& state) {
    CleanupAbi a(state);
    a.capture_link(); a.words(24, 0x10771b34); a.put(27, a.r(3)); a.put(28, a.r(4)); a.put(29, a.r(5));
    a.save_link(0x10771b44); a.frame(96, 0x10771b48);
    for (;;) {
        a.load(31, a.r(28) + 8, 0x10771b4c); a.compare(a.r(31));
        if (!a.r(31)) { a.saved_frame(); state.step(0x10771b4c); continue; }
        a.load<1>(30, a.r(31), 0x10771ba0); a.put(0, a.r(30) & 0x7f); a.compare(a.r(0), 27);
        bool stop = false;
        if (a.r(0) > 27) a.call(0x10772eb0, 0x10772408);
        else {
            a.load(3, a.r(2) - 0x3b8c, 0x10771bb0); a.put(0, a.r(0) * 4);
            a.load(3, a.r(3) + a.r(0), 0x10771bb8); state.wr<4>(4132, a.r(3));
            switch (a.r(3)) {
                case 0x10771bc4:
                    a.load(3, a.r(28), 0x10771bc4); a.load<2>(0, a.r(31) + 2, 0x10771bc8);
                    a.put(0, a.r(3) + a.r(0)); a.store(a.r(28) + 8, a.r(0), 0x10771bd0); break;
                case 0x10771bd8: a.destroy_inline(false); break;
                case 0x10771c04: a.destroy_inline(true); break;
                case 0x10771c30: a.destroy_conditional_inline(false); break;
                case 0x10771c94: a.destroy_conditional_inline(true); break;
                case 0x10771cf8: a.destroy_pointer(false); break;
                case 0x10771d44: a.destroy_pointer(true); break;
                case 0x10771d90: a.reverse_inline_array(false); break;
                case 0x10771de8: a.reverse_inline_array(true); break;
                case 0x10771e40: a.destroy_adjusted_pointer(false, false); break;
                case 0x10771e94: a.destroy_adjusted_pointer(true, false); break;
                case 0x10771ee8: a.destroy_adjusted_pointer(false, true); break;
                case 0x10771f3c: a.destroy_adjusted_pointer(true, true); break;
                case 0x10771f90: a.destroy_conditional_adjusted(false); break;
                case 0x1077201c: a.destroy_conditional_adjusted(true); break;
                case 0x107720a8: a.reverse_adjusted_array(false); break;
                case 0x10772128: a.reverse_adjusted_array(true); break;
                case 0x107721a8: a.call_pointer(false); break;
                case 0x107721f0: a.call_pointer(true); break;
                case 0x10772238: a.call_conditional_pointer(false); break;
                case 0x107722b8: a.call_conditional_pointer(true); break;
                case 0x10772338:
                    a.compare(a.r(29), a.r(31)); stop = a.r(29) == a.r(31);
                    if (!stop) { a.put(0, a.r(31) + 10); a.store(a.r(28) + 8, a.r(0), 0x10772344); } break;
                case 0x1077234c: a.release_exception_object(false); break;
                case 0x10772398: a.release_exception_object(true); break;
                case 0x107723e4:
                    a.compare(a.r(29), a.r(31)); stop = a.r(29) == a.r(31);
                    if (!stop) {
                        a.load<2>(0, a.r(31) + 2, 0x107723ec); a.put(0, a.r(0) * 4); a.put(3, a.r(0) + a.r(31));
                        a.put(0, a.r(3) + 12); a.store(a.r(28) + 8, a.r(0), 0x107723fc);
                    } break;
                case 0x10772404: a.call(0x10772eb0, 0x10772408); break;
                default:
                    resume_exception_unknown_transfer(state, a.r(3), a.guest_return); return;
            }
        }
        if (stop) break;
        a.put(0, a.r(30) & 0x80); a.record(a.r(0));
        if (a.r(0)) { a.put(0, 0); a.store(a.r(28) + 8, a.r(0), 0x10772414); }
        state.step(0x10771b4c);
    }
    a.pop(96, 0x1077241c); a.restore_link(); a.words(24, 0x10772428, true); a.finish();
}
}
