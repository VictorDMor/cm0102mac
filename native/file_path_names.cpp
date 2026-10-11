#include "file_path_internal.hpp"

namespace cm::aot::file_path {
void copy_pascal(Abi& a) {
    a.save(31, 0x107784c0); a.capture_link(); a.save(30, 0x107784c8); a.put(30, a.r(4));
    a.save(29, 0x107784d0); a.put(29, a.r(3)); a.put(3, a.r(30)); a.save_link(0x107784dc); a.frame(80, 0x107784e0);
    a.call(0x1077dbd0, 0x107784e8); a.put(31, a.r(3)); a.put(4, a.r(30)); a.put(5, a.r(31)); a.put(3, a.r(29) + 1);
    a.call(0x10739ef0, 0x107784fc); a.store<1>(a.r(29), a.r(31), 0x107784fc);
    a.pop(80, 0x10778500); a.restore_link(); a.restore(31, 0x1077850c); a.restore(30, 0x10778510); a.restore(29, 0x10778514); a.finish();
}
void script_encoding(Abi& a) {
    a.capture_link(); a.put(3, UINT32_MAX); a.save_link(0x10778828); a.put(4, std::uint32_t(-128)); a.put(5, std::uint32_t(-128));
    a.frame(64, 0x10778834); a.put(6, 0); a.put(7, a.r(1) + 56); a.native_call(0x1077ff10, 0x10778844); a.compare(a.signed_r(3));
    if (a.r(3)) { a.put(0, 0); a.store(a.r(1) + 56, a.r(0), 0x10778854); }
    a.load(3, a.r(1) + 56, 0x10778858); a.pop(64, 0x1077885c); a.restore_link(); a.finish();
}
void reference_from_path(Abi& a) {
    a.save(31, 0x10778870); a.capture_link(); a.put(31, a.r(4)); a.save_link(0x1077887c); a.frame(656, 0x10778880);
    a.put(4, a.r(1) + 56); a.put(5, a.r(1) + 136); a.call(0x10778a80, 0x10778890); a.signed_word_record(0, a.r(3));
    if (!a.r(0)) {
        a.call(0x10778820, 0x1077889c); a.load<2>(4, a.r(1) + 136, 0x1077889c); a.put(6, a.r(3)); a.put(7, a.r(31));
        a.put(3, a.r(1) + 56); a.put(5, a.r(1) + 138); a.native_call(0x1077ff70, 0x107788b4);
    }
    a.pop(656, 0x107788b8); a.restore(31, 0x107788c0); a.restore_link(); a.finish();
}

void unicode_name(Abi& a) {
    a.save(31, 0x107788d0); a.capture_link(); a.put(31, a.r(5)); a.save(30, 0x107788dc); a.put(30, a.r(4));
    a.save(29, 0x107788e4); a.save(28, 0x107788e8); a.put(28, a.r(3)); a.save_link(0x107788f0); a.frame(112, 0x107788f4);
    a.call(0x10778820, 0x107788fc); a.put(4, a.r(1) + 72); a.native_call(0x1077ff28, 0x10778904); a.put(29, a.r(3)); a.record(a.r(29));
    if (!a.r(29)) {
        a.put(0, 510); a.store(a.r(1) + 56, a.r(0), 0x10778914); a.put(0, a.r(1) + 80); a.put(3, a.r(1) + 76);
        a.store(a.r(1) + 60, a.r(0), 0x10778920); a.put(0, a.r(31) + 2); a.put(5, a.r(30));
        a.store(a.r(1) + 64, a.r(3), 0x1077892c); a.signed_word(4, a.r(28)); a.put(6, 0);
        a.store(a.r(1) + 68, a.r(0), 0x10778938); a.put(7, 0); a.put(8, 0); a.load(3, a.r(1) + 72, 0x10778944); a.put(9, 0); a.put(10, 0);
        a.native_call(0x1077ff40, 0x10778954); a.load(0, a.r(1) + 76, 0x10778958); a.put(29, a.r(3)); a.put(3, a.r(1) + 72);
        a.put(0, (a.r(0) >> 1) & 0xffff); a.store<2>(a.r(31), a.r(0), 0x10778968); a.native_call(0x1077ff58, 0x10778970);
    }
    a.compare(a.signed_r(29));
    if (a.r(29)) {
        // A failed native conversion deliberately falls back to sign-extended
        // bytes. This preserves the legacy fallback, not a host UTF-8 policy.
        a.signed_word_record(4, a.r(28)); a.store<2>(a.r(31), a.r(28), 0x10778980); a.put(7, 0);
        if (a.signed_r(4) > 0) {
            a.compare(a.signed_r(4), 8); a.put(3, a.r(4) - 8);
            if (a.signed_r(4) > 8) {
                a.put(0, (a.r(3) + 7) >> 3); a.compare(a.signed_r(3)); const bool groups = a.signed_r(3) > 0;
                a.s.wr<4>(4132, a.r(0)); a.put(3, 0);
                if (groups) do {
                    a.put(5, a.r(30) + a.r(7)); a.load<1>(0, a.r(5), 0x107789b4); a.put(6, a.r(31) + a.r(3)); a.put(3, a.r(3) + 16);
                    a.put(0, std::uint32_t(std::int32_t(std::int8_t(a.r(0))))); a.store<2>(a.r(6) + 2, a.r(0), 0x107789c4); a.put(7, a.r(7) + 8);
                    for (unsigned n = 1; n < 8; ++n) {
                        const auto pc = 0x107789ccu + (n - 1) * 12;
                        a.load<1>(0, a.r(5) + n, pc); a.put(0, std::uint32_t(std::int32_t(std::int8_t(a.r(0))))); a.store<2>(a.r(6) + 2 + n * 2, a.r(0), pc + 8);
                    }
                    a.s.step(0x10778a20);
                } while (a.count_down());
            }
            a.put(0, a.r(4) - a.r(7)); a.compare(a.signed_r(7), a.signed_r(4)); const bool tail = a.signed_r(7) < a.signed_r(4);
            a.s.wr<4>(4132, a.r(0));
            a.put(5, a.r(7) * 2); a.put(3, a.r(30) + a.r(7));
            if (tail) do {
                a.load<1>(4, a.r(3), 0x10778a3c); a.put(0, a.r(5) + 2); a.put(3, a.r(3) + 1);
                a.put(4, std::uint32_t(std::int32_t(std::int8_t(a.r(4))))); a.store<2>(a.r(31) + a.r(0), a.r(4), 0x10778a4c); a.put(5, a.r(5) + 2);
                a.s.step(0x10778a54);
            } while (a.count_down());
        }
    }
    a.pop(112, 0x10778a58); a.restore(31, 0x10778a60); a.restore(30, 0x10778a64); a.restore_link();
    a.restore(29, 0x10778a6c); a.restore(28, 0x10778a70); a.finish();
}
}
