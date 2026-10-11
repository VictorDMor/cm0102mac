#include "file_path_internal.hpp"

namespace cm::aot::file_path {
namespace {
void current_parent(Abi& a, std::uint32_t first) {
    a.load<2>(3, a.r(1) + 56, first); a.signed_word(3, a.r(3)); a.put(5, a.r(2) - 0x34f4);
    a.load(4, a.r(1) + 58, first + 8); a.put(6, a.r(1) + 56);
    a.native_call(0x1077fee0, first == 0x10778ad8 ? 0x10778aec : 0x10778c50); a.put(30, a.r(3)); a.signed_word_record(0, a.r(30));
    if (!a.r(0)) {
        a.put(4, a.r(25)); a.put(3, a.r(1) + 56);
        a.native_call(0x1077fef8, first == 0x10778ad8 ? 0x10778b08 : 0x10778c6c); a.put(30, a.r(3));
    }
}
void scan_colon(Abi& a, bool first) {
    const auto test = first ? 0x10778b48u : 0x10778cb8u;
    for (;;) {
        a.signed_byte_record(0, a.r(27)); if (a.r(0)) break;
        a.compare(a.signed_r(29), a.signed_r(31)); if (a.signed_r(29) >= a.signed_r(31)) break;
        a.load<1>(0, a.r(3), first ? 0x10778b2c : 0x10778c9c); a.compare(a.signed_r(0), ':');
        if (a.r(0) == ':') { a.put(27, 1); continue; }
        a.put(29, a.r(29) + 1); a.put(3, a.r(3) + 1); a.s.step(test);
    }
}
void copy_volume_prefix(Abi& a) {
    // The first colon and its preceding bytes become a live Pascal name. The
    // original copies in eight-byte groups and leaves the remainder for a tail;
    // retaining that access order matters for overlapping output and faults.
    a.put(0, a.r(29) + 1); a.s.wr<1>(1027, a.r(29) == UINT32_MAX); a.record(a.r(0)); a.put(6, 0);
    if (a.signed_r(0) > 0) {
        a.put(0, a.r(29) + 1); a.compare(a.signed_r(0), 8); a.put(3, a.r(29) - 7);
        if (a.signed_r(0) > 8) {
            a.put(0, (a.r(3) + 7) >> 3); a.compare(a.signed_r(3)); a.s.wr<4>(4132, a.r(0));
            if (a.signed_r(3) > 0) do {
                a.put(3, a.r(24) + a.r(6)); a.put(4, a.r(1) + 56); a.load<1>(0, a.r(3), 0x10778b98); a.put(4, a.r(4) + a.r(6));
                a.store<1>(a.r(4) + 7, a.r(0), 0x10778ba0); a.put(6, a.r(6) + 8);
                for (unsigned n = 1; n < 8; ++n) {
                    const auto pc = 0x10778ba8u + (n - 1) * 8;
                    a.load<1>(0, a.r(3) + n, pc); a.store<1>(a.r(4) + 7 + n, a.r(0), pc + 4);
                }
                a.s.step(0x10778be0);
            } while (a.count_down());
        }
        a.put(3, a.r(29) + 1); a.put(0, a.r(3) - a.r(6)); a.compare(a.signed_r(6), a.signed_r(3));
        const bool tail = a.signed_r(6) < a.signed_r(3); a.s.wr<4>(4132, a.r(0)); a.put(4, a.r(1) + 56); a.put(3, a.r(24) + a.r(6));
        if (tail) do {
            a.load<1>(5, a.r(3), 0x10778c00); a.put(0, a.r(6) + 7); a.put(3, a.r(3) + 1); a.store<1>(a.r(4) + a.r(0), a.r(5), 0x10778c0c);
            a.put(6, a.r(6) + 1); a.s.step(0x10778c14);
        } while (a.count_down());
    }
    a.put(0, a.r(29) + 1); a.store<1>(a.r(1) + 62, a.r(0), 0x10778c1c);
}
}

void parent_and_leaf(Abi& a) {
    a.words(24, 0x10778a80); a.capture_link(); a.put(24, a.r(3)); a.put(25, a.r(4)); a.put(26, a.r(5));
    a.save_link(0x10778a94); a.frame(160, 0x10778a98); a.call(0x1077dbd0, 0x10778aa0); a.put(31, a.r(3));
    a.put(5, a.r(1) + 58); a.put(4, a.r(1) + 56); a.put(3, 0); a.native_call(0x1077fec8, 0x10778ab4); a.signed_word_record(0, a.r(3));
    if (a.r(0)) { a.put(0, 0); a.store<2>(a.r(1) + 56, a.r(0), 0x10778ac4); a.store(a.r(1) + 58, a.r(0), 0x10778ac8); }
    a.load<1>(0, a.r(24), 0x10778acc); a.compare(a.signed_r(0), ':');
    if (a.r(0) == ':') { current_parent(a, 0x10778ad8); a.put(28, 1); a.put(27, 1); }
    else {
        a.put(3, a.r(24)); a.put(29, 0); a.put(27, 0); scan_colon(a, true); a.signed_byte_record(0, a.r(27));
        if (a.r(0)) {
            copy_volume_prefix(a); a.put(4, a.r(25)); a.put(3, a.r(1) + 56); a.native_call(0x1077fef8, 0x10778c2c);
            a.put(30, a.r(3)); a.put(28, a.r(29) + 1);
        } else {
            current_parent(a, 0x10778c3c); a.put(4, a.r(24)); a.put(5, a.r(26)); a.signed_word(3, a.r(31)); a.call(0x107788d0, 0x10778c84);
        }
    }
    for (;;) {
        a.signed_word_record(0, a.r(30)); if (a.r(0)) break;
        a.signed_byte_record(0, a.r(27)); if (!a.r(0)) break;
        a.put(29, a.r(28)); a.put(3, a.r(24) + a.r(28)); a.put(27, 0); scan_colon(a, false);
        a.put(0, a.r(29) - a.r(28)); a.put(5, a.r(26)); a.signed_word(3, a.r(0)); a.put(4, a.r(24) + a.r(28));
        a.call(0x107788d0, 0x10778cdc); a.signed_byte_record(0, a.r(27));
        if (a.r(0)) {
            a.put(28, a.r(29) + 1); a.load<1>(0, a.r(24) + a.r(28), 0x10778ce8); a.signed_byte_record(0, a.r(0));
            if (!a.r(0)) a.put(27, 0);
            else {
                a.call(0x10778820, 0x10778d00); a.load<2>(4, a.r(26), 0x10778d00); a.put(6, a.r(3)); a.put(3, a.r(25)); a.put(7, a.r(25));
                a.put(5, a.r(26) + 2); a.native_call(0x1077ff70, 0x10778d18); a.put(30, a.r(3));
            }
        }
        a.s.step(0x10778d20);
    }
    a.put(3, a.r(30)); a.pop(160, 0x10778d34); a.restore_link(); a.words(24, 0x10778d40, true); a.finish();
}
}
