#include "code_fragment_internal.hpp"

namespace cm::aot::code_fragment {
namespace {
void short_actions(Abi& a, bool extended) {
    a.put(4, a.r(5) + (extended ? 4 : 2));
    const auto loop = extended ? 0x10771584u : 0x107715ccu;
    for (;;) {
        a.load<2>(3, a.r(4), loop); a.compare(a.r(3)); if (!a.r(3)) return;
        a.compare(a.r(3), a.r(0));
        if (a.r(3) <= a.r(0)) {
            a.load<2>(3, a.r(4) + 2, extended ? 0x10771560 : 0x107715a8); a.compare(a.r(3), a.r(0));
            if (a.r(3) >= a.r(0)) {
                a.load<2>(0, a.r(4) + 4, extended ? 0x1077156c : 0x107715b4); a.put(0, a.r(5) + a.r(0));
                a.store(a.r(31) + 8, a.r(0), extended ? 0x10771574 : 0x107715bc); return;
            }
        }
        a.put(4, a.r(4) + 6); a.s.step(loop);
    }
}
}
void resolve_metadata(Abi& a) {
    a.capture_link(); a.save(31, 0x107713e4); a.put(31, a.r(4)); a.save(30, 0x107713ec); a.put(30, a.r(3));
    a.save_link(0x107713f4); a.put(0, 0); a.frame(64, 0x107713fc);
    a.store(a.r(4), a.r(0), 0x10771400); a.store(a.r(4) + 8, a.r(0), 0x10771404);
    a.call(0x107712b0, 0x1077140c); a.compare(a.r(3)); if (!a.r(3)) goto complete;
    a.load(0, a.r(3) + 8, 0x10771414); a.put(4, 0x2aaaaaab); a.put(9, 0);
    a.store(a.r(31) + 12, a.r(0), 0x10771424); a.load(0, a.r(3) + 16, 0x10771428);
    a.store(a.r(31) + 16, a.r(0), 0x1077142c); a.load(0, a.r(3) + 24, 0x10771430);
    a.store(a.r(31) + 20, a.r(0), 0x10771434); a.load(6, a.r(3), 0x10771438);
    a.load(0, a.r(3) + 4, 0x1077143c); a.load(7, a.r(3) + 8, 0x10771440); a.put(0, a.r(0) - a.r(6));
    a.put(4, std::uint32_t((std::int64_t(a.signed_r(4)) * a.signed_r(0)) >> 32));
    a.put(0, a.r(30) - a.r(7)); arithmetic_half(a, 4, 4); a.put(5, a.r(4) >> 31); a.put(11, a.r(4) + a.r(5));
    for (;;) {
        a.compare(a.signed_r(9), a.signed_r(11)); if (a.signed_r(9) > a.signed_r(11)) goto complete;
        a.put(5, a.r(9) + a.r(11)); a.put(4, a.r(5) >> 31); a.put(4, a.r(4) + a.r(5));
        arithmetic_half(a, 10, 4); a.put(4, a.r(10) * 12); a.put(8, a.r(6) + a.r(4));
        a.load(5, a.r(8), 0x10771480); a.compare(a.r(0), a.r(5));
        if (a.r(0) < a.r(5)) { a.put(11, a.r(10) - 1); a.s.step(0x10771460); continue; }
        a.load(4, a.r(8) + 4, 0x10771494); a.put(4, a.r(4) & 0x7fffffff); a.put(4, a.r(5) + a.r(4));
        a.compare(a.r(0), a.r(4));
        if (a.r(0) > a.r(4)) { a.put(9, a.r(10) + 1); a.s.step(0x10771460); continue; }
        break;
    }
    a.put(4, a.r(7) + a.r(5)); a.store(a.r(31) + 4, a.r(4), 0x107714b4);
    a.load<1>(4, a.r(8) + 4, 0x107714b8); a.put(4, (a.r(4) >> 7) & 1); a.record(a.r(4));
    if (a.r(4)) a.put(3, a.r(8) + 8);
    else { a.load(4, a.r(3) + 16, 0x107714cc); a.load(3, a.r(8) + 8, 0x107714d0); a.put(3, a.r(4) + a.r(3)); }
    a.store(a.r(31), a.r(3), 0x107714d8); a.load(5, a.r(31), 0x107714dc); a.load(3, a.r(8), 0x107714e0);
    a.load<1>(4, a.r(5) + 1, 0x107714e4); a.put(0, a.r(0) - a.r(3)); a.put(3, (a.r(4) >> 3) & 1); a.record(a.r(3));
    if (a.r(3)) {
        a.put(6, a.r(5) + 4);
        for (;;) {
            a.load(4, a.r(6), 0x10771534); a.compare(a.r(4)); if (!a.r(4)) break;
            a.load<2>(3, a.r(6) + 4, 0x10771500); a.compare(a.r(4), a.r(0));
            a.put(3, a.r(3) * 4); a.put(3, a.r(4) + a.r(3));
            if (a.r(4) <= a.r(0)) {
                a.compare(a.r(3), a.r(0));
                if (a.r(3) >= a.r(0)) {
                    a.load<2>(0, a.r(6) + 6, 0x1077151c); a.put(0, a.r(5) + a.r(0));
                    a.store(a.r(31) + 8, a.r(0), 0x10771524); break;
                }
            }
            a.put(6, a.r(6) + 8); a.s.step(0x10771534);
        }
    } else { a.put(3, (a.r(4) >> 2) & 1); a.record(a.r(3)); short_actions(a, a.r(3) != 0); }
complete:
    a.pop(64, 0x107715d8); a.restore_link(); a.restore(31, 0x107715e4); a.restore(30, 0x107715e8); a.finish();
}
void saved_frame_word(Abi& a) {
    a.load(4, a.r(4), 0x107715f0); a.load<2>(4, a.r(4), 0x107715f4);
    a.put(0, (a.r(4) >> 6) & 1); a.record(a.r(0)); a.put(4, (a.r(4) >> 3) & 0xf8);
    a.put(3, a.r(3) - a.r(4)); if (a.r(0)) a.put(3, a.r(3) - 8);
    a.load(3, a.r(3) - 4, 0x1077160c); a.finish();
}
void action_kind(Abi& a) {
    a.load(3, a.r(3) + 8, 0x10771620); a.compare(a.r(3));
    if (!a.r(3)) a.put(3, 0);
    else { a.load<1>(0, a.r(3), 0x10771634); a.put(3, a.r(0) & 0x7f); }
    a.finish();
}
}
