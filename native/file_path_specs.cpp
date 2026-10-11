#include "file_path_internal.hpp"

namespace cm::aot::file_path {
void classic_spec(Abi& a) {
    a.save(31, 0x10778520); a.capture_link(); a.put(31, 0); a.save(30, 0x1077852c); a.put(30, a.r(4));
    a.save(29, 0x10778534); a.put(29, a.r(3)); a.record(a.r(29)); a.save(28, 0x1077853c); a.save_link(0x10778540); a.frame(816, 0x10778544);
    if (!a.r(29)) goto invalid;
    a.load<1>(0, a.r(29), 0x1077854c); a.signed_byte_record(0, a.r(0)); if (!a.r(0)) goto invalid;
    a.call(0x1077dbd0, 0x1077855c); a.compare(a.r(3), 255); if (a.r(3) > 255) goto invalid;
    a.put(4, a.r(29)); a.put(3, a.r(1) + 288); a.call(0x1077dbf0, 0x10778578);
    a.put(3, a.r(1) + 288); a.put(4, ':'); a.call(0x1077dd50, 0x10778584); a.compare(a.r(3)); a.put(28, a.r(3) + 1);
    if (!a.r(3)) a.put(28, a.r(1) + 288);
    a.put(3, a.r(28)); a.call(0x1077dbd0, 0x1077859c); a.compare(a.signed_r(3));
    if (!a.r(3)) goto invalid;
    a.compare(a.r(3), 63); if (a.r(3) > 63) goto invalid;
    a.put(4, a.r(28)); a.put(3, a.r(30) + 6); a.call(0x107784c0, 0x107785c0);
    a.put(4, a.r(29)); a.put(3, a.r(1) + 544); a.call(0x107784c0, 0x107785cc);
    a.put(0, 0); a.put(4, a.r(1) + 544); a.store<2>(a.r(1) + 78, a.r(0), 0x107785d4); a.put(3, a.r(1) + 56);
    a.store(a.r(1) + 74, a.r(4), 0x107785dc); a.store<2>(a.r(1) + 84, a.r(0), 0x107785e0); a.store(a.r(1) + 104, a.r(0), 0x107785e4);
    a.native_call(0x1077fc10, 0x107785ec); a.signed_word_record(0, a.r(3));
    if (a.r(0)) {
        a.signed_word(0, a.r(3)); a.compare(a.signed_r(0), -43); if (a.signed_r(0) != -43) goto done;
        // Preserve the leaf already published in the output while resolving a
        // missing entry's parent. Colon insertion and truncation are live stores.
        a.put(0, a.r(1) + 288); a.compare(a.r(28), a.r(0));
        if (a.r(28) == a.r(0)) { a.put(0, ':'); a.store<1>(a.r(28), a.r(0), 0x10778618); a.put(28, a.r(28) + 1); }
        a.put(0, 0); a.store<1>(a.r(28), a.r(0), 0x10778624);
        a.put(3, a.r(1) + 544); a.put(4, a.r(1) + 288); a.call(0x107784c0, 0x10778634);
        a.put(3, a.r(1) + 56); a.native_call(0x1077fc10, 0x1077863c); a.signed_word_record(0, a.r(3)); if (a.r(0)) goto done;
        a.load<1>(0, a.r(1) + 86, 0x1077864c); a.put(0, a.r(0) & 16); a.record(a.r(0)); if (!a.r(0)) goto invalid;
        a.load(0, a.r(1) + 104, 0x10778660); a.put(31, std::uint32_t(-43)); a.store(a.r(30) + 2, a.r(0), 0x10778668);
    } else {
        a.load<1>(0, a.r(1) + 86, 0x10778670); a.put(0, a.r(0) & 16); a.record(a.r(0));
        if (a.r(0)) a.put(31, std::uint32_t(-1302));
        a.load(0, a.r(1) + 156, 0x10778680); a.store(a.r(30) + 2, a.r(0), 0x10778684);
    }
    a.put(4, a.r(29)); a.put(3, a.r(1) + 544); a.call(0x107784c0, 0x10778694);
    a.put(0, a.r(1) + 544); a.put(3, 0); a.store(a.r(1) + 182, a.r(0), 0x1077869c); a.put(0, UINT32_MAX);
    a.store<2>(a.r(1) + 186, a.r(3), 0x107786a4); a.put(3, a.r(1) + 164); a.store<2>(a.r(1) + 192, a.r(0), 0x107786ac);
    a.native_call(0x10780000, 0x107786b4); a.signed_word_record(0, a.r(3)); if (a.r(0)) goto done;
    a.load<2>(0, a.r(1) + 186, 0x107786c4); a.signed_word(0, a.r(0)); a.put(3, a.r(31)); a.store<2>(a.r(30), a.r(0), 0x107786cc); goto done;
invalid:
    a.put(3, std::uint32_t(-37));
done:
    a.pop(816, 0x107786d0); a.restore(31, 0x107786d8); a.restore(30, 0x107786dc); a.restore_link(); a.restore(29, 0x107786e4); a.restore(28, 0x107786e8); a.finish();
}

void compatible_spec(Abi& a) {
    a.save(31, 0x107786f0); a.capture_link(); a.put(31, a.r(4)); a.save(30, 0x107786fc); a.put(30, a.r(3)); a.save_link(0x10778704); a.frame(1056, 0x10778708);
    a.call(0x10776bc0, 0x10778710); a.signed_byte_record(0, a.r(3));
    if (!a.r(0)) { a.put(3, a.r(30)); a.put(4, a.r(31)); a.call(0x10778520, 0x107787fc); goto done; }
    a.compare(a.r(30)); if (!a.r(30)) goto invalid;
    a.load<1>(0, a.r(30), 0x10778720); a.signed_byte_record(0, a.r(0)); if (!a.r(0)) goto invalid;
    a.put(3, a.r(30)); a.call(0x1077dbd0, 0x10778734); a.compare(a.r(3), 255); if (a.r(3) > 255) goto invalid;
    a.put(3, a.r(30)); a.put(4, a.r(1) + 56); a.call(0x10778870, 0x10778750); a.signed_word_record(0, a.r(3));
    if (!a.r(0)) {
        a.put(7, a.r(31)); a.put(3, a.r(1) + 56); a.put(4, 0); a.put(5, 0); a.put(6, 0); a.put(8, 0);
        a.native_call(0x1077ff88, 0x10778774); goto done;
    }
    a.signed_word(0, a.r(3)); a.compare(a.signed_r(0), -43); if (a.signed_r(0) != -43) goto done;
    a.put(3, a.r(30)); a.put(4, a.r(1) + 56); a.put(5, a.r(1) + 536); a.call(0x10778a80, 0x10778798); a.signed_word_record(0, a.r(3)); if (a.r(0)) goto done;
    a.put(3, a.r(1) + 56); a.put(5, a.r(1) + 392); a.put(4, 20); a.put(6, 0); a.put(7, 0); a.put(8, 0);
    a.native_call(0x1077ff88, 0x107787bc); a.signed_word_record(0, a.r(3)); if (a.r(0)) goto done;
    a.put(4, a.r(30)); a.put(3, a.r(1) + 136); a.call(0x107784c0, 0x107787d4);
    a.load<2>(3, a.r(1) + 394, 0x107787d4); a.signed_word(3, a.r(3)); a.put(6, a.r(31)); a.load(4, a.r(1) + 400, 0x107787dc);
    a.put(5, a.r(1) + 136); a.native_call(0x1077fee0, 0x107787e8); goto done;
invalid:
    a.put(3, std::uint32_t(-37));
done:
    a.pop(1056, 0x107787fc); a.restore(31, 0x10778804); a.restore_link(); a.restore(30, 0x1077880c); a.finish();
}
}
